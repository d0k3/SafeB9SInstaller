#pragma once

#include "common.h"
#include "unittype.h"

#define NAND_MIN_SECTORS ((!IS_O3DS) ? NAND_MIN_SECTORS_N3DS : NAND_MIN_SECTORS_O3DS)

#define NAND_SYSNAND    (1<<0)
#define NAND_ZERONAND   (1<<3)
#define NAND_TYPE_O3DS  (1<<4)
#define NAND_TYPE_N3DS  (1<<5)
#define NAND_TYPE_NO3DS (1<<6)

// minimum size of NAND memory
#define NAND_MIN_SECTORS_O3DS 0x1D7800
#define NAND_MIN_SECTORS_N3DS 0x26C000

// start sectors of partitions
#define SECTOR_TWL      0x000000
#define SECTOR_SECRET   0x000096
#define SECTOR_TWLN     0x000097
#define SECTOR_TWLP     0x04808D
#define SECTOR_AGBSAVE  0x058800
#define SECTOR_FIRM0    0x058980
#define SECTOR_FIRM1    0x05A980
#define SECTOR_CTR      0x05C980

// sizes of partitions (in sectors)
#define SIZE_TWL        0x058800
#define SIZE_TWLN       0x047DA9
#define SIZE_TWLP       0x0105B3
#define SIZE_AGBSAVE    0x000180
#define SIZE_FIRM0      0x002000
#define SIZE_FIRM1      0x002000
#define SIZE_CTR_O3DS   0x17AE80
#define SIZE_CTR_N3DS   0x20F680

bool InitNandCrypto(void);
bool CheckSlot0x05Crypto(void);
bool CheckSector0x96Crypto(void);
bool CheckFirmCrypto(void);
bool CheckA9lh(void);

void CryptNand(void* buffer, u32 sector, u32 count, u32 keyslot);
void CryptSector0x96(void* buffer, bool encrypt);
int ReadNandBytes(void* buffer, u32 offset, u32 count, u32 keyslot);
int WriteNandBytes(const void* buffer, u32 offset, u32 count, u32 keyslot);
int ReadNandSectors(void* buffer, u32 sector, u32 count, u32 keyslot);
int WriteNandSectors(const void* buffer, u32 sector, u32 count, u32 keyslot);

u64 GetNandSizeSectors(void);
u32 CheckNandHeader(void* header);
u32 CheckNandType(void);
