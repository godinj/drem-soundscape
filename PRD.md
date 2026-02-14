# drem-soundscape — Product Requirements Document

## 1. Overview

**drem-soundscape** is a standalone desktop application for creating personal soundscapes by layering and looping audio files. It targets users seeking ambient audio environments for relaxation, focus, sleep, and meditation.

Users load sound files, layer them, set independent loop regions, adjust per-layer volume and panning, and save complete configurations as presets for instant recall.

## 2. Goals

- Provide a simple, distraction-free tool for building layered ambient soundscapes
- Let users precisely control loop regions per layer so short clips can sustain long sessions
- Persist soundscape configurations as presets so users can switch contexts quickly
- Run natively on all three major desktop platforms

## 3. Non-Goals

- This is **not** a DAW, audio editor, or effects processor
- No real-time audio effects (reverb, EQ, compression) in the MVP
- No audio recording — playback and looping only
- No plugin format (VST/AU/AAX) — standalone app only
- No network features (streaming, sharing, cloud sync)

## 4. Target Users

| Persona | Use Case |
|---------|----------|
| **Focus worker** | Loops rain, café noise, or brown noise while working |
| **Meditator** | Layers singing bowls, nature sounds, and drones for meditation sessions |
| **Sleeper** | Builds a consistent sleep soundscape and saves it as a preset |
| **Sound designer** | Experiments with layering field recordings and textures |

## 5. Platform & Tech

- **Platforms:** macOS, Windows, Linux
- **Language:** C++
- **Framework:** JUCE (audio I/O, file decoding, GUI, state management)
- **Build system:** CMake
- **Audio formats:** All formats supported by JUCE out of the box (WAV, AIFF, FLAC, MP3, OGG Vorbis, etc.)

## 6. MVP Feature Requirements

### 6.1 Multi-File Loading

- User can add audio files via a file browser dialog or drag-and-drop
- Support loading up to **8 layers** simultaneously
- Display the file name and duration for each loaded layer
- User can remove individual layers

### 6.2 Independent Playback & Looping

- Each layer has its own independent playhead
- Each layer loops indefinitely between configurable **loop-start** and **loop-end** markers
- Default loop region is the entire file
- User can adjust loop-start and loop-end by dragging markers on a waveform display or entering numeric values
- Layers play concurrently — starting, stopping, or modifying one layer does not affect others

### 6.3 Transport Controls

- **Global controls:**
  - Play All / Stop All — start or stop every layer at once
  - Master volume knob/slider
- **Per-layer controls:**
  - Play / Stop toggle
  - Volume slider
  - Stereo pan knob
  - Mute / Solo toggles

### 6.4 Visual Playback State

- Each layer displays:
  - Waveform overview of the loaded file
  - Highlighted loop region (between loop-start and loop-end markers)
  - Moving playhead indicator showing current position
  - Playback status (playing / stopped)
  - Current volume and pan values

### 6.5 Preset System

- **Save preset:** Captures the full soundscape state:
  - File paths for all loaded layers
  - Loop-start and loop-end markers per layer
  - Volume and pan settings per layer
  - Mute/solo state per layer
- **Load preset:** Restores a saved configuration, loading files and applying all settings
- **Preset format:** JSON (human-readable, easy to debug and version-control)
- **Preset storage:** User-chosen file location (standard save/open dialogs)
- Handle missing files gracefully (warn the user, load remaining layers)

## 7. User Interface

### 7.1 Layout

```
+-----------------------------------------------+
|  Toolbar: [Add File] [Save Preset] [Load Preset] [Play All] [Stop All]  [Master Vol]  |
+-----------------------------------------------+
|  Layer 1: [file.wav] [Play][Mute][Solo]  [Vol---][Pan-]                                |
|  |████████░░░░░░░░░░████████████████████░░░░░░| ← waveform + loop region + playhead    |
+-----------------------------------------------+
|  Layer 2: [rain.mp3] [Play][Mute][Solo]  [Vol---][Pan-]                                |
|  |████████████████████████████████████████████| ← waveform + loop region + playhead    |
+-----------------------------------------------+
|  ...                                                                                    |
+-----------------------------------------------+
|  Layer 8: (empty — click Add File)                                                      |
+-----------------------------------------------+
```

### 7.2 Interactions

- **Adding a layer:** Click "Add File" or drag-and-drop onto the window
- **Setting loop region:** Drag loop-start/loop-end handles on the waveform
- **Removing a layer:** Click a close/remove button on the layer strip
- **Reordering layers:** Drag-and-drop layer strips (nice-to-have for MVP)

## 8. Audio Engine Requirements

- Mix all active layers to a single stereo output
- Support common sample rates (44.1 kHz, 48 kHz, 96 kHz) — resample as needed
- Crossfade at loop boundaries to avoid clicks/pops (short fade, ~5–10 ms)
- Audio processing must run on a dedicated thread — no blocking the GUI
- Latency is not critical (ambient playback), but aim for responsive transport controls (<100 ms)

## 9. Milestones

### Milestone 1 — Audio Engine Foundation

- JUCE project scaffolding and CMake build system
- Load a single audio file and play it in a loop
- Basic GUI window with a single waveform display

### Milestone 2 — Multi-Layer Playback

- Support loading and playing up to 8 layers simultaneously
- Per-layer play/stop, volume, and pan controls
- Audio mixing of all active layers to stereo output
- Loop-start and loop-end markers per layer

### Milestone 3 — GUI & Waveform Interaction

- Waveform display per layer with loop region visualization
- Draggable loop-start/loop-end markers
- Moving playhead indicator
- Mute/solo per layer
- Global transport controls (Play All / Stop All)

### Milestone 4 — Preset System

- Save/load presets as JSON
- Graceful handling of missing files
- File browser dialogs for save/load

### Milestone 5 — Polish & Release

- Crossfade at loop boundaries
- Drag-and-drop file loading
- Keyboard shortcuts for common actions
- UI polish, consistent styling
- Installers/packages for macOS, Windows, Linux

### Future Milestones (Post-MVP)

- **Unlimited layers** — Remove the 8-layer cap, add scrollable layer list
- **Fade-in / fade-out** per layer (timed fades for sleep/meditation transitions)
- **Timer / scheduler** — Auto-stop after a duration
- **Audio effects** — Per-layer reverb, EQ, filtering
- **Built-in sound library** — Bundled ambient sounds to get started without user files
- **Crossfade between presets** — Smooth transitions when switching soundscapes

## 10. Success Criteria

- User can build a multi-layer soundscape from their own audio files in under 2 minutes
- Soundscape plays continuously without audible glitches, clicks, or dropouts
- Presets reliably save and restore the full soundscape state
- Application runs on macOS, Windows, and Linux without platform-specific bugs

## 11. Open Questions

- [ ] Licensing model — open source? Freeware? Commercial?
- [ ] Application name and branding finalization
- [ ] Minimum supported OS versions per platform
- [ ] Whether to support audio device selection in MVP or use system default
- [ ] Maximum supported file size / duration per layer
