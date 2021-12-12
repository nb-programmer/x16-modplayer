/**
 * The sequencer that plays the loaded MOD file at the correct tempo,
 * loads samples into memory and sends this data to the MOD engine.
 */

#include "modplayer.h"
#include "utils.h"
#include <stddef.h>
#include <stdio.h>

const ModMagic MOD_MAGIC_WORDS[] = { {'M', '.', 'K', '.'} };
#define MAXPAT_IN_BANK (bank_size / sizeof(ModPattern))

/* Private function prototypes for the MOD player */

unsigned char _mod_max_patidx(ModPatternTable pattern_table);
void _mod_correct_hdr_endian(ModFilePtr mod);
void _mod_swap_patdata(ModPatternChannel* channel);
void _mod_correct_pattern_endian(ModPattern *pat);

/* Module operations */

void mod_open(ModFilePtr mod, const char *fname) {
	mod->fname = fname;
	mod->fd = fopen(mod->fname, "rb");
	//Read the header
	fread((char *)&(mod->hdr), sizeof(ModHeader), 1, mod->fd);
	//Swap endian of 16-bit integers to match host
	_mod_correct_hdr_endian(mod);
	mod->numpat = _mod_max_patidx(mod->hdr.pat_table);
}

void mod_close(ModFilePtr mod) {
	if (mod->fd)
		fclose(mod->fd);
	mod->fd = NULL;
}

/* MOD player functions */

void modplayer_init(ModPlayerPtr player, ModEngineData *mengine, void *himem_addr, uint8_t bank_himem_start) {
	//Sound engine communication structure
	player->modengine = mengine;
	player->data_buffer = himem_addr;
	player->modengine->sample_state = &player->sample_state;

	//Pattern data starts on this bank. Subsequently, the sample data in the following banks
	player->pattern_bank = bank_himem_start;
}

void modplayer_loadfile(ModPlayerPtr player, ModFilePtr mod) {
#ifdef _DEBUG
	printf("Loading pattern data...\n");
#endif
	modplayer_usefile(player, mod);
#ifdef _DEBUG
	printf("Loading sample data...\n");
#endif
	modplayer_loadsamples(player, MODLOAD_SAMPLES_ALL);
#ifdef _DEBUG
	printf("Module finished loading\n");
#endif
}

void modplayer_usefile(ModPlayerPtr player, ModFilePtr mod) {
	//Check if the file is loaded
	//We can check for the "M.K." magic field (though it may not always be exactly this in all files)
#ifdef MODPLAYER_CHECK_MAGIC
	if (memcmp(MOD_MAGIC_WORDS[0], mod->hdr.magic, sizeof(ModMagic)) != 0) {
		printf("Failed to switch to the file: Magic word in file did not match (Probably a different magic word used?)\n");
		printf("Magic word found: %c%c%c%c\n", mod->hdr.magic[0], mod->hdr.magic[1], mod->hdr.magic[2], mod->hdr.magic[3]);
		return;
	}
#endif

	//TODO: Stop the sound engine before doing this

	player->mod = mod;

	//Dump pattern data into the High memory
	modplayer_readpatterndata(player);
}

void modplayer_readpatterndata(ModPlayerPtr player) {
	register uint8_t pat_toload, pat_canload;
	int read_count;

#ifdef _DEBUG
	printf("Dump pattern to address: Bank=%d, Addr: %03x\n", player->pattern_bank, (int)player->data_buffer);
#endif

	//Use the sample_bank as a temporary counter
	player->sample_bank = player->pattern_bank;
	pat_toload = player->mod->numpat;

	while (pat_toload) {
		mem_bankswitch(player->sample_bank);

		//Load upto MAXPAT_IN_BANK (8) patterns at once
		pat_canload = MIN(pat_toload, MAXPAT_IN_BANK);
		read_count = fread((char *)player->data_buffer, sizeof(ModPattern), pat_canload, player->mod->fd);
		pat_toload -= read_count;

#if 0
		if (read_count != pat_canload) {
			printf("Loaded unexpected amount of patterns\n");
		}
#endif

#ifdef _DEBUG
		printf("Loaded %d patterns (actual: %d patterns), remaining: %d patterns\n", read_count, pat_canload, pat_toload);
#endif

		//Go to the next bank. If we read less than 8, we still need to go to the next fresh bank to load samples
		player->sample_bank++;

		//Fix the arrangement of the data in each pattern
		while (read_count--)
			_mod_correct_pattern_endian(((ModPattern*)player->data_buffer) + read_count);
	}
}

void modplayer_loadsamples(ModPlayerPtr player, ModSampleLoader load_type) {
	int bank_read_cnt;
	uint8_t spl_bnk;

	spl_bnk = player->sample_bank;
	while (1) {
		mem_bankswitch(spl_bnk);
		bank_read_cnt = fread((char *)player->data_buffer, sizeof(uint8_t), bank_size, player->mod->fd);
#ifdef _DEBUG
		printf("Read sample to bank: %d\n", bank_read_cnt);
#endif
		if (!bank_read_cnt) break;
		spl_bnk++;
	}
}

void modplayer_render_division(ModPlayerPtr player, ModPatternDivision pdiv) {
	printf("Note: %03x ", pdiv[0].packed.note_period);
}


/* Utility functions */

void swap_ed16(uint16_t *in) {
	*in = (*in >> 8) | (*in << 8);
}

unsigned char _mod_max_patidx(ModPatternTable pattern_table) {
	//Inefficient way, but it is one-shot, so who cares :)
	unsigned char i, maxpat = 0;
	for (i = 0; i < PATTBL_LEN; i++)
		maxpat = MAX(pattern_table[i], maxpat);
	//Pattern index starts from 0, so we have max(pattern_table) + 1 patterns total
	return maxpat + 1;
}

void _mod_correct_hdr_endian(ModFilePtr mod) {
	uint8_t i;
	for (i = 0; i < SPL_COUNT; i++) {
		swap_ed16(&(mod->hdr.sample[i].len));
		swap_ed16(&(mod->hdr.sample[i].loop_point));
		swap_ed16(&(mod->hdr.sample[i].loop_len));
	}
}

//TODO: Optimization needed
//Shuffles memory of the channel note data to match 6502 endianness
void _mod_swap_patdata(ModPatternChannel* channel) {
	ModPatternChannel tmp;
	tmp.file.sid_high_period_high = channel->file.sid_high_period_high;
	tmp.file.period_low = channel->file.period_low;
	tmp.file.sid_low_effect_high = channel->file.sid_low_effect_high;
	tmp.file.effect_low = channel->file.effect_low;
	channel->packed.note_sample = tmp.file.sid_high_period_high & 0xF0 | tmp.file.sid_low_effect_high >> 4;
	channel->packed.note_period = tmp.file.period_low + (tmp.file.sid_high_period_high & 0xF) * 0x100;
	channel->packed.note_effect = tmp.file.effect_low + (tmp.file.sid_low_effect_high & 0xF) * 0x100;
}

void _mod_correct_pattern_endian(ModPattern *pat) {
	unsigned char x, y;
	for (x = 0; x < PATTERN_DIVS; x++)
		for (y = 0; y < CHANNEL_COUNT; y++)
			_mod_swap_patdata((ModPatternChannel*)((*pat)[x] + y));
}
