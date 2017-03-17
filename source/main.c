#include "installer.h"
#include "ui.h"
#include "i2c.h"
#include "qff.h"


void Reboot()
{
    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 2);
    while(true);
}


int main()
{
    u32 ret = SafeSigHaxInstaller();
    ShowInstallerStatus(); // update installer status one last time
    if (ret) ShowPrompt(false, "SigHaxed FIRM was not installed!\nCheck lower screen for info.");
    else ShowPrompt(false, "SigHaxed FIRM install success!");
    ClearScreenF(true, true, COLOR_STD_BG);
    fs_deinit();
    Reboot();
    return 0;
}
