#include "PluginEditor.h"

//==============================================================================
HarmLookAndFeel::HarmLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Theme::bg);
    setColour (juce::Label::textColourId, Theme::text);
    setColour (juce::Slider::textBoxTextColourId, Theme::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId, Theme::bg);
    setColour (juce::ComboBox::outlineColourId, Theme::track);
    setColour (juce::ComboBox::textColourId, Theme::text);
    setColour (juce::ComboBox::arrowColourId, Theme::textDim);
    setColour (juce::PopupMenu::backgroundColourId, Theme::panel);
    setColour (juce::PopupMenu::textColourId, Theme::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::accent.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, Theme::text);
    setColour (juce::TextButton::buttonColourId, Theme::panel);
    setColour (juce::TextButton::textColourOffId, Theme::text);
    setColour (juce::TabbedComponent::backgroundColourId, Theme::bg);
    setColour (juce::TabbedComponent::outlineColourId, juce::Colours::transparentBlack);
}

void HarmLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float start, float end, juce::Slider&)
{
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = start + pos * (end - start);

    juce::Path bgArc, valArc;
    bgArc.addCentredArc (c.x, c.y, r - 3.0f, r - 3.0f, 0.0f, start, end, true);
    valArc.addCentredArc (c.x, c.y, r - 3.0f, r - 3.0f, 0.0f, start, angle, true);
    g.setColour (Theme::track);
    g.strokePath (bgArc, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (Theme::accent);
    g.strokePath (valArc, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (Theme::panel.brighter (0.15f));
    g.fillEllipse (c.x - r * 0.62f, c.y - r * 0.62f, r * 1.24f, r * 1.24f);
    juce::Path tick;
    tick.addRoundedRectangle (-1.5f, -r * 0.6f, 3.0f, r * 0.32f, 1.5f);
    g.setColour (Theme::text);
    g.fillPath (tick, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

void HarmLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool onState = b.getToggleState();
    g.setColour (onState ? Theme::accent : Theme::bg);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (onState ? Theme::accent : (over ? Theme::textDim : Theme::track));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    g.setColour (onState ? Theme::bg : Theme::text);
    g.setFont (juce::Font (juce::FontOptions (12.5f, juce::Font::bold)));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

void HarmLookAndFeel::drawTabButton (juce::TabBarButton& b, juce::Graphics& g, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (3.0f, 4.0f);
    const bool front = b.isFrontTab();
    g.setColour (front ? Theme::panel : Theme::bg);
    g.fillRoundedRectangle (r, 6.0f);
    if (front) { g.setColour (Theme::accent); g.fillRect (r.removeFromBottom (2.0f)); }
    g.setColour (front ? Theme::accent : (over ? Theme::text : Theme::textDim));
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

//==============================================================================
Knob::Knob (HarmonizerProcessor& p, const juce::String& id, const juce::String& name)
{
    auto& state = p.apvts;
    slider.proc = &p; slider.id = id;
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 16);
    addAndMakeVisible (slider);
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (12.0f)));
    label.setColour (juce::Label::textColourId, Theme::textDim);
    addAndMakeVisible (label);
    attachment = std::make_unique<APVTS::SliderAttachment> (state, id, slider);
}

void Knob::LearnSlider::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu() || proc == nullptr) { juce::Slider::mouseDown (e); return; }
    const int cc = proc->ccFor (id);
    juce::PopupMenu m;
    m.addSectionHeader (cc >= 0 ? "MIDI CC " + juce::String (cc) : "no MIDI CC");
    m.addItem (1, "MIDI Learn (move a knob on your keyboard next)");
    m.addItem (2, "Forget MIDI CC", cc >= 0);
    juce::Component::SafePointer<LearnSlider> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int r)
    {
        if (safe == nullptr) return;
        if (r == 1) safe->proc->startCcLearn (safe->id);
        if (r == 2) safe->proc->forgetCc (safe->id);
    });
}

void Knob::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromTop (16));
    slider.setBounds (b);
}

