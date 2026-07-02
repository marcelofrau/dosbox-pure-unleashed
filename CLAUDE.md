# Memory — DOSBox Pure Unleashed UWP

## Project Goal
Integrate libretro dosbox-pure core into UWP scaffold so DOS ROMs load and render via Direct2D.

## Key Constraints
- Submodule `extern/dosbox-pure/` is pristine — never commit to it.
- Patches go in `dosbox-uwp/local/dosbox-pure/` with same directory structure.
- `DBP_STANDALONE` defined → core tries HW OpenGL first, falls back to SW.
- Build only **x64** UWP. ARM64, ARM, x86 NOT supported — Xbox Series is x64.
- Build via `.sln` not `.vcxproj` (needs `$(SolutionDir)` for uwp-dep).
- `CompileAsWinRT=false` for legacy C code; `/ZW` for C++/CX files.
- Tested on Windows 11 via VS2022 only. Xbox Series deploy not tested yet.

## Files

| File | Purpose |
|------|---------|
| `dosbox-uwp/Content/RetroCore.cpp/.h` | libretro bridge: init, load, run, callbacks, VFS |
| `dosbox-uwp/Content/RetroScreenRenderer.cpp/.h` | D2D bitmap render with letterboxing |
| `dosbox-uwp/dosbox_uwpMain.cpp/.h` | Main loop (Update/Render), input routing |
| `dosbox-uwp/App.cpp` | Entry point, F1→FileOpenPicker→LoadRom |
| `dosbox-uwp/dosbox_pure_sta.cpp` | DBPS_* stubs (9 functions) |
| `dosbox-uwp/local/dosbox-pure/dosbox_pure_libretro.cpp` | Patched core (vs submodule) |
| `extern/libretro-common/vfs/vfs_implementation_uwp.cpp` | UWP VFS via `CreateFile2FromAppW` |

## Lessons Learned

### 1. D2D: BeginDraw/EndDraw mandatory
`ID2D1RenderTarget::DrawBitmap()` crashes unless wrapped in `BeginDraw()`/`EndDraw()`. D2D debug layer error: `"An attempt to render a primitive outside of BeginDraw/EndDraw"`. `BeginDraw()` returns `void` (not `HRESULT`), `EndDraw()` returns `HRESULT`.

### 2. RETRO_HW_FRAME_BUFFER_VALID = pitch==0
libretro HW core sends `retro_video_cb(RETRO_HW_FRAME_BUFFER_VALID, w, h, 0)` when using HW path. Without `pitch == 0` guard, `memcpy` with size 0 crashes. Log rejected frames.

### 3. Pixel format: IGNORE not PREMULTIPLIED
Framebuffer is raw XRGB8888. `D2D1_ALPHA_MODE_PREMULTIPLIED` distorts colors. Use `D2D1_ALPHA_MODE_IGNORE` for correct rendering.

### 4. DPI must match render target
Create bitmap with same DPI as D2D context (`GetDpi()`). Hardcoded 96.0f causes `D2DERR_BITMAP_BOUND_AS_TARGET` or rendering artifacts on high-DPI displays.

### 5. UWP STA: never .get() on WinRT async
`concurrency::create_task(WinRT IAsyncOperation^).get()` from STA thread throws `Concurrency::invalid_operation`. Use continuation chaining (`.then()`) or fire-and-forget tasks.

### 6. UWP file access: use StorageFile^, not path
`StorageFile::GetFileFromPathAsync(arbitraryPath)` fails with `AccessDeniedException`. The `StorageFile^` from `FileOpenPicker` carries access rights. Read file in picker continuation via `FileIO::ReadBufferAsync(file)` + `DataReader::FromBuffer`.

### 7. SET_HW_RENDER return 0
Return 0 in `retro_env` for `RETRO_ENVIRONMENT_SET_HW_RENDER` to force SW path. Returning 1 causes the core to use GL paths that crash without OpenGL context.

### 8. Logging prefix [dosbox-uwp]
All `OutputDebugStringA` logs prepend `[dosbox-uwp]` for DebugView filtering.

## Build: Release|x64
- `MSBuild.exe "solution.sln" /p:Configuration=Release /p:Platform=x64 /nowarn:MSB4011`
- 0 errors. Warnings: ~1500 C4244 cosmetic.

## Known Issues
- No audio output (XAudio2 not wired yet)
- DBPS_* stubs are no-ops (no OSD, no save slots)
- ARM64 build not supported (Xbox Series is x64)
- File picker → read → load all async; status not shown during load
- No MIDI support (Win32 MIDI #ifdef'd out for UWP)
- Directory enumeration via VFS not implemented (cross.cpp stubs)
- Xbox Series deploy not tested yet
