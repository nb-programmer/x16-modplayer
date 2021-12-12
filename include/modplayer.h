#ifndef MOD_PLAYER_H
#define MOD_PLAYER_H

#include <stdint.h>
#include "modfile.h"

typedef enum {
	MODLOAD_SAMPLES_ALL
} ModSampleLoader;

/* Status of an individual sample playback channel */
//Has to be of a size such that it can fit 31 objects within 256 bytes (8 bytes is good)
typedef struct {
	uint8_t *spl_ptr;
	uint8_t spl_bank;
	uint16_t spl_position;
	uint8_t play_pitch;
	union {
		uint8_t control;
		struct {
			uint8_t play_volume : 6;
			uint8_t is_playing : 1;
		} controls;
	} playback;
} SampleState;

/* Structured format for data exchange between the player and the sound engine */
typedef struct {
	uint8_t *write_buffer;
	uint8_t status;
	uint8_t ticks_per_row;
	uint8_t current_row_tick;
	SampleState *sample_state;
} ModEngineData;

//From stdio.h. This is brought here in case you want to use a different file library than stdio
typedef struct _FILE FILE;

/* Structure to store information regarding a loaded MOD file */
typedef struct {
	//Filename pointer
	const char *fname;
	//Opened file pointer
	FILE *fd;
	//Header data of the MOD
	ModHeader hdr;
    //Number of patterns actually available in the MOD (this is calculated after the MOD header is loaded)
	unsigned char numpat;
} ModFile;
typedef ModFile *ModFilePtr;

/* MOD player data */
typedef struct {
	ModFilePtr mod;
	ModEngineData *modengine;
	//Starting bank of pattern data
	uint8_t pattern_bank;
	//Starting bank of sample data
	uint8_t sample_bank;
	void *data_buffer;
	void *output_buffer;
	SampleState sample_state[SPL_COUNT];
} ModPlayer;
typedef ModPlayer *ModPlayerPtr; 

/**
 * Read a MOD file and load its content into memory
 */
void mod_open(ModFilePtr mod, const char *fname);

/**
 * Closes the file handle used for accessing the MOD file.
 * After calling this, no read operations must be performed
 */ 
void mod_close(ModFilePtr mod);

/**
 * Initialize the MOD player by setting up interrupts, pointers and
 * calling routines for preparing the sound engine.
 * @param player The player instance to be used to initialize
 * @param mengine Pointer to a (shared) ModEngineData structure used to communicate with the sound engine
 * @param himem_addr Which address the banked RAM is situated
 * @param bank_himem_start From which bank onwards the player is allowed to use to load data blocks
*/
void modplayer_init(ModPlayerPtr player, ModEngineData *mengine, void *himem_addr, uint8_t bank_himem_start);

/**
 * Loads the .mod file data into memory by loading each individual segment of the file.
 * @param player The player instance to handle the loading. Must be initialized
 * @param mod Pointer to ModFile to use to load the data
 */
void modplayer_loadfile(ModPlayerPtr player, ModFilePtr mod);

/**
 * Make the MOD player play the given loaded file. The pattern data is loaded into memory and
 * the sequencer will load samples from this file.
 */
void modplayer_usefile(ModPlayerPtr player, ModFilePtr mod);

/**
 * Reads the next 'load_count' number of pattern data from the file into
 * the given ModPattern object / array. Make sure the file pointer is at the beginning
 * of the pattern already. Be careful not to load too much and cross the pattern data
 * as you will overlap with the sample data, and there is no going back
 * to the previous file position on the X16 (at least for now). So load at most
 * 'numpat' number of patterns
 */
void modplayer_readpatterndata(ModPlayerPtr player);

/**
 * 
 */ 
void modplayer_loadsamples(ModPlayerPtr player, ModSampleLoader load_type);

/**
 * Triggers the notes on the given division so the sound engine can
 * mix and play the correct samples
 */
void modplayer_render_division(ModPlayerPtr player, ModPatternDivision pdiv);


/* Functions to abstract interfacing with the hardware */

/* Change to given bank on a bankswitched memory region */
extern void mem_bankswitch(uint8_t bankno);

/* Returns the selected bank number */
extern uint8_t mem_bankcurr();

extern const uint16_t bank_size;

#endif