Toggle::Toggle (APVTS& state, const juce::String& id, const juce::String& name)
{
    button.setButtonText (name);
    addAndMakeVisible (button);
    attachment = std::make_unique<APVTS::ButtonAttachment> (state, id, button);
}

Choice::Choice (APVTS& state, const juce::String& id, const juce::String& name)
{
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter (id)))
        box.addItemList (c->choices, 1);
    addAndMakeVisible (box);
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (12.0f)));
    label.setColour (juce::Label::textColourId, Theme::textDim);
    addAndMakeVisible (label);
    attachment = std::make_unique<APVTS::ComboBoxAttachment> (state, id, box);
}

void Choice::resized()
{
    auto b = getLocalBounds();
    label.setBounds (b.removeFromTop (16));
    box.setBounds (b.withSizeKeepingCentre (b.getWidth() - 4, 26));
}

void Section::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (Theme::edge);
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
    g.setColour (Theme::accent);
    g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    g.drawText (title, 12, 6, getWidth() - 24, 16, juce::Justification::centredLeft);
}

void Section::resized()
{
    auto r = getLocalBounds().reduced (8).withTrimmedTop (18);
    float total = 0.0f; for (auto& it : items) total += it.w;
    if (total <= 0.0f) return;
    const float full = (float) r.getWidth();
    for (auto& it : items) it.c->setBounds (r.removeFromLeft ((int) (full * it.w / total)).reduced (2));
}

void Page::resized()
{
    auto r = getLocalBounds().reduced (6);
    const int gap = 8;
    const int count = (int) rows.size();
    if (count == 0) return;
    int fixed = 0, flexible = 0;
    for (int i = 0; i < count; ++i)
        if (i < (int) rowHeights.size() && rowHeights[(size_t) i] > 0) fixed += rowHeights[(size_t) i]; else ++flexible;
    const int avail = r.getHeight() - gap * (count - 1) - fixed;
    for (int i = 0; i < count; ++i)
    {
        const int h = (i < (int) rowHeights.size() && rowHeights[(size_t) i] > 0) ? rowHeights[(size_t) i] : avail / juce::jmax (1, flexible);
        rows[(size_t) i]->setBounds (r.removeFromTop (h));
        r.removeFromTop (gap);
    }
}

//==============================================================================
PadCell::PadCell (HarmonizerProcessor& p, int fxIndex) : proc (p), fx (fxIndex)
{
    learn.onClick = [this] { proc.startPadLearn (proc.padLearning() == fx ? -1 : fx); };
    addAndMakeVisible (learn);
    refresh();
}

void PadCell::refresh()
{
    const bool l = proc.padActive (fx), le = proc.padLearning() == fx;
    const auto m = proc.padLabel (fx);
    if (l != lit || le != learning || m != mapping) { lit = l; learning = le; mapping = m; repaint(); }
}

void PadCell::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (3.0f);
    g.setColour (lit ? Theme::accent : Theme::bg);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (learning ? juce::Colours::red : (lit ? Theme::accent : Theme::track));
    g.drawRoundedRectangle (r, 8.0f, learning ? 2.0f : 1.0f);
    g.setColour (lit ? Theme::bg : Theme::text);
    g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
    g.drawText (PadFx::name (fx), getLocalBounds().reduced (8).withHeight (22), juce::Justification::centredLeft);
    g.setColour (lit ? Theme::bg : Theme::textDim);
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText (learning ? "hit a pad now..." : mapping, getLocalBounds().reduced (8).withTrimmedTop (24).withHeight (18),
                juce::Justification::centredLeft);
}

void PadCell::resized() { learn.setBounds (getWidth() - 70, getHeight() - 32, 62, 24); }
void PadCell::mouseDown (const juce::MouseEvent&) { proc.triggerPadFromUi (fx, ! (proc.apvts.getRawParameterValue ("pad_latch")->load() > 0.5f && proc.padActive (fx))); }
void PadCell::mouseUp (const juce::MouseEvent&) { if (proc.apvts.getRawParameterValue ("pad_latch")->load() < 0.5f) proc.triggerPadFromUi (fx, false); }

