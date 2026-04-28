# Commander Keen → HP webOS TouchPad Port

## Goal

Port Commander Keen (Vorticons trilogy, episodes 1–3) to the HP TouchPad running webOS, distributed as an installable `.ipk` package. The TouchPad is an ARM Linux device running a legacy SDL 1.2 stack via HP's Palm Development Kit (PDK).

---

## Repository Structure

| Directory | Contents |
|---|---|
| `clonekeen/` | CloneKeen — the SDL port of Keen Vorticons ported to webOS. **Primary work.** See `clonekeen/CLAUDE.md` for full detail. |
| `sdl-sopwith-webos/` | Bos Wars/Sopwith — a working OpenGL SDL game already ported to this device. Reference for build structure only; it uses OpenGL, CloneKeen does not. |
| `keen/` | Commander Keen in Keen Dreams — original Borland C source, reference only. |

---

## Target Platform

- **Device**: HP TouchPad (2011)
- **OS**: webOS 3.0.5 (Linux/ARM, kernel 2.6.35)
- **CPU**: Qualcomm Snapdragon APQ8060, dual-core Cortex-A8
- **Screen**: 1024×768 IPS, landscape
- **SDK**: HP webOS PDK (Palm Development Kit) — provides SDL 1.2, SDL_mixer, PDL (`libpdl.so`)

---

## Toolchain

Cross-compilation from a Linux host:

- **Compiler**: Linaro GCC 4.9.4 for `arm-linux-gnueabi`
  - Expected at `/home/jonwise/linaro-toolchain/bin/` — update `TOOLCHAIN_BIN` in `build-webos.sh`
  - Prefix: `arm-linux-gnueabi-gcc` (not `arm-none-linux-gnueabi-gcc`)
- **PDK**: HP webOS PDK at `/opt/PalmPDK/`
  - Headers: `$PDK/include/`, `$PDK/include/SDL/`
  - ARM device libs: `$PDK/device/lib/` (`libSDL.so`, `libSDL_mixer.so`, `libpdl.so`)
- **Key flags**: `-mcpu=cortex-a8 -mfpu=neon -mfloat-abi=softfp -D__webos__`

---

## Build & Deploy

```bash
cd clonekeen/build/webos
nano build-webos.sh       # set TOOLCHAIN_BIN and PDK paths for your system
bash build-webos.sh       # cross-compile → fbuild/webos/clonekeen
bash package-webos.sh     # assemble IPK → com.cmdrkeen.game_1.0.0_armv7.ipk

# Deploy to device via novacom
novacom put file:///media/internal/com.cmdrkeen.game_1.0.0_armv7.ipk < com.cmdrkeen.game_1.0.0_armv7.ipk
novacom run file:///usr/bin/ipkg install /media/internal/com.cmdrkeen.game_1.0.0_armv7.ipk
```

---

## Status

**Working on device.** Episodes 1–3 load and play. Touch controls functional. See `clonekeen/CLAUDE.md` for architecture details, known limitations, and next steps.
