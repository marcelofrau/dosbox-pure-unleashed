# Architecture — DOSBox Pure Unleashed UWP

## Overview

UWP port of DOSBox Pure Unleashed. Replaces ZillaLib (desktop platform layer) with a DirectX 11 + SDL2 UWP scaffold. The emulation core (`dosbox-pure`) runs unmodified as a libretro core — our app acts as the libretro frontend.

## Project Structure

```
dosbox-pure-unleashed-uwp/
├── dosbox-uwp/                      ← Frontend UWP (nós escrevemos)
│   ├── Content/
│   │   ├── RetroCore.h/cpp         ← [NOVO] libretro bridge: init/load/run/callbacks/VFS
│   │   ├── RetroScreenRenderer.h/cpp ← [NOVO] D2D bitmap render with letterbox
│   │   ├── SdlInput.h/cpp          ← Gamepad + keyboard via SDL, fallback UWP API
│   │   ├── Sample3DSceneRenderer.* ← Renderizador 3D de exemplo (fallback DX11)
│   │   └── SampleFpsTextRenderer.* ← Overlay de debug (FPS, diagnóstico)
│   ├── Common/
│   │   ├── DeviceResources.h/cpp   ← Gerenciamento DX11 device/swap chain
│   │   └── StepTimer.h             ← Game loop timer
│   ├── dosbox_uwpMain.h/cpp        ← Loop principal, Update/Render, core lifecycle
│   ├── dosbox_pure_sta.cpp         ← DBPS_* stubs (9 funções)
│   ├── local/dosbox-pure/          ← Patches locais do core (submodule intocado)
│   ├── App.xaml / App.cpp          ← Entry point UWP, picker F1, async load
│   └── dosbox-uwp.vcxproj
├── extern/
│   ├── dosbox-pure/                ← SUBMODULE: schellingb/dosbox-pure (core libretro)
│   │   ├── dosbox_pure_libretro.cpp
│   │   ├── src/ (cpu/, dos/, hardware/, ints/, gui/, fpu/, ...)
│   │   └── include/
│   └── libretro-common/            ← Cópia seletiva: libretro/RetroArch
│       ├── vfs/vfs_implementation_uwp.cpp
│       ├── include/vfs/vfs.h
│       ├── include/retro_environment.h
│       └── uwp/ (uwp_file_handle_access.h, uwp_func.h, std_filesystem_compat.h)
├── CLAUDE.md                       ← [NOVO] Memory file (AI agent context)
├── AGENTS.md                       ← [NOVO] Project-level agent instructions
├── certs/                          ← Certificado PFX + CER (gitignored)
├── scripts/                        ← Build, package, deploy
│   ├── build.ps1
│   ├── package.ps1
│   ├── new-cert.ps1
│   └── install.ps1
├── docs/                           ← Documentação
└── .gitignore
```

## Dependency Graph

```
┌─────────────────────────────────────────────────┐
│              dosbox-uwp (App UWP)                │
│  (C++/CX, XAML, DirectX 11, SDL2, XAudio2→SDL) │
├──────────────────────┬──────────────────────────┤
│   libretro frontend  │  Platform layer (SDL)     │
│   ┌───────────────┐  │  ┌────────────────────┐   │
│   │ retro_run()   │  │  │ SDL_GameController │   │
│   │ retro_video_  │  │  │ SDL_QueueAudio     │   │
│   │   refresh_cb  │  │  │ DX11 render        │   │
│   │ retro_input_  │  │  └────────────────────┘   │
│   │   poll/state  │  │                           │
│   │ retro_audio_  │  │                           │
│   │   sample_batch│  │                           │
│   └───────┬───────┘  │                           │
├───────────┼──────────┴───────────────────────────┤
│           │                                      │
│  ┌────────▼────────┐                             │
│  │  dosbox-pure    │  ← submodule (core)         │
│  │  (libretro)     │                             │
│  └─────────────────┘                             │
│  ┌─────────────────┐                             │
│  │  libretro-common│  ← VFS UWP impl            │
│  └─────────────────┘                             │
└──────────────────────────────────────────────────┘
```

## Libretro Frontend — What We Implement

The core calls these callbacks; our frontend provides them:

### Video
- `retro_video_refresh_cb(data, width, height, pitch)`
  - **SW path** (nosso plano): `data` = ponteiro para framebuffer XRGB8888 em RAM
  - Copiamos pixels para textura ID3D11Texture2D e renderizamos com um quad em tela cheia
  - **HW path** (futuro): `data == RETRO_HW_FRAME_BUFFER_VALID`, core renderiza via OpenGL ES (ANGLE→DX11)

