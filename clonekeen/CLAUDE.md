# CloneKeen → HP webOS TouchPad Port

## Status

**Working on device.** Touch input, menus, sprites, and gameplay are all functional. The port was developed and tested on a real HP TouchPad running webOS 3.0.5.

---

## What Was Done

### Files modified

**`src/sdl/viddrv.c`**
- `PDL_Init(0)` called before `SDL_Init()`, with `PDL_SetTouchAggression(PDL_AGGRESSION_MORETOUCHES)` and `PDL_GesturesEnable(PDL_FALSE)` to suppress the system gesture layer
- On webOS, `VidDrv_SetFullscreen()` creates a 1024×768 fullscreen SDL screen plus a separate 320×240 off-screen `webos_game_surface`
- `BlitSurface` and `VRAMPtr` both point to `webos_game_surface` (game renders there at native 320×240)
- `VidDrv_flipbuffer()` calls `SDL_SoftStretch(webos_game_surface → screen)` then draws the touch overlay, then `SDL_Flip()`
- **Critical fix**: `setpixel()` and `getpixel()` were writing to `screen` (1024×768). `SDL_SoftStretch` overwrites screen every frame, erasing everything written there. Fixed to use `BlitSurface` (the 320×240 game surface) — this is what made sprites and menus appear.
- Zoom options forced off (`OPT_ZOOM=0`, `OPT_ZOOMONRESTART=0`) to prevent double-scaling

**`src/sdl/keydrv.c`**
- Touch events delivered as SDL mouse events (SDL 1.2 on webOS)
- 7-zone touch layout on 1024×768 screen (see diagram below)
- `keytable[]` set directly for gameplay (rising-edge detection)
- `sdl_keysdown[]` mirrored for menu navigation (level-sensitive, checked via `KeyDrv_KeyIsDown()`)
- **Minhold timing**: key stays active for 150ms after finger lift (`SDL_GetTicks()` based). Prevents tap-release race between the game's independent logic and render timers.

**`build/webos/build-webos.sh`**
- Toolchain: Linaro GCC 4.9.4, prefix `arm-linux-gnueabi-`
- Paths: `TOOLCHAIN_BIN="/home/jonwise/linaro-toolchain/bin"`, `PDK="/opt/PalmPDK"`

**`build/webos/package-webos.sh`**
- Strip: `arm-linux-gnueabi-strip`
- Game data source: `../../gamedata-extracted/`
- Copies `icon.png` and `icon-256.png` from repo root

### Files created

**`src/platform/webos.cpp`** — entry point (identical to `linux.cpp`; webOS is Linux/ARM)

**`build/webos/build-webos.sh`** — ARM cross-compile script

**`build/webos/package-webos.sh`** — assembles the `.ipk` from binary + game data

**`build/webos/webos-pkg/appinfo.json`** — webOS app metadata, app ID `com.cmdrkeen.game`

**`build/webos/webos-pkg/CONTROL/control`** — Debian-format package control file

---

## Touch Layout

7 zones mapped on the 1024×768 screen:

```
┌─────────────────┬──────────────────┐
│    UP (KUP)     │   JUMP (KCtrl)   │  y < 192
├────────┬────────┴──────────┬────────┤
│  LEFT  │   MENU (KEsc)     │ RIGHT  │  192–575
│        │   also Confirm    │(Right  │
│        │   via sdl_keysdown│ =Enter)│
├────────┴──────────────────┴────────┤
│   DOWN (KDown)  │  FIRE (KAlt)     │  y > 575
└────────────────┬───────────────────┘
   x < 300    300–723    x > 723
              (split at x=512 for top/bottom zones)
```

Key-to-SDL mapping for menu navigation:
- `KCTRL` → `SDLK_UP` (menu up)
- `KALT` → `SDLK_DOWN` (menu down)
- `KRIGHT` → `SDLK_RETURN` (menu confirm/select)
- `KESC` → `SDLK_ESCAPE` (open/close menu)

---

## Architecture Notes

- **No OpenGL** — CloneKeen uses SDL 1.2 software surfaces. `PDL_LoadOGL` is not called (only needed for OpenGL apps).
- **8-bit palettized rendering** — Game renders into an 8-bit 320×240 surface. `SDL_SoftStretch` copies palette indices to the 1024×768 screen. Both surfaces share the same palette via `VidDrv_pal_apply()`.
- **Scale math** — 320×240 × 3.2 = 1024×768 exactly. No letterboxing.
- **PDL lifecycle** — `PDL_Init(0)` → `PDL_SetTouchAggression` → `PDL_GesturesEnable(PDL_FALSE)` → `SDL_Init()`.
- **Dual input arrays** — `keytable[]` uses rising-edge detection (gameplay); `sdl_keysdown[]` is level-sensitive (menus). Both must be kept in sync for touch input.

---

## Build Prerequisites (Linux host)

### 1. Linaro ARM toolchain

Linaro GCC 4.9.4 for `arm-linux-gnueabi`.

The build script expects it at `/home/jonwise/linaro-toolchain/bin/` — update `TOOLCHAIN_BIN` in `build-webos.sh`.

Verify: `arm-linux-gnueabi-gcc --version`

### 2. HP webOS PDK

Provides the device-side SDL 1.2, SDL_mixer, and PDL libraries for ARM.

The build script expects it at `/opt/PalmPDK/` — update `PDK` in `build-webos.sh`.

Key paths:
- Headers: `$PDK/include/` and `$PDK/include/SDL/`
- ARM device libs: `$PDK/device/lib/` — `libSDL.so`, `libSDL_mixer.so`, `libpdl.so`

### 3. Game data

Extract the game data before packaging:

```bash
cd clonekeen
unzip Releases/Linux/clonekeen-linux.zip -d gamedata-extracted/ -x keen
```

The packaging script copies everything from `gamedata-extracted/` into the IPK.

### 4. novacom (for deployment)

`novacom` is part of the PDK. Used to push the IPK to the device.

---

## Building

```bash
cd build/webos

# Edit TOOLCHAIN_BIN and PDK if your paths differ
nano build-webos.sh

# Build (incremental)
bash build-webos.sh

# Package into IPK
bash package-webos.sh
```

Output: `build/webos/com.cmdrkeen.game_1.0.0_armv7.ipk`

### Deploy to TouchPad

```bash
novacom put file:///media/internal/clonekeen.ipk < com.cmdrkeen.game_1.0.0_armv7.ipk
novacom run file:///usr/bin/ipkg install /media/internal/clonekeen.ipk
```

---

## Debugging on Device

The game writes a log to `ck.log` in its working directory:

```bash
novacom run file:///bin/cat /media/cryptofs/apps/usr/palm/applications/com.cmdrkeen.game/ck.log
```

---

## Known Limitations

1. **Single-touch only** — SDL 1.2 delivers one touch at a time. Can't hold LEFT and tap JUMP simultaneously.

2. **No pogo/fire split** — `KALT` (bottom-right) handles both pogo stick and raygun depending on game state.

3. **Episodes 2 & 3 data** — require separate data files not included in the repo.
