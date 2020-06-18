// --------------------------------------------------------
// From https://gitlab.com/gpsd/gpsd/-/blob/master/crc24q.c
// Slightly modified for data types and includes.
// --------------------------------------------------------

/* Interface for CRC-24Q cyclic redundancy chercksum code
 *
 * This file is Copyright 2010 by the GPSD project
 * SPDX-License-Identifier: BSD-2-clause
 */
#ifndef _CRC24Q_H_
#define _CRC24Q_H_

#include <stdbool.h>

void crc24q_sign(const unsigned char *data, int len);

bool crc24q_check(const unsigned char *data, int len);

unsigned crc24q_hash(const unsigned char *data, int len);

#endif /* _CRC24Q_H_ */
// vim: set expandtab shiftwidth=4
