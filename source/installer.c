#include "installer.h"
#include "validator.h"
#include "safewrite.h"
#include "nand.h"
#include "ui.h"
#include "qff.h"

#define COLOR_STATUS(s) ((s == STATUS_GREEN) ? COLOR_BRIGHTGREEN : (s == STATUS_YELLOW) ? COLOR_BRIGHTYELLOW : (s == STATUS_RED) ? COLOR_RED : COLOR_DARKGREY)

#define MIN_SD_FREE (16 * 1024 * 1024) // 16MB
#define FIRM_NAND_OFFSET    0x0B130000
#define FIRM_NAND_SIZE      0x800000

#define NAME_SIGHAXFIRM     (INPUT_PATH "/sighaxfirm.bin")
#define NAME_SIGHAXFIRMSHA  (INPUT_PATH "/sighaxfirm.bin.sha")
#define NAME_SECTOR0x96     (INPUT_PATH "/secret_sector.bin")
#define NAME_FIRMBACKUP     (INPUT_PATH "/firm0firm1.bak")
#define NAME_SECTORBACKUP   (INPUT_PATH "/sector0x96.bak")

#define STATUS_GREY    -1
#define STATUS_GREEN    0
#define STATUS_YELLOW   1
#define STATUS_RED      2

static int  statusA9lh         = STATUS_GREY;
static int  statusSdCard       = STATUS_GREY;
static int  statusFirm         = STATUS_GREY;
static int  statusSector       = STATUS_GREY;
static int  statusCrypto       = STATUS_GREY;
static int  statusBackup       = STATUS_GREY;
static int  statusInstall      = STATUS_GREY;
static char msgA9lh[64]        = "not started";
static char msgSdCard[64]      = "not started";
static char msgFirm[64]        = "not started";
static char msgSector[64]      = "not started";
static char msgCrypto[64]      = "not started";
static char msgBackup[64]      = "not started";
static char msgInstall[64]     = "not started";
    
u32 ShowInstallerStatus(void) {
    const u32 pos_xb = 10;
    const u32 pos_x0 = pos_xb;
    const u32 pos_x1 = pos_x0 + (16*FONT_WIDTH_EXT);
    const u32 pos_yb = 10;
    const u32 pos_y0 = pos_yb + 20;
    
    DrawStringF(BOT_SCREEN, pos_xb, pos_yb, COLOR_STD_FONT, COLOR_STD_BG, "SafeSigHaxInstaller v" VERSION);
    
    DrawStringF(BOT_SCREEN, pos_x0, pos_y0 +  0, COLOR_STD_FONT, COLOR_STD_BG, "ARM9LoaderHax :");
    DrawStringF(BOT_SCREEN, pos_x0, pos_y0 + 10, COLOR_STD_FONT, COLOR_STD_BG, "MicroSD Card  :");
    DrawStringF(BOT_SCREEN, pos_x0, pos_y0 + 20, COLOR_STD_FONT, COLOR_STD_BG, "Sighaxed FIRM :");
    DrawStringF(BOT_SCREEN, pos_x0, pos_y0 + 30, COLOR_STD_FONT, COLOR_STD_BG, "Secret Sector :");
    DrawStringF(BOT_SCREEN, pos_x0, pos_y0 + 40, COLOR_STD_FONT, COLOR_STD_BG, "Crypto Status :");
    DrawStringF(BOT_SCREEN, pos_x0, pos_y0 + 50, COLOR_STD_FONT, COLOR_STD_BG, "Backup Status :");
    DrawStringF(BOT_SCREEN, pos_x0, pos_y0 + 60, COLOR_STD_FONT, COLOR_STD_BG, "Install Status:");
    
    DrawStringF(BOT_SCREEN, pos_x1, pos_y0 +  0, COLOR_STATUS(statusA9lh)   , COLOR_STD_BG, "%-22.22s", msgA9lh   );
    DrawStringF(BOT_SCREEN, pos_x1, pos_y0 + 10, COLOR_STATUS(statusSdCard) , COLOR_STD_BG, "%-22.22s", msgSdCard );
    DrawStringF(BOT_SCREEN, pos_x1, pos_y0 + 20, COLOR_STATUS(statusFirm)   , COLOR_STD_BG, "%-22.22s", msgFirm   );
    DrawStringF(BOT_SCREEN, pos_x1, pos_y0 + 30, COLOR_STATUS(statusSector) , COLOR_STD_BG, "%-22.22s", msgSector );
    DrawStringF(BOT_SCREEN, pos_x1, pos_y0 + 40, COLOR_STATUS(statusCrypto) , COLOR_STD_BG, "%-22.22s", msgCrypto );
    DrawStringF(BOT_SCREEN, pos_x1, pos_y0 + 50, COLOR_STATUS(statusBackup) , COLOR_STD_BG, "%-22.22s", msgBackup );
    DrawStringF(BOT_SCREEN, pos_x1, pos_y0 + 60, COLOR_STATUS(statusInstall), COLOR_STD_BG, "%-22.22s", msgInstall);
    
    return 0;
}

