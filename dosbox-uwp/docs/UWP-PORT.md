# dosbox-pure — UWP Port Status

## Estrutura

```
dosbox-uwp/
  dosbox-uwp.vcxproj              — projeto principal, referência $(DBPDir)=..\extern\dosbox-pure\
  dosbox_pure_libretro.cpp        — bridge: inclui vfs_implementation_uwp.cpp
  libretro-common/
    include/
      compat/msvc.h               — [CRIADO] compat MSVC (strcasecmp, ssize_t, etc)
      retro_inline.h              — [CRIADO] define macro INLINE
    vfs/
      vfs_implementation_uwp.cpp  — [CRIADO] stub VFS UWP (WinRT)
    uwp/
      std_filesystem_compat.h     — existente no submodule
      uwp_async.h                 ┐
      uwp_file_handle_access.h    ┤ existentes
      uwp_func.h                  ┘
extern/
  dosbox-pure/                    — submodule (modificado localmente, 8 arquivos)
  libretro-common/                — submodule
```

## Includes no vcxproj

```
$(DBPDir)
$(DBPDir)include
$(DBPDir)libretro-common\include   — libretro.h (versão do dosbox-pure)
$(LrCommonDir)                     — uwp/*.h
$(LrCommonDir)include              — retro_miscellaneous.h, retro_environment.h, etc
```

## Modificações no dosbox-pure

### 1. MIDI — stubbed para UWP

**Arquivo:** `src/gui/midi.cpp:95`

```cpp
// Antes:
#elif defined(WIN32)
// Depois:
#elif defined(WIN32) && (!defined(WINAPI_FAMILY) || WINAPI_FAMILY != WINAPI_FAMILY_APP)
```

**Motivo:** UWP não tem `winmm.lib`/`mmeapi.h` (API Win32 MIDI).
**Impacto:** MIDI Win32 desligado no UWP. Se precisar MIDI, implementar via WinRT ou stub.

---

### 2. C4703 — ponteiros não inicializados

| Arquivo | Linha | Variável |
|---|---|---|
| `src/cpu/core_dyn_x86/string.h` | 32 | `tmp_reg = NULL` |
| `src/cpu/core_dyn_x86/string.h` | 82 | `rep_ecx_jmp = NULL` |
| `src/cpu/core_dyn_x86/decoder.h` | 1087 | `segbase = NULL` |
| `src/cpu/core_dyn_x86/risc_x64.h` | 1123 | `func = NULL` |
| `src/dbp_network.cpp` | 1091 | `char const *str = ""` + `int code = 0` |

**Motivo:** MSVC UWP trata variante mais agressiva de análise de fluxo.
**Impacto:** Nenhum — inicializações defensivas, caminhos sempre setam valor antes de uso real.

---

### 3. C2051 — case expression not constant

**Arquivo:** `src/dbp_serialize.cpp:302-360`

**Problema:** `case __LINE__:` não é aceito como constante pelo MSVC no toolchain UWP (Arcade/SDL).

**Solução:** Substituiu `switch(__LINE__)` por array de structs:

```cpp
struct { void (*setup)(DBPArchive&); bool (*check)(DBPArchive&); } entries[] = {
    DBPSERIALIZE_ENTRY(DBPSerialize_DOS),
    DBPSERIALIZE_ENTRY_FVER(DBPSerialize_Mounts, >=8),
    // ... todas as 36 funções serialize
};
for (i in entries) {
    if (entry.check && !entry.check(ar)) continue;
    entry.setup(ar);
}
```

**Forward declarations** explícitas adicionadas (macro `DBPSERIALIZE_DECL`).
**Impacto:** Comportamento idêntico — preserva ordem de serialização, version checks, e layout compat.

---

### 4. C2664 — Win32 APIs ANSI vs Unicode no UWP

#### drive_cache.cpp

**Arquivo:** `src/dos/drive_cache.cpp:140-148`

```cpp
// Antes:
#if defined(WIN32) || defined(OS2)
// Depois:
#if (defined(WIN32) || defined(OS2)) && (!defined(WINAPI_FAMILY) || WINAPI_FAMILY != WINAPI_FAMILY_APP)
```

**APIs removidas:** `GetVolumeInformationA`, `GetDriveTypeA`
**Impacto:** Volume label e CD-ROM detection desligados no UWP. Sem impacto funcional em jogos.

#### cross.cpp — diretórios

**Arquivo:** `src/misc/cross.cpp:188`

```cpp
// Antes:
#if defined (WIN32)
// Depois:
#if defined (WIN32) && (!defined(WINAPI_FAMILY) || WINAPI_FAMILY != WINAPI_FAMILY_APP)
```

**APIs removidas:** `FindFirstFile`, `FindNextFile`, `FindClose`
**Stubs UWP adicionados** (`#elif defined(WIN32)`) — todas 4 funções de diretório retornam NULL/false:

| Função | Stub |
|---|---|
| `open_directory` | `return NULL` |
| `read_directory_first` | `return false` |
| `read_directory_next` | `return false` |
| `close_directory` | `no-op` |

**Impacto:** Directory enumeration não funciona no UWP via Win32. Filesystem deve usar VFS (`retro_vfs_file_*`, `retro_vfs_dir_*`). Jogos que dependem de `dir_information` (drive_cache, drives local) podem quebrar no UWP.

---