//==============================================================================
HarmonizerEditor::HarmonizerEditor (HarmonizerProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    auto add = [this] (const char* name, void (HarmonizerEditor::*build) (Page&))
    {
        auto* pg = new Page(); (this->*build) (*pg);
        tabs.addTab (name, Theme::bg, pg, true);
    };
    add ("MAIN", &HarmonizerEditor::buildMain);
    add ("VOICE", &HarmonizerEditor::buildVoice);
    add ("VOCODER", &HarmonizerEditor::buildVocoder);
    add ("ARP", &HarmonizerEditor::buildArp);
    add ("EFFECTS", &HarmonizerEditor::buildFx);
    add ("PADS", &HarmonizerEditor::buildPads);
    add ("AT & VEL", &HarmonizerEditor::buildAtVel);
    tabs.setTabBarDepth (34);
    tabs.setOutline (0);
    addAndMakeVisible (tabs);

    tapButton.onClick = [this] { proc.tapTempo(); };
    addAndMakeVisible (tapButton);

    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { loadPreset (presetBox.getSelectedId() - 1); };
    prevPreset.onClick = [this] { loadPreset ((proc.getCurrentProgram() + proc.getNumPrograms() - 1) % proc.getNumPrograms()); };
    nextPreset.onClick = [this] { loadPreset ((proc.getCurrentProgram() + 1) % proc.getNumPrograms()); };
    addAndMakeVisible (presetBox); addAndMakeVisible (prevPreset); addAndMakeVisible (nextPreset);

    setSize (1200, 640);
    startTimerHz (20);
}

void HarmonizerEditor::loadPreset (int index)
{
    if (index < 0 || index >= proc.getNumPrograms()) return;
    proc.setCurrentProgram (index);
    proc.updateHostDisplay (juce::AudioProcessorListener::ChangeDetails().withProgramChanged (true));
    presetBox.setSelectedId (index + 1, juce::dontSendNotification);
}

HarmonizerEditor::~HarmonizerEditor()
{
    setLookAndFeel (nullptr);
}

juce::Label* HarmonizerEditor::info (Page& pg, const juce::String& text)
{
    auto* l = pg.make<juce::Label> ("", text);
    l->setColour (juce::Label::textColourId, Theme::textDim);
    l->setFont (juce::Font (juce::FontOptions (13.0f)));
    l->setJustificationType (juce::Justification::topLeft);
    l->setMinimumHorizontalScale (1.0f);
    return l;
}

void HarmonizerEditor::buildMain (Page& pg)
{
    auto& s = proc.apvts;
    auto* a = pg.section ("SOUND");
    a->add (knob (pg, "blend", "Harmonizer <> Vocoder"), 1.3f);
    a->add (knob (pg, "dry", "Solo Voice"));
    a->add (knob (pg, "harmony", "Harmonies"));
    a->add (knob (pg, "attack", "Attack"));
    a->add (knob (pg, "release", "Release"));
    a->add (knob (pg, "sustain", "Pedal Sustain"));
    a->add (knob (pg, "output", "Output"));

    auto* b = pg.section ("FILTERS & EFFECTS  -  right click any knob for MIDI learn");
    b->add (knob (pg, "wah_mix", "Env Filter"));
    b->add (knob (pg, "wah_range", "Env Range"));
    b->add (knob (pg, "tone", "Filter"));
    b->add (knob (pg, "chorus_mix", "Motion"));
    b->add (knob (pg, "phaser_mix", "Swirl"));
    b->add (knob (pg, "delay_mix", "Delay"));
    b->add (knob (pg, "reverb", "Reverb"));
    b->add (knob (pg, "shimmer", "Shimmer"));

    auto* c = pg.section ("PYRAMID / ARP  -  more on the ARP tab");
    c->add (pg.make<Choice> (s, "arp_mode", "Mode"));
    c->add (pg.make<Choice> (s, "arp_order", "Order"));
    c->add (pg.make<Choice> (s, "arp_style", "Sound"));
    c->add (pg.make<Choice> (s, "arp_rate", "Rate"));
    c->add (pg.make<Choice> (s, "voc_type", "Vocoder Sound"));
}

