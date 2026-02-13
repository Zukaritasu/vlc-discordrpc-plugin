/*****************************************************************************
 * settings.h: Discord Rich Presence plugin for VLC
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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

/* VLC Module Configuration IDs */
#define CFG_PREFIX "discord-"
#define ID_RPC_CLIENT_ID    CFG_PREFIX "client-id"
#define ID_RPC_ENABLED      CFG_PREFIX "enabled"
#define ID_RPC_HIDE_ALBUM   CFG_PREFIX "hide-album"
#define ID_RPC_HIDE_ARTIST  CFG_PREFIX "hide-artist"

/**
 * @brief Default Discord Application ID.
 * This is used if the user doesn't provide their own in the settings.
 */
#define DEFAULT_CLIENT_ID "1041018234058571847"

/**
 * @struct vlc_discord_settings_t
 * @brief Configuration state for the Discord RPC plugin.
 * Holds the preferences retrieved from VLC's configuration database.
 */
typedef struct 
{
    uint64_t i_client_id;   /**< Discord Application Client ID */
    bool     b_enabled;     /**< Master switch for the plugin */
    bool     b_hide_artist; /**< Toggle to privacy-mask the artist name */
    bool     b_hide_album;  /**< Toggle to privacy-mask the album name */
} vlc_discord_settings_t;

/**
 * @brief Loads plugin settings from the VLC configuration store.
 * * Accesses VLC's internal variable system to populate the settings structure.
 * 
 * * @param p_stgs Pointer to the settings structure to fill.
 * @param p_intf Pointer to the VLC interface thread (intf_thread_t).
 */
void DiscordRPC_LoadSettings(vlc_discord_settings_t *p_stgs, void *p_intf);

#endif // SETTINGS_H