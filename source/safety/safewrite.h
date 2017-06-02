#pragma once

#include "common.h"
#include "ff.h"

u32 SafeWriteFile(FIL* file, void* buff, FSIZE_t ofs, UINT btw);
u32 SafeQWriteFile(const TCHAR* path, void* buff, UINT btw);
u32 SafeWriteNand(void* buff, u32 ofs, u32 btw, u32 keyslot);
