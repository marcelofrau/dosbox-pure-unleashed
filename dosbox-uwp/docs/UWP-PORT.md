# dosbox-pure — UWP Port Status

## Build Status

**Build: ✅ 0 errors** (Release|x64, ~1500 warnings C4244 cosméticos)
**Linker: ✅ 0 unresolved**
**Deploy: ❌ App não testada em device/emulador ainda**

## Estrutura

```
dosbox-uwp/
├── dosbox-uwp.vcxproj              — projeto principal (UWP AppX)
├── dosbox-uwp.sln                  — solução (requerida para $(SolutionDir) no x64)
├── Package.appxmanifest            — identidade do pacote UWP
├── App.cpp / App.h                 — entry point UWP (Direct3DApplicationSource)
├── dosbox_uwpMain.cpp / .h         — game loop (DirectX + SDL input)
├── dosbox_pure_sta.cpp             — [CRIADO] stubs DBPS_* standalone (9 funções + 2 globais)
├── uwp-dep.props                   — dependências SDL/uwp (x64 apenas)
│
├── local/dosbox-pure/              — [CRIADO] patches locais (submodule intocado)
│   ├── dosbox_pure_libretro.cpp    — bridge libretro (copiado, build via local)
│   └── src/
│       ├── dbp_serialize.cpp       — C2051 fix (switch __LINE__ → array)
│       ├── dos/drive_cache.cpp     — C2664 guard (GetVolumeInformationA UWP)
│       ├── gui/midi.cpp            — C2664 guard (midi_win32.h UWP)
│       └── misc/cross.cpp          — C2664 guard (FindFirstFile UWP) + stubs diretório
│
├── docs/
│   └── UWP-PORT.md                 — este documento
│
├── libretro-common/
│   ├── include/
│   │   ├── compat/msvc.h           — [CRIADO] strcasecmp→_stricmp, ssize_t, etc
│   │   └── retro_inline.h          — [CRIADO] macro INLINE
│   ├── vfs/
│   │   └── vfs_implementation_uwp.cpp — [CRIADO] stub VFS (StorageFile pendente)
│   ├── uwp/
│   │   ├── std_filesystem_compat.h
│   │   ├── uwp_async.h
│   │   ├── uwp_file_handle_access.h
│   │   └── uwp_func.h
│   ├── compat/compat_strl.c        — [CRIADO] strlcpy/strlcat
│   └── encodings/encoding_utf.c    — [CRIADO] UTF-8/16 conversion
│
├── Content/                        — DirectX shaders
├── Common/                         — DirectX helpers
├── Assets/                         — ícones/logos AppX
└── pch.h / pch.cpp                 — precompiled header

extern/
├── dosbox-pure/                    — submodule (pristine, commit 65dd07d)
└── libretro-common/                — submodule
```

## Estratégia de Patches

**Decisão:** Submodule `extern/dosbox-pure/` permanece intocável (pristine).
Arquivos que precisam de modificação são **copiados** para `dosbox-uwp/local/dosbox-pure/`
com a mesma estrutura de diretórios, e o vcxproj aponta para o local.

```
$(DBPDir)src\gui\midi.cpp                    → local\dosbox-pure\src\gui\midi.cpp
$(DBPDir)src\dos\drive_cache.cpp             → local\dosbox-pure\src\dos\drive_cache.cpp
$(DBPDir)src\misc\cross.cpp                  → local\dosbox-pure\src\misc\cross.cpp
$(DBPDir)src\dbp_serialize.cpp               → local\dosbox-pure\src\dbp_serialize.cpp
$(DBPDir)dosbox_pure_libretro.cpp            → local\dosbox-pure\dosbox_pure_libretro.cpp
```

**Próximos arquivos que precisarem de patch** devem seguir o mesmo padrão.

---

## Includes no vcxproj

```
$(ProjectDir)                          — dosbox-uwp\
$(IntermediateOutputPath)              — x64\Release\ (precompiled headers)
$(DBPDir)                              — extern\dosbox-pure\
$(DBPDir)include                       — dosbox.h, etc
$(DBPDir)libretro-common\include       — libretro.h (versão do dosbox-pure)
$(DBPDir)src                           — headers raiz de src/
$(DBPDir)src\gui                       — midi_oss.h, midi_alsa.h (para local\midi.cpp)
$(DBPDir)src\misc                      — cross.h (para local\cross.cpp)
$(DBPDir)src\dos                       — drives.h, etc (para local\drive_cache.cpp)
$(LrCommonDir)                         — uwp/*.h
$(LrCommonDir)include                  — retro_miscellaneous.h, retro_environment.h, etc
```

