#include <stdio.h>
#include <string.h>
#include "ff_stuff.h"
#include "ff_ubx.h"
// gcc -o make_fake_ubxesfstatus -I../ff -I../ubloxcfg make_fake_ubxesfstatus.c  ../ff/ff_ubx.c  ../ff/ff_debug.c  ../ubloxcfg/*.c

#define NUM_SEN_TYPE 64 // 2**6
int main(int argc, char **argv)
{
    FILE *f = fopen("fake_ubx_esf_status.ubx", "w");
    UBX_ESF_STATUS_V2_GROUP0_t status;
    memset(&status, 0, sizeof(status));
    status.version = UBX_ESF_STATUS_V2_VERSION;
    status.numSens = NUM_SEN_TYPE;

    uint8_t msg[UBX_FRAME_SIZE + sizeof(UBX_ESF_STATUS_V2_GROUP0_t) + (NUM_SEN_TYPE * sizeof(UBX_ESF_STATUS_V2_GROUP1_t))];

    int offs = 0;
    memcpy(&msg[offs], &status, sizeof(status));
    offs += sizeof(status);
    for (int sensType = 0; sensType < NUM_SEN_TYPE; sensType++, offs += (int)sizeof(UBX_ESF_STATUS_V2_GROUP1_t))
    {
        UBX_ESF_STATUS_V2_GROUP1_t sens;
        sens.sensStatus1 = sensType;
        sens.sensStatus2 = 0;
        memcpy(&msg[offs], &sens, sizeof(sens));
    }

    const int size = ubxMakeMessage(UBX_ESF_CLSID, UBX_ESF_STATUS_MSGID, msg, offs, msg);
    fwrite(msg, size, 1, f);
    fclose(f);
    return 0;
}