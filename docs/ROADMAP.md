# Roadmap — DOSBox Pure Unleashed UWP

Implementation phases ordered by dependency. Each phase is a runnable (or at least buildable) increment.

Legend: ✅ done  🏗️ partial  🔲 not started  ⏸️ blocked  ❌ broken

---

## Phase 0 — Scaffold + Infrastructure
**Goal: DirectX 11 UWP template compiling + SDL2 + build tooling**
**Status: ✅ DONE**

### 0.1 — Template
- [x] DirectX 11 UWP `App` + `IFrameworkView` loop compiles
- [x] `DeviceResources` (DXGI swap chain, D3D/D2D/DWrite devices)
- [x] `StepTimer` for fixed/variable timestep
- [x] `Sample3DSceneRenderer` (3D rotating cube fallback)
- [x] `SampleFpsTextRenderer` (DWrite debug overlay)

### 0.2 — SDL2 integration
- [x] `SdlInput` class: SDL init (gamecontroller + haptic + audio)
- [x] SDL_GameController open close add/remove events
- [x] UWP `Gamepad` API fallback (works on Xbox without SDL)
- [x] Button state tracking (m_buttonHeld / m_buttonJustPressed)
- [x] Gamepad A button → beep + background color change (test)

### 0.3 — Audio (test)
- [x] SDL audio device init (44100Hz, AUDIO_S16SYS, mono)
- [x] `PlayBeep()` generates sine wave → `SDL_QueueAudio`

### 0.4 — Build system
- [x] `.sln` with `uwp-dep.props` for SDL paths
- [x] `build.ps1`, `package.ps1`, `new-cert.ps1`, `install.ps1`
- [x] `.gitignore` (certs, build artifacts)
- [x] PFX cert with password `dev` for AppX packaging

---

## Phase 1 — Core Integration (Build)
**Goal: dosbox-pure compiles as part of the UWP app (0 errors)**
**Status: ✅ DONE**

### 1.1 — Submodule
- [x] `git submodule add` — `extern/dosbox-pure` at `e166344` (has UWP compat patches)
- [x] `extern/libretro-common/` — selective copy of VFS + UWP helpers (~10 files)

### 1.2 — Add core sources to vcxproj
- [x] 15 source files from `$(DBPDir)src/`: cpu (callback, core_dyn_x86, core_dynrec, core_full, core_normal, core_prefetch, core_simple, cpu, flags, modrm, paging), dos (cdrom, cdrom_image, dos_classes, dos_devices, dos_execute, dos_files, dos_ioctl, dos_keyboard_layout, dos_memory, dos_misc, dos_mscdex, drives), hardware (adlib, disney, dma, gameblaster, gus, ipx, joystick, mame, mixer, mouse, pci, pcspeaker, serialport, sblaster, tandy_sound, vga_*, voodoo_*, ...), ints, fpu, gui, misc, hardware/serialport, dbp_network, plus more
- [x] `$(DBPDir)dosbox_pure_libretro.cpp` (main libretro bridge — 4073 lines)
- [x] `$(DBPDir)dosbox_pure_ver.h`, `dosbox_pure_pad.h`, `dosbox_pure_run.h`, `dosbox_pure_osd.h` (core OS/menu logic)
- [x] `$(DBPDir)core_options.h` (~1177 lines — 70+ config variables)
- [x] 5 local patches in `dosbox-uwp/local/dosbox-pure/` (cross.cpp, midi.cpp, dbp_serialize.cpp, drive_cache.cpp, dosbox_pure_libretro.cpp)

### 1.3 — Compile flags
- [x] Defines: `__LIBRETRO__`, `DBP_STANDALONE`, `_CRT_SECURE_NO_WARNINGS`, `_CRT_NONSTDC_NO_DEPRECATE`, `_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING`
- [x] `CompileAsWinRT=false` for all .cpp from core (legacy C++)
- [x] `/bigobj` for large object files
- [x] Include paths: `$(DBPDir)`, `$(DBPDir)include`, `$(DBPDir)src`, `$(DBPDir)libretro-common/include`, `$(LrCommonDir)`, `$(LrCommonDir)include`
- [x] Optimizations: `MaxSpeed` for core files even in Debug

