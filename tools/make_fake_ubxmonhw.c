#include <stdio.h>
#include <string.h>
#include "ff_stuff.h"
#include "ff_ubx.h"
// gcc -o make_fake_ubxmonhw -I../ff -I../ubloxcfg make_fake_ubxmonhw.c  ../ff/ff_ubx.c  ../ff/ff_debug.c  ../ubloxcfg/*.c
int main(int argc, char **argv)
{
    FILE *f = fopen("fake_ubx_mon_hw.ubx", "w");
    UBX_MON_HW_V0_GROUP0_t hw;
    memset(&hw, 0, sizeof(hw));
    uint8_t vPinIx = 0;
    //hw.pinSel = 0x0001ffff;
    //hw.pinVal = 0x0001ffff;
    hw.pinBank = 0x0001ffff;
    //hw.pinDir = 0x0001ffff;
    hw.usedMask = 0x0001ffff;
    hw.reserved0 = 0x85;
    hw.reserved1[0] = 0xee;
    hw.reserved1[1] = 0x5a;
    while (vPinIx < 0xff)
    {
        for (int ix = 0; ix < NUMOF(hw.VP); ix++)
        {
            hw.VP[ix] = vPinIx;
            vPinIx++;
        }
        uint8_t msg[sizeof(hw) + UBX_FRAME_SIZE];
        const int size = ubxMakeMessage(UBX_MON_CLSID, UBX_MON_HW_MSGID, (uint8_t *)&hw, sizeof(hw), msg);
        fwrite(msg, size, 1, f);
    }
    fclose(f);
    return 0;
}