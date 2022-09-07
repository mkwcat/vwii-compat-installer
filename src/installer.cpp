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
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CINS_DEBUG

void WUPI_putstr(const char*);

#ifdef CINS_DEBUG
#define CINS_Log(...)                                                          \
    do                                                                         \
    {                                                                          \
        char _wupi_print_str[256];                                             \
        snprintf(_wupi_print_str, 255, __VA_ARGS__);                           \
        WUPI_putstr(_wupi_print_str);                                          \
    } while (0)
#else
#define CINS_Log(...)
#endif

#define CINS_STAGE_INIT 1
#define CINS_STAGE_TICKET 2
#define CINS_STAGE_TITLEDIR 3
#define CINS_STAGE_TMD 4
#define CINS_STAGE_CONTENT 5
#define CINS_STAGE_FINAL 6
#define CINS_STAGE_FINAL_DATA 7
#define CINS_STAGE_DELETE_TIK 8
#define CINS_STAGE_DELETE_TITLE 9

#define IOS_SUCCESS 0

#define CINS_PATH_LEN (sizeof("slccmpt:") + 63)

#define CINS_ID_HI ((uint32_t)(CINS_TITLEID >> 32))
#define CINS_ID_LO ((uint32_t)(CINS_TITLEID & 0xFFFFFFFF))

