# drem-soundscape

## Project Overview

Standalone desktop audio application for creating personal soundscapes (relaxation, focus, sleep, meditation). Users load sound files, layer them, set independent loop points, and save configurations as presets.

## Tech Stack

- **Language:** C++
- **Framework:** JUCE (included as git submodule)
- **Build system:** CMake (TBD)
- **Target:** Standalone desktop app (not a plugin)

## Core Features

1. **Multi-file loading** — User selects one or more audio files to play simultaneously
2. **Independent playback** — Each file has its own playhead and loop start/end markers, looping indefinitely
3. **Visual playback state** — GUI displays each file's playhead position, loop region, and playback status
4. **Presets** — Save/load complete soundscape configurations (files, loop markers, settings)

## Architecture Notes

- Project is in early stages — no application code yet, just the JUCE submodule
- JUCE provides audio I/O, file reading, GUI, and preset/state management primitives
