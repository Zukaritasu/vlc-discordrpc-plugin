/*****************************************************************************
 * discordipc.h: Discord Rich Presence plugin for VLC
 *****************************************************************************
 * Copyright (C) 2026 Zukaritasu
 *
 * Authors: Zukaritasu <zukaritasu@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *****************************************************************************/

#ifndef DISCORDIPC_H
#define DISCORDIPC_H

#include <stdbool.h>
#include <stdint.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>

/**
 * @brief Exception callback for internal IPC errors.
 * @param p_intf Pointer to the VLC interface thread.
 * @param psz_excep Error message string.
 */
typedef void (*DiscordIPCException)(intf_thread_t *p_intf, const char *psz_excep);

/**
 * @brief Presence metadata structure (Rich Presence state).
 * * Defines the visual information sent to Discord, including strings for
 * status, images, and session timestamps.
 */
typedef struct 
{
    char sz_state[128];       /**< User's current status (e.g., "Playing") */
    char sz_details[128];     /**< Track details (e.g., "Artist - Title") */
    char sz_large_image[128]; /**< Key for the large asset image */
    char sz_large_text[128];  /**< Hover text for the large image */
    char sz_small_image[128]; /**< Key for the small asset image */
    char sz_small_text[128];  /**< Hover text for the small image */

    int64_t i_start_time;     /**< Epoch timestamp for the start of the activity */
    int64_t i_end_time;       /**< Epoch timestamp for the end of the activity */
} discord_presence_t;

/**
 * @brief Discord IPC Manager structure.
 * * Handles the lifecycle of the IPC connection, including establishment via
 * named pipes/sockets, data transmission, and resource cleanup.
 */
typedef struct DiscordIPC
{
    /**
     * @brief Closes the active IPC pipe connection.
     * @param p_self Pointer to the DiscordIPC instance.
     * @return true on success, false if the instance is invalid.
     */
    bool (*pf_close)(struct DiscordIPC *p_self);

    /**
     * @brief Destroys the DiscordIPC object and frees internal resources.
     * @param p_self Pointer to the DiscordIPC instance.
     * @return true on success.
     */
    bool (*pf_destroy)(struct DiscordIPC *p_self);

    /**
     * @brief Updates the user's Rich Presence on Discord.
     * @param p_self Pointer to the DiscordIPC instance.
     * @param presence The presence data to be transmitted.
     * @return false if the connection is invalid or memory allocation fails.
     */
    bool (*pf_set_presence)(struct DiscordIPC *p_self, discord_presence_t presence);

    /**
     * @brief Establishes a connection with Discord using IPC pipes.
     * @param p_self Pointer to the DiscordIPC instance.
     * @param i_id The Discord Application Client ID.
     * @return false if Discord is not running or all pipes are busy.
     */
    bool (*pf_connect)(struct DiscordIPC *p_self, uint64_t i_id);

    /**
     * @brief Checks the current status of the IPC connection.
     * @param p_self Pointer to the DiscordIPC instance.
     * @return true if the pipe connection is stable, false otherwise.
     */
    bool (*pf_is_connected)(const struct DiscordIPC *p_self);

    /** 
	 * @brief Private internal data for the IPC implementation.
     */
    void *p_sys; 
} vlc_discord_ipc_t;

/**
 * @brief Initializes the DiscordIPC object.
 * * Allocates necessary resources and sets up function pointers for IPC interaction.
 * * @param p_ipc Pointer to the structure to be initialized.
 * @param p_intf Pointer to the VLC interface thread.
 * @param pf_err (Optional) Callback for internal error reporting.
 * @return true on successful initialization, false on invalid parameters or OOM.
 */
bool DiscordRPC_CreateIPC(vlc_discord_ipc_t *p_ipc, intf_thread_t *p_intf, DiscordIPCException pf_err);

#endif // DISCORDIPC_H
