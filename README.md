# Commander Keen: Invasion of the Vorticons — HP TouchPad Port

A port of [CloneKeen](https://sourceforge.net/projects/clonekeen/) (the open-source SDL remake of Commander Keen episodes 1–3) to the HP TouchPad running webOS 3.0.5. All three episodes are playable with multi-touch d-pad controls.

---

## Repository Layout

```
clonekeen/          CloneKeen source — the primary work
  src/              Game source code
    platform/       Platform entry points; webos.cpp is the webOS main()
    sdl/            SDL drivers: keydrv.c (touch input), viddrv.c (rendering)
  build/webos/      Build and packaging scripts for webOS
    build-webos.sh  Cross-compile script
    package-webos.sh  Assemble IPK
    webos-pkg/      IPK metadata (appinfo.json, CONTROL/)
    fbuild/webos/   Compiled binary output (generated)
keen/               Original Keen Dreams Borland C source — reference only
sdl-sopwith-webos/  Reference webOS PDK app — build structure reference only
```

---

## Target Platform

| | |
|---|---|
| **Device** | HP TouchPad (2011) |
| **OS** | webOS 3.0.5 (Linux/ARM, kernel 2.6.35) |
| **CPU** | Qualcomm Snapdragon APQ8060, dual-core Cortex-A8 |
| **Display** | 1024×768 IPS, landscape |
| **SDK** | HP webOS PDK — SDL 1.2, SDL_mixer, PDL (`libpdl.so`) |

---

## Build Prerequisites

### 1. Linaro ARM Toolchain

Linaro GCC 4.9.4 for `arm-linux-gnueabi`.

Download from [releases.linaro.org](https://releases.linaro.org/archive/14.09/components/toolchain/binaries/) and unpack somewhere convenient (e.g. `~/linaro-toolchain/`).

Verify: `arm-linux-gnueabi-gcc --version`

Update `TOOLCHAIN_BIN` in `build-webos.sh` if your path differs from `/home/jonwise/linaro-toolchain/bin`.

### 2. HP webOS PDK

The Palm Development Kit provides the device-side SDL 1.2, SDL_mixer, and PDL headers and ARM libraries.

Default install path: `/opt/PalmPDK/`

Key paths used by the build:
- Headers: `$PDK/include/` and `$PDK/include/SDL/`
- ARM libraries: `$PDK/device/lib/` (`libSDL.so`, `libSDL_mixer.so`, `libpdl.so`)

### 3. CloneKeen Game Data

The bundled shareware data (episode 1) ships inside the IPK. Extract it before packaging:

```bash
cd clonekeen
unzip Releases/Linux/clonekeen-linux.zip -d gamedata-extracted/ -x keen
```

The packaging script copies everything from `gamedata-extracted/` into the IPK, including the `data/`, `gfx/`, and `custom/` directories.

### 4. novacom (deploy only)

`novacom` is included with the PDK. Used to push files to the TouchPad over USB.

---

## Building and Packaging

```bash
cd clonekeen/build/webos

# Edit paths if needed
nano build-webos.sh   # set TOOLCHAIN_BIN and PDK

# Compile
bash build-webos.sh

# Assemble IPK
bash package-webos.sh
```

Output: `clonekeen/build/webos/com.cmdrkeen.game_<version>_armv7.ipk`

The version is read automatically from `webos-pkg/appinfo.json` — bump it there and the IPK filename follows.

**Incremental builds:** `build-webos.sh` skips unchanged `.c` files. Run `rm -rf fbuild/` for a clean build.

---

## Deploying to the TouchPad

Connect the TouchPad via USB with Developer Mode enabled (tap the version number in *Device Info* five times).

```bash
# Push IPK to device storage
novacom put file:///media/internal/com.cmdrkeen.game_1.2.0_armv7.ipk \
    < com.cmdrkeen.game_1.2.0_armv7.ipk

# Install
novacom run file:///usr/bin/ipkg install \
    /media/internal/com.cmdrkeen.game_1.2.0_armv7.ipk
```

The app installs to `/media/cryptofs/apps/usr/palm/applications/com.cmdrkeen.game/`.

### Reading the log

```bash
novacom run file:///bin/cat \
    /media/cryptofs/apps/usr/palm/applications/com.cmdrkeen.game/ck.log
```

You can also launch directly from a novacom shell for live console output:

```bash
novacom open tty://
cd /media/cryptofs/apps/usr/palm/applications/com.cmdrkeen.game
./clonekeen
```

---

## Commercial Episode Data

The bundled IPK contains only the episode 1 shareware files. To play episodes 2 and 3 you need the original game data files from a legitimate copy of Commander Keen: Invasion of the Vorticons.

### Required files

Place these files in `/media/internal/keen/` on the TouchPad:

| File | Episode |
|---|---|
| `EGAHEAD.CK2`, `EGALATCH.CK2`, `EGASPRIT.CK2` | Episode 2 graphics |
| `EGAHEAD.CK3`, `EGALATCH.CK3`, `EGASPRIT.CK3` | Episode 3 graphics |
| `LEVEL01.CK2` – `LEVEL16.CK2`, `LEVEL80.CK2`, `LEVEL81.CK2`, `LEVEL90.CK2` | Episode 2 levels |
| `LEVEL01.CK3` – `LEVEL16.CK3`, `LEVEL80.CK3`, `LEVEL81.CK3`, `LEVEL90.CK3` | Episode 3 levels |
| `FINALE.CK2`, `FINALE.CK3` | Ending screens |

> **Note on sounds:** The Vorticons trilogy shared one sound engine across all three episodes. `sounds.ck2` and `sounds.ck3` stub files are bundled with CloneKeen in the app's `data/` directory. You do not need `SOUNDS.CK2` or `SOUNDS.CK3` from the original game.

### Pushing files to the device

```bash
novacom put file:///media/internal/keen/EGAHEAD.CK2 < EGAHEAD.CK2
novacom put file:///media/internal/keen/EGALATCH.CK2 < EGALATCH.CK2
# ... repeat for each file
```

### How the data directory works

At startup, episode 1 always loads from the bundled `data/` directory inside the app (the shareware files). When the player selects episode 2 or 3, `Load_Episode()` in `misc.c` checks at runtime whether `/media/internal/keen/` exists:

- **Directory exists** → episode loads from `/media/internal/keen/`
- **Directory absent** → falls back to `data/` (episode will fail gracefully if commercial files are missing there)

This means episode 1 shareware always works out of the box, and commercial content is opt-in by placing files on the user-writable `/media/internal` partition.

---

## Architecture Notes

### Rendering

CloneKeen renders into an 8-bit palettized 320×240 SDL surface (`webos_game_surface`). Each frame, `SDL_SoftStretch` scales it to the 1024×768 screen surface, then the touch control overlay is drawn on top, and `SDL_Flip()` presents it. Scale factor is exactly 3.2× (320×3.2 = 1024, 240×3.2 = 768 — no letterboxing).

### Touch Controls

SDL 1.2 on webOS delivers multi-touch as mouse button/motion events. The webOS PDK extends the standard SDL event fields so that `event.button.which` and `event.motion.which` carry the finger index (0–4). `keydrv.c` maintains a per-finger state struct (`WebOSFinger`) tracking which zone each finger is in, which directional keys it's holding, and a 150ms minimum-hold timer to prevent tap-release races.

**Control layout on the 1024×768 screen:**

```
┌────────────────────────────────────────┐
│ [MENU]                                 │
│                                        │
│                          ( JUMP )      │
│    (  d-pad  )               ( FIRE )  │
└────────────────────────────────────────┘
```

- D-pad: circle centred at (120, 655), radius 110px, deadzone 28px
- Jump: circle centred at (900, 600), radius 60px
- Fire: circle centred at (790, 680), radius 60px
- Menu/ESC: rectangle at top-left (10, 10), 100×38px

All controls are drawn as white outlines with a 2px black drop shadow so they remain visible against any game background.

### Key Files Changed from Upstream CloneKeen

| File | Change |
|---|---|
| `src/platform/webos.cpp` | Entry point; detects `/media/internal/keen/` at startup |
| `src/sdl/viddrv.c` | 1024×768 fullscreen + stretch; touch overlay rendering |
| `src/sdl/keydrv.c` | Full per-finger multi-touch d-pad input system |
| `src/globals.c` | `data_dir` runtime variable (webOS only) |
| `src/keen.h` | `DATA_DIR` macro → runtime `data_dir` pointer on webOS |
| `src/misc.c` | `Load_Episode()` switches `data_dir` per episode on webOS |
| `src/sanity.c` | Skips ep2/ep3 file-presence check on webOS |
| `src/latch.c` | `sprintf` paths updated for runtime `DATA_DIR` |
| `src/maploader.c` | `sprintf` paths updated for runtime `DATA_DIR` |
| `src/FinaleScreenLoader.c` | `sprintf` paths updated for runtime `DATA_DIR` |
| `src/sdl/snddrv.c` | `sprintf` paths updated for runtime `DATA_DIR` |
| `src/menumanager.c` | UI strings updated for webOS file paths |
| `build/webos/` | Build and packaging scripts (new) |

---

## Credits

- **CloneKeen** by Caitlin Shaw (2003–2010) — GPL v3
- **Commander Keen: Invasion of the Vorticons** — id Software / Apogee Software (1990)
- **webOS TouchPad port** by Jonathan Wise (2026)
