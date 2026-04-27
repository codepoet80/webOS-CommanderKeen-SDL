# Commander Keen → HP webOS TouchPad Port

## Goal

Port Commander Keen (Vorticons trilogy, episodes 1–3) to the HP TouchPad running webOS, distributed as an installable `.ipk` package. The TouchPad is an ARM Linux device running a legacy SDL 1.2 stack via HP's Palm Development Kit (PDK).

---

## Repository Structure

| Directory | Contents |
|---|---|
| `clonekeen/` | CloneKeen — the SDL port of Keen Vorticons being ported to webOS. **This is the primary work.** See `clonekeen/CLAUDE.md` for full detail. |
| `sdl-sopwith-webos/` | Sopwith (Bos Wars) — a working SDL game already ported to this exact device. Serves as the build and PDL init reference. |
| `keen/` | Commander Keen in Keen Dreams — original Borland C source, included for reference only. |

---

## Target Platform

- **Device**: HP TouchPad (2011)
- **OS**: webOS (Linux/ARM, kernel 2.6)
- **CPU**: Qualcomm Snapdragon APQ8060, dual-core Cortex-A8
- **Screen**: 1024×768 IPS
- **SDK**: HP webOS PDK (Palm Development Kit) — provides SDL 1.2, SDL_mixer, PDL (`libpdl.so`)

---

## Toolchain

Cross-compilation from a Linux host:

- **Compiler**: Linaro GCC 4.x for `arm-none-linux-gnueabi` (2011.03 vintage)
  - Expected at `/home/stark/arm-2011.03/bin/` — update `TOOLCHAIN_BIN` in build scripts
- **PDK**: HP webOS PDK
  - Expected at `/home/stark/HPwebOS/PDK/`
  - Headers: `$PDK/include/`, `$PDK/include/SDL/`
  - ARM device libs: `$PDK/device/lib/` (`libSDL.so`, `libSDL_mixer.so`, `libpdl.so`)
- **Key flags**: `-mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -D__webos__`

---

## Build & Deploy (CloneKeen)

```bash
cd clonekeen/build/webos
nano build-webos.sh       # set TOOLCHAIN_BIN and PDK paths
bash build-webos.sh       # cross-compile → produces binary
bash package-webos.sh     # assemble IPK

# Deploy to device via novacom
novacom put file:///media/internal/clonekeen.ipk < com.cmdrkeen.game_1.0.0_armv7.ipk
novacom run file:///usr/bin/ipkg install /media/internal/clonekeen.ipk
```

Output: `build/webos/com.cmdrkeen.game_1.0.0_armv7.ipk`

---

## Reference: sopwith PDL Init Pattern

The `sdl-sopwith-webos/` port is the proven template. Its PDL init in `engine/video/sdl.cpp`:

```cpp
#ifdef __webos__
    PDL_Init(0);
    PDL_LoadOGL(PDL_OGL_1_1);  // sopwith uses GL; clonekeen skips this
#endif
SDL_Init(...);
```

CloneKeen uses software rendering (SDL 1.2 surfaces only), so `PDL_LoadOGL` is omitted.

---

## Status

Code changes to CloneKeen are complete. The build has not yet been compiled or tested on hardware — that requires a Linux host with the ARM toolchain and PDK installed.

See `clonekeen/CLAUDE.md` for the full list of modified and created files, touch input layout, known limitations, and next steps.