void HarmonizerEditor::buildVoice (Page& pg)
{
    auto& s = proc.apvts;
    auto* a = pg.section ("INPUT  -  watch the meter: it should move into the upper half when you sing");
    a->add (knob (pg, "input", "Input Gain"));
    a->add (knob (pg, "sensitivity", "Sensitivity"));
    a->add (knob (pg, "gate", "Soft Gate"));
    a->add (knob (pg, "leveling", "Leveling"));
    a->add (pg.make<Choice> (s, "lowest", "Lowest note"), 1.4f);
    a->add (pg.make<Choice> (s, "dry_timing", "Solo Voice"), 1.4f);

    auto* b = pg.section ("CHARACTER");
    b->add (knob (pg, "formant", "Formant"));
    b->add (knob (pg, "consonants", "Consonants"));
    b->add (knob (pg, "follow", "Follow Pitch"));
    b->add (knob (pg, "warmth", "Warmth"));
    b->add (knob (pg, "suboct", "Octave Down"));
    b->add (knob (pg, "width", "Width"));

    auto* c = pg.section ("TIPS");
    c->add (info (pg, "Sensitivity: lower (more to the left) = it reacts to quieter singing.  Soft Gate: lower = less of the room noise is cut, "
                      "the voice keeps more character.\nLeveling: evens out loud and quiet singing.  Follow Pitch: the harmonies follow your "
                      "bends and vibrato."));
    pg.rowHeights = { 0, 0, 80 };
}

void HarmonizerEditor::buildVocoder (Page& pg)
{
    auto& s = proc.apvts;
    auto* a = pg.section ("VOCODER  -  a synth that speaks with your voice (turn Harmonizer <> Vocoder to the right)");
    a->add (knob (pg, "blend", "Harmonizer <> Vocoder"), 1.3f);
    a->add (pg.make<Choice> (s, "voc_type", "Sound"), 1.4f);
    a->add (knob (pg, "voc_shift", "Formant"));
    a->add (knob (pg, "voc_sib", "S / T sounds"));
    a->add (knob (pg, "voc_resp", "Smoothness"));
    a->add (knob (pg, "voc_width", "Width"));

    auto* b = pg.section ("HOW IT WORKS");
    b->add (info (pg, "Harmonizer = the voices are your real voice, moved to the keys you hold. It needs a sung note.\n"
                      "Vocoder = a synth plays the keys and your voice only shapes it (vowels, words). It works even when you whisper or talk.\n"
                      "In the middle you get both. The same Attack / Release / Arp settings apply to both."));
    pg.rowHeights = { 0, 110 };
}

void HarmonizerEditor::buildArp (Page& pg)
{
    auto& s = proc.apvts;
    auto* a = pg.section ("PATTERN");
    a->add (pg.make<Choice> (s, "arp_mode", "Mode"), 1.2f);
    a->add (pg.make<Choice> (s, "arp_order", "Order"), 1.3f);
    a->add (pg.make<Choice> (s, "arp_rate", "Rate"));
    a->add (knob (pg, "arp_steps", "Steps"));
    a->add (knob (pg, "arp_oct", "Octaves"));
    a->add (knob (pg, "arp_gate", "Note Length"));

    auto* b = pg.section ("SOUND OF EACH NOTE");
    b->add (pg.make<Choice> (s, "arp_style", "Style"), 1.2f);
    b->add (knob (pg, "arp_release", "Arp Release"));
    b->add (knob (pg, "arp_spread", "Stereo Spread"));
    b->add (pg.make<Choice> (s, "tempo_src", "Tempo"));
    b->add (knob (pg, "bpm", "Own BPM"));

    auto* c = pg.section ("TIPS");
    c->add (info (pg, "Pyramid: hold a chord, the voices come in one after another and stay. Arp: the voices take turns.\n"
                      "Pluck = short and clear, Legato = smooth, Swell = each note fades in.  For a clean pattern keep Arp Release short "
                      "and Delay / Reverb low.  The pyramid uses Arp Release when you let go, not the chord Release."));
    pg.rowHeights = { 0, 0, 90 };
}