### 1.4 — Resolve incompatibilities
- [x] `cross.cpp`: UWP paths, `getcwd`/`chdir` replacements via local patch
- [x] `midi.cpp`: disabled MIDI output (no MIDI UWP API)
- [x] `drive_cache.cpp`: std::filesystem compat via local patch
- [x] `dosbox_pure_libretro.cpp`: local copy with UWP tweaks (SET_HW_RENDER handling, etc.)
- [x] VFS: `vfs_implementation_uwp.cpp` from RetroArch — uses `CreateFile2FromAppW`

### 1.5 — Verify
- [x] Build succeeds 0 errors (Release|x64)
- [x] ~~1500 warnings C4244 (cosmetic — "conversion possible loss of data")

---

## Phase 2 — Libretro Frontend Stub
**Goal: UWP app calls retro_init() + retro_load_game(), core runs, video renders**
**Status: 🏗️ PARTIAL — env callbacks implementados, GET_VARIABLE retorna 0, launcher não aparece**

### 2.1 — Lifecycle
- [x] `retro_set_environment()` → `retro_env_wrap` static wrapper
- [x] `retro_set_video_refresh()` → `RetroCore::retro_video`
- [x] `retro_set_audio_sample_batch()` → `RetroCore::retro_audio`
- [x] `retro_set_input_poll()` → `RetroCore::retro_input_poll`
- [x] `retro_set_input_state()` → `RetroCore::retro_input_state`
- [x] `retro_init()` / `retro_deinit()`
- [x] `retro_load_game(retro_game_info*)` — data passed as pointer + size
- [x] `retro_get_system_av_info()` — geometry + timing
- [x] `retro_unload_game()`
- [x] `retro_run()` — called every frame from Update()

### 2.2 — retro_environment handler — implemented
**Status: 🟢 IMPLEMENTED**

| Cmd | Nome | Handler |
|-----|------|---------|
| 36 | `GET_VFS_INTERFACE` | entrega `uwp_vfs_iface` (v3) ✅ |
| 6 | `GET_SYSTEM_DIRECTORY` | `LocalFolder` (UTF-8) ✅ |
| 7 | `GET_SAVE_DIRECTORY` | `LocalFolder` (UTF-8) ✅ |
| 9 | `SET_PIXEL_FORMAT` | retorna 1 ✅ |
| 33 | `SET_HW_RENDER` | **retorna 0** — rejeita HW, força SW ✅ |
| 47 | `SET_MESSAGE_EXT` | log via OutputDebugStringA ✅ |
| 44 | `SET_KEYBOARD_CALLBACK` | push held keys ✅ |
| 62 | `GET_THROTTLE_STATE` | `RETRO_THROTTLE_NONE` ✅ |
| 35 | `SHUTDOWN` | log ✅ |

### 2.3 — retro_environment handler — NOT implemented
**Status: 🔴 UNSUPPORTED (fall to default, return 0)**

| Cmd | Nome | Consequência |
|-----|------|-------------|
| **15** | **`GET_VARIABLE`** | **CRÍTICO — core pergunta centenas de vezes, sem resposta launcher não aparece** |
| 13 | `SET_VARIABLE` | core notifica mudança de variável |
| **16** | **`SET_VARIABLES`** | **core tenta registrar ~70 variáveis, sem registro variáveis ficam com default e GET_VARIABLE nunca é chamado corretamente** |
| 55 | `GET_CURRENT_SOFTWARE_FRAMEBUFFER` | core pergunta se framebuffer SW está válido |
| 56 | `GET_HW_RENDER_INTERFACE` | core pergunta sobre HW render (ok retornar false) |
| 27 | `SET_SUPPORT_ACHIEVEMENTS` | achievements desligados (ok) |
| 18 | `GET_LIBRETRO_PATH` | core query path (ok retornar false) |
| 58 | `SET_SUPPORT_NO_GAME` | core pergunta se aceita "no game" |
| 69 | `GET_AUDIO_VIDEO_ENABLE` | core verifica se AV está habilitado |

