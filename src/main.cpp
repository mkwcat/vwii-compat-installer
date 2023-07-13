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
#include <coreinit/cache.h>
#include <coreinit/debug.h>
#include <coreinit/filesystem_fsa.h>
#include <coreinit/ios.h>
#include <coreinit/mcp.h>
#include <coreinit/screen.h>
#include <coreinit/thread.h>
#include <cstring>
#include <malloc.h>
#include <mocha/mocha.h>
#include <padscore/kpad.h>
#include <sndcore2/core.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <whb/proc.h>

#include "InputUtils.h"
#include "StateUtils.h"

#define FS_ALIGN(x) ((x + 0x3F) & ~(0x3F))

void WUPI_printTop();
void WUPI_putstr(const char *str);
void WUPI_resetScreen();

int32_t wupiLine;
uint8_t *screen_buffer;
uint32_t screen_size;

FSAClientHandle fsaClient;

/* Title data */
extern const uint8_t title_cetk_bin[];
extern const uint32_t title_cetk_bin_size;
extern const uint8_t title_tmd_bin[];
extern const uint32_t title_tmd_bin_size;
extern const uint8_t title_00000000_bin[];
extern const uint32_t title_00000000_bin_size;
extern const uint8_t title_00000001_bin[];
extern const uint32_t title_00000001_bin_size;

bool mounted = false, exploit = false;
CINS_Content contents[2];
int32_t ret, fsaFd = -1;

bool initFS() {
    FSAInit();
    fsaClient = FSAAddClient(nullptr);
    bool retUnlock =
            Mocha_UnlockFSClientEx(fsaClient) == MOCHA_RESULT_SUCCESS;
    if (retUnlock) {
        FSAMount(fsaClient, "/dev/slccmpt01", "/vol/slccmpt01", FSA_MOUNT_FLAG_LOCAL_MOUNT, nullptr, 0);
        return true;
    }
    return false;
}

void deinitFS() {
    FSAUnmount(fsaClient, "/vol/slccmpt01", FSA_UNMOUNT_FLAG_NONE);
    Mocha_DeInitLibrary();
    FSADelClient(fsaClient);
    FSAShutdown();
}

static void wupiPrintln(int32_t line, const char *str) {
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

void WUPI_printTop() {
    wupiPrintln(0, "Compat Title Installer v1.5");
    wupiPrintln(1, "COPYRIGHT (c) 2021-2023 TheLordScruffy, DaThinkingChair");
}

/* I don't care enough to implement a va arg function */
#define WUPI_printf(...)                             \
    do {                                             \
        char _wupi_print_str[256];                   \
        snprintf(_wupi_print_str, 255, __VA_ARGS__); \
        WUPI_putstr(_wupi_print_str);                \
    } while (0)

void WUPI_putstr(const char *str) {
    wupiPrintln(wupiLine++, str);
}

void WUPI_resetScreen() {
    memset((void *) screen_buffer, 0, screen_size);
    wupiLine = 4;

    WUPI_printTop();
}

void WUPI_waitHome() {
    WUPI_putstr("Press HOME to exit.");
    while (State::AppRunning())
        continue;
    return;
}

int32_t WUPI_setupInstall() {
    if (Mocha_InitLibrary() == MOCHA_RESULT_SUCCESS)
        return 0;
    return -1;
}

void WUPI_install() {
    /* We should only end up here if the A button was pressed. */
    WUPI_resetScreen();
    if (WUPI_setupInstall() < 0) {
        WUPI_putstr("Error: IOS exploit failed.");
        WUPI_waitHome();
        return;
    }
    exploit = true;

    if (!(ret = initFS())) {
        WUPI_putstr("Error: Failed to mount /vol/slccmpt01.\n");
        WUPI_waitHome();
        return;
    }
    mounted = true;

    WUPI_putstr("Installing the Homebrew Channel...\n");

    void *title_cetk_bin_aligned = aligned_alloc(0x40, FS_ALIGN(title_cetk_bin_size));
    void *title_tmd_bin_aligned = aligned_alloc(0x40, FS_ALIGN(title_tmd_bin_size));
    memmove(title_cetk_bin_aligned, title_cetk_bin, title_cetk_bin_size);
    memmove(title_tmd_bin_aligned, title_tmd_bin, title_tmd_bin_size);

    void *title_00000000_bin_aligned = aligned_alloc(0x40, FS_ALIGN(title_00000000_bin_size));
    void *title_00000001_bin_aligned = aligned_alloc(0x40, FS_ALIGN(title_00000001_bin_size));
    memmove(title_00000000_bin_aligned, title_00000000_bin, title_00000000_bin_size);
    memmove(title_00000001_bin_aligned, title_00000001_bin, title_00000001_bin_size);

    contents[0].data = (const void *) title_00000000_bin_aligned;
    contents[0].length = FS_ALIGN(title_00000000_bin_size);
    contents[1].data = (const void *) title_00000001_bin_aligned;
    contents[1].length = FS_ALIGN(title_00000001_bin_size);
    ret = CINS_Install((const void *) title_cetk_bin_aligned, FS_ALIGN(title_cetk_bin_size),
                       (const void *) title_tmd_bin_aligned, FS_ALIGN(title_tmd_bin_size), contents,
                       2);
    free(title_cetk_bin_aligned);
    free(title_tmd_bin_aligned);
    free(title_00000000_bin_aligned);
    free(title_00000001_bin_aligned);
    if (ret < 0)
        WUPI_printf("Install failed. Error Code: %06X\n", -ret);
    WUPI_waitHome();
}

int main() {
    int32_t tv_screen_size, drc_screen_size;
    Input input;

    State::init();
    AXInit();
    AXQuit();

    WPADInit();
    KPADInit();
    WPADEnableURCC(1);

    /* Initialize screen */
    OSScreenInit();
    tv_screen_size = OSScreenGetBufferSizeEx(SCREEN_TV);
    drc_screen_size = OSScreenGetBufferSizeEx(SCREEN_DRC);
    screen_size = tv_screen_size + drc_screen_size;
    screen_buffer = (uint8_t *) memalign(0x100, screen_size);
    OSScreenSetBufferEx(SCREEN_TV, screen_buffer);                   /* TV */
    OSScreenSetBufferEx(SCREEN_DRC, screen_buffer + tv_screen_size); /* DRC */
    OSScreenEnableEx(SCREEN_TV, 1);
    OSScreenEnableEx(SCREEN_DRC, 1);
    OSScreenClearBufferEx(SCREEN_TV, 0);
    OSScreenClearBufferEx(SCREEN_DRC, 0);

    WUPI_resetScreen();
    WUPI_putstr("Press A to install the Homebrew Channel to the Wii Menu.");
    WUPI_putstr("Press HOME to exit.");

    while (State::AppRunning()) {
        input.read();
        if (input.get(TRIGGER, PAD_BUTTON_ANY)) {
            WUPI_resetScreen();
            WUPI_putstr("Press A to install the Homebrew Channel to the Wii Menu.");
            WUPI_putstr("Press HOME to exit.");
        }
        if (input.get(TRIGGER, PAD_BUTTON_A)) {
            WUPI_install();
            break;
        }
    }

    deinitFS();

    if (screen_buffer)
        free(screen_buffer);
    State::shutdown();
    return 0;
}
