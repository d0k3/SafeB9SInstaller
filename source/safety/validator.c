#include "validator.h"
#include "unittype.h"
#include "sha.h"

#define FIRM_MAGIC  'F', 'I', 'R', 'M'
#define FIRM_MAX_SIZE  0x400000 // 4MB, due to FIRM partition size

#define B9S_MAGIC   "B9S"
#define B9S_OFFSET  (0x40 - strnlen(B9S_MAGIC, 0x10))

#define FB3_MAGIC   "FASTBOOT 3DS   "
#define FB3_OFFSET  0x200 // this is not actually used

// see: https://www.3dbrew.org/wiki/FIRM#Firmware_Section_Headers
typedef struct {
    u32 offset;
    u32 address;
    u32 size;
    u32 type;
    u8  hash[0x20];
} __attribute__((packed)) FirmSectionHeader;

// see: https://www.3dbrew.org/wiki/FIRM#FIRM_Header
typedef struct {
    u8  magic[4];
    u8  priority[4];
    u32 entry_arm11;
    u32 entry_arm9;
    u8  reserved1[0x30];
    FirmSectionHeader sections[4];
    u8  signature[0x100];
} __attribute__((packed, aligned(16))) FirmHeader;

// from: https://github.com/AuroraWright/SafeA9LHInstaller/blob/master/source/installer.c#L9-L17
const u8 sectorHash[0x20] = {
    0x82, 0xF2, 0x73, 0x0D, 0x2C, 0x2D, 0xA3, 0xF3, 0x01, 0x65, 0xF9, 0x87, 0xFD, 0xCC, 0xAC, 0x5C,
    0xBA, 0xB2, 0x4B, 0x4E, 0x5F, 0x65, 0xC9, 0x81, 0xCD, 0x7B, 0xE6, 0xF4, 0x38, 0xE6, 0xD9, 0xD3
};

// from: https://github.com/SciresM/CTRAesEngine/tree/master/CTRAesEngine/Resources/_byte
const u8 sectorHash_dev[0x20] = {
    0xB2, 0x91, 0xD9, 0xB1, 0x33, 0x05, 0x79, 0x0D, 0x47, 0xC6, 0x06, 0x98, 0x4C, 0x67, 0xC3, 0x70,
    0x09, 0x54, 0xE3, 0x85, 0xDE, 0x47, 0x55, 0xAF, 0xC6, 0xCB, 0x1D, 0x8D, 0xC7, 0x84, 0x5A, 0x64
};

// sighax signature hash - thanks go to Myria & SciresM for bruteforcing this 
const u8 sighaxHash[0x20] = {
    0x07, 0x8C, 0xC0, 0xCF, 0xD8, 0x50, 0xA2, 0x70, 0x93, 0xDD, 0xA2, 0x63, 0x0C, 0x36, 0x03, 0xCA,
    0x0C, 0x96, 0x96, 0x9B, 0xD1, 0xF2, 0x6D, 0xA4, 0x8A, 0xC7, 0xB1, 0xBA, 0xE5, 0xDD, 0x52, 0x19
};

// sighax dev signature hash - thanks go to Myria & SciresM for bruteforcing this 
const u8 sighaxHash_dev[0x20] = {
    0xE6, 0x35, 0xC6, 0x36, 0xDC, 0x62, 0x59, 0xD6, 0x22, 0x8A, 0xF5, 0xBE, 0xD2, 0x84, 0x6E, 0x33,
    0x96, 0xD3, 0x78, 0x6E, 0xDF, 0x50, 0x3D, 0x11, 0x86, 0x84, 0x01, 0x59, 0x97, 0x50, 0x42, 0x26
};

// see: http://www.sighax.com/
const u8 sighaxHash_alt[0x20] = {
    0xAD, 0xB7, 0x3A, 0xBC, 0x35, 0x70, 0x8E, 0xF1, 0xDF, 0xE9, 0xEF, 0x9C, 0xA5, 0xFA, 0xC8, 0xBF,
    0xC2, 0xDF, 0x91, 0x6B, 0xB2, 0xE3, 0x81, 0x01, 0x85, 0x84, 0x82, 0x40, 0x9F, 0x0D, 0x45, 0x0A
};