void HarmonizerEditor::buildFx (Page& pg)
{
    auto& s = proc.apvts;
    auto* a = pg.section ("FILTERS  -  only on the harmonies, your solo voice stays clean");
    a->add (pg.make<Toggle> (s, "wah_on", "ENV FILTER"), 0.9f);
    a->add (knob (pg, "wah_mix", "Mix")); a->add (knob (pg, "wah_sens", "Sensitivity")); a->add (knob (pg, "wah_range", "Range"));
    a->add (knob (pg, "tone", "Lowpass"));
    a->add (pg.make<Toggle> (s, "trem_on", "TREMOLO"), 0.9f);
    a->add (knob (pg, "trem_depth", "Depth")); a->add (pg.make<Choice> (s, "trem_rate", "Rate"), 1.1f);
    a->add (pg.make<Toggle> (s, "lofi_on", "LO-FI"), 0.9f);
    a->add (knob (pg, "lofi_mix", "Mix")); a->add (knob (pg, "lofi_amount", "Amount"));

    auto* b = pg.section ("MOTION");
    b->add (pg.make<Toggle> (s, "chorus_on", "CHORUS"), 0.9f);
    b->add (knob (pg, "chorus_mix", "Mix")); b->add (knob (pg, "chorus_rate", "Rate")); b->add (knob (pg, "chorus_depth", "Depth"));
    b->add (pg.make<Toggle> (s, "phaser_on", "PHASER"), 0.9f);
    b->add (knob (pg, "phaser_mix", "Mix")); b->add (knob (pg, "phaser_rate", "Rate")); b->add (knob (pg, "phaser_depth", "Depth"));

    auto* c = pg.section ("SPACE");
    c->add (pg.make<Toggle> (s, "delay_on", "DELAY"), 0.9f);
    c->add (knob (pg, "delay_mix", "Mix")); c->add (pg.make<Choice> (s, "delay_sync", "Sync"), 1.2f);
    c->add (knob (pg, "delay_time", "Free Time")); c->add (knob (pg, "feedback", "Feedback"));
    c->add (pg.make<Toggle> (s, "reverb_on", "REVERB"), 0.9f);
    c->add (knob (pg, "reverb", "Mix")); c->add (knob (pg, "size", "Size"));
    c->add (pg.make<Toggle> (s, "shimmer_on", "SHIMMER"), 0.9f);
    c->add (knob (pg, "shimmer", "Amount"));
}

void HarmonizerEditor::buildPads (Page& pg)
{
    auto& s = proc.apvts;
    auto* top = pg.section ("KEYLAB PADS  -  hold a pad = effect on, let go = off.  Default: pads on MIDI channel 10, notes C1 upward");
    top->add (pg.make<Toggle> (s, "pad_latch", "LATCH"), 0.8f);
    top->add (knob (pg, "glide_time", "Glide Time"), 0.8f);
    top->add (info (pg, "LATCH: tap once = on, tap again = off.\nLEARN: click LEARN, then hit the pad you want. Click a pad here to try it with the mouse."), 4.0f);
    for (int row = 0; row < 3; ++row)
    {
        auto* r = pg.section ("");
        for (int i = 0; i < 4; ++i)
        {
            auto* cell = pg.make<PadCell> (proc, row * 4 + i);
            padCells.push_back (cell);
            r->add (cell);
        }
    }
    pg.rowHeights = { 110, 0, 0, 0 };
}