u32 SafeSigHaxInstaller(void) {
    UINT bt;
    
    // initialization
    ShowString("Initializing, please wait...");
    
    
    // step #0 - a9lh check
    snprintf(msgA9lh, 64, CheckA9lh() ? "installed" : "not installed");
    statusA9lh = STATUS_GREEN;
    ShowInstallerStatus();
    
    // step #1 - init/check SD card
    snprintf(msgSdCard, 64, "checking...");
    statusSdCard = STATUS_YELLOW;
    ShowInstallerStatus();
    u64 sdFree = 0;
    u64 sdTotal = 0;
    if ((fs_init() != FR_OK) ||
        (f_getfreebyte("0:", &sdFree) != FR_OK) ||
        (f_gettotalbyte("0:", &sdTotal) != FR_OK)) {
        snprintf(msgSdCard, 64, "init failed");
        statusSdCard = STATUS_RED;
        return 1;
    }
    InitNandCrypto(); // for sector0x96 crypto and NAND drives
    snprintf(msgSdCard, 64, "%lluMB/%lluMB free", sdFree / (1024 * 1024), sdTotal / (1024 * 1024));
    statusSdCard = (sdFree < MIN_SD_FREE) ? STATUS_RED : STATUS_GREEN;
    ShowInstallerStatus();
    if (sdFree < MIN_SD_FREE) return 1;
    // SD card okay!
    
    
    // step #2 - check sighaxed firm
    snprintf(msgFirm, 64, "checking...");
    statusFirm = STATUS_YELLOW;
    ShowInstallerStatus();
    u8 firm_sha[0x20];
    UINT firm_size;
    if ((f_qread(NAME_SIGHAXFIRM, FIRM_BUFFER, 0, FIRM_BUFFER_SIZE, &firm_size) != FR_OK) ||
        (firm_size < 0x200)) {
        snprintf(msgFirm, 64, "file not found");
        statusFirm = STATUS_RED;
        return 1;
    }
    if ((f_qread(NAME_SIGHAXFIRMSHA, firm_sha, 0, 0x20, &bt) != FR_OK) || (bt != 0x20)) {
        snprintf(msgFirm, 64, ".sha file not found");
        statusFirm = STATUS_RED;
        return 1;
    }
    if (ValidateFirm(FIRM_BUFFER, firm_sha, firm_size, NULL) != 0) {
        snprintf(msgFirm, 64, "invalid FIRM");
        statusFirm = STATUS_RED;
        return 1;
    }
    if (CheckFirmSigHax(FIRM_BUFFER) != 0) {
        snprintf(msgFirm, 64, "not sighaxed");
        statusFirm = STATUS_RED;
        return 1;
    }
    snprintf(msgFirm, 64, "loaded & verified");
    statusFirm = STATUS_GREEN;
    ShowInstallerStatus();
    // provided FIRM is okay!
    
    
    // step #3 - check secret_sector.bin file
    u8 secret_sector[0x20];
    if (CheckA9lh()) {
        snprintf(msgSector, 64, "checking...");
        statusSector = STATUS_YELLOW;
        ShowInstallerStatus();
        if ((f_qread(NAME_SECTOR0x96, secret_sector, 0, 0x200, &bt) != FR_OK) || (bt != 0x200)) {
            snprintf(msgSector, 64, "file not found");
            statusSector = STATUS_RED;
            return 1;
        }
        if (ValidateSector(secret_sector) != 0) {
            snprintf(msgSector, 64, "invalid file");
            statusSector = STATUS_RED;
            return 1;
        }
        snprintf(msgSector, 64, "loaded & verified");
    } else snprintf(msgSector, 64, "not required");
    statusSector = STATUS_GREEN;
    ShowInstallerStatus();
    // secret_sector.bin okay or not required!
    
    
    // step #3 - check NAND crypto
    snprintf(msgCrypto, 64, "checking...");
    statusCrypto = STATUS_YELLOW;
    ShowInstallerStatus();
    if (!CheckFirmCrypto()) {
        snprintf(msgCrypto, 64, "FIRM crypto fail");
        statusCrypto = STATUS_RED;
        return 1;
    }
    if (CheckA9lh() && !CheckSector0x96Crypto()) {
        snprintf(msgCrypto, 64, "OTP crypto fail");
        statusCrypto = STATUS_RED;
        return 1;
    }
    snprintf(msgCrypto, 64, "all checks passed");
    statusCrypto = STATUS_GREEN;
    ShowInstallerStatus();
    
    
    // step #X - point of no return
    if (!ShowUnlockSequence(1, "All input files verified.\nTo install FIRM, enter the sequence\nbelow or press B to cancel.")) {
        snprintf(msgBackup, 64, "cancelled by user");
        snprintf(msgInstall, 64, "cancelled by user");
        statusBackup = STATUS_YELLOW;
        statusInstall = STATUS_YELLOW;
        return 1;
    }
    
    
    // step #5 - backup of current FIRMs and sector 0x96
    snprintf(msgBackup, 64, "FIRM backup...");
    statusBackup = STATUS_YELLOW;
    ShowInstallerStatus();
    FIL fp;
    u32 ret = 0;
    if (f_open(&fp, NAME_FIRMBACKUP, FA_READ|FA_WRITE|FA_CREATE_ALWAYS) != FR_OK) {
        snprintf(msgBackup, 64, "FIRM backup fail");
        statusBackup = STATUS_RED;
        return 1;
    }
    ShowProgress(0, 0, "FIRM backup");
    for (u32 pos = 0; (pos < FIRM_NAND_SIZE) && (ret == 0); pos += WORK_BUFFER_SIZE) {
        UINT bytes = min(WORK_BUFFER_SIZE, FIRM_NAND_SIZE - pos);
        snprintf(msgBackup, 64, "FIRM backup (%luMB/%luMB)",
            pos / (1024*1024), (u32) FIRM_NAND_SIZE / (1024 * 1024));
        ShowInstallerStatus();
        if ((ReadNandBytes(WORK_BUFFER, FIRM_NAND_OFFSET + pos, bytes, 0xFF) != 0) ||
            (SafeWriteFile(&fp, WORK_BUFFER, pos, bytes) != 0))
            ret = 1;
        ShowProgress(pos + bytes, FIRM_NAND_SIZE, "FIRM backup");
    }
    f_close(&fp);
    if (ret != 0) {
        snprintf(msgBackup, 64, "FIRM backup fail");
        statusBackup = STATUS_RED;
        return 1;
    }
    if (CheckA9lh()) {
        snprintf(msgBackup, 64, "0x96 backup...");
        ShowInstallerStatus();
        u8 sector_backup0[0x200];
        u8 sector_backup1[0x200];
        f_unlink(NAME_SECTORBACKUP);
        if ((ReadNandSectors(sector_backup0, 0x96, 1, 0xFF) != 0) ||
            (f_qwrite(NAME_SECTORBACKUP, sector_backup0, 0, 0x200, &bt) != FR_OK) || (bt != 0x200) ||
            (f_qread(NAME_SECTORBACKUP, sector_backup1, 0, 0x200, &bt) != FR_OK) || (bt != 0x200) ||
            (memcmp(sector_backup0, sector_backup1, 0x200) != 0)) {
            snprintf(msgBackup, 64, "0x96 backup fail");
            statusBackup = STATUS_RED;
            return 1;
        }
    }
    snprintf(msgBackup, 64, "backed up & verified");
    statusBackup = STATUS_GREEN;
    ShowInstallerStatus();
    // backups done
    
    // step #6 - install sighaxed FIRM
    ShowPrompt(false, "Install not finished - this is only a preview");
    
    
    return 0;
}
