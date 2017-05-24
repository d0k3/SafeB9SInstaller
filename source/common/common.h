#pragma once

#include <inttypes.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define vu8 volatile u8
#define vu16 volatile u16
#define vu32 volatile u32
#define vu64 volatile u64

#define max(a,b) \
    (((a) > (b)) ? (a) : (b))
#define min(a,b) \
    (((a) < (b)) ? (a) : (b))
#define getbe16(d) \
    (((d)[0]<<8) | (d)[1])
#define getbe32(d) \
    ((((u32) getbe16(d))<<16) | ((u32) getbe16(d+2)))
#define getbe64(d) \
    ((((u64) getbe32(d))<<32) | ((u64) getbe32(d+4)))
#define getle16(d) \
    (((d)[1]<<8) | (d)[0])
#define getle32(d) \
    ((((u32) getle16(d+2))<<16) | ((u32) getle16(d)))
#define getle64(d) \
    ((((u64) getle32(d+4))<<32) | ((u64) getle32(d)))
#define align(v,a) \
    (((v) % (a)) ? ((v) + (a) - ((v) % (a))) : (v))

#define ENTRY_BRAHMA        (1)
#define ENTRY_GATEWAY       (2)

// SafeB9SInstaller version
#define VERSION     "0.0.6"

// testfing flags, only useful to devs
// #define NO_WRITE     // disables all NAND writes, just for testing
// #define FAIL_TEST    // to test the emergency screen, only works with NO_TRANSFER defined

// name of the FIRM to install (also name of the input path)
#define NAME_FIRM   "boot9strap"

// input / output paths
#define INPUT_PATH  "0:/" NAME_FIRM

// buffer area defines (big buffer for firm)
#define WORK_BUFFER         ((u8*) 0x21000000)
#define WORK_BUFFER_SIZE    (0x100000)
#define NAND_BUFFER         ((u8*) 0x22000000)
#define NAND_BUFFER_SIZE    (0x100000)
#define FIRM_BUFFER         ((u8*) 0x23000000)
#define FIRM_BUFFER_SIZE    (0x400000)

inline u32 strchrcount(const char* str, char symbol) {
    u32 count = 0;
    for (u32 i = 0; str[i] != '\0'; i++) {
        if (str[i] == symbol)
            count++;
    }
    return count;
}
