/*****************************************************************************
 * discord.h: Discord Rich Presence plugin for VLC
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

#ifndef DISCORD_H
#define DISCORD_H

#include <stdbool.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>

#include "settings.h"

/**
 * @struct vlc_discord_t
 * @brief High-level controller for the Discord RPC plugin.
 * * This structure acts as the main interface for the VLC module, 
 * abstracting the thread management and periodic updates.
 */
typedef struct vlc_discord_t
{
    /**
     * @brief Starts the background worker thread and IPC connection.
     * @return true if the thread and presence were initialized successfully.
     */
    bool (*pf_initialize_presence)(struct vlc_discord_t *p_self);

    /**
     * @brief Triggers a manual update of the Rich Presence data.
     * @return true if the update was dispatched to the IPC layer.
     */
    bool (*pf_update)(struct vlc_discord_t *p_self);

    /**
     * @brief Signals the worker thread to stop and closes IPC.
     * @return true on successful shutdown.
     */
    bool (*pf_close)(struct vlc_discord_t *p_self);

    /**
     * @brief Frees all resources associated with the Discord instance.
     * @return true if memory was cleaned up correctly.
     */
    bool (*pf_destroy)(struct vlc_discord_t *p_self);

    /**
     * @brief Toggles the Discord Rich Presence status.
     *
     * Disabling the presence suspends updates and closes the active session
     * without destroying the internal vlc_discord_t instance. This preserves
     * the configuration and state, allowing the presence to be re-enabled
     * later without the overhead of full re-initialization.
     *
     * @param p_self    Pointer to the Discord instance.
     * @param b_enable  true to enable presence, false to disable.
     * @return          true if the operation was successful.
     */
    bool (*pf_set_enabled)(struct vlc_discord_t *p_self, bool b_enable);

    /** Private internal data (vlc_discord_internal_data_t) */
    void *p_sys;

} vlc_discord_t;

/**
 * @brief Factory function to create and initialize a Discord instance.
 * @param p_discord Pointer to the structure to be populated.
 * @param settings  Initial plugin configuration.
 * @param p_intf    Pointer to the VLC interface thread.
 * @return true if the instance was created and function pointers assigned.
 */
bool Discord_CreateInstance(vlc_discord_t *p_discord, vlc_discord_settings_t settings, intf_thread_t *p_intf);

#endif // DISCORD_H