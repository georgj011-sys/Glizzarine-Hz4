#pragma once
// Glizzarine Harmonizer engine v1: real-time pitch-synchronous overlap-add (PSOLA) + vocoder layer.
// The singer's voice is analysed (pitch + one mark per vocal cycle). Every held key gets its own
// voice that rebuilds the singing at the key's pitch from those cycles (timbre stays), and the
// same keys play a vocoder synth shaped by the voice. No JUCE dependency, testable offline.
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include "Vocoder.h"

class HarmonizerEngine
{
public:
    static constexpr int maxVoices = 16;

    struct Params
    {
        float dry = 0.8f, harmony = 0.9f, formant = 0.0f, consonants = 0.3f;
        float attack = 0.015f, release = 0.6f;       // chord envelope (seconds)
        float width = 0.7f, lowestHz = 80.0f;
        float gate = 0.0003f, sensitivity = 0.00025f;
        float sustain = 4.0f, leveling = 0.5f, subOctave = 0.0f, follow = 0.5f;
        bool  velBright = false; float velBrightAmt = 0.3f;
        bool  velAttack = false; float velAttackAmt = 0.5f;
        bool  velSend   = false; float velSendAmt = 0.5f;
        float vibratoCents = 0.0f, detuneCents = 0.0f, formantMod = 0.0f, harmonyBoost = 1.0f;
        float glide = 0.0f;       // seconds, voices slide to new notes (pad FX)
        bool  chaos = false;      // voices jump octaves at random (pad FX)
        float pitchShift = 0.0f;  // semitones added to all voices (octave pads, riser)
        float blend = 0.0f;       // 0 = harmonizer only, 1 = vocoder only
        Vocoder::Params vocoder;
    };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        inSize = 1 << 15;
        in.assign ((size_t) inSize, 0.0f);
        lp.assign ((size_t) inSize, 0.0f);
        vflag.assign ((size_t) inSize, 0);
        diff.assign (4096, 0.0f);
        marks.clear(); marks.reserve (600);
        n = 0; lastMark = 0;
        t0 = (float) (sr / 150.0);
        voiced = false;
        lpState = lpState2 = 0.0f;
        lpC = 1.0f - std::exp (-2.0f * 3.14159265f * 900.0f / (float) sr);
        setLowest (params.lowestHz);
        for (auto& v : voices) resetVoice (v);
        resetVoice (sub);
        hopCount = 0; uvSmooth = 0.0f;
        gateEnv = 0.0f; gateGain = 1.0f;
        gateRel = std::exp (-1.0f / (0.1f * (float) sr));
        gateAtkC = 1.0f - std::exp (-1.0f / (0.01f * (float) sr));
        gateRelC = 1.0f - std::exp (-1.0f / (0.12f * (float) sr));
        for (auto& c : cap) { c.data.assign ((size_t) (2 * 1200 + 4), 0.0f); c.period = 0.0f; }
        capCount = 0; capW = 0; lastVoicedN = -1000000;
        levelSlow = 0.01f; levelGain = 1.0f; vibPhase = 0.0;
        devCents = devSmooth = 0.0f; countGain = 1.0f;
        hpC = 1.0f - std::exp (-2.0f * 3.14159265f * 120.0f / (float) sr);
        smC = 1.0f - std::exp (-1.0f / (0.005f * (float) sr));
        vocoder.prepare (sampleRate);
        vocL.assign (8192, 0.0f); vocR.assign (8192, 0.0f); vocIn.assign (8192, 0.0f);
        rng = 98765u;
    }

    void setParams (const Params& p) { if (p.lowestHz != params.lowestHz) setLowest (p.lowestHz); params = p; vocoder.setParams (p.vocoder); }
    void setVelocitySensitivity (float s) { velSens = std::min (1.0f, std::max (0.0f, s)); }
    int activeVoices() const { int k = 0; for (auto& v : voices) if (v.active && (v.gate || v.sustained)) ++k; return k; }
    int getLatencySamples() const { return delay; }

    static float panOrder (int k)
    {
        static const float seq[8] = { 0.0f, -1.0f, 1.0f, -0.6f, 0.6f, -0.3f, 0.3f, -0.85f };
        return seq[k & 7];
    }

    // pan < -1.5 = automatic (centre, hard left, hard right, ...); env = nullptr: chord envelope
    void noteOn (int note, float velocity, float pan = -2.0f, const EnvShape* env = nullptr, bool accent = false)
    {
        EnvShape shape;
        if (env) shape = *env;
        else
        {
            shape.attack = std::max (0.005f, params.attack) * (params.velAttack ? 1.0f + params.velAttackAmt * 4.0f * (1.0f - velocity) : 1.0f);
            shape.decay = 1.0f; shape.sustain = 1.0f; shape.release = std::max (0.03f, params.release);
        }

        Voice* v = nullptr;
        for (auto& x : voices) if (x.active && x.note == note) v = &x;            // same key again
        if (! v) for (auto& x : voices) if (! x.active) { v = &x; break; }
        bool steal = false;
        if (! v)
        {
            // steal: prefer a releasing voice, then the oldest; it fades out over 5 ms first
            for (auto& x : voices) if (! x.gate && ! x.sustained && (! v || x.age < v->age)) v = &x;
            if (! v) { v = &voices[0]; for (auto& x : voices) if (x.age < v->age) v = &x; }
            steal = true;
        }

        if (pan < -1.5f)
        {
            int k = 0; for (auto& x : voices) if (x.active && &x != v) ++k;
            pan = panOrder (k);
        }

        if (steal || (v->active && v->note != note))
        {
            v->pending = true; v->pNote = note; v->pVel = velocity; v->pPan = pan; v->pShape = shape; v->pAccent = accent;
            v->fade = 1.0f;
            return;
        }
        begin (*v, note, velocity, pan, shape, accent);
    }

    void noteOff (int note)
    {
        for (auto& v : voices)
        {
            if (v.pending && v.pNote == note) { v.pReleased = ! pedal; }
            if (v.active && v.note == note && v.gate) { v.gate = false; v.sustained = pedal; if (! pedal) v.env.release(); }
        }
        if (! pedal) vocoder.noteOff (note);   // the vocoder follows the same keys (with the pedal: on pedal up)
    }

    void setSustain (bool down)
    {
        pedal = down;
        if (! down)
            for (auto& v : voices)
                if (v.sustained) { v.sustained = false; v.env.release(); if (! v.gate) vocoder.noteOff (v.note); }
    }

    void allNotesOff() { for (auto& v : voices) { v.gate = false; v.sustained = false; v.pending = false; v.env.release(); } vocoder.allOff(); }

    // mono in; harmonies stereo (harmonizer + vocoder blend), your voice mono (delayed and direct), velocity send
    void process (const float* input, float* harmL, float* harmR, float* dryOut, float* dryDirect, float* sendL, float* sendR, int num)
    {
        const float fShiftBase = std::exp2 ((params.formant + params.formantMod) * 0.5f);
        const float holdC = params.sustain > 0.01f ? std::exp (std::log (0.001f) / (params.sustain * (float) sr)) : 0.0f;
        const float slowC = 1.0f / (1.0f * (float) sr);
        const float glideC = params.glide > 0.001f ? 1.0f - std::exp (-1.0f / (params.glide * (float) sr)) : 1.0f;
        updateSub();
        int sounding = 0;
        for (auto& v : voices) if (v.active && (v.gate || v.sustained)) ++sounding;
        const float wantCount = std::pow ((float) std::max (1, sounding), -0.4f);
        const float hBlend = std::cos (params.blend * 1.5707963f), vBlend = std::sin (params.blend * 1.5707963f);
        const float fsr = (float) sr;
        if ((int) vocIn.size() < num) { vocIn.resize ((size_t) num, 0.0f); }

        for (int i = 0; i < num; ++i)
        {
            // soft gate: slow and gentle, so it never chops your singing
            const float ax = std::abs (input[i]);
            gateEnv = ax > gateEnv ? ax : gateEnv * gateRel;
            const float gt = gateEnv >= params.gate ? 1.0f : std::max (0.1f, gateEnv / std::max (1.0e-7f, params.gate));
            gateGain += (gt - gateGain) * (gt > gateGain ? gateAtkC : gateRelC);
            const float x = input[i] * gateGain;
            in[(size_t) (n & (inSize - 1))] = x;
            lpState += lpC * (x - lpState);
            lpState2 += lpC * (lpState - lpState2);
            lp[(size_t) (n & (inSize - 1))] = lpState2;
            ++n;

            if (++hopCount >= 256) { hopCount = 0; detectPitch(); }
            vflag[(size_t) (n & (inSize - 1))] = voiced ? 1 : 0;
            if (voiced) lastVoicedN = n;
            placeMarks();

            const int64_t t = n - delay;
            const float tIn = in[(size_t) (t & (inSize - 1))];
            countGain += 0.0005f * (wantCount - countGain);
            devSmooth += 0.002f * ((voiced ? devCents : 0.0f) - devSmooth);

            if (vflag[(size_t) (t & (inSize - 1))])
                levelSlow += slowC * (std::abs (tIn) * 1.4f - levelSlow);
            const float wantGain = std::pow (0.08f / std::max (1.0e-4f, levelSlow), params.leveling);
            levelGain += 0.00005f * (std::min (30.0f, std::max (0.3f, wantGain)) - levelGain);

            vibPhase += 5.5 / sr; if (vibPhase >= 1.0) vibPhase -= 1.0;
            const float vib = (float) std::sin (6.283185307 * vibPhase) * params.vibratoCents + devSmooth * params.follow;

            float l = 0.0f, r = 0.0f, sl = 0.0f, srr = 0.0f, envMax = 0.0f;
            for (int vi = 0; vi <= maxVoices; ++vi)
            {
                Voice& v = vi < maxVoices ? voices[vi] : sub;
                if (! v.active) continue;

                // steal fade, then the new note starts from silence
                if (v.pending)
                {
                    v.fade -= 1.0f / (0.005f * fsr);
                    if (v.fade <= 0.0f)
                    {
                        const bool rel = v.pReleased;
                        v.pending = false; v.pReleased = false;
                        v.env.kill();
                        v.active = false;
                        begin (v, v.pNote, v.pVel, v.pPan, v.pShape, v.pAccent);
                        if (rel) { v.gate = false; v.env.release(); }
                    }
                }

                const float e = v.env.next (fsr);
                if (! v.env.active() && ! v.pending) { v.active = false; continue; }

                // smoothed per-voice values: nothing ever jumps
                v.velS += (v.vel - v.velS) * smC;
                v.panS += (v.pan - v.panS) * smC;
                v.accent *= 0.9997f;
                v.accentS += (v.accent - v.accentS) * smC;
                if (v.glidePitch < 0.0f) v.glidePitch = (float) v.note;
                v.glidePitch += ((float) v.note + (float) v.chaosOct * 12.0f + params.pitchShift - v.glidePitch) * std::max (glideC, params.glide > 0.001f ? 0.0f : 0.0015f);

                const float detune = params.detuneCents * (v.haas * 2.0f - 1.0f);
                const float cents = vib + detune;
                const float targetPeriod = (float) (sr / (440.0 * std::pow (2.0, (v.glidePitch - 69.0) / 12.0 + cents / 1200.0)));
                int guard = 0;
                while (v.nextMark <= (double) t + (double) maxT0 && guard++ < 8)
                    emitGrain (v, targetPeriod, fShiftBase, t);

                if (vi < maxVoices) envMax = std::max (envMax, e);
                const size_t idx = (size_t) (t & (olaSize - 1));
                float raw = v.ola[idx];
                v.ola[idx] = 0.0f;
                if (v.live)
                {
                    v.pIn  += 0.00002f * (tIn * tIn - v.pIn);
                    v.pOut += 0.00002f * (raw * raw - v.pOut);
                    const float want = std::sqrt ((v.pIn + 1.0e-10f) / (v.pOut + 1.0e-10f));
                    v.agc += 0.00003f * (std::min (3.0f, std::max (0.5f, want)) - v.agc);
                }
                // hold: back to full smoothly while singing, slow ring-out after you stop
                if (v.ringing) v.hold *= holdC; else v.hold += (1.0f - v.hold) * smC;

                if (params.velBright)
                {
                    const float cutVel = 1200.0f * std::exp2 (3.9f * v.velocity);
                    const float cut = std::exp ((1.0f - params.velBrightAmt) * std::log (18000.0f) + params.velBrightAmt * std::log (cutVel));
                    v.lpf += (1.0f - std::exp (-6.2831853f * cut / fsr)) * (raw - v.lpf);
                    raw = v.lpf;
                }
                if (vi < maxVoices) { v.hpLp += hpC * (raw - v.hpLp); raw -= v.hpLp * 0.85f; }

                const float lvl = (vi < maxVoices ? 1.0f : params.subOctave);
                const float fade = v.pending ? std::max (0.0f, v.fade) : 1.0f;
                const float s = raw * v.agc * e * (1.0f - velSens * (1.0f - v.velS)) * v.hold * lvl * fade * (1.0f + v.accentS * 0.6f);

                const float pan = v.panS * params.width;
                const float pl = std::sqrt (0.5f * (1.0f - pan));
                const float pr = std::sqrt (0.5f * (1.0f + pan));
                // fractional Haas delay: it can glide without skipping samples (no clicks when the pan moves)
                const float hdf = (0.3f + 0.7f * v.haas) * std::abs (pan) * 0.012f * fsr;
                const int hd = (int) hdf; const float hfr = hdf - (float) hd;
                v.haasBuf[(size_t) (v.haasW & 1023)] = s;
                const float sd = v.haasBuf[(size_t) ((v.haasW - hd) & 1023)] * (1.0f - hfr)
                               + v.haasBuf[(size_t) ((v.haasW - hd - 1) & 1023)] * hfr;
                v.haasW = (v.haasW + 1) & 0xFFFFF;
                const float oL = pan < 0.0f ? s * pl : sd * pl;
                const float oR = pan < 0.0f ? sd * pr : s * pr;
                l += oL; r += oR;
                if (params.velSend) { const float g = params.velSendAmt * v.velocity; sl += oL * g; srr += oR * g; }
            }

            const float uv = vflag[(size_t) (t & (inSize - 1))] ? 0.0f : 1.0f;
            uvSmooth += (uv - uvSmooth) * 0.01f;
            const bool singingNearby = (n - lastVoicedN) < (int64_t) (0.3 * sr);
            const float cons = singingNearby ? tIn * uvSmooth * params.consonants * envMax : 0.0f;

            const float hg = params.harmony * params.harmonyBoost * levelGain * countGain;
            const float cg = params.harmony * std::min (2.0f, levelGain);
            harmL[i] = (l * hg + cons * cg) * hBlend;
            harmR[i] = (r * hg + cons * cg) * hBlend;
            sendL[i] = sl * hg;
            sendR[i] = srr * hg;
            const float dg = params.dry * std::pow (std::max (0.3f, levelGain), 0.75f);
            dryOut[i] = tIn * dg;
            dryDirect[i] = x * dg;
            vocIn[(size_t) i] = tIn * std::min (8.0f, levelGain);   // vocoder hears the leveled, aligned voice
        }

        // vocoder layer (in chunks, any block size is fine)
        if (vBlend > 0.001f)
        {
            int done = 0;
            while (done < num)
            {
                const int len = std::min (num - done, 4096);
                std::fill (vocL.begin(), vocL.begin() + len, 0.0f);
                std::fill (vocR.begin(), vocR.begin() + len, 0.0f);
                vocoder.process (vocIn.data() + done, vocL.data(), vocR.data(), len, devSmooth * params.follow + params.pitchShift * 100.0f);
                const float g = params.harmony * params.harmonyBoost * countGain * vBlend;
                for (int i = 0; i < len; ++i) { harmL[done + i] += vocL[(size_t) i] * g; harmR[done + i] += vocR[(size_t) i] * g; }
                done += len;
            }
        }
    }

    bool isVoiced() const { return voiced; }
    float currentPitchHz() const { return (float) (sr / t0); }
    float getLevelGain() const { return levelGain; }

