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
#include <coreinit/screen.h>
#include <coreinit/cache.h>
#include <coreinit/debug.h>
#include <coreinit/ios.h>
#include <coreinit/thread.h>
#include <coreinit/mcp.h>
#include <vpad/input.h>
#include <whb/log_cafe.h>
#include <whb/log_udp.h>
#include <whb/log.h>
#include <whb/proc.h>
#include <iosuhax.h>
#include <iosuhax_devoptab.h>
#include <malloc.h>
#include "installer.h"

/* Title data */
extern const u8 title_cetk_bin[];
extern const u32 title_cetk_bin_size;
extern const u8 title_tmd_bin[];
extern const u32 title_tmd_bin_size;
extern const u8 title_00000000_bin[];
extern const u32 title_00000000_bin_size;
extern const u8 title_00000001_bin[];
extern const u32 title_00000001_bin_size;

int fsaFd, ret;

void initIOSUHax() {
    int res = IOSUHAX_Open(NULL);
    if (res < 0) {
        OSFatal("IOSUHAX_open failed, please start this installer with\n Tiramisu/Aroma and verify you have the\n 01_sigpatches.rpx in your SD card.");
    }
}

void clearScreen() {
    OSScreenClearBufferEx(SCREEN_TV, 0x00000000);
    OSScreenClearBufferEx(SCREEN_DRC, 0x00000000);
}

void writeToScreen(int line, char* text) {
    OSScreenPutFontEx(SCREEN_TV, 0, line, text);
    OSScreenPutFontEx(SCREEN_DRC, 0, line, text);
}

void flipScreen() {
    OSScreenFlipBuffersEx(SCREEN_TV);
    OSScreenFlipBuffersEx(SCREEN_DRC);
}

void resetScreen(void* tv, void* drc, size_t tvSize, size_t drcSize) {
    memset(tv, 0, tvSize);
    memset(drc, 0, drcSize);
}

void flushScreen(void* tv, void* drc, size_t tvSize, size_t drcSize) {
    DCFlushRange(tv, tvSize);
    DCFlushRange(drc, drcSize);
}

int main() {
    WHBProcInit();
    OSScreenInit();
    size_t tvBufferSize = OSScreenGetBufferSizeEx(SCREEN_TV);
    size_t drcBufferSize = OSScreenGetBufferSizeEx(SCREEN_DRC);

    void* tvBuffer = memalign(0x100, tvBufferSize);
    void* drcBuffer = memalign(0x100, drcBufferSize);

    OSScreenSetBufferEx(SCREEN_TV, tvBuffer);
    OSScreenSetBufferEx(SCREEN_DRC, drcBuffer);

    OSScreenEnableEx(SCREEN_TV, true);
    OSScreenEnableEx(SCREEN_DRC, true);
    
    CINS_Content contents[2];
    VPADStatus status;
    VPADReadError error;
    bool installed = false;
    while(WHBProcIsRunning()) {
        VPADRead(VPAD_CHAN_0, &status, 1, &error);
        clearScreen();
        if(!installed){
            writeToScreen(1, "Compat Title Installer v1.1");
            writeToScreen(2, "COPYRIGHT (c) 2021 TheLordScruffy");
            writeToScreen(3, "Press A to install the Homebrew Channel to the Wii Menu.");
            writeToScreen(4, "Press HOME to exit.");
        }
        if((status.trigger & VPAD_BUTTON_A) && !installed) {
            initIOSUHax();
            fsaFd = IOSUHAX_FSA_Open();
            ret = mount_fs("slccmpt", fsaFd, "/dev/slccmpt01", "/vol/storage_slccmpt01");
            resetScreen(tvBuffer, drcBuffer, tvBufferSize, drcBufferSize);
            clearScreen();
            writeToScreen(1, "Installing the Homebrew Channel...");
            flushScreen(tvBuffer, drcBuffer, tvBufferSize, drcBufferSize);
            flipScreen();
            contents[0].data = (const void*) title_00000000_bin;
            contents[0].length = title_00000000_bin_size;
            contents[1].data = (const void*) title_00000001_bin;
            contents[1].length = title_00000001_bin_size;
            ret = CINS_Install((const void*) title_cetk_bin, title_cetk_bin_size,
                            (const void*) title_tmd_bin, title_tmd_bin_size,
                            contents, 2);
            installed = true;
            resetScreen(tvBuffer, drcBuffer, tvBufferSize, drcBufferSize);
            clearScreen();
        } 
        if(installed) {
            writeToScreen(1, "The Homebrew Channel installed correctly.");
            writeToScreen(2, "Press HOME to exit.");
        }
        flipScreen();
    }
    if(drcBuffer) free(drcBuffer);
    if(tvBuffer) free(tvBuffer);
    
    unmount_fs("slccmpt");
    OSScreenShutdown();
    WHBProcShutdown();
}