**Nota:** `$(DBPDir)src\gui;$(DBPDir)src\misc;$(DBPDir)src\dos` adicionados para que
arquivos locais (`local\dosbox-pure\src\gui\midi.cpp` etc.) consigam incluir headers
irmãos via `#include "midi_oss.h"` — que antes resolviam pelo diretório do source original.

---

## Modificações nos Arquivos Locais

### 1. MIDI — guard UWP

**Arquivo:** `local/dosbox-pure/src/gui/midi.cpp:93-95`

```cpp
// Original:
#elif defined(WIN32)
// Patch:
#elif defined(WIN32) && (!defined(WINAPI_FAMILY) || WINAPI_FAMILY != WINAPI_FAMILY_APP)
```

**Motivo:** UWP não tem `winmm.lib`/`mmeapi.h` (API Win32 MIDI).
**Impacto:** MIDI Win32 desligado no UWP. Compila `midi_oss.h` no `#else` (não funcional).

---

### 2. C4703 — suprimido globalmente

**Decisão:** Ao invés de patchear 5 headers do submodule, C4703 é suprimido globalmente
no vcxproj via `DisableSpecificWarnings`:

```xml
<DisableSpecificWarnings>4453;28204;4703</DisableSpecificWarnings>
```

**Afetados (não precisam mais de patch):**
- `src/cpu/core_dyn_x86/string.h` — `tmp_reg`, `rep_ecx_jmp`
- `src/cpu/core_dyn_x86/decoder.h` — `segbase`
- `src/cpu/core_dyn_x86/risc_x64.h` — `func`
- `src/dbp_network.cpp` — `str`, `code`

**Motivo:** MSVC UWP usa análise de fluxo mais agressiva. Variáveis são sempre setadas
antes do uso; warnings são falsos positivos.

---

### 3. C2051 — case expression not constant

**Arquivo:** `local/dosbox-pure/src/dbp_serialize.cpp:302-360`

**Problema:** `case __LINE__:` não aceito como constante pelo MSVC no toolchain UWP.

**Solução:** `switch(__LINE__)` substituído por array de structs:

```cpp
struct { void (*setup)(DBPArchive&); bool (*check)(DBPArchive&); } entries[] = {
    DBPSERIALIZE_ENTRY(DBPSerialize_DOS),
    DBPSERIALIZE_ENTRY_FVER(DBPSerialize_Mounts, >=8),
    // ... todas as 36 funções serialize
};
```

**Forward declarations** via macro `DBPSERIALIZE_DECL`.
**Impacto:** Comportamento idêntico — preserva ordem, version checks, layout.

---

### 4. C2664 — Win32 APIs ANSI no UWP

#### drive_cache.cpp

**Arquivo:** `local/dosbox-pure/src/dos/drive_cache.cpp:140-148`

```cpp
// Original:
#if defined(WIN32) || defined(OS2)
// Patch:
#if (defined(WIN32) || defined(OS2)) && (!defined(WINAPI_FAMILY) || WINAPI_FAMILY != WINAPI_FAMILY_APP)
```

**APIs removidas:** `GetVolumeInformationA`, `GetDriveTypeA`
**Impacto:** Volume label e CD-ROM detection desligados no UWP.

#### cross.cpp — diretórios

**Arquivo:** `local/dosbox-pure/src/misc/cross.cpp:188`

```cpp
// Original:
#if defined (WIN32)
// Patch:
#if defined (WIN32) && (!defined(WINAPI_FAMILY) || WINAPI_FAMILY != WINAPI_FAMILY_APP)
```

**APIs removidas:** `FindFirstFile`, `FindNextFile`, `FindClose`
**Stubs UWP adicionados** (`#elif defined(WIN32)`):

| Função | Stub |
|---|---|
| `open_directory` | `return NULL` |
| `read_directory_first` | `return false` |
| `read_directory_next` | `return false` |
| `close_directory` | `no-op` |

**Impacto:** Directory enumeration não funciona no UWP via Win32.
Usar VFS (`retro_vfs_file_*`, `retro_vfs_dir_*`).

---

### 5. Libretro-common compat — headers criados

