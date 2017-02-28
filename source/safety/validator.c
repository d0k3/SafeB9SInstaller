#include "validator.h"
#include "sha.h"

#define FIRM_MAGIC  'F', 'I', 'R', 'M'
#define FIRM_MAX_SIZE  0x400000 // 4MB, due to FIRM partition size

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

// standard sighax signature hash - still unknown (= missing puzzle piece)
const u8 sighaxHash[0x20] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
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
    return (sha_cmp(sectorHash, sector, 0x200, SHA256_MODE) == 0) ? 0 : 1;
}

u32 CheckFirmSigHax(void* firm) {
    FirmHeader* header = (FirmHeader*) firm;
    return (sha_cmp(sighaxHash, header->signature, 0x100, SHA256_MODE) == 0) ? 0 : 1;
}
