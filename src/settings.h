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
#define ID_RPC_CLIENT_ID         CFG_PREFIX "client-id"
#define ID_RPC_ENABLE            CFG_PREFIX "enabled"

#define ID_RPC_DETAILS_FORMAT    CFG_PREFIX "details-format"
#define ID_RPC_STATE_FORMAT      CFG_PREFIX "state-format"
#define ID_RPC_LARGE_TEXT_FORMAT CFG_PREFIX "large-text-format"
#define ID_RPC_SMALL_TEXT_FORMAT CFG_PREFIX "small-text-format"

#define ID_RPC_ENABLE_DETAILS    CFG_PREFIX "enable-details-field"
#define ID_RPC_ENABLE_STATE      CFG_PREFIX "enable-state-field"

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
    uint64_t i_client_id;      /**< Discord Application Client ID */
    bool     b_enable;        /**< Master switch for the plugin */
    bool     b_enable_details; /**< Toggle for the details field in Rich Presence */
    bool     b_enable_state;   /**< Toggle for the state field in Rich Presence */

    char*    psz_details_format;    /**< Format string for the details field */
    char*    psz_state_format;      /**< Format string for the state field */
    char*    psz_large_text_format; /**< Format string for the large image text */
    char*    psz_small_text_format; /**< Format string for the small image text */
} vlc_discord_settings_t;

/**
 * @brief Loads plugin settings from the VLC configuration store.
 * * Accesses VLC's internal variable system to populate the settings structure.
 * 
 * * @param p_stgs Pointer to the settings structure to fill.
 * @param p_intf Pointer to the VLC interface thread (intf_thread_t).
 */
void DiscordRPC_LoadSettings(vlc_discord_settings_t *p_stgs, void *p_intf);

void DiscordRPC_FreeSettings(vlc_discord_settings_t *p_stgs);

#endif // SETTINGS_H