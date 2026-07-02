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
**Goal: dosbox-pure core compila como parte do projeto UWP**

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
- [ ] Configurar plataforma UWP (x64, ARM64) nos builds

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

### 2.1 — Libretro environment callbacks
- [ ] Implementar `retro_environment()` handler:
  - `GET_VFS_INTERFACE` → nossa VFS
  - `GET_VARIABLE` / `SET_VARIABLE` → config stubs
  - `GET_SYSTEM_DIRECTORY` / `GET_SAVE_DIRECTORY` → `LocalFolder`
  - `GET_PREFERRED_HW_RENDER` → retornar false (força SW render)
  - Demais: retornar default (false)

### 2.2 — Core lifecycle
- [ ] `retro_init()`
- [ ] `retro_set_environment()`, `retro_set_video_refresh()`, etc.
- [ ] `retro_load_game(const struct retro_game_info*)` — carregar ROM
- [ ] `retro_get_system_av_info()` → obter geometry/timing

### 2.3 — First smoke test
- [ ] App chama `retro_init()` → `retro_load_game()` no startup
- [ ] Nenhum frame ainda, só verificar se core carrega sem crash
- [ ] Log de diagnóstico via OutputDebugString

---

## Phase 3 — Video Pipeline
**Goal: Ver DOS rodando na tela (mesmo que sem áudio ou input)**

### 3.1 — SW framebuffer renderer
- [ ] Callback `retro_video_refresh_cb(data, width, height, pitch)`
- [ ] Criar `ID3D11Texture2D` (formato `DXGI_FORMAT_B8G8R8A8_UNORM`, `D3D11_USAGE_DYNAMIC`)
- [ ] Copiar pixels do framebuffer para a texture via `Map()/Unmap()`
- [ ] Render quad fullscreen com a textura (vertex shader + pixel shader simples)
- [ ] Manter aspect ratio (letterboxing)

### 3.2 — Render loop
- [ ] `Update()` chama `retro_run()`
- [ ] `Render()` exibe framebuffer
- [ ] FPS vsync controlado pelo StepTimer

### 3.3 — Verify
- [ ] Carregar ROM DOS (.zip com .exe dentro)
- [ ] Ver boot do DOS na tela
- [ ] Medir FPS baseline

---

## Phase 4 — Input
**Goal: Jogar com gamepad e teclado**

### 4.1 — SDL → Libretro input mapping
- [ ] `retro_input_poll()`: ler estado atual do SdlInput
- [ ] `retro_input_state(port, device, index, id)`:
  - `RETRO_DEVICE_JOYPAD`: mapear botões SDL → libretro (A→B, B→A, Start, Select, D-Pad)
  - `RETRO_DEVICE_ANALOG`: thumbsticks esquerdo/direito
  - `RETRO_DEVICE_KEYBOARD`: teclado SDL → libretro key codes
  - `RETRO_DEVICE_MOUSE`: mouse/touch

### 4.2 — Test
- [ ] Navegar menus do DOS (se houver)
- [ ] Jogar jogo simples (ex: Commander Keen, Doom)

---

## Phase 5 — Audio
**Goal: Som saindo corretamente**

### 5.1 — Libretro → SDL audio bridge
- [ ] `retro_audio_sample_batch(samples, frames)`: recebe PCM do core
- [ ] Buffer circular interno
- [ ] `SDL_QueueAudio()` para playback
- [ ] Taxa de amostragem: 44100 ou 48000 (negociar com core via `RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO`)

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
- [ ] Tela de seleção de ROM (file picker ou pasta)
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
Phase 0 (scaffold) ───────────────────── done
    │
    ▼
Phase 1 (core compila) ─── VFS stub ───── 2-3 dias
    │
    ▼
Phase 2 (frontend stub) ────────────────── 1-2 dias
    │
    ├────► Phase 3 (video) ─────────────── 2-3 dias
    │
    ├────► Phase 4 (input) ─────────────── 1 dia
    │
    ├────► Phase 5 (audio) ─────────────── 1-2 dias
    │
    ├────► Phase 6 (config/saves) ───────── 2-3 dias
    │
    └────► Phase 7 (deploy) ─────────────── 2-3 dias
```

Fase 3, 4, 5 podem ser paralelizadas após a fase 2.