private:
    struct Mark { int64_t pos; float period; bool voiced; };
    struct Voice
    {
        bool active = false, gate = false, sustained = false, live = false, ringing = false;
        int note = -1; float vel = 1.0f, velS = 1.0f, velocity = 1.0f, pan = 0.0f, panS = 0.0f, haas = 0.0f;
        double nextMark = 0.0; uint64_t age = 0;
        float pIn = 0.0f, pOut = 0.0f, agc = 1.5f, hold = 1.0f, lpf = 0.0f, hpLp = 0.0f, accent = 0.0f, accentS = 0.0f;
        float glidePitch = -1.0f; int chaosOct = 0;
        int capIdx = 0, haasW = 0;
        SmoothEnv env;
        bool pending = false, pReleased = false, pAccent = false; int pNote = 60; float pVel = 1.0f, pPan = 0.0f; EnvShape pShape; float fade = 1.0f;
        std::vector<float> ola, haasBuf;
    };

    static constexpr int olaSize = 1 << 13;

    void resetVoice (Voice& v)
    {
        v = Voice();
        v.ola.assign ((size_t) olaSize, 0.0f);
        v.haasBuf.assign (1024, 0.0f);
    }

    void startVoice (Voice& v)
    {
        v.pIn = v.pOut = 0.0f; v.agc = 1.5f; v.hold = 1.0f; v.live = false; v.ringing = false; v.capIdx = 0;
        v.lpf = 0.0f; v.hpLp = 0.0f; v.glidePitch = -1.0f; v.chaosOct = 0;
        v.nextMark = (double) (n - delay);
        std::fill (v.ola.begin(), v.ola.end(), 0.0f);
        std::fill (v.haasBuf.begin(), v.haasBuf.end(), 0.0f);
    }

    void begin (Voice& v, int note, float velocity, float pan, const EnvShape& shape, bool accent)
    {
        const bool fresh = ! v.active;
        if (fresh) { startVoice (v); v.panS = pan; }
        else if (v.env.out < 0.05f) v.panS = pan;   // almost silent: jump to the new place instead of sliding through the stereo field
        const int prevNote = v.note;
        v.active = true; v.note = note; v.gate = true; v.sustained = false;
        v.velocity = velocity; v.vel = velocity;
        if (fresh) v.velS = velocity;
        v.age = ++ageCounter;
        v.pan = pan;
        if (fresh) v.haas = (float) ((v.age * 7919u) % 100u) / 100.0f;
        if (params.glide > 0.001f && prevNote >= 0 && ! fresh) v.glidePitch = (float) prevNote;
        else if (params.glide > 0.001f && lastNoteOn >= 0) v.glidePitch = (float) lastNoteOn;
        else v.glidePitch = (float) note;
        v.chaosOct = params.chaos ? (int) (rand01() * 3.0f) - 1 : 0;
        v.accent = accent ? 1.0f : 0.0f;
        v.env.start (shape);
        lastNoteOn = note;
        vocoder.noteOn (note, velocity, pan, shape);
    }

    // the sub-octave voice follows the lowest sounding key, one octave down
    void updateSub()
    {
        int lowest = 1000; float vel = 0.0f;
        for (auto& v : voices)
            if (v.active && (v.gate || v.sustained) && v.note < lowest) { lowest = v.note; vel = v.velocity; }
        const bool want = params.subOctave > 0.001f && lowest < 1000;
        if (want)
        {
            if (! sub.active) { startVoice (sub); sub.active = true; EnvShape s; s.attack = 0.05f; s.release = std::max (0.05f, params.release); sub.env.start (s); }
            else if (sub.env.stage == 3) { EnvShape s = sub.env.shape; sub.env.start (s); }
            sub.note = lowest - 12; sub.gate = true; sub.sustained = false;
            sub.velocity = vel; sub.vel = vel; sub.pan = 0.0f; sub.haas = 0.0f;
        }
        else if (sub.active && sub.gate) { sub.gate = false; sub.env.release(); }
    }

    void setLowest (float hz)
    {
        params.lowestHz = hz;
        maxT0 = (int) std::ceil (sr / std::max (50.0f, hz));
        delay = (int) (maxT0 * 1.6f) + 32;
    }

    float at (int64_t p) const { return in[(size_t) (p & (inSize - 1))]; }
    float atLp (int64_t p) const { return lp[(size_t) (p & (inSize - 1))]; }

    // YIN pitch detection on the newest samples
    void detectPitch()
    {
        const int W = 768;
        const int tauMin = (int) (sr / 1000.0), tauMax = maxT0;
        if (n < W + tauMax + 2) return;
        const int64_t start = n - W - tauMax;
        double energy = 0.0;
        for (int j = 0; j < W; ++j) { const float s = atLp (start + tauMax + j); energy += s * s; }
        const float rms = (float) std::sqrt (energy / W);
        if ((int) diff.size() < tauMax + 2) diff.resize ((size_t) tauMax + 2, 0.0f);
        float running = 0.0f; int best = -1; float bestVal = 1.0f;
        std::vector<float>& d = diff;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            double sum = 0.0;
            for (int j = 0; j < W; j += 2)
            {
                const float a = atLp (start + tauMax + j) - atLp (start + tauMax + j - tau);
                sum += a * a;
            }
            running += (float) sum;
            d[(size_t) tau] = (float) sum * (float) tau / std::max (1.0e-12f, running);
        }
        for (int tau = std::max (2, tauMin); tau < tauMax; ++tau)
            if (d[(size_t) tau] < 0.15f && d[(size_t) tau] < d[(size_t) tau - 1] && d[(size_t) tau] <= d[(size_t) tau + 1])
            { best = tau; bestVal = d[(size_t) tau]; break; }
        if (best < 0)
            for (int tau = std::max (2, tauMin); tau < tauMax; ++tau)
                if (d[(size_t) tau] < bestVal) { bestVal = d[(size_t) tau]; best = tau; }

        const bool periodic = best > 0 && bestVal < (voiced ? 0.65f : 0.5f);   // lenient: breathy, quiet singing still counts
        const bool loud = rms > params.sensitivity;
        unvoicedHops = (periodic && loud) ? 0 : unvoicedHops + 1;
        const bool isVoice = (periodic && loud) || (voiced && loud && unvoicedHops < 2);
        if (isVoice && periodic)
        {
            float tau = (float) best;
            if (best > 1 && best < tauMax)
            {
                const float a = d[(size_t) best - 1], b = d[(size_t) best], c = d[(size_t) best + 1];
                const float den = a - 2.0f * b + c;
                if (std::abs (den) > 1.0e-9f) tau += 0.5f * (a - c) / den;
            }
            const bool jump = voiced && std::abs (tau - t0) > 0.2f * t0;
            if (jump && std::abs (tau - pendingTau) > 0.1f * tau) { pendingTau = tau; }
            else { t0 = voiced && ! jump ? 0.6f * t0 + 0.4f * tau : tau; pendingTau = 0.0f; }
            const double semis = 69.0 + 12.0 * std::log2 ((sr / t0) / 440.0);
            devCents = (float) ((semis - std::round (semis)) * 100.0);
        }
        voiced = isVoice;
    }

    // one analysis mark per vocal cycle, placed on the cycle's peak
    void placeMarks()
    {
        const float period = voiced ? t0 : 256.0f;
        const int64_t search = (int64_t) (period * 0.25f);
        if (n < lastMark + (int64_t) period + search + 1) return;
        int64_t m = lastMark + (int64_t) period;
        if (voiced)
        {
            int64_t bestP = m; float bestV = -1.0e9f;
            for (int64_t p = m - search; p <= m + search; ++p)
            { const float v = atLp (p); if (v > bestV) { bestV = v; bestP = p; } }
            m = bestP;
        }
        // keep copies of the last few sung cycles, so held notes can ring on after you stop
        if (! marks.empty() && marks.back().voiced)
        {
            const Mark& pm = marks.back();
            const int half = std::min (1200, (int) pm.period);
            if (pm.pos + half + 2 <= n)
            {
                auto& c = cap[(size_t) (capW++ & 3)];
                for (int k = -half; k <= half + 1; ++k) c.data[(size_t) (k + half)] = at (pm.pos + k);
                c.period = (float) half;
                capCount = std::min (4, capCount + 1);
            }
        }
        marks.push_back ({ m, period, voiced });
        lastMark = m;
        while (marks.size() > 512) marks.erase (marks.begin());
    }

    // t = current output position: grain parts before it are already played and must not be written
    // (they would sit in the ring buffer and pop up as a click one buffer length later)
    void emitGrain (Voice& v, float targetPeriod, float fShift, int64_t t)
    {
        const Mark* src = nullptr; double bestDist = 1.0e18;
        for (auto it = marks.rbegin(); it != marks.rend(); ++it)
        {
            if (it->pos + (int64_t) maxT0 + 2 > n) continue;
            const double dd = std::abs ((double) it->pos - (double) v.nextMark);
            if (dd < bestDist) { bestDist = dd; src = &*it; }
            if ((double) it->pos < v.nextMark - 2.0 * maxT0) break;
        }

        const bool liveOk = src != nullptr && src->voiced && bestDist < 2.0 * maxT0;
        const int64_t centre = (int64_t) std::llround (v.nextMark);
        v.ringing = false;

        if (liveOk)
        {
            v.live = true;
            const float P = src->period;
            const int half = std::min (maxT0, std::max ((int) (P / fShift), (int) (0.6f * targetPeriod)));
            const float gain = std::min (2.0f, targetPeriod / (float) half);
                for (int k = std::max (-half, (int) (t - centre)); k <= half; ++k)
            {
                const float w = 0.5f + 0.5f * std::cos (3.14159265f * (float) k / (float) half);
                const double sp = (double) src->pos + (double) k * fShift;
                const int64_t s0 = (int64_t) std::floor (sp); const float fr = (float) (sp - (double) s0);
                const float s = at (s0) + fr * (at (s0 + 1) - at (s0));
                v.ola[(size_t) ((centre + k) & (olaSize - 1))] += s * w * gain;
            }
        }
        else
        {
            v.live = false;
            const bool longGap = (n - lastVoicedN) > (int64_t) (0.15 * sr);
            if (longGap && params.sustain > 0.01f && capCount > 0 && v.hold > 1.0e-4f)
            {
                v.ringing = true;
                const auto& c = cap[(size_t) (v.capIdx++ % capCount)];
                const float P = c.period;
                const int half = std::max (1, std::min (maxT0, (int) (P / fShift)));
                const float gain = std::min (2.0f, targetPeriod / (float) half);
                for (int k = std::max (-half, (int) (t - centre)); k <= half; ++k)
                {
                    const float w = 0.5f + 0.5f * std::cos (3.14159265f * (float) k / (float) half);
                    const double sp = (double) P + (double) k * fShift;
                    const int s0 = (int) std::floor (sp); const float fr = (float) (sp - (double) s0);
                    if (s0 < 0 || s0 + 1 >= (int) c.data.size()) continue;
                    const float s = c.data[(size_t) s0] + fr * (c.data[(size_t) s0 + 1] - c.data[(size_t) s0]);
                    v.ola[(size_t) ((centre + k) & (olaSize - 1))] += s * w * gain;
                }
            }
        }
        v.nextMark += targetPeriod;
    }

    float rand01() { rng = rng * 1664525u + 1013904223u; return (float) (rng >> 8) / 16777216.0f; }

    Params params;
    double sr = 44100.0;
    std::vector<float> in, lp, diff;
    std::vector<uint8_t> vflag;
    float uvSmooth = 0.0f;
    int inSize = 0, hopCount = 0, maxT0 = 600, delay = 1000;
    int64_t n = 0, lastMark = 0;
    float t0 = 300.0f, lpState = 0.0f, lpState2 = 0.0f, lpC = 0.12f;
    bool voiced = false, pedal = false;
    int unvoicedHops = 0;
    float pendingTau = 0.0f;
    float gateEnv = 0.0f, gateGain = 1.0f, gateRel = 0.999f, gateAtkC = 0.5f, gateRelC = 0.001f;
    struct Captured { std::vector<float> data; float period = 0.0f; };
    Captured cap[4];
    int capCount = 0; uint32_t capW = 0;
    int64_t lastVoicedN = -1000000;
    float levelSlow = 0.01f, levelGain = 1.0f;
    double vibPhase = 0.0;
    float devCents = 0.0f, devSmooth = 0.0f, countGain = 1.0f, hpC = 0.02f, smC = 0.01f;
    std::vector<Mark> marks;
    Voice voices[maxVoices];
    Voice sub;
    uint64_t ageCounter = 0;
    float velSens = 0.6f;
    int lastNoteOn = -1;
    uint32_t rng = 98765u;
    Vocoder vocoder;
    std::vector<float> vocL, vocR, vocIn;
};
