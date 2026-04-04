/*****************************************************************************
 * metadata.h: Discord Rich Presence plugin for VLC
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

#ifndef METADATA_H
#define METADATA_H

#include <stdint.h>
#include <stdbool.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_arrays.h>

/**
 * @struct playlist_info_t
 * @brief Playlist info
 * * This structure provides information to the rich presence to display
 * the playlist
 */
typedef struct 
{
    int i_curr_pos;      /**< Current position (1-based) */
    int i_total_items;   /**< Total in playlist */
    bool b_has_playlist; /**< If there is an active playlist */
} playlist_info_t;

/**
 * @struct vlc_discord_metadata_t
 * @brief Container for media track information.
 * * This structure holds the processed metadata extracted from the VLC 
 * input item, formatted for Discord Rich Presence transmission.
 */
typedef struct 
{
    char sz_title[128];  /**< Track or filename title */
    char sz_artist[128]; /**< Performer or creator name */
    char sz_album[128];  /**< Album or collection title */

    int64_t i_start_time; /**< Playback start timestamp (Epoch) */
    int64_t i_end_time;   /**< Estimated playback end timestamp (Epoch) */

    bool b_is_video;   /**< True if the current media has a video track */
    bool b_is_audio;   /**< True if the current media has a audio track */
    bool b_is_paused;  /**< True if playback is currently suspended */
    bool b_is_playing; /**< True if there is an active input item */
    playlist_info_t playlist_info; /**< Current playlist */

} vlc_discord_metadata_t;

/**
 * @brief Extracts current media metadata from the VLC playlist/input.
 * * Accesses the internal VLC input thread to retrieve meta tags (Artist, Title, etc.)
 * and calculates the current playback state and timestamps. 
 * * @param p_intf Pointer to the VLC interface thread.
 * @param p_md   Pointer to the metadata structure to be populated.
 * @return true if metadata was successfully retrieved, false otherwise.
 */
bool DiscordRPC_GetCurrentMetadata(intf_thread_t *p_intf, vlc_discord_metadata_t *p_md);

/**
 * @brief Converts the metadata structure into a dictionary format.
 * * This function is intended to transform the structured metadata into a key-value
 * dictionary that can be used for dynamic formatting of the Discord Rich Presence fields.
 * * @param p_md   Pointer to the metadata structure containing media information.
 * @param p_dict  Pointer to the dictionary structure to be filled with metadata entries.
 */
void DiscordRPC_MetadataToDictionary(vlc_discord_metadata_t *p_md, vlc_dictionary_t *p_dict);

#endif // METADATA_H