### 2.4 — Smoke test
- [x] App chama `BootCore()` → retro_init ✅ → mostra "Core ready. Press F1 to load a game."
- [x] F1 picker → `LoadGame(path, data)` → `retro_load_game` ✅
- [x] Frame renderizado na tela (DOSBox core visível) ✅
- [x] Logging via `OutputDebugStringA` com prefixo `[dosbox-uwp]` ✅
- [x] FPS/debug overlay com status do core ✅

### 2.5 — Config variables & launcher (PENDENTE)
- [ ] Levantar lista completa de variáveis do core (source: `core_options.h` — ~70 vars)
- [ ] Implementar `SET_VARIABLES` (cmd 16) — registrar variáveis no core
- [ ] Implementar `GET_VARIABLE` (cmd 15) — responder com valores default para cada key
- [ ] Implementar `GET_CURRENT_SOFTWARE_FRAMEBUFFER` (cmd 55) — retornar true
- [ ] Opcional: `GET_VARIABLE_UPDATE` (cmd 65) — notificar mudanças
- [ ] Testar: launcher do DOSBox Pure aparece ao carregar ROM sem jogo configurado

---

## Phase 3 — Video Pipeline
**Goal: DOSBox screen rendering via D2D**
**Status: ✅ DONE (1 sub-item pending)**

### 3.1 — retro_video callback
- [x] `retro_video_refresh_cb(data, width, height, pitch)` implemented
- [x] Reject frames with `pitch == 0` (catches `RETRO_HW_FRAME_BUFFER_VALID`)
- [x] Copy pixels to `std::vector<uint8_t>` with mutex protection
- [x] `GrabVideoFrame()` — thread-safe read + invalidate

### 3.2 — D2D bitmap renderer
- [x] `RetroScreenRenderer` class
- [x] `CreateBitmap` with `DXGI_FORMAT_B8G8R8A8_UNORM` + `D2D1_ALPHA_MODE_IGNORE`
- [x] `GetDpi()` from context (not hardcoded) — previne D2DERR_BITMAP_BOUND_AS_TARGET
- [x] `CopyFromMemory` — copy framebuffer pixels to D2D bitmap
- [x] `BeginDraw()` → `DrawBitmap()` with letterbox → `EndDraw()`
- [x] `D2DERR_RECREATE_TARGET` handling (reset bitmap)

### 3.3 — Render loop
- [x] `Update()` calls `retro_run()`
- [x] `Render()` grabs frame → renders via D2D (not D3D)
- [x] Fallback: 3D cube (Sample3DSceneRenderer) when no ROM loaded
- [x] StepTimer for FPS vsync

### 3.4 — Verify
- [x] Load ROM (.dosz with .exe or .zip)
- [x] DOS boot screen visible
- [ ] Measure FPS baseline (not done)

---

## Phase 4 — Input
**Goal: Keyboard + gamepad working in DOS games**
**Status: 🏗️ PARTIAL**

### 4.1 — Libretro callbacks
- [x] `retro_input_poll()` — empty (libretro pattern: frontend reads state before callback)
- [x] `retro_input_state(port, device, index, id)` implemented for:
  - [x] `RETRO_DEVICE_KEYBOARD` — reads `s_keyboardState[id]`
  - [x] `RETRO_DEVICE_JOYPAD` — reads `s_keyboardState[id]` (partial: uses keyboard state, not gamepad)
  - [x] `RETRO_DEVICE_MOUSE` — relative X/Y (accumulated via `SetMouseMove`)
  - [x] `RETRO_DEVICE_POINTER` — normalized X/Y + press state

### 4.2 — OnKeyEvent (App.cpp → dosbox_uwpMain)
- [x] Space → BUTTON_A (gamepad A emulation)
- [ ] **BUG: uses Windows VirtualKey codes** (`0x0D`=Enter, `0x1B`=Escape, `0x08`=Back, `0x26`=Up, etc.) **not RETROK_* constants** — core calls `SET_KEYBOARD_CALLBACK` but pushes wrong key values. Core ignores them.
- [ ] Missing: A-Z and 0-9 map correctly only by accident (VK codes match ASCII for letters/numbers)
- [ ] Missing: F1-F12 mapping (F1 used by file picker)
- [ ] Missing: Numpad, punctuation, modifiers (Shift/Ctrl/Alt use VK codes)
- [ ] Missing: `RETRO_DEVICE_ANALOG` — thumbsticks (stub returns 0)