| Header | Conteúdo |
|---|---|
| `compat/msvc.h` | `strcasecmp→_stricmp`, `strncasecmp→_strnicmp`, `ssize_t`, `PATH_MAX`, `SIZE_MAX`, `mkdir→_mkdir` |
| `retro_inline.h` | `#define INLINE __inline` (Win32) |
| `compat/compat_strl.c` | `strlcpy`, `strlcat` |
| `encodings/encoding_utf.c` | UTF-8/16 conversion |

Ausentes no checkout atual do libretro-common.

---

## Linker: DBPS_* — resolvido (16 → 0)

Adicionados 2 arquivos ao build:

| Arquivo | Símbolos |
|---|---|
| `extern/dosbox-pure/keyb2joypad.cpp` | `map_keys`, `map_buckets` |
| `dosbox-uwp/dosbox_pure_sta.cpp` | `DBPS_SaveSlotIndex`, `DBPS_BrowsePath`, `DBPS_OnContentLoad`, `DBPS_SubmitOSDFrame`, `DBPS_GetMouse`, `DBPS_StartCaptureJoyBind`, `DBPS_HaveJoy`, `DBPS_GetJoyBind`, `DBPS_RequestSaveLoad`, `DBPS_HaveSaveSlot`, `DBPS_ApplyConfigOverrides`, `DBPS_IsConfigOverride`, `DBPS_ToggleConfigOverride`, `DBPS_GetConfigOverrideJSON` |

**7 DBPS_* fornecidos por `dosbox_pure_osd.h`** (incluído por `dosbox_pure_libretro.cpp` quando `DBP_STANDALONE`):
`DBPS_AddDisc`, `DBPS_OpenContent`, `DBPS_IsGameRunning`, `DBPS_IsShowingOSD`,
`DBPS_GetContentName`, `DBPS_ToggleOSD`, `DBPS_InitLEDs`

**Stubs** (`dosbox_pure_sta.cpp`) compilam mas não implementam — retornam valores default
ou no-op. Implementação real (SDL2 input, DirectX OSD, WinRT file picker) pendente.

---

## Próximos Passos

### Prioridade 1 — mínimo funcional

- [ ] **Libretro loop**: Inicializar `retro_init`, `retro_load_game`, `retro_run` no
      `dosbox_uwpMain::Update()` — chamar core em vez de renderizar fundo azul
- [ ] **VFS real**: Implementar `vfs_implementation_uwp.cpp` com `StorageFile`/`StorageFolder`
      para abrir arquivos de ROM
- [ ] **Video output**: Renderizar buffer RGBA do core (`retro_video_refresh_t`) como
      textura DirectX 2D

### Prioridade 2 — interação

- [ ] **Input mapping**: Mapear eventos de teclado/touch UWP → botões RetroArch
      (`CoreWindow::KeyDown` → `retro_key_state`)
- [ ] **File picker**: UI para selecionar ROM via WinRT `FileOpenPicker`
- [ ] **Áudio**: Roteir `retro_audio_sample_batch` → XAudio2

### Prioridade 3 — deploy & polish

- [ ] **Assinar AppX**: Certificado de teste para deploy local
- [ ] **Deploy**: Instalar em device/emulador Windows 10/11
- [ ] **Test**: Verificar inicialização do core, carregamento de jogo
- [ ] **MIDI OSS**: Stub ou implementar `midi_oss.h` compilável no UWP
- [ ] **Directory enum**: Implementar via `StorageFolder::GetFilesAsync`
- [ ] **DBPS_* reais**: Implementar `OpenContent`, `AddDisc`, `ToggleOSD`, etc.

---

## Notas Técnicas

- **UWP SDK** `10.0.26100.0` (VS2022 v143 toolchain)
- `CompileAsWinRT=false` para código legado (dosbox-pure) — evita conflitos com WinRT
- `AppxPackageSigningEnabled=true` — requer certificado para deploy
- `$(SolutionDir)` necessário no x64 para resolver `uwp-dep.props` (SDL include path)
- ARM64 build pelo vcxproj direto (uwp-dep.props não se aplica)
- Submodule `extern/dosbox-pure/` NÃO recebe commits — patches em `local/dosbox-pure/`
- Qualquer novo arquivo do submodule que precise de modificação: copiar para
  `local/dosbox-pure/` com mesma estrutura, apontar vcxproj