u32 ValidateFirmHeader(FirmHeader* header, u32 data_size) {
    u8 magic[] = { FIRM_MAGIC };
    if (memcmp(header->magic, magic, sizeof(magic)) != 0)
        return 1;
    
    u32 firm_size = sizeof(FirmHeader);
    for (u32 i = 0; i < 4; i++) {
        FirmSectionHeader* section = header->sections + i;
        if (!section->size) continue;
        if (section->offset < firm_size) return 1;
        firm_size = section->offset + section->size;
    }
    
    if ((firm_size > FIRM_MAX_SIZE) || (data_size && (firm_size > data_size)))
        return 1;
    
    return 0;
}

u32 ValidateFirm(void* firm, u8* firm_sha, u32 firm_size, char* output) {
    FirmHeader* header = (FirmHeader*) firm;
    int section_arm11 = -1;
    int section_arm9 = -1;
    
    // validate firm header
    if (ValidateFirmHeader(header, firm_size) != 0)
        return 1;
    
    // hash verify all available sections
    for (u32 i = 0; i < 4; i++) {
        FirmSectionHeader* section = header->sections + i;
        if (!section->size) continue;
        if (sha_cmp(section->hash, (u8*) firm + section->offset, section->size, SHA256_MODE) != 0) {
            if (output) snprintf(output, 64, "Section %lu hash mismatch", i);
            return 1;
        }
        if ((header->entry_arm11 >= section->address) &&
            (header->entry_arm11 < section->address + section->size))
            section_arm11 = i;
        if ((header->entry_arm9 >= section->address) &&
            (header->entry_arm9 < section->address + section->size))
            section_arm9 = i;
    }
    
    // sections for arm11 / arm9 entrypoints not found?
    if ((section_arm11 < 0) || (section_arm9 < 0)) {
        if (output) snprintf(output, 64, "ARM11/ARM9 entrypoints not found");
        return 1;
    }
    
    // check provided .SHA
    if (sha_cmp(firm_sha, firm, firm_size, SHA256_MODE) != 0) {
        if (output) snprintf(output, 64, "SHA hash mismatch");
        return 1;
    }
    
    return 0;
}

u32 ValidateSector(void* sector) {
    return (sha_cmp((IS_DEVKIT) ? sectorHash_dev : sectorHash, sector, 0x200, SHA256_MODE) == 0) ? 0 : 1;
}

u32 CheckFirmSigHax(void* firm) {
    FirmHeader* header = (FirmHeader*) firm;
    return ((sha_cmp((IS_DEVKIT) ? sighaxHash_dev : sighaxHash, header->signature, 0x100, SHA256_MODE) == 0) ||
        (!IS_DEVKIT && (sha_cmp(sighaxHash_alt, header->signature, 0x100, SHA256_MODE) == 0))) ? 0 : 1;
}

u32 CheckBoot9Strap(void* firm) {
    return (memcmp(((u8*) firm) + B9S_OFFSET, B9S_MAGIC, strnlen(B9S_MAGIC, 0x10)) == 0) ? 0 : 1;
}

u32 CheckFastBoot3DS(void* firm) {
    FirmHeader* header = (FirmHeader*) firm;
    u32 offset = 0;
    for (u32 i = 0; (i < 4) && !offset; i++) { // find ARM9 section
        FirmSectionHeader* section = header->sections + i;
        if (section->size && (section->type == 0))
            offset = section->offset;
    }
    return (offset && (memcmp(((u8*) firm) + offset, FB3_MAGIC, strnlen(FB3_MAGIC, 0x10)) == 0)) ? 0 : 1;
}

u32 CheckFirmPayload(void* firm, char* result) {
    if (CheckBoot9Strap(firm) == 0) {
        if (result) snprintf(result, 32, "boot9strap firm");
        return 0;
    #ifdef OPEN_INSTALLER 
    } else if (CheckFastBoot3DS(firm) == 0) {
        if (result) snprintf(result, 32, "fastboot3ds firm");
        return 0;
    #endif
    }
    if (result) snprintf(result, 32, "unknown firm");
    return 1;
}
