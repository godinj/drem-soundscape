---
name: add-component
description: Generate boilerplate for a new JUCE component class with header and implementation files. Use when adding new audio sources, GUI components, or processors to the soundscape engine.
allowed-tools: Read, Write, Grep, Glob, Bash
argument-hint: [ComponentName]
user-invocable: true
---

# Add New JUCE Component

Create a new JUCE component named `$0`.

## Before generating

1. Read the existing source files in `src/` to understand the project's coding conventions (naming, includes, namespaces, file structure).
2. Determine what kind of component is being requested from the name and context:
   - **Audio source/processor** — inherits from `juce::AudioSource` or similar
   - **GUI component** — inherits from `juce::Component`
   - **Data model** — plain C++ class

## Generate files

Create two files:

### `src/$0.h`

- `#pragma once` header guard
- Necessary JUCE includes
- Class declaration with appropriate JUCE base class
- Override required virtual methods
- `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR($0)` in private section

### `src/$0.cpp`

- Include the header
- Implement constructor, destructor, and all overridden methods with stub implementations
- Add brief comments only where the purpose isn't obvious

## After generating

1. Update `CMakeLists.txt` to include the new source files in the target's source list
2. Verify the project still configures: `cmake -S /home/godinj/dev/drem-soundscape -B /home/godinj/dev/drem-soundscape/build`
3. Report what was created and what base class was chosen