#define CINS_TRY(c)                                                            \
    if (!(c))                                                                  \
        do                                                                     \
        {                                                                      \
            CINS_Log("'%s' failed with errno: %d\n", #c, errno);               \
            ret = errno < 0 ? errno : -errno;                                  \
            goto error;                                                        \
    } while (0)

int32_t CINS_Install(const void* ticket, uint32_t ticket_size, const void* tmd,
                     uint32_t tmd_size, CINS_Content* contents,
                     uint16_t numContents)
{
    int32_t ret, stage, i;
    FILE* fd = NULL;
    char path[CINS_PATH_LEN], pathd[CINS_PATH_LEN];
    char titlePath[CINS_PATH_LEN], ticketPath[CINS_PATH_LEN],
        ticketFolder[CINS_PATH_LEN];

    CINS_Log("Starting install\n");

    /* This installer originally created a temporary directory for the
     * installation, wrote everything to flash there, then renamed it all to
     * other directories. The wupserver doesn't already support renaming files,
     * and my attempt to add it failed so I gave up. */
    snprintf(titlePath, CINS_PATH_LEN, "slccmpt:/title/%08x/%08x", CINS_ID_HI,
             CINS_ID_LO);
    snprintf(path, CINS_PATH_LEN, "slccmpt:/title/%08x", CINS_ID_HI);
    snprintf(ticketPath, CINS_PATH_LEN, "slccmpt:/ticket/%08x/%08x.tik", CINS_ID_HI,
             CINS_ID_LO);
    snprintf(ticketFolder, CINS_PATH_LEN, "slccmpt:/ticket/%08x", CINS_ID_HI);
    /* Init stage is not needed anymore. */

    CINS_Log("Writing ticket...\n");
    stage = CINS_STAGE_TICKET;
    {
        unlink(ticketPath);

        ret = mkdir(ticketFolder, 0x666);
        if (ret == 0 || errno == EEXIST)
        {
            CINS_TRY(fd = fopen(ticketPath, "wb"));
            CINS_TRY(fwrite(ticket, ticket_size, 1, fd) == 1);

            fclose(fd);
            fd = NULL;

            ret = 0;
        }

        CINS_TRY(!ret); // ret == 0
    }

    CINS_Log("Creating title directory...\n");
    stage = CINS_STAGE_TITLEDIR;
    {
        /* Create the title directory if it doesn't already exist. The first
         * word (type) should exist, but the second one (the unique title)
         * shouldn't unless there is save data. */
        ret = mkdir(path, -1);
        if (ret == 0 || errno == EEXIST)
        {
            ret = mkdir(titlePath, -1);
            if (ret != 0 && errno == EEXIST)
            {
                /* The title is already installed, delete content but preserve
                 * the data directory. */
                CINS_Log(
                    "Title directory already exists, deleting content...\n");
                snprintf(path, CINS_PATH_LEN, "slccmpt:/title/%08x/%08x/content",
                         CINS_ID_HI, CINS_ID_LO);
                if (unlink(path) == 0 || errno == ENOENT)
                    ret = 0;
            }
        }

        CINS_TRY(!ret); // ret == 0

        /* This directory is necessary for the Wii Menu to function
         * correctly, but also don't overwrite any data that might already
         * exist. */
        strncpy(pathd, titlePath, CINS_PATH_LEN);
        strncat(pathd, "/data", CINS_PATH_LEN - 1);
        if (mkdir(pathd, -1) != 0 && errno != EEXIST)
        {
            CINS_Log("Failed to create the data directory, ret = %d\n", ret);
            goto error;
        }

        strncpy(pathd, titlePath, CINS_PATH_LEN);
        strncat(pathd, "/content", CINS_PATH_LEN - 1);
        CINS_TRY(mkdir(pathd, -1) == 0);
    }

    CINS_Log("Writing TMD...\n");
    stage = CINS_STAGE_TMD;
    {
        /* pathd should be the content directory */
        strncpy(path, pathd, CINS_PATH_LEN);
        strncat(path, "/title.tmd", CINS_PATH_LEN - 1);

        CINS_TRY(fd = fopen(path, "wb"));
        CINS_TRY(fwrite(tmd, tmd_size, 1, fd) == 1);

        fclose(fd);
        fd = NULL;
    }

    CINS_Log("Writing contents...\n");
    stage = CINS_STAGE_CONTENT;
    {
        for (i = 0; i < numContents; i++)
        {
            // CINS_Log("Writing content %08x.app\n", i);
            snprintf(path, CINS_PATH_LEN,
                     "slccmpt:/title/%08x/%08x/content/%08x.app", CINS_ID_HI,
                     CINS_ID_LO, i);

            CINS_TRY(fd = fopen(path, "wb"));
            CINS_TRY(fwrite(contents[i].data, contents[i].length, 1, fd) == 1);

            fclose(fd);
            fd = NULL;
        }
    }
    ret = IOS_SUCCESS;
    CINS_Log("Install succeeded!\n");

error:
    if (fd != NULL)
        fclose(fd);
    if (ret < 0)
    {
        CINS_Log("Install failed, attempting to delete title...\n");
        /* Installation failed in the final stages. Delete these to be sure
         * there is no 'half installed' title lurking in the filesystem. */
        unlink(titlePath);
        unlink(ticketPath);
    }

    if (ret < -0x99999)
        ret = -0x800;
    if (ret > 0)
        ret = 0;
    return ret < 0 ? ret - stage * 0x100000 : 0;
}

/* This doesn't seem to work */
int32_t CINS_Uninstall(void)
{
    char titlePath[CINS_PATH_LEN], ticketPath[CINS_PATH_LEN];
    int32_t ret, stage;
    // uint32_t cnt;

    CINS_Log("Begin uninstall...\n");
    snprintf(titlePath, CINS_PATH_LEN, "slccmpt:/title/%08X/%08X", CINS_ID_HI,
             CINS_ID_LO);
    snprintf(ticketPath, CINS_PATH_LEN, "slccmpt:/ticket/%08X/%08X.tik", CINS_ID_HI,
             CINS_ID_LO);

    stage = CINS_STAGE_DELETE_TIK;
    if (unlink(ticketPath) != 0 && errno != ENOENT)
    {
        /* Deleting the ticket is not 100% necessary. */
        CINS_Log("Failed to delete ticket\n");
    }

    /* Set ret to less than zero so it deletes the full directory if this is not
     * true. */
    ret = -1;
#if 0
    if (!deleteSaveData)
    {
        char path[CINS_PATH_LEN];
        stage = CINS_STAGE_DELETE_KEEP;
        //CINS_TRY(ES_DeleteTitleContent(CINS_TITLEID));

        snprintf(path, CINS_PATH_LEN,
                 "slccmpt:/title/%08X/%08X/data",
                 CINS_ID_HI, CINS_ID_LO);
        ret = ISFS_ReadDir(path, NULL, &cnt);
        /* ret < 0 would normally cause a failure in the Wii Menu channel
         * uninstaller, but we're smarter than that. */
        if (ret >= 0 && cnt == 0)
            ret = -1;
        else CINS_Log(
            "The /data directory is not empty, skipping full uninstall.\n");
    }
#endif
    // for no compilation warning
    // error:
    if (ret < 0)
    {
        /* Delete the entire title directory on an error. */
        ret = unlink(titlePath);
        if (ret < 0)
        {
            if (ret != ENOENT)
                CINS_Log("Failed to delete title\n");
            else
                ret = IOS_SUCCESS;
        }
#if 0
        if (ret >= IOS_SUCCESS)
        {
            /* Delete title top directory if there are no other titles.
             * (untested) */
            uint32_t num;

            snprintf(titlePath, 64, "slccmpt:/title/%08X",
                     CINS_ID_HI);
            ret = ISFS_ReadDir(titlePath, NULL, &num);

            if (ret >= IOS_SUCCESS && num == 0 &&
                ISFS_Delete(titlePath) < IOS_SUCCESS)
                CINS_Log("Failed to delete title top directory\n");

            ret = IOS_SUCCESS;
        }
#endif
    }

    if (ret < -0x99999)
        ret = -0x800;
    if (ret > 0)
        ret = 0;
    return ret < 0 ? ret - stage * 0x100000 : 0;
}