## Headers faltantes do libretro-common — criados

| Header | Conteúdo |
|---|---|
| `compat/msvc.h` | `strcasecmp→_stricmp`, `strncasecmp→_strnicmp`, `ssize_t`, `PATH_MAX`, `SIZE_MAX`, warning disables, `mkdir→_mkdir` |
| `retro_inline.h` | `#define INLINE __inline` (Win32) ou `inline` |

Ambos copiados do upstream libretro-common. Ausentes no checkout atual.

---

## To-Do (próximos passos)

## Linker: DBPS_* — resolved (16 undefined → 0)

Adicionados 2 arquivos ao build:

| Arquivo | Símbolos fornecidos |
|---|---|
| `extern\dosbox-pure\keyb2joypad.cpp` | `map_keys`, `map_buckets` |
| `dosbox-uwp\dosbox_pure_sta.cpp` | `DBPS_SaveSlotIndex`, `DBPS_BrowsePath`, `DBPS_OnContentLoad`, `DBPS_SubmitOSDFrame`, `DBPS_GetMouse`, `DBPS_StartCaptureJoyBind`, `DBPS_HaveJoy`, `DBPS_GetJoyBind`, `DBPS_RequestSaveLoad`, `DBPS_HaveSaveSlot`, `DBPS_ApplyConfigOverrides`, `DBPS_IsConfigOverride`, `DBPS_ToggleConfigOverride`, `DBPS_GetConfigOverrideJSON` |

**Nota:** `DBPS_AddDisc`, `DBPS_OpenContent`, `DBPS_IsGameRunning`, `DBPS_IsShowingOSD`, `DBPS_GetContentName`, `DBPS_ToggleOSD`, `DBPS_InitLEDs` são fornecidos por `dosbox_pure_osd.h` (incluído por `dosbox_pure_libretro.cpp`) quando `DBP_STANDALONE` está definido — **não duplicar** no sta.cpp.

**Status:** Build succeeded (0 errors, ~1500 warnings — maioria C4244).

---

### Prioridade Alta — compilar

- [x] **Linker: DBPS_\*, map_keys, map_buckets** — resolvido com `keyb2joypad.cpp` + `dosbox_pure_sta.cpp`
- [ ] **`Winsock2.h` / `windows.h`**: Muitos arquivos incluem `windows.h` direta ou indiretamente. No UWP, certas APIs são bloqueadas em tempo de link (ex: `CreateFile`, `GetProcAddress`, `regapi.h`). Será necessário stub ou guardar cada chamada.
- [ ] **`direct.h`**: `_getcwd`, `_chdir`, `_mkdir` não disponíveis no UWP. Alternativa: WinRT `Windows::Storage`.
- [ ] **`process.h`**: `_beginthreadex`, `_endthreadex` — verificar se estão disponíveis. Alternativa: `CreateThread` (UWP permite threads).
- [ ] **`io.h`**: `_access`, `_chmod`, `_open`, `_read`, `_write`, `_close` — muitos têm equivalentes WinRT ou devem ser redirecionados ao VFS.
- [ ] **`sys/stat.h`**: `stat`, `fstat`, `S_ISDIR`, `S_ISREG` — stubs ou VFS.
- [ ] **`signal.h`**: `signal()` — UWP restringe sinais. Stub se necessário.
- [ ] **`timeb.h`**: `_ftime`, `_ftime64` — verificar disponibilidade no SDK UWP.
- [ ] **VFS implementation**: `vfs_implementation_uwp.cpp` atual é apenas stub. Implementar de verdade com `StorageFile`, `StorageFolder`, etc.

### Prioridade Média — funcionais

- [ ] **MIDI alternativo**: Implementar MIDI via WinRT `Windows.Devices.Midi` se necessário.
- [ ] **Directory enumeration**: Implementar `open_directory`/`read_directory_first`/`read_directory_next`/`close_directory` para UWP usando `StorageFolder::GetFilesAsync` / `GetFoldersAsync`.
- [ ] **Drive cache / volume label**: Stub no UWP. Se necessário, implementar com `Windows::Storage::KnownFolders`.
- [ ] **Network (dbp_network.cpp)**: Verificar compatibilidade de sockets (`Winsock2.h`) no UWP (`Windows::Networking::Sockets`).
- [ ] **Registry access**: Se houver chamadas a `regapi.h`/registry, stubbear ou reimplementar com `ApplicationData::LocalSettings`.

### Prioridade Baixa — polish

- [ ] **Performance**: Verificar se `CompileAsWinRT=false` está correto para todos os arquivos (exceto UWP bridge).
- [ ] **Warning cleanup**: Habilitar warnings gradualmente.
- [ ] **Testes**: Verificar se o core carrega no RetroArch UWP e executa jogos.

---

## Notas Técnicas

- **UWP SDK** disponível via VS2022 (`10.0.28000.0`)
- `CompileAsWinRT=false` para código legado C++ (dosbox-pure) — evita conflitos com WinRT tipos
- Ponteiros UWP usam `Platform::String^` e `Windows::Storage` — código UWP deve ficar isolado em `libretro-common/vfs/vfs_implementation_uwp.cpp`
- Submodule `extern/dosbox-pure/` NÃO dá commit — modificações são locais. Decisão: copiar arquivos para o projeto se o submodule for atualizado frequentemente, ou manter patches locais.
