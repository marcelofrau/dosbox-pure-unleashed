# AGENTS.md — DOSBox Pure Unleashed UWP

## Project
Integrate libretro dosbox-pure core into UWP scaffold. DOS emulator on Windows/Xbox.

## Critical Rules
- **NEVER commit to submodule** `extern/dosbox-pure/`. Patches go in `dosbox-uwp/local/dosbox-pure/` mirroring same directory structure.
- **Build via .sln** not .vcxproj. `$(SolutionDir)` needed for `uwp-dep.props` SDL paths.
- **x64 only.** ARM64/ARM/x86 NOT supported (Xbox Series is x64).
- **`CompileAsWinRT=false`** for legacy C (dosbox-pure source). `/ZW` for C++/CX files.

## Build
```powershell
MSBuild.exe "dosbox-pure-unleashed-uwp.sln" /p:Configuration=Release /p:Platform=x64 /nowarn:MSB4011
```
Current: 0 errors, ~1500 warnings C4244 (cosmetic).

## Key Files

| File | Purpose |
|------|---------|
| `dosbox-uwp/Content/RetroCore.cpp/.h` | libretro bridge: init, load, run, callbacks, retro_env, VFS |
| `dosbox-uwp/Content/RetroScreenRenderer.cpp/.h` | D2D bitmap render + letterbox |
| `dosbox-uwp/dosbox_uwpMain.cpp/.h` | Main loop, Update/Render, input routing |
| `dosbox-uwp/App.cpp` | Entry point, F1 → FileOpenPicker → async file read → LoadRom |
| `dosbox-uwp/dosbox_pure_sta.cpp` | DBPS_* stubs (9 no-ops) |
| `dosbox-uwp/local/dosbox-pure/dosbox_pure_libretro.cpp` | Patched core (copied from submodule) |
| `extern/libretro-common/vfs/vfs_implementation_uwp.cpp` | UWP VFS via CreateFile2FromAppW |

## Known Bugs & Pitfalls

### 1. D2D: BeginDraw/EndDraw mandatory
`DrawBitmap()` crashes without `BeginDraw()`/`EndDraw()`. `BeginDraw()` returns `void`.

### 2. RETRO_HW_FRAME_BUFFER_VALID = pitch==0
Core sends `(RETRO_HW_FRAME_BUFFER_VALID, w, h, 0)` for HW frames. Guard `if (pitch == 0) return` or `memcpy` with size 0 crashes.

### 3. Pixel format: IGNORE not PREMULTIPLIED
Framebuffer is raw XRGB8888. `PREMULTIPLIED` distorts colors.

### 4. DPI must match D2D context
Always `GetDpi()` from render target. Hardcoded 96.0f breaks on high-DPI.

### 5. NEVER .get() on WinRT async from STA
`create_task(IAsyncOperation^).get()` from STA throws `Concurrency::invalid_operation`. Use `.then()` chaining.

### 6. StorageFile^ required, not path
`GetFileFromPathAsync(arbitraryPath)` throws `AccessDeniedException`. Read file in picker continuation via `FileIO::ReadBufferAsync(file)`.

### 7. SET_HW_RENDER return 0
Return 0 to force SW path. Returning 1 causes GL crash (no OpenGL context).

## Defines
`__LIBRETRO__`, `DBP_STANDALONE`, `_CRT_SECURE_NO_WARNINGS`, `_CRT_NONSTDC_NO_DEPRECATE`

## Logging
All `OutputDebugStringA` prepend `[dosbox-uwp]` for DebugView filtering.

## Status
- Phase 0-3 done (scaffold, core compiles, libretro frontend, video pipeline)
- Phase 4 WIP (input mapping partial)
- Phase 5 blocked (audio buffer exists, not routed to output)
- Tested on Windows 11 via VS2022. Xbox Series deploy not tested.
