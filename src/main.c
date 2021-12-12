/**
 * Amiga ProTracker MOD player / synth
 * Mixes the 4 channels into one in software and plays it using the VERA
 * PCM output channel.
 * Inspired by GameHut (https://www.youtube.com/watch?v=x3m3JrVImmU).
 * You *must* have an SD card (image) attached in order for the files to load
 * as the emulator won't use the Host Filesystem Interface for non-prg files.
 * 
 * The objective of this player is to be as _MODular_ as possible, having easily
 * replacable components for every stage of the playback process, so that one can
 * reuse this player in other projects with relative ease.
 */

//The MOD file player
#include "modplayer.h"

#include <stdint.h>
#include <cx16.h>
#include <stdlib.h>
#include <stdio.h>

//The module player engine
extern ModEngineData modengine;
#pragma zpsym("modengine")

void __fastcall__ modengine_init();
void __fastcall__ modengine_free();
void __fastcall__ modengine_loop();
void __fastcall__ modengine_enable_audio();
void __fastcall__ modengine_disable_audio();

/**
 * Pattern and sample data will follow contiguous banks. If the data overflows the system's
 * memory, we will need to overwrite previous banks and reload when we need them.
 * Sample data immediately follows the last bank used by the pattern data.
 * The MOD header is loaded into the 'Low memory', or the heap (in the ModFile object created below)
 * 
 * There can be max 64 patterns, each pattern taking 1kbytes, it can be max
 * 8 banks of 8kb each max. That leaves at least (512k - 64k - 8k) = 440kbytes
 * for samples (512k is base model RAM, 64k if module uses all 64 patterns, 
 * 8k reserved for Kernal).
 * Each sample can take up a maximum of 64kbytes, and there can be a total of 31 samples
 * maximum loaded. Hence, a total of 1984kbytes can be taken up by samples.
 */

//Where we want the MOD player to load data to
const uint8_t mod_load_start_bank = 0x01;

const char mod_filename[] = "notetest.mod";

//This will communicate with the sound engine
ModPlayer modplayer;

//This will store details of the MOD
ModFile modfile;

void main() {
	//Pointers to the player and MOD file
	ModEngineData *modeg;
	ModPlayerPtr modpl;
	ModFilePtr mod;

	unsigned int i;

	modpl = &modplayer;
	mod = &modfile;
	modeg = &modengine;

	modengine_init();

	modplayer_init(modpl, modeg, BANK_RAM, mod_load_start_bank);

	printf("Sample count: %d\nSample size: %d\n", sizeof(modpl->sample_state), sizeof(SampleState));

	mod_open(mod, mod_filename);
	printf("Name: %s\n", mod->hdr.title);
	printf("Song length: %d patterns\n", mod->hdr.song_len);
	printf("Patterns: %d\n", mod->numpat);

	modplayer_loadfile(modpl, mod);

	mod_close(mod);

	printf("Starting playback\n");

	modengine_enable_audio();

	printf("Pattern Bank=%d, Sample Bank=%d\n", modpl->pattern_bank, modpl->sample_bank);

	//TODO: Here will be the playback functions. Placeholders kept for now

	mem_bankswitch(modpl->pattern_bank);

	for (i=0;i<8;i++) {
		modplayer_render_division(modpl, ((ModPattern*)modpl->data_buffer)[0][i]);
	}

	modengine_loop();

	modengine_free();
}

/* Platform-specific implementations */

const uint16_t bank_size = 0x2000;

void mem_bankswitch(uint8_t bankno) {
	RAM_BANK = bankno;
#ifdef _DEBUG
	printf("Switched bank to %d\n", RAM_BANK);
#endif
}

uint8_t mem_bankcurr() {
	return RAM_BANK;
}