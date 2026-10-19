# Geometry Dash standalone SafeClick experiment, v0.1

**NO GEODE REQUIRED.** This is a Windows x64 native DLL project, built with
Visual Studio via GitHub Actions. It relies on Windows loading XINPUT9_1_0.dll
from the Geometry Dash executable folder and forwards the three public XInput
calls to the original library in System32.

## What v0.1 actually does

- Loads as a native Windows DLL (via XInput proxy), *without Geode*.
- Logs successful startup to `gd_native_safeclick.log` beside `GeometryDash.exe`.
- Attaches a window-message observer to the Geometry Dash window.
- Press F6 to enable/disable logging of mouse-left, space, and up-arrow presses.
- Preserves the standalone trajectory-prediction and buffer engine in `src/`
  for the next integration step; all its tests are compiled and run.

**Not yet implemented:** connecting to GD's internal player state, simulating
collisions, suppressing unsafe clicks, or automatically firing a safe click.
Therefore **v0.1 is NOT a working predictive click cheat**. We are first
validating a clean, Geode-independent loading and input-observation path.

## Compile

Upload all contents (including `.github`) to a new, otherwise empty GitHub
repository. Actions > `Build native standalone GD mod` > Run workflow.
Download artifact `GD-Standalone-SafeClick-v0.1-Win64`, extract it, and find
`XINPUT9_1_0.dll`. Or compile locally with VS 2022 and CMake:

```
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Install SAFELY

1. Close Geometry Dash.
2. Open the folder containing `GeometryDash.exe`.
3. **STOP if `XINPUT9_1_0.dll` already exists.** It may belong to Geode or
   another mod. Do NOT overwrite it; use a separate clean GD installation or
   first remove/disable the existing loader only if you understand the impact.
4. Copy the built `XINPUT9_1_0.dll` into the same directory as `GeometryDash.exe`.
5. Launch Geometry Dash normally. Look for `gd_native_safeclick.log` in that folder.
6. Press F6 and click/jump. Close GD; check for `observed:` log lines.
7. Remove only **our** `XINPUT9_1_0.dll` to uninstall. If GD has trouble starting,
   remove it while GD is closed.

Do not use this proof-of-life DLL with other injected mods or in competitive
settings until validated. Win64 only. This input observer handles window messages,
not necessarily raw input; log results will tell us which events are visible.

## Next integration milestone

Hook/observe the actual GD 2.2081 gameplay input and physics tick and capture
player/world state. Then connect `Controller.hpp` and `Predictor.hpp` to actual
game data. No estimated/unverified GD function addresses have been hard-coded.
