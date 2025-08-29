// Library test program
// Copyright (c) 2021 Charles Parent (charles.parent@orolia2s.com)

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <ubloxcfg/ubloxcfg.h>
#include <ff/ff_rx.h>
#include <ff/ff_parser.h>
#include <ff/ff_epoch.h>
#include <ff/ff_stuff.h>

int main()
{
	RX_ARGS_t args = RX_ARGS_DEFAULT();
	RX_t *rx = rxInit("/dev/ttyS2", &args);
	if ((rx == NULL || !rxOpen(rx))) {
		free(rx);
		printf("rx init failed\n");
		return -1;
	}

	uint32_t nMsgs = 0, sMsgs = 0;

	const uint64_t tOffs = TIME() - timeOfDay(); // Offset between wall clock and parser time reference

	EPOCH_t coll;
	EPOCH_t epoch;
	epochInit(&coll);

	printf("Dumping received data...\n");
	while (true)
	{
		PARSER_MSG_t *msg = rxGetNextMessage(rx);
		if (msg != NULL)
		{
			nMsgs++;
			sMsgs += msg->size;
			const uint32_t latency = (msg->ts - tOffs) % 1000; // Relative to wall clock top of second


			const char *prot = "?";
			switch (msg->type)
			{
				case PARSER_MSGTYPE_UBX:
					prot = "UBX";
					break;
				case PARSER_MSGTYPE_NMEA:
					prot = "NMEA";
					break;
				case PARSER_MSGTYPE_RTCM3:
					prot = "RTMC3";
					break;
				case PARSER_MSGTYPE_GARBAGE:
					prot = "GARBAGE";
					break;
			}

			if (msg->type == PARSER_MSGTYPE_UBX) {
				printf("message %4u, dt %4u, size %4d, %-8s %-20s %s\n",
						msg->seq, latency, msg->size, prot, msg->name, msg->info != NULL ? msg->info : "n/a");
				epochCollect(&coll, msg, &epoch);
				printf("EPOCH data is:\n");
				printf("Fix: %d, Fix Ok %s, LepSecKnow %s, Hours %d, Minutes %d Seconds %f\n",
					epoch.fix,
					epoch.fixOk ? "TRUE" : "FALSE",
					epoch.leapSecKnown ? "TRUE" : "FALSE",
					epoch.hour,
					epoch.minute,
					epoch.second
				);

				struct tm t;
				time_t t_of_day;
				t.tm_year = epoch.year - 1900;  // Year - 1900
				t.tm_mon = epoch.month - 1;           // Month, where 0 = jan
				t.tm_mday = epoch.day;          // Day of the month
				t.tm_hour = epoch.hour;
				t.tm_min = epoch.minute;
				t.tm_sec = epoch.second;
				t.tm_isdst = -1;        // Is DST on? 1 = yes, 0 = no, -1 = unknown
				t_of_day = mktime(&t);

    			printf("seconds since the Epoch: %ld\n", (long) t_of_day);

				if (epoch.haveLeapSeconds) {
					printf("leaps seconds count is %d\n", epoch.leapSeconds);
				}
				if (epoch.haveLeapSecondEvent) {
					printf("lsChange is %d, dateOfLsGpsWn is %d and dateofLsFpsDn is %d\n",
						epoch.lsChange,
						epoch.dateOfLsGpsWn,
						epoch.dateOfLsGpsDn
					);
				}
			}

		} else {
			usleep(5 * 1000);
		}
	}

	rxClose(rx);
	free(rx);
	return 0;
}
