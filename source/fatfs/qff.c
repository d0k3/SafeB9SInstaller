#include "qff.h"

#define NUM_FS 4

// don't use this area for anything else!
static FATFS* fs = (FATFS*) 0x20316000;

// currently open file systems
static FRESULT fs_mounted[NUM_FS];

FRESULT f_qread (const TCHAR* path, void* buff, FSIZE_t ofs, UINT btr, UINT* br) {
    FIL fp;
    FRESULT res;
    UINT brl;
    if (!br) br = &brl;
    
    res = f_open(&fp, path, FA_READ | FA_OPEN_EXISTING);
    if (res != FR_OK) return res;
    
    res = f_lseek(&fp, ofs);
    if (res != FR_OK) {
        f_close(&fp);
        return res;
    }
    
    res = f_read(&fp, buff, btr, br);
    f_close(&fp);
    
    return res;
}

FRESULT f_qwrite (const TCHAR* path, const void* buff, FSIZE_t ofs, UINT btw, UINT* bw) {
    FIL fp;
    FRESULT res;
    UINT bwl;
    if (!bw) bw = &bwl;
    
    res = f_open(&fp, path, FA_WRITE | FA_OPEN_ALWAYS);
    if (res != FR_OK) return res;
    
    res = f_lseek(&fp, ofs);
    if (res != FR_OK) {
        f_close(&fp);
        return res;
    }
    
    res = f_write(&fp, buff, btw, bw);
    f_close(&fp);
    
    return res;
}

FATFS* fs_getobj (const TCHAR* path) {
    UINT fsnum = (path[1] == ':') ? *path - '0' : -1;
    return ((fsnum < NUM_FS) && (fs_mounted[fsnum] == FR_OK)) ? &fs[fsnum] : (FATFS*) 0;
}

FRESULT f_getfreebyte (const TCHAR* path, QWORD* bt) {
    DWORD free_clusters;
    FATFS* fsptr;
    FRESULT res;
    
    res = f_getfree(path, &free_clusters, &fsptr);
    if (res != FR_OK) return res;
    
    FATFS* fsobj = fs_getobj(path);
    if (!fsobj) return FR_NOT_READY;
    *bt = (QWORD) free_clusters * fsobj->csize * _MAX_SS;

    return res;
}

FRESULT f_gettotalbyte (const TCHAR* path, QWORD* bt) {
    FATFS* fsobj = fs_getobj(path);
    *bt = (fsobj) ? ((QWORD) (fsobj->n_fatent - 2) * fsobj->csize * _MAX_SS) : 0;
    return *bt ? FR_OK : FR_NOT_READY;
}

FRESULT fs_init(void) {
    for (UINT i = 0; i < NUM_FS; i++) {
        TCHAR* fsname = "X:";
        *fsname = (TCHAR) ('0' + i);
        fs_mounted[i] = f_mount(fs + i, fsname, 1);
        if ((fs_mounted[i] != FR_OK) && (i == 0)) return fs_mounted[i]; // SD can't fail
    }
    return FR_OK;
}

FRESULT fs_deinit(void) {
    for (UINT i = 0; i < NUM_FS; i++) {
        TCHAR* fsname = "X:";
        *fsname = (TCHAR) ('0' + i);
        if (fs_mounted[i] != FR_OK) continue;
        f_mount(0, fsname, 1);
        fs_mounted[i] = FR_NOT_READY;
    }
    return FR_OK;
}
