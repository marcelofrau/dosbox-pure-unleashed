# Roadmap — DOSBox Pure Unleashed UWP

Implementation phases, ordered by dependency. Each phase produces a runnable (or at least buildable) increment.

---

## Phase 0 — Scaffold + Infrastructure
**Status: ✅ DONE**

- [x] DirectX 11 UWP template funcionando
- [x] SDL2 integrado (input + audio)
- [x] Gamepad A button → beep + background color (teste SDL)
- [x] Debug overlay (FPS, controller, audio status)
- [x] Package scripts (build, package, new-cert)
- [x] .gitignore (certs, build artifacts)
- [x] Certificado PFX com senha fixa `dev`

---

## Phase 1 — Core Integration (Build)
**Status: ✅ DONE**

### 1.1 — Add submodules
- [ ] `git submodule add https://github.com/schellingb/dosbox-pure extern/dosbox-pure`
- [ ] Decidir: `libretro-common` como submodule raso ou cópia seletiva
  - Necessários: `vfs/vfs_implementation_uwp.cpp`, `include/vfs/vfs.h`, `include/vfs/vfs_implementation.h`, `uwp/uwp_file_handle_access.h`, `uwp/uwp_func.h`, `uwp/std_filesystem_compat.h`
  - Recomendação: cópia seletiva em `extern/libretro-common/` para evitar puxar 10k+ arquivos do RetroArch

### 1.2 — Add core sources to vcxproj
- [ ] Listar todos os ~120 `.cpp` do core no `dosbox-uwp.vcxproj`
- [ ] Adicionar include paths: `$(DBPDir)`, `$(DBPDir)include`, `$(DBPDir)src`
- [ ] Adicionar defines: `__LIBRETRO__`, `DBP_STANDALONE`, `_CRT_SECURE_NO_WARNINGS`

### 1.3 — Resolver incompatibilidades de compilação
- [ ] `cross.cpp` no UWP: paths, `getcwd`, `chdir`, environment variables
- [ ] `drives.cpp` / `drive_local.cpp`: acesso a arquivos (substituir por VFS calls)
- [ ] `midi.cpp`: MIDI output (opcional, pode desabilitar)
- [ ] `joystick.cpp`: desabilitar, vamos usar SDL → libretro input
- [ ] Configurar plataforma UWP (x64 apenas — ARM64, ARM, x86 NÃO suportados)
  - Xbox Series é x64, outras arquiteturas não são alvo

### 1.4 — VFS stub
- [ ] Copiar `vfs_implementation_uwp.cpp` do RetroArch
- [ ] Adaptar includes para nosso projeto
- [ ] Implementar `retro_vfs_file_open_impl` com `CreateFile2FromApp`
- [ ] Implementar dir operations: `opendir`, `readdir`, `closedir`

### 1.5 — Verify
- [ ] Build completo sem erros (pode linkar como lib estática ainda sem main)
- [ ] Output: core compila como .obj files, linkáveis no app

---

## Phase 2 — Libretro Frontend Stub
**Goal: App UWP inicializa o core, chama retro_init(), retro_load_game()**
**Status: ✅ DONE**

### 2.1 — Libretro environment callbacks
- [x] Implementar `retro_environment()` handler:
  - `GET_VFS_INTERFACE` → nossa VFS
  - `GET_VARIABLE` / `SET_VARIABLE` → config stubs
  - `GET_SYSTEM_DIRECTORY` / `GET_SAVE_DIRECTORY` → `LocalFolder`
  - `GET_PREFERRED_HW_RENDER` → retornar false (força SW render)
  - Demais: retornar default (false)
  - `SET_HW_RENDER` → retornar 0 (rejeitar HW)
  - `SET_KEYBOARD_CALLBACK` → push held keys
  - `GET_THROTTLE_STATE` → NONE
  - `SET_MESSAGE_EXT` → log via OutputDebugStringA

### 2.2 — Core lifecycle
- [x] `retro_init()`
- [x] `retro_set_environment()`, `retro_set_video_refresh()`, etc.
- [x] `retro_load_game(const struct retro_game_info*)` — carregar ROM
- [x] `retro_get_system_av_info()` → obter geometry/timing

### 2.3 — First smoke test
- [x] App chama `retro_init()` → `retro_load_game()` no startup
- [x] Frame renderizado na tela (DOSBox core visível)
- [x] Log de diagnóstico via OutputDebugString

---

## Phase 3 — Video Pipeline
**Goal: Ver DOS rodando na tela (mesmo que sem áudio ou input)**
**Status: ✅ DONE**

### 3.1 — SW framebuffer renderer
- [x] Callback `retro_video_refresh_cb(data, width, height, pitch)`
- [x] Criar `ID2D1Bitmap1` (formato `DXGI_FORMAT_B8G8R8A8_UNORM`, `D2D1_ALPHA_MODE_IGNORE`)
- [x] Copiar pixels do framebuffer para bitmap via memcpy
- [x] Render quad fullscreen com a textura (DrawBitmap com letterbox)
- [x] Manter aspect ratio (letterboxing)