### Audio
- `retro_audio_sample_batch(left, right, samples)` ou callback por frame
  - Buffer circular → `SDL_QueueAudio()` — já implementado

### Input
- `retro_input_poll()` → ler estado atual dos controles SDL/hardware
- `retro_input_state(port, device, index, id)` → retorna bool para cada botão
  - `RETRO_DEVICE_JOYPAD` → mapeado do gamepad SDL / teclado
  - `RETRO_DEVICE_ANALOG` → thumbsticks
  - `RETRO_DEVICE_MOUSE` / `RETRO_DEVICE_POINTER` → touch/mouse

### Environment
- `retro_environment(cmd, data)` → responde a requisições do core:
  - `RETRO_ENVIRONMENT_GET_VFS_INTERFACE` → entrega nossa VFS adaptada
  - `RETRO_ENVIRONMENT_GET_VARIABLE` → configurações (render resolution, etc.)
  - `RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY` → `ApplicationData.Current.LocalFolder`
  - `RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY` → `ApplicationData.Current.LocalFolder`
  - `RETRO_ENVIRONMENT_SET_HW_RENDER` → **retorna 0** (rejeita HW path, força SW)
  - `RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK` → push held keys
  - `RETRO_ENVIRONMENT_SET_MESSAGE_EXT` → log via OutputDebugString
  - `RETRO_ENVIRONMENT_GET_THROTTLE_STATE` → NONE

### VFS (Virtual File System)
Interface que o core usa pra ler arquivos (ROMs .zip/.iso, saves, config). Implementação UWP adaptada do RetroArch:
- Usa `CreateFile2FromApp()` da Win10 SDK
- Funções: open, close, read, write, seek, tell, size, stat, mkdir, opendir, readdir
- Paths relativos ao sandbox UWP (`LocalFolder`, `InstalledLocation`)

## Rendering Pipeline (SW path)

```
retro_run()
    ↓
core renderiza frame em framebuffer RAM (XRGB8888)
    ↓
retro_video_refresh_cb(data, w, h, pitch)
    ↓
pitch == 0? → REJECT (RETRO_HW_FRAME_BUFFER_VALID, loga e descarta)
    ↓
Copy pixels → std::vector (com mutex)
    ↓
Update(): retro_run()
    ↓
Render(): GrabVideoFrame() → Get frame from mutex buffer
    ↓
RetroScreenRenderer::UpdateVideoFrame(data, w, h, pitch)
    ↓
memcpy → ID2D1Bitmap1 (DXGI_FORMAT_B8G8R8A8_UNORM, ALPHA_IGNORE)
    ↓
RetroScreenRenderer::Render()
    ↓
BeginDraw() → ComputeLetterbox() → DrawBitmap() → EndDraw()
    ↓
SwapChain::Present()
```

## Key Technical Decisions

| Decisão | Escolha | Motivo |
|---------|---------|--------|
| Render path | Direct2D (D2D) framebuffer | Sem dependência OpenGL/ANGLE, sem shaders. `ID2D1Bitmap1` aceita `XRGB8888` diretamente via `CopyFromMemory`. |
| D2D pixel format | `DXGI_FORMAT_B8G8R8A8_UNORM` + `ALPHA_MODE_IGNORE` | Framebuffer é raw XRGB8888. `PREMULTIPLIED` distorce cores. |
| D2D DPI | `GetDpi()` do render target | Hardcoded 96.0f causa `D2DERR_BITMAP_BOUND_AS_TARGET` em high-DPI. |
| Audio API | SDL_QueueAudio | Já implementado, wrapper consistente (mas sem saída UWP ainda). |
| Input API | SDL + UWP fallback | Já implementado, cobre teclado + gamepad + Xbox |
| VFS | Cópia do RetroArch | Já testado no sandbox UWP, usa `CreateFile2FromAppW` |
| File reading | `StorageFile^` do picker → `FileIO::ReadBufferAsync` | UWP não permite `fopen` ou `GetFileFromPathAsync` para paths arbitrários. |
| Async pattern | Continuation chaining (`.then()`), sem `.get()` | `.get()` em WinRT `IAsyncOperation` da STA thread lança `invalid_operation`. |
| Lang | C++/CX (manter) | Scaffold já usa, migrar p/ C++/WinRT seria retrabalho |
| Core | Submodule schellingb/dosbox-pure | Atualizações do upstream via git pull |
| libretro-common | Cópia seletiva no `extern/` | Só precisamos de ~10 arquivos, não o repositório inteiro |
| Platforms | **x64 only** | ARM64/ARM/x86 não suportados. Xbox Series é x64. |
| HW render | Rejeitado (`SET_HW_RENDER` return 0) | Core fallback para SW automaticamente. Sem dependência OpenGL/ANGLE. |
