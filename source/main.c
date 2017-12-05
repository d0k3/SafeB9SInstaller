#include "installer.h"
#include "ui.h"
#include "i2c.h"
#include "qff.h"


void Reboot()
{
    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 2);
    while(true);
}


u8 *top_screen, *bottom_screen;

void main(int argc, char** argv)
{
    // Fetch the framebuffer addresses
    if(argc >= 2) {
        // newer entrypoints
        u8 **fb = (u8 **)(void *)argv[1];
        top_screen = fb[0];
        bottom_screen = fb[2];
    } else {
        // outdated entrypoints
        top_screen = (u8*)(*(u32*)0x23FFFE00);
        bottom_screen = (u8*)(*(u32*)0x23FFFE08);
    }
    ClearScreenF(true, true, COLOR_STD_BG);
    u32 ret = SafeB9SInstaller();
    ShowInstallerStatus(); // update installer status one last time
    fs_deinit();
    if (ret) ShowPrompt(false, "SigHaxed FIRM was not installed!\nCheck lower screen for info.");
    else ShowPrompt(false, "SigHaxed FIRM install success!");
    ClearScreenF(true, true, COLOR_STD_BG);
    Reboot();
}