void HarmonizerEditor::buildAtVel (Page& pg)
{
    auto& s = proc.apvts;
    auto* m = pg.section ("AFTERTOUCH  -  hold a key and press it down harder");
    m->add (pg.make<Toggle> (s, "at_on", "ON"), 0.7f);
    m->add (knob (pg, "at_response", "Response"));
    m->add (pg.make<Toggle> (s, "at_swell_on", "LOUDER"), 0.8f);   m->add (knob (pg, "at_swell", "Amount"));
    m->add (pg.make<Toggle> (s, "at_shimmer_on", "SHIMMER"), 0.8f); m->add (knob (pg, "at_shimmer", "Amount"));
    m->add (pg.make<Toggle> (s, "at_reverb_on", "REVERB"), 0.8f);  m->add (knob (pg, "at_reverb", "Amount"));

    auto* b = pg.section ("AFTERTOUCH COLOUR");
    b->add (pg.make<Toggle> (s, "at_filter_on", "FILTER"), 0.8f);  b->add (knob (pg, "at_filter", "Amount"));
    b->add (pg.make<Toggle> (s, "at_vib_on", "VIBRATO"), 0.8f);    b->add (knob (pg, "at_vib", "Amount"));
    b->add (pg.make<Toggle> (s, "at_detune_on", "DETUNE"), 0.8f);  b->add (knob (pg, "at_detune", "Amount"));
    b->add (pg.make<Toggle> (s, "at_formant_on", "FORMANT"), 0.8f); b->add (knob (pg, "at_formant", "Amount"));

    auto* v = pg.section ("VELOCITY  -  how hard you hit the key");
    v->add (knob (pg, "velocity", "To Volume"));
    v->add (pg.make<Toggle> (s, "vel_bright_on", "BRIGHTNESS"), 0.9f); v->add (knob (pg, "vel_bright", "Amount"));
    v->add (pg.make<Toggle> (s, "vel_attack_on", "SOFT ATTACK"), 0.9f); v->add (knob (pg, "vel_attack", "Amount"));
    v->add (pg.make<Toggle> (s, "vel_send_on", "ECHO SEND"), 0.9f);    v->add (knob (pg, "vel_send", "Amount"));

    auto* c = pg.section ("WHAT IT DOES");
    c->add (info (pg, "Aftertouch = pressure AFTER the hit. The bar on the left shows it. If it stays empty, your keyboard sends no aftertouch "
                      "(turn it on in the KeyLab settings / Arturia MIDI Control Center).\n"
                      "Velocity = how hard you hit. Brightness: soft = warm, hard = open.  Soft attack: soft = fades in.  Echo send: hard keys go into the delay."));
    pg.rowHeights = { 0, 0, 0, 80 };
}

juce::String HarmonizerEditor::noteName (float hz)
{
    if (hz <= 0.0f) return "-";
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const int note = (int) std::lround (69.0 + 12.0 * std::log2 (hz / 440.0));
    return juce::String (names[((note % 12) + 12) % 12]) + juce::String (note / 12 - 1);
}

void HarmonizerEditor::timerCallback()
{
    statusSinging = proc.singing.load();
    statusNote = statusSinging ? noteName (proc.detectedHz.load()) : "-";
    statusVoices = juce::String (proc.heldVoices.load());
    statusLevel = proc.inputLevel.load();
    statusAt = proc.aftertouchShown.load();
    statusBpm = proc.bpmShown.load();
    for (auto* p : padCells) p->refresh();
    repaint (secStatus);
}

void HarmonizerEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::bg);
    g.setColour (Theme::text);
    g.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    g.drawText ("GLIZZARINE", 18, 10, 150, 26, juce::Justification::centredLeft);
    g.setColour (Theme::accent);
    g.drawText ("HARMONIZER", 150, 10, 200, 26, juce::Justification::centredLeft);
    g.setColour (Theme::textDim);
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText (juce::String (GLZH_VERSION) + "   build " + __DATE__, 290, 14, 220, 20, juce::Justification::centredLeft);

    // status column
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (secStatus.toFloat(), 8.0f);
    g.setColour (Theme::edge);
    g.drawRoundedRectangle (secStatus.toFloat().reduced (0.5f), 8.0f, 1.0f);
    g.setColour (Theme::accent);
    g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    g.drawText ("INPUT", secStatus.getX() + 12, secStatus.getY() + 6, 100, 16, juce::Justification::centredLeft);

    auto s = secStatus.reduced (12).withTrimmedTop (20);
    auto meterArea = s.removeFromLeft (14);
    auto meter = meterArea.removeFromTop (200);
    g.setColour (Theme::track);
    g.fillRoundedRectangle (meter.toFloat(), 3.0f);
    const float lvl = juce::jlimit (0.0f, 1.0f, 1.0f + juce::Decibels::gainToDecibels (statusLevel + 1.0e-6f) / 60.0f);
    auto filled = meter.toFloat();
    filled = filled.removeFromBottom (filled.getHeight() * lvl);
    g.setColour (statusLevel > 0.9f ? juce::Colours::red : Theme::accent);
    g.fillRoundedRectangle (filled, 3.0f);

    s.removeFromLeft (12);
    int y = s.getY();
    g.setColour (statusSinging ? Theme::accent : Theme::track);
    g.fillEllipse ((float) s.getX(), (float) y + 4.0f, 10.0f, 10.0f);
    g.setColour (Theme::textDim);
    g.setFont (juce::Font (juce::FontOptions (12.5f)));
    g.drawText (statusSinging ? "singing" : "silent", s.getX() + 16, y, 120, 18, juce::Justification::centredLeft);
    y += 30;
    g.drawText ("detected note", s.getX(), y, 140, 16, juce::Justification::centredLeft);
    g.setColour (Theme::text);
    g.setFont (juce::Font (juce::FontOptions (30.0f, juce::Font::bold)));
    g.drawText (statusNote, s.getX(), y + 16, 140, 36, juce::Justification::centredLeft);
    y += 62;
    g.setColour (Theme::textDim);
    g.setFont (juce::Font (juce::FontOptions (12.5f)));
    g.drawText ("keys held: " + statusVoices, s.getX(), y, 140, 16, juce::Justification::centredLeft);
    y += 20;
    const double ms = 1000.0 * proc.latencySamples.load() / juce::jmax (1.0, proc.getCurrentSampleRate());
    g.drawText ("latency: " + juce::String (ms, 1) + " ms", s.getX(), y, 140, 16, juce::Justification::centredLeft);
    y += 20;
    g.drawText ("tempo: " + juce::String (statusBpm, 1) + " BPM", s.getX(), y, 140, 16, juce::Justification::centredLeft);
    y += 30;
    g.drawText ("aftertouch", s.getX(), y, 140, 16, juce::Justification::centredLeft);
    y += 18;
    auto atBar = juce::Rectangle<float> ((float) s.getX(), (float) y, (float) s.getWidth(), 8.0f);
    g.setColour (Theme::track);
    g.fillRoundedRectangle (atBar, 3.0f);
    g.setColour (Theme::accent);
    g.fillRoundedRectangle (atBar.withWidth (atBar.getWidth() * juce::jlimit (0.0f, 1.0f, statusAt)), 3.0f);
}

void HarmonizerEditor::resized()
{
    auto top = getLocalBounds().reduced (12).removeFromTop (28);
    auto pr = top.removeFromRight (330);
    nextPreset.setBounds (pr.removeFromRight (30));
    pr.removeFromRight (4);
    presetBox.setBounds (pr.removeFromRight (260));
    pr.removeFromRight (4);
    prevPreset.setBounds (pr.removeFromRight (30));
    auto area = getLocalBounds().reduced (12).withTrimmedTop (34);
    secStatus = area.removeFromLeft (180);
    tapButton.setBounds (secStatus.getX() + 12, secStatus.getBottom() - 44, secStatus.getWidth() - 24, 30);
    area.removeFromLeft (10);
    tabs.setBounds (area);
}