### 4.3 — SDL/UWP gamepad → libretro JOYPAD mapping
- [x] SDL_GameController connected/disconnected events handled
- [x] UWP Gamepad API fallback (Xbox without SDL)
- [ ] BUTTON_A → RETRO_DEVICE_ID_JOYPAD_A (currently maps to keyboard, not joypad)
- [ ] Left stick → RETRO_DEVICE_ANALOG (not implemented)
- [ ] D-pad → RETRO_DEVICE_JOYPAD up/down/left/right (not implemented)
- [ ] Shoulder buttons, triggers → RETRO_DEVICE_JOYPAD (not implemented)

### 4.4 — Verify
- [ ] Navigate DOSBox Pure launcher menus (blocked by Phase 2.5)
- [ ] Play simple game (Commander Keen, Doom, etc.)
- [ ] Test gamepad in-game

---

## Phase 5 — Audio
**Goal: DOS audio (PC speaker, AdLib, Sound Blaster) plays through SDL**
**Status: 🔲 NOT STARTED — buffer exists, not routed to output**

### 5.1 — Libretro → frontend
- [x] `retro_audio_sample_batch(data, frames)` — stores PCM samples in `s_audioBuffer` with mutex
- [x] `GrabAudio()` — returns pointer + frame count
- [ ] **BUG: `retro_audio` replaces buffer instead of appending** — if main loop doesn't consume, data is lost each frame
- [ ] `Update()` never calls `GrabAudio()` — data sits in buffer forever

### 5.2 — Route to SDL
- [ ] SDL audio device is initialized in `SdlInput::Initialize()` (44100Hz, S16SYS, mono) — **already works for beep test**
- [ ] Call `SDL_QueueAudio(m_audioDevice, data, size)` with samples from `GrabAudio()`
- [ ] Thread safety: `s_audioMutex` already exists — use same mutex in main loop when reading
- [ ] **Note**: no blocker here — just wire `GrabAudio()` → `SDL_QueueAudio()` in `Update()`

### 5.3 — Verify
- [ ] PC speaker beeps in DOS games
- [ ] AdLib/Sound Blaster music plays
- [ ] No crackling or dropouts

---

## Phase 6 — Config & Save States
**Goal: Persistent config, save/load states, battery-backed SRAM**
**Status: 🔲 NOT STARTED**

### 6.1 — RetroCore config variables
- [ ] Duplicate of Phase 2.5 — implement GET_VARIABLE first
- [ ] Default values for all ~70 config keys (from `core_options.h`)
- [ ] Optional: load/save config to JSON or CFG in LocalFolder

