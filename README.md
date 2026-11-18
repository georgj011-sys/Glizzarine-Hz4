# Glizzarine Harmonizer

Live-Harmonizer: Du singst ins Mikro und spielst auf dem Keyboard Akkorde. Jede gedrückte Taste wird zu einer eigenen Stimme aus deiner Stimme, genau auf der Tonhöhe der Taste (wie bei Jacob Collier).

## Bauen (GitHub Actions)

1. Neues GitHub-Repository anlegen, z. B. `GlizzarineHarmonizer`.
2. Alle Dateien hochladen. `build.yml` muss in `.github/workflows/build.yml` liegen.
3. Unter **Actions** läuft der Build automatisch. Am Ende bei **Artifacts** `GlizzarineHarmonizer-Windows` herunterladen.
4. Darin sind zwei Sachen:
   - `VST3/Glizzarine Harmonizer.vst3`: nach `C:\Program Files\Common Files\VST3\` kopieren
   - `Standalone/Glizzarine Harmonizer.exe`: läuft ohne DAW

## Schnellster Test: Standalone

1. `Glizzarine Harmonizer.exe` starten, oben auf **Options → Audio/MIDI Settings**.
2. Audio-Gerät: dein Interface (am besten **ASIO**), Eingang = Mikro-Kanal, Ausgang = Kopfhörer.
3. Unter **Active MIDI inputs** die KeyLab anhaken.
4. Singen und Akkorde spielen.

## In Cakewalk Sonar

1. **Treiber:** Edit → Preferences → Audio → Playback and Recording → Driver Mode **ASIO**. Notfalls WASAPI Exclusive, nicht Shared (zu viel Verzögerung). Puffer klein (64–128 Samples).
2. **Gesangsspur:** Audiospur anlegen, Eingang = Mikro, **Input Echo** (Monitoring) an.
3. In die FX-Box dieser Spur **Glizzarine Harmonizer** einfügen.
4. **Keyboard-Spur:** MIDI-Spur anlegen, Eingang = KeyLab.
5. Den **Ausgang** der MIDI-Spur auf **Glizzarine Harmonizer** stellen. MIDI-fähige Effekte erscheinen dort in der Ausgangs-Liste.
6. Auch bei der MIDI-Spur **Input Echo** an, dann singen und spielen.

## Neu in v1.0

- Versionsnummer oben links.
- Klicks beim Anschlagen und Ausklingen behoben (Hauptursache: alte Grain-Reste kamen 0,19 s später wieder hoch; dazu ein Sprung im Stereo-Delay bei Arp/Pyramide).
- Abstürze: alle Effekte bekommen das Audio jetzt in festen Stücken von max. 1024 Samples, egal welche Puffergröße Sonar schickt. Zwei Instanzen parallel laufen getrennt.
- Eingang empfindlicher (Sensitivity bis -85 dB, Soft Gate bis -90 dB, nachsichtigere Tonerkennung).
- Vocoder-Schicht mit 5 Klängen, stufenlos mit dem Harmonizer mischbar.
- Arp/Pyramide: 9 Reihenfolgen, eigener Klang pro Note (Pluck/Legato/Swell), eigenes Release, damit der Akkord-Release nichts zuschmiert. Betonung und Stereo-Platz pro Stimme, damit man jeden Einsatz hört.
- 12 Pad-Effekte für die KeyLab-Pads (mit Learn).
- MIDI-Learn: Rechtsklick auf jeden Regler.

## Seiten

**Links (immer):** Pegel, Gesang erkannt, Ton, Tasten, Latenz, Tempo, Aftertouch-Balken, TAP.

- **MAIN:** Harmonizer↔Vocoder, Solo-Stimme, Harmonien, Attack, Release, Pedal-Sustain, Output. Darunter 8 Live-Regler für Filter und Effekte: Env Filter, Env Range, Filter, Motion, Swirl, Delay, Reverb, Shimmer. Unten Arp-Schnellwahl.
- **VOICE:** Input Gain, Sensitivity, Soft Gate, Leveling, tiefster Ton, Formant, Konsonanten, Follow Pitch, Warmth, Oktave tiefer, Breite.
- **VOCODER:** Klang (Saw Pad, Strings, Glass, Hollow Pulse, Noise Choir), Formant, S/T-Laute, Glätte, Breite.
- **ARP:** Mode (Off/Pyramid/Arp), Order (Up, Down, Up-Down, Down-Up, Crawl, Outside-In, Inside-Out, Random, As played), Rate, Steps, Octaves, Note Length, Style, Arp Release, Stereo Spread, Tempo.
- **EFFECTS:** Envelope Filter, Lowpass, Tremolo, Lo-Fi, Chorus, Phaser, Delay, Reverb, Shimmer. Alles außer Delay/Reverb nur auf den Harmonien.
- **PADS:** Freeze, Stutter 1/8, Stutter 1/16, Reverse, Tape Stop, Octave Up, Octave Down, Granular Cloud, Robot, Riser, Glide, Chaos. Standard: Pads auf MIDI-Kanal 10 ab C1 (Note 36). Sonst LEARN klicken und Pad drücken. LATCH = einmal tippen an, nochmal aus.
- **AT & VEL:** Aftertouch (Druck nach dem Anschlag: lauter, Shimmer, Hall, Filter, Vibrato, Detune, Formant) und Velocity (Lautstärke, Helligkeit, weicher Einsatz, Echo-Send).

## Aufnehmen in Sonar

Am besten die trockene Stimme und die MIDI-Spur aufnehmen, den Harmonizer als Effekt drauflassen. Dann kannst du später Akkorde und Sound noch ändern. Wenn es fertig ist: Spur einfrieren (Freeze) oder bouncen.

## Zwei Harmonizer parallel

Gesangsspur → zwei Sends auf zwei Busse. Auf jeden Bus einen Glizzarine Harmonizer. Für jeden eine eigene MIDI-Spur, deren Ausgang auf die jeweilige Instanz zeigt. Bei der Gesangsspur selbst den Ausgang leise oder Solo Voice in einer Instanz auf 0, sonst ist die Stimme doppelt.

## Presets

Collier Choir · Close Harmony (dry) · Cathedral Shimmer Choir · Ghost Choir · Deep Octave Pad · Envelope Filter Funk · Dark Filter Sweep · Bedroom Lo-Fi · Robot Voices · Pyramid Clear · Pyramid Swell · Pyramid Top-Down · Pyramid Outside-In · Arp Pluck 1/16 · Arp Up-Down Ping-Pong · Arp Legato Crawl · Arp Swell Triplets · Arp 2 Octaves Random · Vocoder Saw Pad · Vocoder Strings · Vocoder Glass Shimmer · Hybrid Choir + Vocoder · Noise Choir Whisper · Hollow Robot Arp
