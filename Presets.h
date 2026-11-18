#pragma once
// Harmonizer presets. The sound always comes from your voice; a preset sets everything else.
// Values are plain parameter values. Anything not listed goes back to its default,
// except your setup (input, sensitivity, gate, lowest note, tempo, output, solo timing).
#include <vector>
#include <utility>

struct HarmPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

inline const std::vector<HarmPreset>& harmPresets()
{
    static const std::vector<HarmPreset> p = {
        // ---------- Harmonizer ----------
        { "Collier Choir", {
            { "dry", 0.8f }, { "harmony", 0.9f }, { "leveling", 0.5f }, { "follow", 0.6f }, { "width", 0.8f },
            { "chorus_mix", 0.2f }, { "reverb", 0.35f }, { "size", 0.8f }, { "release", 0.8f }, { "sustain", 4.0f } } },
        { "Close Harmony (dry)", {
            { "dry", 0.9f }, { "harmony", 0.8f }, { "follow", 0.8f }, { "width", 0.5f }, { "chorus_mix", 0.0f },
            { "reverb", 0.15f }, { "size", 0.4f }, { "release", 0.3f }, { "sustain", 1.5f },
            { "at_shimmer_on", 0 }, { "at_reverb_on", 0 } } },
        { "Cathedral Shimmer Choir", {
            { "dry", 0.7f }, { "harmony", 1.0f }, { "leveling", 0.6f }, { "width", 1.0f }, { "attack", 120.0f },
            { "release", 2.0f }, { "sustain", 6.0f }, { "reverb", 0.5f }, { "size", 0.95f },
            { "shimmer", 0.3f }, { "chorus_mix", 0.25f } } },
        { "Ghost Choir", {
            { "formant", -0.35f }, { "follow", 0.3f }, { "width", 1.0f }, { "attack", 150.0f }, { "release", 2.5f },
            { "phaser_mix", 0.35f }, { "phaser_rate", 0.12f }, { "reverb", 0.55f }, { "size", 0.95f }, { "shimmer", 0.2f },
            { "at_detune_on", 1 }, { "at_detune", 0.5f } } },
        { "Deep Octave Pad", {
            { "suboct", 0.6f }, { "warmth", 0.35f }, { "attack", 200.0f }, { "release", 2.5f }, { "sustain", 6.0f },
            { "chorus_mix", 0.3f }, { "reverb", 0.4f }, { "dry", 0.6f } } },
        { "Envelope Filter Funk", {
            { "wah_mix", 0.7f }, { "wah_sens", 0.6f }, { "wah_range", 0.75f }, { "release", 0.2f }, { "sustain", 0.0f },
            { "chorus_mix", 0.0f }, { "delay_mix", 0.2f }, { "delay_sync", 2 }, { "feedback", 0.25f }, { "reverb", 0.15f } } },
        { "Dark Filter Sweep", {
            { "tone", 900.0f }, { "warmth", 0.3f }, { "phaser_mix", 0.4f }, { "phaser_rate", 0.08f }, { "phaser_depth", 0.8f },
            { "attack", 100.0f }, { "release", 1.5f }, { "reverb", 0.4f }, { "at_filter_on", 1 }, { "at_filter", 0.8f } } },
        { "Bedroom Lo-Fi", {
            { "warmth", 0.3f }, { "lofi_on", 1 }, { "lofi_mix", 0.5f }, { "lofi_amount", 0.4f },
            { "trem_on", 1 }, { "trem_depth", 0.2f }, { "trem_rate", 1 }, { "reverb", 0.25f }, { "size", 0.5f }, { "chorus_mix", 0.3f } } },
        { "Robot Voices", {
            { "follow", 0.0f }, { "formant", 0.3f }, { "consonants", 0.1f }, { "chorus_mix", 0.0f },
            { "lofi_on", 1 }, { "lofi_mix", 0.3f }, { "lofi_amount", 0.35f }, { "reverb", 0.2f },
            { "trem_on", 1 }, { "trem_depth", 0.25f }, { "trem_rate", 2 } } },

        // ---------- Pyramid (clear: one voice enters per step, no echo, short room) ----------
        { "Pyramid Clear", {
            { "arp_mode", 1 }, { "arp_order", 0 }, { "arp_rate", 1 }, { "arp_style", 0 }, { "arp_release", 0.6f },
            { "arp_spread", 0.8f }, { "chorus_mix", 0.0f }, { "reverb", 0.2f }, { "size", 0.6f }, { "dry", 0.7f } } },
        { "Pyramid Swell", {
            { "arp_mode", 1 }, { "arp_order", 0 }, { "arp_rate", 0 }, { "arp_style", 2 }, { "arp_release", 1.0f },
            { "arp_spread", 0.9f }, { "chorus_mix", 0.1f }, { "reverb", 0.3f }, { "size", 0.8f } } },
        { "Pyramid Top-Down", {
            { "arp_mode", 1 }, { "arp_order", 1 }, { "arp_rate", 1 }, { "arp_style", 0 }, { "arp_release", 0.5f },
            { "arp_spread", 0.8f }, { "chorus_mix", 0.0f }, { "reverb", 0.2f }, { "size", 0.6f } } },
        { "Pyramid Outside-In", {
            { "arp_mode", 1 }, { "arp_order", 5 }, { "arp_rate", 1 }, { "arp_style", 1 }, { "arp_release", 0.7f },
            { "arp_spread", 1.0f }, { "width", 1.0f }, { "chorus_mix", 0.0f }, { "reverb", 0.25f }, { "shimmer", 0.1f } } },

        // ---------- Arp (dry-ish, short release, so every step is audible) ----------
        { "Arp Pluck 1/16", {
            { "arp_mode", 2 }, { "arp_order", 0 }, { "arp_rate", 2 }, { "arp_steps", 8 }, { "arp_gate", 0.5f },
            { "arp_style", 0 }, { "arp_release", 0.12f }, { "arp_spread", 0.6f }, { "dry", 0.6f },
            { "chorus_mix", 0.0f }, { "reverb", 0.15f }, { "size", 0.5f } } },
        { "Arp Up-Down Ping-Pong", {
            { "arp_mode", 2 }, { "arp_order", 2 }, { "arp_rate", 1 }, { "arp_steps", 8 }, { "arp_gate", 0.5f },
            { "arp_style", 0 }, { "arp_release", 0.15f }, { "arp_spread", 0.9f }, { "dry", 0.6f }, { "chorus_mix", 0.0f },
            { "delay_mix", 0.2f }, { "delay_sync", 3 }, { "feedback", 0.25f }, { "reverb", 0.2f } } },
        { "Arp Legato Crawl", {
            { "arp_mode", 2 }, { "arp_order", 4 }, { "arp_rate", 1 }, { "arp_steps", 16 }, { "arp_gate", 0.9f },
            { "arp_style", 1 }, { "arp_release", 0.2f }, { "arp_spread", 0.5f }, { "chorus_mix", 0.1f }, { "reverb", 0.25f } } },
        { "Arp Swell Triplets", {
            { "arp_mode", 2 }, { "arp_order", 6 }, { "arp_rate", 4 }, { "arp_steps", 6 }, { "arp_gate", 0.8f },
            { "arp_style", 2 }, { "arp_release", 0.3f }, { "arp_spread", 0.8f }, { "phaser_mix", 0.25f }, { "reverb", 0.3f } } },
        { "Arp 2 Octaves Random", {
            { "arp_mode", 2 }, { "arp_order", 7 }, { "arp_rate", 2 }, { "arp_steps", 16 }, { "arp_oct", 2 }, { "arp_gate", 0.4f },
            { "arp_style", 0 }, { "arp_release", 0.1f }, { "arp_spread", 1.0f }, { "chorus_mix", 0.0f }, { "reverb", 0.2f } } },

        // ---------- Vocoder ----------
        { "Vocoder Saw Pad", {
            { "blend", 1.0f }, { "voc_type", 0 }, { "dry", 0.4f }, { "attack", 30.0f }, { "release", 0.8f },
            { "chorus_mix", 0.3f }, { "reverb", 0.3f } } },
        { "Vocoder Strings", {
            { "blend", 1.0f }, { "voc_type", 1 }, { "voc_resp", 0.5f }, { "dry", 0.3f }, { "attack", 120.0f }, { "release", 1.5f },
            { "chorus_mix", 0.2f }, { "reverb", 0.4f }, { "size", 0.9f } } },
        { "Vocoder Glass Shimmer", {
            { "blend", 1.0f }, { "voc_type", 2 }, { "voc_shift", 0.3f }, { "dry", 0.3f }, { "release", 1.2f },
            { "reverb", 0.45f }, { "shimmer", 0.35f } } },
        { "Hybrid Choir + Vocoder", {
            { "blend", 0.45f }, { "voc_type", 0 }, { "dry", 0.7f }, { "chorus_mix", 0.2f }, { "reverb", 0.35f } } },
        { "Noise Choir Whisper", {
            { "blend", 1.0f }, { "voc_type", 4 }, { "voc_sib", 0.7f }, { "dry", 0.2f }, { "release", 1.5f },
            { "phaser_mix", 0.3f }, { "reverb", 0.5f }, { "size", 0.95f } } },
        { "Hollow Robot Arp", {
            { "blend", 1.0f }, { "voc_type", 3 }, { "arp_mode", 2 }, { "arp_order", 2 }, { "arp_rate", 2 }, { "arp_steps", 8 },
            { "arp_style", 0 }, { "arp_release", 0.12f }, { "dry", 0.4f }, { "chorus_mix", 0.0f }, { "reverb", 0.2f } } },
    };
    return p;
}

inline bool isSetupParam (const juce::String& id)
{
    return id == "input" || id == "sensitivity" || id == "gate" || id == "lowest" || id == "tempo_src"
        || id == "bpm" || id == "output" || id == "dry_timing" || id == "pad_latch";
}