### 6.2 — Save states
- [ ] `retro_serialize_size()` — query state size
- [ ] `retro_serialize(void* data, size_t size)` — serialize state
- [ ] `retro_unserialize(const void* data, size_t size)` — restore state
- [ ] Save/load to `LocalFolder\saves\`
- [ ] Hotkey: F5 save, F7 load (or F9 quick load — see core_options)
- [ ] Requires `dosbox_pure_sta.cpp` stubs: `DBPS_RequestSaveLoad`, `DBPS_HaveSaveSlot`

### 6.3 — SRAM (battery-backed saves)
- [ ] `retro_get_memory_data(RETRO_MEMORY_SAVE_RAM)`
- [ ] `retro_get_memory_size(RETRO_MEMORY_SAVE_RAM)`
- [ ] Auto-save on suspend/shutdown

### 6.4 — dosbox_pure_sta.cpp stubs
Current: all 9 functions are no-ops. Need proper implementations for:

| Stub | Purpose | Priority |
|------|---------|----------|
| `DBPS_OnContentLoad` | Notify content loaded | Low |
| `DBPS_SubmitOSDFrame` | OSD rendering | Medium (for launcher/messages) |
| `DBPS_GetMouse` | Mouse input from OS | High (for pointer/touch) |
| `DBPS_StartCaptureJoyBind` | Joypad binding UI | Low |
| `DBPS_HaveJoy` | Joypad available | Medium |
| `DBPS_GetJoyBind` | Joypad binding query | Low |
| `DBPS_RequestSaveLoad` | Save state triggers | High (for F5/F7) |
| `DBPS_HaveSaveSlot` | Save slot available | High |
| `DBPS_ApplyConfigOverrides` | Config file support | Medium |
| `DBPS_IsConfigOverride` | Config check | Medium |
| `DBPS_ToggleConfigOverride` | Toggle config | Medium |
| `DBPS_GetConfigOverrideJSON` | Export config | Low |

---

## Phase 7 — Deployment & Polish
**Goal: App runs on Xbox Dev Mode / Windows Store**
**Status: 🏗️ PARTIAL**

### 7.1 — File picker
- [x] F1 opens `FileOpenPicker` with 11 file types (.zip, .dosz, .exe, .com, .bat, .iso, .chd, .cue, .img, .ima, .vhd, .conf)
- [x] Read file via `FileIO::ReadBufferAsync(file)` (STA-safe — no .get())
- [x] Data passed as `std::vector<uint8_t>` to `LoadGame`

### 7.2 — Packaging
- [ ] MSIX package signing with PFX
- [ ] Deploy to Xbox Dev Mode (test)
- [ ] Cert install script

### 7.3 — Performance
- [ ] Profile GPU/CPU of libretro loop
- [ ] Dynamic resolution scaling (if needed)
- [ ] Shader CRT/scanlines (optional)

### 7.4 — UX
- [ ] Splash screen / loading state (currently shows "Core ready. Press F1")
- [ ] Savestate thumbnails
- [ ] On-screen keyboard (needed for text input in DOS)

---

## Phase 8 — Future / Stretch
**Status: ⏳ WISHLIST**

- [ ] Netplay (`dbp_network.cpp` already compiled into project — untested)
- [ ] RetroAchievements (need `SET_SUPPORT_ACHIEVEMENTS` + login UI)
- [ ] Cheats (need cheat API integration)
- [ ] Shaders GLSL → HLSL (CRT, scanlines, etc.)
- [ ] Mouse/touch pointer input — **PARTIALLY done** (MOUSE/POINTER in retro_input_state)
- [ ] Virtual keyboard XAML overlay
- [ ] C++/WinRT migration (scaffold uses C++/CX — deprecated but working)
- [ ] ARM64 target (Xbox Series is x64 but future Surface?)
- [ ] Investigate: ~1500 C4244 warnings cleanup

---

## Dependencies Between Phases

```
Phase 0 (scaffold) ───────────────────── ✅ done
    │
    ▼
Phase 1 (core compila) ───────────────── ✅ done
    │
    ▼
Phase 2 (frontend stub) ──────────────── 🏗️ partial (env var stubs)
    │
    ├────► Phase 3 (video) ─────────────── ✅ done (1 item pending)
    │
    ├────► Phase 4 (input) ─────────────── 🏗️ partial (VK codes broken)
    │
    ├────► Phase 5 (audio) ─────────────── 🔲 not started (buffer exists)
    │
    ├────► Phase 6 (config/saves) ──────── 🔲 not started
    │
    └────► Phase 7 (deploy) ────────────── 🏗️ partial (picker done)
```

Fases 3, 4, 5, 7 podem ser trabalhadas parcialmente em paralelo (não bloqueiam entre si).
Fase 2 (especialmente 2.5 — GET_VARIABLE) é pré-requisito para launcher.
Fase 4.2 (RETROK_ mapping) é pré-requisito para qualquer input de teclado.
Fase 6 depende de Phase 2.5 + dosbox_pure_sta.cpp.

---

## Known Bugs & Status

| Bug | Fase | Status |
|-----|------|--------|
| RETROK_ mapping: VK codes instead of RETROK_ constants | 4 | ❌ broken |
| GET_VARIABLE returns 0 — launcher blocked | 2 | ❌ broken |
| retro_audio replaces buffer instead of append | 5 | ⚠️ needs ring buffer |
| GrabAudio never called in Update() | 5 | ❌ not wired |
| MOUSE/POINTER exist but no real mouse input (no mouse event handler) | 4 | 🏗️ partial |
| No joystick axis → libretro analog mapping | 4 | 🔲 todo |
| ~1500 C4244 warnings | 1 | 🟡 cosmetic |
