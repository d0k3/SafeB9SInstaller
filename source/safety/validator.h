#pragma once

#include "common.h"

u32 ValidateFirm(void* firm, u8* firm_sha, u32 firm_size, char* output);
u32 ValidateSector(void* sector);
u32 CheckFirmSigHax(void* firm);
