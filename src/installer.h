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

#ifndef _CINS_INSTALLER_H
#define _CINS_INSTALLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>
#include <stdbool.h>
#include <string.h>

typedef struct
{
    const void* data;
    size_t length;
} CINS_Content;

#define CINS_TITLEID 0x000100014F484243
#define CINS_GROUPID 0x5453
/* IOS58 */
#define CINS_SYSVERSION 0x000000010000003A
#define CINS_TITLE_VERSION 0x0000

extern s32 CINS_iosuhaxFd;
extern s32 CINS_fsaFd;
extern s32 CINS_logLine;

s32 CINS_Install(
    const void* ticket, u32 ticket_size,
    const void* tmd, u32 tmd_size,
    CINS_Content* contents, u16 numContents
);
s32 CINS_Uninstall();

#ifdef __cplusplus
}
#endif

#endif // _CINS_INSTALLER_H
