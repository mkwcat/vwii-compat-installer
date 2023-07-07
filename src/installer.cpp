/* Wii title installer for Wii U Mode
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
#include <coreinit/filesystem_fsa.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

void WUPI_putstr(const char *);

#define CINS_Log(...)                                \
    do {                                             \
        char _wupi_print_str[256];                   \
        snprintf(_wupi_print_str, 255, __VA_ARGS__); \
        WUPI_putstr(_wupi_print_str);                \
    } while (0)

#define IOS_SUCCESS             FS_ERROR_OK

#define CINS_PATH_LEN           (sizeof("/vol/slccmpt01") + 63)

#define CINS_ID_HI              ((uint32_t) (CINS_TITLEID >> 32))
#define CINS_ID_LO              ((uint32_t) (CINS_TITLEID & 0xFFFFFFFF))

#define CINS_TRY(c)                        \
    if (!(c))                              \
        do {                               \
            CINS_Log("Failed, please exit and try again\n"); \
            goto error;                    \
    } while (0)

extern FSAClientHandle fsaClient;

int32_t CINS_Install(const void *ticket, uint32_t ticket_size, const void *tmd,
                     uint32_t tmd_size, CINS_Content *contents,
                     uint16_t numContents) {
    FSError ret;
    int32_t i;
    FSAFileHandle fd;
    char path[CINS_PATH_LEN], pathd[CINS_PATH_LEN];
    char titlePath[CINS_PATH_LEN], ticketPath[CINS_PATH_LEN],
            ticketFolder[CINS_PATH_LEN];

    CINS_Log("Starting install\n");

    /* This installer originally created a temporary directory for the
     * installation, wrote everything to flash there, then renamed it all to
     * other directories. The wupserver doesn't already support renaming files,
     * and my attempt to add it failed so I gave up. */
    snprintf(titlePath, CINS_PATH_LEN, "/vol/slccmpt01/title/%08x/%08x", CINS_ID_HI,
             CINS_ID_LO);
    snprintf(path, CINS_PATH_LEN, "/vol/slccmpt01/title/%08x", CINS_ID_HI);
    snprintf(ticketPath, CINS_PATH_LEN, "/vol/slccmpt01/ticket/%08x/%08x.tik", CINS_ID_HI,
             CINS_ID_LO);
    snprintf(ticketFolder, CINS_PATH_LEN, "/vol/slccmpt01/ticket/%08x", CINS_ID_HI);
    /* Init stage is not needed anymore. */

    CINS_Log("Writing ticket...\n");
    {
        FSARemove(fsaClient, ticketPath);

        ret = FSAMakeDir(fsaClient, ticketFolder, (FSMode) 0x666);
        if (ret == FS_ERROR_OK || ret == FS_ERROR_ALREADY_EXISTS) {
            CINS_TRY(FSAOpenFileEx(fsaClient, ticketPath, "wb", (FSMode) 0x666, FS_OPEN_FLAG_NONE, 0, &fd) == FS_ERROR_OK);
            CINS_TRY(FSAWriteFile(fsaClient, const_cast<void *>(ticket), ticket_size, 1, fd, FSA_WRITE_FLAG_NONE) == 1);

            FSACloseFile(fsaClient, fd);

            ret = FS_ERROR_OK;
        }

        CINS_TRY(ret == FS_ERROR_OK); // ret == 0
    }

    CINS_Log("Creating title directory...\n");
    {
        /* Create the title directory if it doesn't already exist. The first
         * word (type) should exist, but the second one (the unique title)
         * shouldn't unless there is save data. */
        ret = FSAMakeDir(fsaClient, path, (FSMode) 0x666);
        if (ret == FS_ERROR_OK || ret == FS_ERROR_ALREADY_EXISTS) {
            ret = FSAMakeDir(fsaClient, titlePath, (FSMode) 0x666);
            if (ret == FS_ERROR_ALREADY_EXISTS) {
                /* The title is already installed, delete content but preserve
                 * the data directory. */
                CINS_Log(
                        "Title directory already exists, deleting content...\n");
                snprintf(path, CINS_PATH_LEN, "/vol/slccmpt01/title/%08x/%08x/content",
                         CINS_ID_HI, CINS_ID_LO);
                ret = FSARemove(fsaClient, path);
                if (ret == FS_ERROR_OK || ret == FS_ERROR_NOT_FOUND)
                    ret = FS_ERROR_OK;
            }
        }

        CINS_TRY(ret == FS_ERROR_OK); // ret == 0

        /* This directory is necessary for the Wii Menu to function
         * correctly, but also don't overwrite any data that might already
         * exist. */
        strncpy(pathd, titlePath, CINS_PATH_LEN);
        strncat(pathd, "/data", CINS_PATH_LEN - 1);
        ret = FSAMakeDir(fsaClient, pathd, (FSMode) 0x666);
        if (ret != FS_ERROR_OK && ret != FS_ERROR_ALREADY_EXISTS) {
            CINS_Log("Failed to create the data directory, ret = %d\n", ret);
            goto error;
        }

        strncpy(pathd, titlePath, CINS_PATH_LEN);
        strncat(pathd, "/content", CINS_PATH_LEN - 1);
        CINS_TRY(FSAMakeDir(fsaClient, pathd, (FSMode) 0x666) == FS_ERROR_OK);
    }

    CINS_Log("Writing TMD...\n");
    {
        /* pathd should be the content directory */
        strncpy(path, pathd, CINS_PATH_LEN);
        strncat(path, "/title.tmd", CINS_PATH_LEN - 1);

        CINS_TRY(FSAOpenFileEx(fsaClient, path, "wb", (FSMode) 0x666, FS_OPEN_FLAG_NONE, 0, &fd) == FS_ERROR_OK);
        CINS_TRY(FSAWriteFile(fsaClient, const_cast<void *>(tmd), tmd_size, 1, fd, FSA_WRITE_FLAG_NONE) == 1);

        FSACloseFile(fsaClient, fd);
    }

    CINS_Log("Writing contents...\n");
    {
        for (i = 0; i < numContents; i++) {
            // CINS_Log("Writing content %08x.app\n", i);
            snprintf(path, CINS_PATH_LEN,
                     "/vol/slccmpt01/title/%08x/%08x/content/%08x.app", CINS_ID_HI,
                     CINS_ID_LO, i);

            CINS_TRY(FSAOpenFileEx(fsaClient, path, "wb", (FSMode) 0x666, FS_OPEN_FLAG_NONE, 0, &fd) == FS_ERROR_OK);
            CINS_TRY(FSAWriteFile(fsaClient, const_cast<void *>(contents[i].data), contents[i].length, 1, fd, FSA_WRITE_FLAG_NONE) == 1);

            FSACloseFile(fsaClient, fd);
        }
    }
    ret = IOS_SUCCESS;
    CINS_Log("Install succeeded!\n");

error:
    FSACloseFile(fsaClient, fd);
    if (ret < 0) {
        CINS_Log("Install failed, attempting to delete title...\n");
        /* Installation failed in the final stages. Delete these to be sure
         * there is no 'half installed' title lurking in the filesystem. */
        FSARemove(fsaClient, titlePath);
        FSARemove(fsaClient, ticketPath);
    }

    return ret > 0 ? 0 : ret;
}
