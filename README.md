# Commander X16 Amiga MOD player

Proof of Concept Amiga Mod player for the Commander X16 (not working yet).

If [Jon Burton](https://www.youtube.com/watch?v=x3m3JrVImmU) can fit a MOD player in the Genesis, so can the X16!

## Setup

1. After downloading the repo, extract the `modfiles.img.zip` file from `sdcard` folder. It should extract a single `modfiles.img` file that has to be attached to the x16 emulator.

2. Currently, there is no directory menu in the program, so in `main.c`, change the `mod_filename` variable to any of the available MOD files from the below list.

## Building

Open a Command Prompt window to this folder, and type `make`. It should compile the project and output `modplayer.prg` inside the `build` directory. This is the binary file that you will run.

## Running

Execute `make run` to launch the emulator, attach the SD card image, load and launch the ROM on startup.

If you want to run it manually, launch the emulator as so:

```bash
x16emu -run -prg -sdcard sdcard/modfiles.img build/modplayer.prg
```

## Song list

These are the tracks currently present in the SD card image for testing.
Note that the names in the SD card are UPPERCASE, but in the program they must be loaded with a lowercase name.

---
| File name | Source |
|---|---|
| actimpls.mod | [Act of Impulse](https://modarchive.org/index.php?request=view_by_moduleid&query=32865) |
| addiction.mod | [addiction](https://modarchive.org/index.php?request=view_by_moduleid&query=167100) |
| boneless.mod | [boneless](https://modarchive.org/index.php?request=view_by_moduleid&query=38114) |
| ghz.mod | [Green Hill Zone](https://modarchive.org/index.php?request=view_by_moduleid&query=180827) (Recreation of GHZ, 8 Channels!) |
| notetest.mod | (Myself, made to test simple melody) |
| resorips.mod | [resonance rips](https://modarchive.org/index.php?request=view_by_moduleid&query=158674) |
| toystory.mod | [toy story](https://modarchive.org/index.php?request=view_by_moduleid&query=63939) (from the Sega Genesis game) |
| xmastune.mod | [christmasong](https://modarchive.org/index.php?request=view_by_moduleid&query=76567) |
