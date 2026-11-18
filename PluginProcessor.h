#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>
#include "HarmonizerEngine.h"
#include "Sequencer.h"
#include "Effects.h"
#include "WeirdFX.h"
#include "Presets.h"

#define GLZH_VERSION "v1.0"

namespace PadFx
{
    enum { Freeze = 0, Stutter8, Stutter16, Reverse, TapeStop, OctUp, OctDown, Cloud, Robot, Riser, Glide, Chaos, Count };
    inline const char* name (int i)
    {
        static const char* n[Count] = { "Freeze", "Stutter 1/8", "Stutter 1/16", "Reverse", "Tape Stop", "Octave Up",
                                        "Octave Down", "Granular Cloud", "Robot", "Riser", "Glide", "Chaos" };
        return n[juce::jlimit (0, Count - 1, i)];
    }
}

class HarmonizerProcessor : public juce::AudioProcessor
{
public:
    HarmonizerProcessor();
    ~HarmonizerProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Glizzarine Harmonizer"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return (int) harmPresets().size(); }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override
    { return juce::isPositiveAndBelow (index, (int) harmPresets().size()) ? juce::String (harmPresets()[(size_t) index].name) : juce::String(); }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    void tapTempo() { ++tapRequests; }

    // MIDI learn for knobs (right click) and pad learn
    void startCcLearn (const juce::String& paramID);
    void forgetCc (const juce::String& paramID);
    int ccFor (const juce::String& paramID) const;
    void startPadLearn (int fx) { padLearn = fx; }
    int padLearning() const { return padLearn.load(); }
    juce::String padLabel (int fx) const;
    bool padActive (int fx) const { return juce::isPositiveAndBelow (fx, (int) PadFx::Count) && padOn[(size_t) fx].load(); }
    void triggerPadFromUi (int fx, bool on) { uiPadRequest[(size_t) fx] = on ? 1 : 2; }

    // for the display
    std::atomic<float> detectedHz { 0.0f };
    std::atomic<bool> singing { false };
    std::atomic<int> heldVoices { 0 };
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> aftertouchShown { 0.0f };
    std::atomic<float> bpmShown { 120.0f };
    std::atomic<int> latencySamples { 0 };
    double getCurrentSampleRate() const { return currentSampleRate; }

private:
    void processChunk (float* L, float* R, const float* const* inputs, int inCh, int n, const juce::MidiBuffer& midi, int offset);
    void handleMidi (const juce::MidiMessage& m);
    void setPad (int fx, bool on);
    float val (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    bool on (const char* id) const { return val (id) > 0.5f; }
    double currentBpm();

    HarmonizerEngine engine;
    Sequencer seq { engine };
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Phaser<float> phaser;
    juce::dsp::StateVariableTPTFilter<float> atFilter, toneFilter;
    juce::dsp::IIR::Filter<float> warmL, warmR;
    AutoWah wah;
    LoFi lofi;
    PingPongDelay delay;
    ShimmerReverb reverb;
    WeirdFX weird;
    Limiter limiter;
    int currentPreset = 0;

    static constexpr int maxChunk = 1024;   // every effect is prepared for this and fed in pieces of it
    std::vector<float> mono, hL, hR, dryV, dryD, sL, sR;
    std::vector<std::pair<int, juce::MidiMessage>> events;
    juce::LinearSmoothedValue<float> outGain, inGain;
    double currentSampleRate = 44100.0;
    float lastLowest = -1.0f, lastWarmth = -1.0f;
    float aftertouch = 0.0f, atSmooth = 0.0f, toneSm = 20000.0f;
    double tremPhase = 0.0, bpmNow = 120.0;

    double lastTapMs = 0.0;
    std::atomic<int> tapRequests { 0 };
    std::vector<double> taps;

    std::array<std::atomic<int>, 128> ccMap;           // CC number -> parameter index (-1 = none)
    std::atomic<int> ccLearn { -1 };
    std::array<std::atomic<int>, PadFx::Count> padMap;   // channel * 128 + note (-1 = none)
    std::array<std::atomic<bool>, PadFx::Count> padOn;
    std::array<std::atomic<int>, PadFx::Count> uiPadRequest;
    std::atomic<int> padLearn { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HarmonizerProcessor)
};
