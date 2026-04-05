/*****************************************************************************
 * metadata.c: Discord Rich Presence plugin for VLC
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

#include "metadata.h"

#include <vlc_input.h>
#include <vlc_meta.h>
#include <vlc_variables.h>
#include <vlc_common.h>
#include <vlc_interface.h>
#include <vlc_playlist.h>
#include <time.h>

static playlist_info_t GetPlaylistInfo(intf_thread_t *p_intf)
{
    playlist_info_t info;
	memset(&info, 0, sizeof(playlist_info_t));
    
    playlist_t *p_playlist = pl_Get(p_intf);
    if (!p_playlist)
        return info;
    
    playlist_Lock(p_playlist);
    
    info.i_total_items = p_playlist->current.i_size;
    info.i_curr_pos = p_playlist->i_current_index + 1;

	// It is considered a playlist if it has at least two items
    info.b_has_playlist = (info.i_curr_pos > 0 && info.i_total_items > 1);
    
    playlist_Unlock(p_playlist);
    
    return info;
}

bool DiscordRPC_GetCurrentMetadata(intf_thread_t *p_intf, vlc_discord_metadata_t *p_md)
{
	if (!p_intf || !p_md) return false;

	memset(p_md, 0, sizeof(vlc_discord_metadata_t));

	input_thread_t *p_input = pl_CurrentInput(p_intf);
	if (!p_input) return false;

	input_item_t *p_item = input_GetItem(p_input);
	if (!p_item)
	{
		vlc_object_release(p_input);
		return false;
	}

	p_md->b_is_playing = true;
	p_md->playlist_info = GetPlaylistInfo(p_intf);

	int i_state = var_GetInteger(p_input, "state");
	p_md->b_is_paused = i_state == PAUSE_S;

	int i_nb_tracks = p_item->i_es;

	for (int i = 0; i < i_nb_tracks; i++)
	{
		if (p_item->es[i]->i_cat == VIDEO_ES)
			p_md->b_is_video = true;
		else if (p_item->es[i]->i_cat == AUDIO_ES)
			p_md->b_is_audio = true;
	}

	char *psz_title = input_item_GetMeta(p_item, vlc_meta_Title);
	char *psz_artist = input_item_GetMeta(p_item, vlc_meta_Artist);
	char *psz_album = input_item_GetMeta(p_item, vlc_meta_Album);

	if (!psz_title)
	{
		psz_title = input_item_GetName(p_item);
		if (psz_title)
		{
			// Find the last point, but do NOT delete it if it is at the beginning
			// example: /.mp3
			for (int64_t i = strlen(psz_title) - 1; i > 0; i--)
			{
				if (psz_title[i] == '.')
				{
					psz_title[i] = '\0';
					break;
				}
			}
		}
	}

	snprintf(p_md->sz_title, sizeof(p_md->sz_title), "%s", psz_title ? psz_title : "VLC Media Player");
	snprintf(p_md->sz_artist, sizeof(p_md->sz_artist), "%s", psz_artist ? psz_artist : "");
	snprintf(p_md->sz_album, sizeof(p_md->sz_album), "%s", psz_album ? psz_album : "");

	mtime_t i_vlc_time = var_GetInteger(p_input, "time");
	mtime_t i_vlc_len = input_item_GetDuration(p_item);

	int64_t i_now = (int64_t)time(NULL);
	p_md->i_start_time = i_now - (i_vlc_time / 1000000);

	if (i_vlc_len > 0)
		p_md->i_end_time = p_md->i_start_time + (i_vlc_len / 1000000);
	else
		p_md->i_end_time = 0;

	free(psz_title);
	free(psz_artist);
	free(psz_album);

	vlc_object_release(p_input);
	
	return true;
}

void DiscordRPC_MetadataToDictionary(vlc_discord_metadata_t *p_md, vlc_dictionary_t *p_dict)
{
	char sz_number[12];

	vlc_dictionary_init(p_dict, 0);
	
	vlc_dictionary_insert(p_dict, "title", p_md->sz_title);

	vlc_dictionary_insert(p_dict, "artist", p_md->sz_artist);
	vlc_dictionary_insert(p_dict, "album", p_md->sz_album);
	vlc_dictionary_insert(p_dict, "status", p_md->b_is_playing ? (p_md->b_is_paused ? "Paused" : "Playing") : "Stopped");

	snprintf(sz_number, sizeof(sz_number), "%d", p_md->playlist_info.i_curr_pos);
	vlc_dictionary_insert(p_dict, "pls_pos", sz_number);

	snprintf(sz_number, sizeof(sz_number), "%d", p_md->playlist_info.i_total_items);
	vlc_dictionary_insert(p_dict, "pls_total", sz_number);
}
