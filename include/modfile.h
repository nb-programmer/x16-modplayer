#ifndef MOD_H
#define MOD_H

#include <stdint.h>

/* Amiga MOD file format */
//Referred from: https://www.aes.id.au/modformat.html

//Keep all magic numbers here :)
#define MODNAME_LEN     20
#define SPLNAME_LEN     22
#define SPL_COUNT       31
#define PATTBL_LEN      128
#define MAGICBYTES_LEN  4
#define PATTERN_DIVS    64
#define CHANNEL_COUNT   4

typedef char ModMagic[MAGICBYTES_LEN];
typedef uint8_t ModPatternTable[PATTBL_LEN];

/* Sound sample information */
typedef struct {
    char name[SPLNAME_LEN]; //Sample name, null-terminated
    uint16_t len;           //Sample length (of 2-byte chunks, multiply by 2 to get in bytes)
    uint8_t finetune;       //Only lower 4 bits are used
    uint8_t volume;         //Playback volume, between 0 and 64 (inclusive)
    uint16_t loop_point;    //Sample offset for loop start (same chunk restriction as above)
    uint16_t loop_len;      //Sample length of looping segment (same chunk restriction as above)
} ModSampleInfo;

/* MOD file header, containing song information, sample information, pattern table, etc. */
typedef struct {
    char title[MODNAME_LEN];
    ModSampleInfo sample[SPL_COUNT];
    uint8_t song_len;
    int8_t unknown;                     //Usually 127
    ModPatternTable pat_table;
    ModMagic magic;                     //Usually "M.K."
} ModHeader;

/**
 * Note data for a single channel. It is 32-bits long,
 * with varying bit lengths for each field.
 * We can't use bit-fields directly given the way the Period and
 * Effect fields are stored in the file
 */
typedef union {
    //Data arrangement straight from the file
    struct {
        uint8_t sid_high_period_high;
        uint8_t period_low;
        uint8_t sid_low_effect_high;
        uint8_t effect_low;
    } file;

    //Use after moving the data around to a proper format.
    //I have no clue how this packs into 4-bytes with this order
    struct {
        uint16_t note_period : 12;
        uint16_t note_effect : 12;
        uint8_t note_sample;
    } packed;
} ModPatternChannel;

/* Each division has data for 4 channels */
typedef ModPatternChannel ModPatternDivision[CHANNEL_COUNT];

/* Each pattern contains 64 rows of note data (64 divisions) */
typedef ModPatternDivision ModPattern[PATTERN_DIVS];

/* Variable sized sample array (8-bit raw sample for each sample point) */
typedef uint8_t *ModSample;

#endif