### 3.2 — Render loop
- [x] `Update()` chama `retro_run()`
- [x] `Render()` exibe framebuffer (via D2D, não Direct3D)
- [x] FPS vsync controlado pelo StepTimer

### 3.3 — Verify
- [x] Carregar ROM DOS (.dosz com .exe dentro)
- [x] Ver boot do DOS na tela
- [ ] Medir FPS baseline

---

## Phase 4 — Input
**Goal: Jogar com gamepad e teclado**
**Status: 🏗️ IN PROGRESS**

### 4.1 — SDL → Libretro input mapping
- [x] `retro_input_poll()`: ler estado atual (função vazia, estados globais)
- [x] `retro_input_state(port, device, index, id)`:
  - `RETRO_DEVICE_KEYBOARD`: teclado UWP → libretro key codes (parcial)
  - `RETRO_DEVICE_JOYPAD`: mapeado do keyboard state global (função vazia, depende do key mapping)
  - `RETRO_DEVICE_MOUSE`: stub (retorna 0)
  - `RETRO_DEVICE_POINTER`: stub (retorna 0)
- [ ] `RETRO_DEVICE_ANALOG`: thumbsticks (não implementado)
- [ ] `RETRO_DEVICE_JOYPAD`: botões SDL gamepad → libretro (não implementado)
- [ ] Keyboard mapping completo: F1-F12, numpad, etc.

### 4.2 — Test
- [ ] Navegar menus do DOS (se houver)
- [ ] Jogar jogo simples (ex: Commander Keen, Doom)

---

## Phase 5 — Audio
**Goal: Som saindo corretamente**
**Status: ⏸️ BLOCKED (SDL audio roda mas sem saída UWP)**

### 5.1 — Libretro → SDL audio bridge
- [x] `retro_audio_sample_batch(samples, frames)`: recebe PCM do core
- [x] Buffer via `std::vector` com mutex
- [ ] Roteir para `SDL_QueueAudio()` (atualmente o buffer é coletado mas não enviado ao SDL)

### 5.2 — Test
- [ ] Áudio do DOS (PC speaker, AdLib, Sound Blaster)

---

## Phase 6 — Config & Save States
**Goal: Configurações persistentes, save/load states**

### 6.1 — Config variables
- [ ] `RETRO_ENVIRONMENT_GET_VARIABLE`: responder com valores default
- [ ] Arquivo de config em JSON ou CFG no `LocalFolder`
- [ ] Expor opções no debug overlay ou futura UI XAML

### 6.2 — Save states
- [ ] `retro_serialize_size()` + `retro_serialize()` / `retro_unserialize()`
- [ ] Salvar em `LocalFolder\saves\`
- [ ] Suporte a savestate slots (F5/F7)

### 6.3 — SRAM
- [ ] `retro_get_memory_data(RETRO_MEMORY_SAVE_RAM)` → bateria dos saves
- [ ] Salvar automaticamente ao suspender/fechar

---

## Phase 7 — Deployment & Polish
**Goal: App publicável no Xbox Dev Mode / Windows Store**

### 7.1 — Packaging
- [ ] MSIX package assinado
- [ ] Testar deploy no Xbox Dev Mode
- [ ] Certificado `.cer` instalável no Device Portal

### 7.2 — Performance
- [ ] Profile GPU/CPU do loop libretro
- [ ] Dynamic resolution scaling se necessário
- [ ] Shader de CRT/scanlines (opcional)

### 7.3 — UX
- [x] Tela de seleção de ROM (file picker via F1)
- [ ] Splash screen / loading state
- [ ] Savestate thumbnails

---

## Phase 8 — Future / Stretch
- [ ] Netplay (`dbp_network.cpp` + `Windows::Networking::Sockets`)
- [ ] RetroAchievements
- [ ] Cheats
- [ ] Shaders GLSL → HLSL
- [ ] Mouse/touch pointer input
- [ ] Teclado virtual XAML overlay
- [ ] C++/WinRT migration (se necessário)

---

## Dependencies Between Phases

```
Phase 0 (scaffold) ───────────────────── ✅ done
    │
    ▼
Phase 1 (core compila) ─── VFS stub ───── ✅ done
    │
    ▼
Phase 2 (frontend stub) ────────────────── ✅ done
    │
    ├────► Phase 3 (video) ─────────────── ✅ done
    │
    ├────► Phase 4 (input) ─────────────── 🏗️ in progress
    │
    ├────► Phase 5 (audio) ─────────────── ⏸️ blocked
    │
    ├────► Phase 6 (config/saves) ───────── ⏳ todo
    │
    └────► Phase 7 (deploy) ─────────────── ⏳ todo
```

Fase 3, 4, 5 podem ser paralelizadas após a fase 2.
