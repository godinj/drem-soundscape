---
name: run-tests
description: Build and run the test suite for drem-soundscape. Use after making changes to verify correctness.
allowed-tools: Bash, Read, Grep
user-invocable: true
---

# Run Tests

Build and run the drem-soundscape test suite.

## Steps

1. Build the test target:
   ```bash
   cmake --build /home/godinj/dev/drem-soundscape/build --target tests --config Debug
   ```

2. Run tests using CTest:
   ```bash
   cd /home/godinj/dev/drem-soundscape/build && ctest --output-on-failure
   ```

3. Report results:
   - If all tests pass, report the count and confirm success
   - If any tests fail, show the full failure output and suggest fixes based on the error messages

## If no test target exists

If the project doesn't have tests configured yet, say so and suggest adding a test target to CMakeLists.txt using JUCE's testing utilities or Catch2.
