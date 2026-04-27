# CloneKeen → HP webOS TouchPad Port

## Goal

Port CloneKeen (Commander Keen: Invasion of the Vorticons, episodes 1-3) to the HP TouchPad tablet running webOS. The TouchPad runs a legacy ARM Linux with a 3-layer compositor managed by HP's PDL (Palm Development Library).

Reference port: `../sdl-sopwith-webos/` — a working SDL game ported to the same device. Its `build-webos.sh` and the PDL init pattern in its SDL code are the template for everything here.

---

## What Has Been Done

All code changes are complete. The build infrastructure exists but has not yet been compiled — that requires a Linux host with the ARM toolchain installed.

### Files modified

**`src/bootstrap.cpp`**
Added `#elif defined(__webos__)` before `__unix__` so the webOS build picks `platform/webos.cpp` instead of `platform/linux.cpp`.

**`src/sdl/viddrv.c`**
- `PDL_Init(0)` called before `SDL_Init()` (required for all webOS PDK apps)
- On webOS, `VidDrv_SetFullscreen()` creates a 1024×768 fullscreen SDL screen plus a separate 320×240 off-screen `webos_game_surface`
- `BlitSurface` points to `webos_game_surface` (game renders here at native resolution)
- `VidDrv_flipbuffer()` calls `SDL_SoftStretch()` to scale 320×240 → 1024×768 before `SDL_Flip()`. 320×240 scales to exactly 3.2× in both axes — perfect fullscreen with no letterboxing.
- Palette is applied to both surfaces; `webos_game_surface` is freed in `VidDrv_Stop()`

**`src/sdl/keydrv.c`**
Touch input mapped via SDL mouse events (SDL 1.2 on webOS delivers touch as mouse). Touch zone layout on 1024×768 screen:

```
┌──────────────────────────────────┐
│            JUMP (Ctrl)           │  y < 192
├────────┬─────────────────┬───────┤
│  LEFT  │  ENTER (menus)  │ RIGHT │  192–575
├────────┴─────────────────┴───────┤
│         FIRE/POGO (Alt)          │  y > 575
└──────────────────────────────────┘
  x<300        300–723        x>723
```

Finger drags in the mid zone update the directional key live. Only one touch tracked at a time (SDL 1.2 limitation).

### Files created

**`src/platform/webos.cpp`** — entry point (identical to `linux.cpp`; webOS is Linux/ARM)

**`build/webos/build-webos.sh`** — ARM cross-compile script. Edit `TOOLCHAIN_BIN` and `PDK` paths before running.

**`build/webos/package-webos.sh`** — assembles the `.ipk` from the binary and game data.

**`build/webos/webos-pkg/appinfo.json`** — webOS app metadata, app ID `com.cmdrkeen.game`

**`build/webos/webos-pkg/CONTROL/control`** — Debian-format package control file

---

## Build Prerequisites (Linux host)

### 1. Linaro ARM toolchain

The same toolchain used by the reference sopwith port: **Linaro GCC 2011.03** for `arm-none-linux-gnueabi`.

Download from: https://launchpad.net/gcc-arm-embedded or the CodeSourcery archive.

The build script expects it at `/home/stark/arm-2011.03/bin/` — change `TOOLCHAIN_BIN` in `build-webos.sh` to match your install path.

Verify: `arm-none-linux-gnueabi-gcc --version` should print a 4.x GCC.

### 2. HP webOS PDK

The Palm Development Kit provides the device-side SDL 1.2, SDL_mixer, and PDL libraries for ARM. It also includes `PDL.h`.

The build script expects it at `/home/stark/HPwebOS/PDK/` — change `PDK` in `build-webos.sh` to match.

Key paths inside the PDK:
- Headers: `$PDK/include/` and `$PDK/include/SDL/`
- ARM device libs: `$PDK/device/lib/` — contains `libSDL.so`, `libSDL_mixer.so`, `libpdl.so`

### 3. novacom (for deployment)

`novacom` is the webOS device communication tool, also part of the PDK. Used to push the IPK to the device.

---

## Building

```bash
cd build/webos

# Edit TOOLCHAIN_BIN and PDK variables at the top of the script
nano build-webos.sh

# Build (incremental — only recompiles changed files)
bash build-webos.sh

# Package into IPK
bash package-webos.sh
```

Output: `build/webos/com.cmdrkeen.game_1.0.0_armv7.ipk`

### Deploy to TouchPad

```bash
# Copy IPK to device internal storage
novacom put file:///media/internal/clonekeen.ipk < com.cmdrkeen.game_1.0.0_armv7.ipk

# Install
novacom run file:///usr/bin/ipkg install /media/internal/clonekeen.ipk
```

---

## Game Data

CloneKeen needs the original Keen episode data files. Episode 1 is included in the repo under `data/GAMEDATA/KEEN1/`. The packaging script copies these into the IPK alongside the binary.

The game also uses files from `bin/` (graphics, sound, config, demo files) — these are also packaged.

At runtime the game looks for data files relative to its working directory. The webOS launcher sets the working directory to the app's install path (`/media/cryptofs/apps/usr/palm/applications/com.cmdrkeen.game/`).

---

## Architecture Notes

- **No OpenGL needed** — CloneKeen uses SDL 1.2 software surfaces throughout. The reference sopwith port uses OpenGL ES, but we avoid that complexity entirely.
- **SDL version** — SDL 1.2 (the version in the webOS PDK). CloneKeen was written for SDL 1.2.
- **8-bit palettized rendering** — The game renders into an 8-bit surface. `SDL_SoftStretch` between two 8-bit surfaces copies palette indices; colors come from the screen surface's palette. Both surfaces get the same palette via `VidDrv_pal_apply()`.
- **Scale math** — 320×240 × 3.2 = 1024×768 exactly. No letterboxing, no aspect ratio correction needed.
- **PDL lifecycle** — `PDL_Init(0)` must be called before `SDL_Init()`. We don't call `PDL_LoadOGL()` since we have no OpenGL.

---

## Known Limitations / Next Steps

1. **Single-touch only** — SDL 1.2 delivers one touch at a time as mouse events. Can't hold LEFT and tap JUMP simultaneously. A future improvement would use PDL's native multitouch API or restructure input around gesture zones.

2. **No pogo stick shortcut** — currently KALT (bottom zone) fires both pogo and raygun depending on game state. May want a dedicated fire zone separate from pogo.

3. **Episode 2 & 3 data** — only episode 1 data (`data/GAMEDATA/KEEN1/`) is in the repo. Episodes 2 and 3 require the original shareware/commercial files.

4. **Icon** — add a 64×64 `icon.png` to `build/webos/webos-pkg/` before packaging so the app appears correctly in the webOS launcher.

5. **First run: test with `lprintf` log** — the game writes a log to `ck.log` in its working directory. If the game crashes on startup, `novacom run file:///bin/cat /media/cryptofs/apps/usr/palm/applications/com.cmdrkeen.game/ck.log` will show the reason.

---

## Reference: sopwith PDL init pattern (from `../sdl-sopwith-webos/engine/video/sdl.cpp`)

```cpp
#ifdef __webos__
    PDL_Init(0);
    PDL_LoadOGL(PDL_OGL_1_1);  // sopwith uses GL; we skip this
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
#endif
SDL_Init(...);
```

Our version (in `src/sdl/viddrv.c`) omits `PDL_LoadOGL` since we use software rendering.
