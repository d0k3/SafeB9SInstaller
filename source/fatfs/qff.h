#pragma once

#include "ff.h"

// additional quick read / write functions
FRESULT f_qread (const TCHAR* path, void* buff, FSIZE_t ofs, UINT btr, UINT* br);
FRESULT f_qwrite (const TCHAR* path, const void* buff, FSIZE_t ofs, UINT btw, UINT* bw);

// find out drive free / total space
FRESULT f_getfreebyte (const TCHAR* path, QWORD* bt);
FRESULT f_gettotalbyte (const TCHAR* path, QWORD* bt);

// stuff for initializing / deinitializing drives
FRESULT fs_init(void);
FRESULT fs_deinit(void);


