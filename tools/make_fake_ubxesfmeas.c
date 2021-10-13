#include <stdio.h>
#include <string.h>
#include "ff_stuff.h"
#include "ff_ubx.h"
// gcc -o make_fake_ubxesfmeas -I../ff -I../ubloxcfg make_fake_ubxesfmeas.c  ../ff/ff_ubx.c  ../ff/ff_debug.c  ../ubloxcfg/*.c

#define NUM_DATA_TYPE 64 // 2**6
int main(int argc, char **argv)
{
    FILE *f = fopen("fake_ubx_esf_meas.ubx", "w");

    const int numDataTypePerMsg = 32; // 2**5
    for (int dataTypeOffs = 0; dataTypeOffs < NUM_DATA_TYPE; dataTypeOffs += numDataTypePerMsg)
    {
        UBX_ESF_MEAS_V0_GROUP0_t meas;
        memset(&meas, 0, sizeof(meas));
        meas.flags = numDataTypePerMsg << 11;

        uint8_t msg[UBX_FRAME_SIZE + sizeof(UBX_ESF_MEAS_V0_GROUP0_t) + (NUM_DATA_TYPE * sizeof(UBX_ESF_MEAS_V0_GROUP1_t))];

        int offs = 0;
        memcpy(&msg[offs], &meas, sizeof(meas));
        offs += sizeof(meas);
        for (int dataType = dataTypeOffs; dataType < dataTypeOffs + numDataTypePerMsg; dataType++, offs += (int)sizeof(UBX_ESF_MEAS_V0_GROUP1_t))
        {
            UBX_ESF_MEAS_V0_GROUP1_t meas;
            meas.data = (dataType << 24) | 1234;
            memcpy(&msg[offs], &meas, sizeof(meas));
        }

        const int size = ubxMakeMessage(UBX_ESF_CLSID, UBX_ESF_MEAS_MSGID, msg, offs, msg);
        fwrite(msg, size, 1, f);

    }
    fclose(f);
    return 0;
}