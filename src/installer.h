/* installer.h
 *   Copyright (C) 2021  TheLordScruffy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
    const void *data;
    size_t length;
} CINS_Content;

#define CINS_TITLEID       0x000100014F484243
#define CINS_GROUPID       0x5453
/* IOS58 */
#define CINS_SYSVERSION    0x000000010000003A
#define CINS_TITLE_VERSION 0x0000

extern int32_t CINS_iosuhaxFd;
extern int32_t CINS_fsaFd;
extern int32_t CINS_logLine;

int32_t CINS_Install(const void *ticket, uint32_t ticket_size, const void *tmd,
                     uint32_t tmd_size, CINS_Content *contents,
                     uint16_t numContents);

#ifdef __cplusplus
}
#endif
