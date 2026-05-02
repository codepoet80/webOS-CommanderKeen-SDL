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

---

## Lessons Learned — Porting SDL Games to webOS

Captured here for future ports. This project benefited from studying emu7800forwebos, sdl-sopwith-webos, and PCSX ReARMed for webOS.

### PDL initialization order matters

Always call PDL before SDL:
```c
PDL_Init(0);
PDL_SetTouchAggression(PDL_AGGRESSION_MORETOUCHES);  // enables multi-touch
PDL_GesturesEnable(PDL_FALSE);                        // suppress system swipe gestures
SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | ...);
```
If `PDL_GesturesEnable(PDL_FALSE)` is omitted, the system gesture layer intercepts screen-edge touches before your app sees them.

### Multi-touch via SDL 1.2

webOS extends SDL 1.2 mouse events with a non-standard `which` field carrying the finger index (0–4). Standard SDL ignores it.

```c
// In SDL_MOUSEBUTTONDOWN / SDL_MOUSEBUTTONUP:
int finger = event.button.which;   // 0–4; 0 is also single-touch
// In SDL_MOUSEMOTION:
int finger = event.motion.which;
```

Design implications:
- Track per-finger state (which zone, which keys held) in a struct array indexed by finger
- Never release a key when one finger lifts if another finger is still holding the same key — use a `webos_any_holding()` check
- A minimum-hold timer (~150ms) prevents tap-release races when the game logic and render loop are on separate timings

### 8-bit palettized surfaces and SDL_SoftStretch

If the game renders into an 8-bit surface, create a small off-screen game surface at native resolution and a separate full-screen surface, then stretch each frame:

```c
// Setup
screen = SDL_SetVideoMode(1024, 768, 8, SDL_FULLSCREEN);
game_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 8, 0,0,0,0);

// Each frame
SDL_SoftStretch(game_surface, NULL, screen, NULL);
// draw overlay on screen here
SDL_Flip(screen);
```

**Critical:** Any game code that calls `setpixel()`/`getpixel()` must target the small game surface, not the screen surface. `SDL_SoftStretch` overwrites the screen every frame, so anything written directly to `screen` between flips is erased. Point all internal render pointers at the game surface.

Both surfaces must share the same 8-bit palette — call your palette-apply function on both after any palette change.

### Touch overlay on an 8-bit surface

No alpha blending is available on 8-bit surfaces. Draw control overlays using `SDL_FillRect` only, using palette indices directly. Strategy that works well:
- Draw shapes twice: once in black at +2px offset (shadow), then in white at original position
- This gives contrast against any game background without transparency
- For text labels, use hardcoded 5×7 pixel bitmap glyphs rendered with `SDL_FillRect` at 3× scale

### Read-only app filesystem

On webOS, the app's install directory (`/media/cryptofs/apps/...`) is read-only at runtime. Any user-writable data must go to `/media/internal/` (the user storage partition, always writable).

Pattern for handling bundled vs. user-supplied data:
- Ship required data (shareware, stubs) inside the IPK
- At episode/level load time, `stat()` check a user directory; if it exists, switch the data path to it
- Keep this check per-load, not just at startup — different episodes may need different directories

### IPK packaging

An IPK is an `ar` archive with three members: `debian-binary`, `control.tar.gz`, `data.tar.gz`. The data tree must be rooted at `usr/palm/applications/<app-id>/`. No special tools needed beyond `tar` and `ar`.

Read the version from `appinfo.json` in the packaging script rather than hardcoding it:
```bash
VERSION=$(grep '"version"' appinfo.json | sed 's/.*"version": *"\([^"]*\)".*/\1/')
```

### Debugging on device

The game binary runs directly from a novacom shell — you get live stdout. The app also writes a log file to its working directory. Check it with:
```bash
novacom run file:///bin/cat /media/cryptofs/apps/usr/palm/applications/<app-id>/ck.log
```

Add `lprintf()` / `fprintf(stderr, ...)` calls liberally when diagnosing path, stat, or file-open failures — the log is the only debugger you have.
