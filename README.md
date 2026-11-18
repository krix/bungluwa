# Bungluwa

Bungluwa is a 2D pixel-art platform game, rebuilt in C from the original 2010
Flash game. It runs natively on Windows, Linux and macOS, and can be compiled
to WebAssembly for the browser.

The game is rendered at 320×240 and scaled up with nearest-neighbour filtering.
It uses a vendored copy of the [high_impact](https://github.com/phoboslab/high_impact)
engine and the Sokol platform backend.

## Gameplay

Climb (or descend) through **24 levels** — 18 main stages and 6 bonus stages —
before the timer runs out. Collect bulbs to recharge your energy, chain combos,
reach the exit and beat your best time and score. Progress, achievements,
statistics and ghost replays of your runs are saved locally.

Hazards include crumbling crates, spikes, spikeballs, patrolling autobots,
laser barriers and alarm-triggered robots. You can jump again while in mid-air,
which is essential for the later levels.

## Controls

| Action | Keys |
| --- | --- |
| Move / look | Arrow keys |
| Jump | <kbd>Space</kbd>, <kbd>X</kbd> or <kbd>C</kbd> |
| Pause | <kbd>P</kbd> |
| Confirm / menu select | <kbd>Enter</kbd> |
| Back / cancel | <kbd>Esc</kbd> |
| Show hitboxes (debug) | <kbd>H</kbd> |
| Kill player (debug) | <kbd>Backspace</kbd> |
| Reset save data | Choose the reset option in the main menu, then hold <kbd>O</kbd> + <kbd>K</kbd> |

## Building

The repository root is the working directory: the game loads its assets from
`assets/` using relative paths, so always launch the executable from there.

### Windows (native, MinGW GCC)

Run `build.bat`. It compiles every `.c` file under `src/` and copies `assets/`
to `build/`. The script defaults to `C:\mingw64\bin\gcc.exe`; adjust the `GCC`
variable if your compiler is installed elsewhere.

```
build.bat
build\bungluwa.exe
```

### Makefile (Windows / Linux / macOS)

```
make                     # Sokol platform, OpenGL renderer
make DEBUG=true          # unoptimized build with debug symbols
make RENDER=SOFTWARE     # software renderer
make RENDER=METAL        # macOS Metal renderer
make clean
```

Linux needs OpenGL, X11, Xcursor, XInput, ALSA and pthread development
libraries. macOS uses the Foundation, Cocoa, QuartzCore, AudioToolbox and
OpenGL/Metal frameworks. The Makefile builds the executable only; assets are
read from `assets/` in the repository root.

`make qoiconv` and `make qoaconv` build the image/audio conversion tools into
`high_impact/tools/`.

### Browser (WebAssembly)

Activate the Emscripten SDK, then run `build_wasm.bat` on Windows:

```
build_wasm.bat
cd build\wasm
python -m http.server 8080
```

Open <http://localhost:8080/game.html>. Because the assets are preloaded into
the WASM build, the page must be served over HTTP rather than opened directly
from disk.

## Project layout

| Path | Contents |
| --- | --- |
| `src/` | Game source: main loop, scenes, entities, save system, ghost replays and the engine unity build (`high_impact.c`) |
| `assets/levels/` | 24 JSON tilemaps, editable with `high_impact/weltmeister.html` |
| `assets/sprites/`, `assets/tiles/` | QOI images; `font_tool.html` generates bitmap fonts |
| `assets/music/`, `assets/sounds/` | QOA audio |
| `high_impact/` | Vendored, locally adapted engine, bundled libraries, editor tools and its MIT license |
| `Makefile`, `build.bat`, `build_wasm.bat` | Native and WASM build entry points |
| `build/` | Generated binaries and copied assets (not tracked by git) |

The checkout in `high_impact/` contains changes required by this game. Please
avoid editing the engine unless necessary; game-specific code belongs in `src/`.

## Credits

- Original game design and code: Dirk "krix" Bunk
- Music, sound effects and additional support: Andreas "fate" Loesch
- Beta testers and special thanks: see `src/scenes/scene_credits.c`
- Engine: [high_impact](https://github.com/phoboslab/high_impact) by Dominic Szablewski — MIT license, see `high_impact/LICENSE`
