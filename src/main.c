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

#include "installer.h"
#include "ios_exploit.h"
#include <coreinit/cache.h>
#include <coreinit/debug.h>
#include <coreinit/ios.h>
#include <coreinit/mcp.h>
#include <coreinit/screen.h>
#include <coreinit/thread.h>
#include <errno.h>
#include <iosuhax.h>
#include <iosuhax_devoptab.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vpad/input.h>
#include <whb/proc.h>

void WUPI_printTop();
void WUPI_putstr(const char* str);
void WUPI_resetScreen();

int32_t wupiLine;
uint8_t* screen_buffer;
uint32_t screen_size;

/* Title data */
extern const uint8_t title_cetk_bin[];
extern const uint32_t title_cetk_bin_size;
extern const uint8_t title_tmd_bin[];
extern const uint32_t title_tmd_bin_size;
extern const uint8_t title_00000000_bin[];
extern const uint32_t title_00000000_bin_size;
extern const uint8_t title_00000001_bin[];
extern const uint32_t title_00000001_bin_size;

extern const uint8_t wupserver_bin[];
extern const uint32_t wupserver_bin_size;

bool mounted = false, exploit = false;
CINS_Content contents[2];
int32_t ret, fsaFd = -1;

static void wupiPrintln(int32_t line, const char* str)
{
    /* put line twice for double buffer */
    OSScreenPutFontEx(SCREEN_TV, 0, line, str);
    OSScreenPutFontEx(SCREEN_DRC, 0, line, str);
    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);

    OSScreenPutFontEx(SCREEN_TV, 0, line, str);
    OSScreenPutFontEx(SCREEN_DRC, 0, line, str);
    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);
}

void WUPI_printTop(void)
{
    wupiPrintln(0, "Compat Title Installer v1.1");
    wupiPrintln(1, "COPYRIGHT (c) 2021 TheLordScruffy");
}

/* I don't care enough to implement a va arg function */
#define WUPI_printf(...)                                                       \
    do                                                                         \
    {                                                                          \
        char _wupi_print_str[256];                                             \
        snprintf(_wupi_print_str, 255, __VA_ARGS__);                           \
        WUPI_putstr(_wupi_print_str);                                          \
    } while (0)

void WUPI_putstr(const char* str)
{
    wupiPrintln(wupiLine++, str);
}

void WUPI_resetScreen(void)
{
    memset((void*)screen_buffer, 0, screen_size);
    wupiLine = 4;

    WUPI_printTop();
}

int32_t WUPI_pollVPAD(VPADStatus* button)
{
    int32_t status;
    while (1)
    {
        VPADRead(0, button, 1, &status);
        if (status == 0 && button->trigger)
            break;
        usleep(50000);
    }
    return status;
}

void WUPI_waitHome()
{
    WUPI_putstr("Press HOME to exit.");
    while(WHBProcIsRunning())
        continue;
    return;
}

static bool mcp = false;

int32_t WUPI_setupInstall(void)
{
    if (IOSUHAX_Open(NULL) >= 0)
        return 0;
    else if (MCPHookOpen() >= 0)
    {
        mcp = true;
        return 0;
    }

    WUPI_putstr("Doing IOS exploit...");
    *(volatile unsigned int*)0xF5E70000 = wupserver_bin_size;
    memcpy((void*)0xF5E70020, &wupserver_bin, wupserver_bin_size);
    DCStoreRange((void*)0xF5E70000, wupserver_bin_size + 0x40);
    IOSUExploit();
    WUPI_putstr("Done!");

    if (MCPHookOpen() < 0)
        return -1;

    mcp = true;
    return 0;
}

void WUPI_install() {
    /* We should only end up here if the A button was pressed. */
    WUPI_resetScreen();
    if (WUPI_setupInstall() < 0)
    {
        WUPI_putstr("Error: IOS exploit failed.");
        WUPI_waitHome();
        return;
    }
    exploit = true;

    /* Setup IOSUHAX */
    if ((fsaFd = IOSUHAX_FSA_Open()) < 0)
    {
        WUPI_putstr("Error: FSA failed to open.");
        WUPI_waitHome();
        return;
    }

    if ((ret = mount_fs("slccmpt", fsaFd, "/dev/slccmpt01",
                        "/vol/storage_slccmpt01")) < 0)
    {
        WUPI_putstr("Error: Failed to mount slccmpt:.\n");
        WUPI_waitHome();
        return;
    }
    mounted = true;

    WUPI_putstr("Installing the Homebrew Channel...\n");
    contents[0].data = (const void*)title_00000000_bin;
    contents[0].length = title_00000000_bin_size;
    contents[1].data = (const void*)title_00000001_bin;
    contents[1].length = title_00000001_bin_size;
    ret = CINS_Install((const void*)title_cetk_bin, title_cetk_bin_size,
                       (const void*)title_tmd_bin, title_tmd_bin_size, contents,
                       2);
    if (ret < 0)
        WUPI_printf("Install failed. Error Code: %06X\n", -ret);
    WUPI_waitHome();
}

int main()
{
    int32_t tv_screen_size, drc_screen_size;
    VPADStatus vpad;

    WHBProcInit();

    /* Initialize Gamepad */
    VPADInit();

    /* Initialize screen */
    OSScreenInit();
    tv_screen_size = OSScreenGetBufferSizeEx(SCREEN_TV);
    drc_screen_size = OSScreenGetBufferSizeEx(SCREEN_DRC);
    screen_size = tv_screen_size + drc_screen_size;
    screen_buffer = memalign(0x100, screen_size);
    OSScreenSetBufferEx(SCREEN_TV, screen_buffer); /* TV */
    OSScreenSetBufferEx(SCREEN_DRC, screen_buffer + tv_screen_size); /* DRC */
    OSScreenEnableEx(SCREEN_TV, 1);
    OSScreenEnableEx(SCREEN_DRC, 1);
    OSScreenClearBufferEx(SCREEN_TV, 0);
    OSScreenClearBufferEx(SCREEN_DRC, 0);

    WUPI_resetScreen();
    WUPI_putstr("Press A to install the Homebrew Channel to the Wii Menu.");
    WUPI_putstr("Press HOME to exit.");

    while (WHBProcIsRunning())
    {
        if ((ret = WUPI_pollVPAD(&vpad)) == 0) {
            if (vpad.trigger & VPAD_BUTTON_A) {
                WUPI_install();
                break;
            }
                
        }  
        else
        {
            WUPI_resetScreen();
            WUPI_printf("Error: VPAD Read failed (%d).", ret);
            break;
        }
    }

    if (mounted)
        unmount_fs("slccmpt");
    if (fsaFd >= 0)
        IOSUHAX_FSA_Close(fsaFd);

    if (exploit)
        if (mcp)
            MCPHookClose();
        else
            IOSUHAX_Close();

    if(screen_buffer) free(screen_buffer);
    OSScreenShutdown();
    WHBProcShutdown();
    return 0;
}
