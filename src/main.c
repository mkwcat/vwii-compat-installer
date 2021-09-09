/* Compat Title Installer main source file
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

#include <gctypes.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <dynamic_libs/os_functions.h>
#include <dynamic_libs/sys_functions.h>
#include <dynamic_libs/vpad_functions.h>
#include <iosuhax.h>
#include <iosuhax_devoptab.h>
#include <system/memory.h>
#include "ios_exploit.h"
#include "installer.h"
#include "../wupserver/wupserver_bin.h"
#include <errno.h>

void WUPI_printTop();
void WUPI_putstr(const char* str);
void WUPI_resetScreen();

s32 wupiLine;
u8* screen_buffer;
u32 screen_size;

/* Title data */
extern const u8 title_cetk_bin[];
extern const u32 title_cetk_bin_size;
extern const u8 title_tmd_bin[];
extern const u32 title_tmd_bin_size;
extern const u8 title_00000000_bin[];
extern const u32 title_00000000_bin_size;
extern const u8 title_00000001_bin[];
extern const u32 title_00000001_bin_size;


static void wupiPrintln(s32 line, const char* str)
{
    /* put line twice for double buffer */
    OSScreenPutFontEx(0, 0, line, str);
    OSScreenPutFontEx(1, 0, line, str);
    OSScreenFlipBuffersEx(0);
    OSScreenFlipBuffersEx(1);

    OSScreenPutFontEx(0, 0, line, str);
    OSScreenPutFontEx(1, 0, line, str);
    OSScreenFlipBuffersEx(0);
    OSScreenFlipBuffersEx(1);
}


void WUPI_printTop (void)
{
    wupiPrintln(0, "Compat Title Installer v1.1");
    wupiPrintln(1, "COPYRIGHT (c) 2021 TheLordScruffy");
}

/* I don't care enough to implement a va arg function */
#define WUPI_printf(...) \
    do { \
        char _wupi_print_str[256]; \
        snprintf(_wupi_print_str, 255, __VA_ARGS__); \
        WUPI_putstr(_wupi_print_str); \
    } while (0)


void WUPI_putstr (const char* str)
{
    wupiPrintln(wupiLine++, str);
}

void WUPI_resetScreen (void)
{
    memset((void*) screen_buffer, 0, screen_size);
    wupiLine = 4;

    WUPI_printTop();
}

s32 WUPI_pollVPAD(VPADData* button)
{
    s32 status;
    while (1)
    {
        VPADRead(0, button, 1, &status);
        if (status == 0 && (button->btns_d | button->btns_h))
            break;
        usleep(50000);
    }
    return status;
}

void WUPI_waitHome()
{
    s32 ret;
    VPADData vpad;

    WUPI_putstr("Press HOME to exit.");
    while (1)
    {
        if (((ret = WUPI_pollVPAD(&vpad)) == 0)
            && ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_HOME))
        {
            OSScreenClearBufferEx(0, 0);
            OSScreenClearBufferEx(1, 0);
            return;
        }
    }
}

static bool mcp = false;

s32 WUPI_setupInstall(void)
{
    if (IOSUHAX_Open(NULL) >= 0)
        return 0;
    else if (MCPHookOpen() >= 0) {
        mcp = true;
        return 0;
    }
    
    WUPI_putstr("Doing IOS exploit...");
	*(vu32*) 0xF5E70000 = wupserver_bin_len;
	memcpy((void*) 0xF5E70020, &wupserver_bin, wupserver_bin_len);
	DCStoreRange((void*) 0xF5E70000, wupserver_bin_len + 0x40);
    IOSUExploit();
    WUPI_putstr("Done!");

	if(MCPHookOpen() < 0)
        return -1;

    mcp = true;
    return 0;
}

int entry(void)
{
    s32 ret, fsaFd = -1;
    s32 tv_screen_size, drc_screen_size;
    CINS_Content contents[2];
    VPADData vpad;
    bool mcp = false, mounted = false, exploit = false;

    InitOSFunctionPointers();
	InitSysFunctionPointers();
	InitVPadFunctionPointers();
    memoryInitialize();

    /* Initialize Gamepad */
	VPADInit();

    /* Initialize screen */
    OSScreenInit();
    tv_screen_size = OSScreenGetBufferSizeEx(0);
    drc_screen_size = OSScreenGetBufferSizeEx(1);
    screen_size = tv_screen_size + drc_screen_size;
    screen_buffer = MEMBucket_alloc(screen_size, 0x100);
    OSScreenSetBufferEx(0, screen_buffer); /* TV */
    OSScreenSetBufferEx(1, screen_buffer + tv_screen_size); /* DRC */
    OSScreenEnableEx(0, 1);
    OSScreenEnableEx(1, 1);
    OSScreenClearBufferEx(0, 0);
    OSScreenClearBufferEx(1, 0);

    WUPI_resetScreen();
    WUPI_putstr(
        "Press A to install the Homebrew Channel to the Wii Menu.");
    WUPI_putstr(
        "Press HOME to exit.");

    while (1)
    {
        if ((ret = WUPI_pollVPAD(&vpad)) == 0)
        {
            if ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_A)
                break;
            else if ((vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_HOME) {
                OSScreenClearBufferEx(0, 0);
                OSScreenClearBufferEx(1, 0);
                goto exit;
            }
        }
        else
        {
            WUPI_resetScreen();
            WUPI_printf("Error: VPAD Read failed (%d).", ret);
            goto exit;
        }
    }

    /* We should only end up here if the A button was pressed. */
    WUPI_resetScreen();
    if (WUPI_setupInstall() < 0) {
        WUPI_putstr("Error: IOS exploit failed.");
        WUPI_waitHome();
        goto exit;
    }
    exploit = true;

    /* Setup IOSUHAX */
    if ((fsaFd = IOSUHAX_FSA_Open()) < 0) {
        WUPI_putstr("Error: FSA failed to open.");
        WUPI_waitHome();
        goto exit;
    }

    if ((ret = mount_fs("fs", fsaFd, "/dev/slccmpt01",
                                     "/vol/storage_slccmpt01")) < 0) {
        WUPI_putstr("Error: Failed to mount FS.\n");
        WUPI_waitHome();
        goto exit;
    }
    mounted = true;

    WUPI_putstr("Installing the Homebrew Channel...\n");
    contents[0].data = (const void*) title_00000000_bin;
    contents[0].length = title_00000000_bin_size;
    contents[1].data = (const void*) title_00000001_bin;
    contents[1].length = title_00000001_bin_size;
    ret = CINS_Install((const void*) title_cetk_bin, title_cetk_bin_size,
                       (const void*) title_tmd_bin, title_tmd_bin_size,
                       contents, 2);
    if (ret < 0)
        WUPI_printf("Install failed. Error Code: %06X\n", -ret);
    WUPI_waitHome();

exit:
    if (mounted)
        unmount_fs("fs");
    if (fsaFd >= 0)
        IOSUHAX_FSA_Close(fsaFd);
    ret = 0;

    if (exploit) {
        if (mcp)
            MCPHookClose();
        else
            IOSUHAX_Close();
	    /* Reload IOS on exit */
        OSForceFullRelaunch();
        SYSLaunchMenu();
        ret = -3;
    }

    OSScreenEnableEx(0, 0);
    OSScreenEnableEx(1, 0);
    MEMBucket_free(screen_buffer);
    memoryRelease();
    return ret;
}
