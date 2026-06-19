# Super Star Trek

A modernized, strictly C17-compliant port of the classic 1978 game **Super Star Trek**

(originally by Mike Mayfield, Dave Ahl, and Bob Leedom).

This port preserves the exact mathematical algorithms, and pacing of the original 1978 game, while drastically expanding the engine to support dynamic universe scaling, ANSI color rendering, an intelligent 8x8 viewport camera, and enhanced Klingon A.I.

## Features
* **Strict C17 Compliance:** Compiles flawlessly with zero warnings under `gcc -Wall -O3` and MSVC.
* **ANSI Color Output:** Color-coded grid rendering (Klingons are Red, Starbases are Blue, etc.) natively supported on Linux (ncurses/ANSI) and Windows 11.
* **Dynamic Grid Scaling:** Scale the universe from a tiny 4x4 skirmish up to a massive 64x64 galactic war.
* **8x8 Viewport Camera:** No matter how large the macro-grid gets, your Short Range Sensors and Computer map will intelligently lock to an 8x8 sliding viewport centered around the *Enterprise*.
* **Upgraded Klingon A.I.:** Klingons now have randomized behaviors. 50% will guard their positions, while 50% will actively hunt the *Enterprise* across the sector grid when you engage warp drive.
* **Built-in Manual:** Type `HELP` at the command prompt to view the original 1978 formatted instruction manual.


The game engine mathematically scales mission time, Klingon populations, and Starbase generation based on the size of the grid.

    --easy : Generates a tiny 4x4 grid.

    --medium : Generates a 16x16 grid.

    --hard : Generates a 32x32 grid.

    --impossible : Generates a 64x64 grid.

    --kobayashi-maru=256x256 : Generates an absolutely colossal 256x256 grid, vastly increases Klingon damage RNG, and gives the Enterprise random starting damage and low energy.

## Compiling

**Linux (GCC / Clang):**
You can use the included Makefile:
```bash
make
