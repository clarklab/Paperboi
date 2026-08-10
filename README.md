# PAPERBOI

A faithful *Paperboy* homage — isometric pixel suburbs, mailbox throws, dogs,
grates, oncoming traffic, a Monday-to-Sunday route — built the novel way:

**One freestanding C file. No engine, no libc, no libraries, no assets.**
Every pixel on screen — the diagonally scrolling isometric world, the houses,
the sprites, even the font — is computed at runtime inside an 11 KB
WebAssembly module compiled straight from `paperboi.c` with stock clang
(`--target=wasm32 -nostdlib`). No emscripten, no toolchain beyond
`clang` + `wasm-ld` + `python3`.

The build deflates the wasm to 5 KB and embeds it in a tiny HTML shell
(the browser inflates it natively via `DecompressionStream`), so **the
entire game is a single 15 KB `index.html`** that runs offline, from
`file://`, on any phone or desktop browser. Open it and ride.

## Play

- **Hold and drag** anywhere — your first finger is an analog stick:
  horizontal steers, vertical pedals/brakes
- **Tap** — throw a paper (left, toward the houses); while steering,
  **any second finger** throws too
- Desktop: arrows/WASD + space
- The **?** button in-game explains why the whole thing is 15 KB

Hit **mailboxes** (250) and **porches/doorways** (100) of subscribers
(the pastel houses). Smash windows of non-subscribers (grey houses) for 50 —
but smash a subscriber's window and you lose them. Miss a subscriber and
they cancel. Deliver to everyone for a perfect-day bonus and a new
subscriber. Dodge hydrants, trash cans, grates, dogs, and cars.
Survive Monday through Sunday.

## How it works

- `paperboi.c` — the whole game (~330 lines). The world is a pure function:
  the ground pass inverts the 2:1 isometric transform per pixel, so lawns,
  driveways, doormats, flowers, sidewalk joints and lane dashes are just
  arithmetic on world coordinates. Houses, cars and sprites are procedural
  iso boxes depth-sorted by screen Y. Fixed-point everything; the only
  exports are `boot`, `frame`, and the framebuffer address.
- `shell.html` — canvas blit + pointer/keyboard input + a square-wave chip
  synth keyed off event bits returned by `frame()`. No game logic in JS.
- `build.sh` — compiles and inlines the wasm into `index.html`.

```sh
./build.sh   # needs clang with wasm32 target, wasm-ld, python3
```
