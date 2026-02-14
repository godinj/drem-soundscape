---
name: review-audio
description: Review audio processing code for real-time safety, correctness, and best practices. Use when writing or modifying audio playback, mixing, or DSP code.
allowed-tools: Read, Grep, Glob
argument-hint: [file-or-directory]
user-invocable: true
---

# Audio Code Review

Review audio code in `$0` (or the entire `src/` directory if no argument given) against the following checklist.

## Real-Time Safety (Audio Thread)

The audio callback must never block. Flag any of these in code that runs on the audio thread:

- [ ] Memory allocation (`new`, `malloc`, `std::vector::push_back`, `std::string` construction)
- [ ] Locks that could contend (`std::mutex::lock`, non-try locks)
- [ ] File I/O or network calls
- [ ] System calls that may block
- [ ] Logging or console output
- [ ] Exceptions being thrown

## Memory Safety

- [ ] Buffer accesses are bounds-checked or provably in-range
- [ ] No dangling pointers to audio buffers after resize/reallocation
- [ ] RAII or explicit cleanup for all allocated resources
- [ ] Circular buffers handle wrap-around correctly

## Audio Correctness

- [ ] Gain/volume calculations clamp output to prevent clipping beyond [-1.0, 1.0]
- [ ] Sample rate is handled correctly (no hardcoded 44100)
- [ ] Channel count mismatches are handled (mono-to-stereo, etc.)
- [ ] Loop boundary crossfades prevent clicks/pops
- [ ] Pan law is applied correctly (equal-power or constant-power)

## Thread Safety

- [ ] Shared state between GUI and audio threads uses lock-free mechanisms (atomics, lock-free queues)
- [ ] No data races on playback position, volume, or pan values
- [ ] File loading happens off the audio thread

## Output

For each finding, report:
1. **File and line number**
2. **Severity** (critical / warning / suggestion)
3. **What the issue is**
4. **How to fix it**
