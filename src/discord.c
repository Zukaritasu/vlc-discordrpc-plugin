/*****************************************************************************
 * discord.c: Discord Rich Presence plugin for VLC
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

#include "discord.h"
#include "discordipc.h"
#include "metadata.h"
#include "pluginimages.h"

#include <vlc_common.h>
#include <vlc_threads.h>

/**
 * @struct vlc_discord_internal_data_t
 * @brief Global state container for the Discord RPC plugin.
 *
 * This structure manages the lifecycle of the worker thread,
 * the IPC connection, and the cached media state.
 */
typedef struct
{
	/**
	 * VLC thread handle for the Discord callback loop.
	 * Managed by the Open() function and joined in Close().
	 */
	vlc_thread_t thread;

	/**
	 * Mutex for thread synchronization.
	 * Used to protect the presence data from concurrent access.
	 */
	vlc_mutex_t lock;

	/**
	 * Execution flag for the worker thread.
	 * Set to true on start; set to false to trigger a clean exit of the loop.
	 */
	bool b_run;

	/**
	 * Pointer to the VLC interface object for logging and configuration access.
	 */
	intf_thread_t *p_intf;

	/**
	 * Discord Inter-Process Communication (IPC) instance.
	 * Handles the low-level pipe/socket communication with the Discord client.
	 */
	vlc_discord_ipc_t ipc;

	/**
	 * Current presence data being displayed on Discord.
	 */
	discord_presence_t presence;

	/**
	 * Extracted media metadata (Title, Artist, Album).
	 */
	vlc_discord_metadata_t metadata;

	/**
	 * Plugin user preferences.
	 */
	vlc_discord_settings_t settings;

} vlc_discord_internal_data_t;

/**
 * @brief Internal exception handler for Discord IPC events.
 * * Logs internal messages and errors from the Discord IPC layer to the
 * VLC message log for debugging purposes.
 * * @param p_intf    Pointer to the VLC interface thread.
 * @param psz_msg   The error or status message to be logged.
 */
static void Discord_Exception(intf_thread_t *p_intf, const char *psz_msg)
{
	if (p_intf != NULL && psz_msg != NULL)
	{
		/* msg_Dbg only shows up when VLC is in debug mode (-vv) */
		msg_Dbg(p_intf, "%s", psz_msg);
	}
}

/**
 * @brief Worker thread function for Discord Rich Presence.
 * * Handles the lifecycle of the Discord IPC connection, including connection attempts,
 * presence updates, and error handling.
 * * @param p_data Pointer to the VLC Discord plugin instance.
 * @return NULL
 */
static void *Discord_Callbacks(void *p_data)
{
	vlc_discord_t *self = (vlc_discord_t *)p_data;
	vlc_discord_internal_data_t *p_sys = (vlc_discord_internal_data_t *)self->p_sys;

	if (!DiscordRPC_CreateIPC(&p_sys->ipc, p_sys->p_intf, Discord_Exception))
	{
		return NULL;
	}

	while (p_sys->b_run)
	{
		while (p_sys->b_run)
		{
			// TODO: casting int64_t to uint64_t
			if (p_sys->ipc.pf_connect(&p_sys->ipc, (uint64_t)p_sys->settings.i_client_id))
				break;
			msleep(VLC_TICK_FROM_SEC(2));
		}

		if (!p_sys->b_run)
			break;

		// Set the start time to the current time
		p_sys->presence.i_start_time = SEC_FROM_VLC_TICK(mdate());

		while (p_sys->b_run)
		{
			vlc_mutex_lock(&p_sys->lock);
			p_sys->ipc.pf_set_presence(&p_sys->ipc, p_sys->presence);
			vlc_mutex_unlock(&p_sys->lock);

			if (!p_sys->b_run)
				break;
			if (!p_sys->ipc.pf_is_connected(&p_sys->ipc))
				break;

			msleep(VLC_TICK_FROM_SEC(2));
		}

		if (!p_sys->b_run)
			break;
	}

	// Close the IPC connection and destroy the IPC instance
	p_sys->ipc.pf_close(&p_sys->ipc);
	p_sys->ipc.pf_destroy(&p_sys->ipc);

	memset(&p_sys->ipc, 0, sizeof(vlc_discord_ipc_t));

	return NULL;
}

/**
 * @brief Initializes the Discord presence and starts the worker thread.
 * @param self  Pointer to the VLC Discord plugin instance.
 * @return true if initialization was successful, false otherwise.
 */
static bool Impl_InitializePresence(vlc_discord_t *self)
{
	if (!self || !self->p_sys)
	{
		return false;
	}

	vlc_discord_internal_data_t *p_sys = (vlc_discord_internal_data_t *)self->p_sys;
	if (!p_sys->settings.b_enabled)
	{
		/* true must be returned even if the presence is not active
		becauseif false is returned, it would be considered an initialization error */
		return true;
	}

	p_sys->b_run = true;

	if (vlc_clone(&p_sys->thread, Discord_Callbacks, self, VLC_THREAD_PRIORITY_LOW))
	{
		p_sys->b_run = false;
		return false;
	}

	return true;
}

static bool Impl_Update(vlc_discord_t *self)
{
	if (!self || !self->p_sys)
	{
		return false;
	}

	vlc_discord_internal_data_t *p_sys = (vlc_discord_internal_data_t *)self->p_sys;

	if (!p_sys->settings.b_enabled)
	{
		return true;
	}

	DiscordRPC_GetCurrentMetadata(p_sys->p_intf, &p_sys->metadata);

	vlc_mutex_lock(&p_sys->lock);

	memset(&p_sys->presence, 0, sizeof(discord_presence_t));

	if (p_sys->metadata.b_is_playing)
	{
		if (p_sys->metadata.b_is_paused)
		{
			snprintf(p_sys->presence.sz_small_image, sizeof(p_sys->presence.sz_small_image), PLUGIN_IMAGE_SMALL_PAUSE);
			snprintf(p_sys->presence.sz_small_text, sizeof(p_sys->presence.sz_small_text), "Paused");
		}
		else
		{
			snprintf(p_sys->presence.sz_small_image, sizeof(p_sys->presence.sz_small_image), PLUGIN_IMAGE_SMALL_PLAY);
			snprintf(p_sys->presence.sz_small_text, sizeof(p_sys->presence.sz_small_text), "Playing");

			p_sys->presence.i_start_time = p_sys->metadata.i_start_time;
			p_sys->presence.i_end_time = p_sys->metadata.i_end_time;
		}

		snprintf(p_sys->presence.sz_large_image, sizeof(p_sys->presence.sz_large_image), PLUGIN_IMAGE_LARGE_DEFAULT);
		snprintf(p_sys->presence.sz_large_text, sizeof(p_sys->presence.sz_large_text), "VLC Media Player");

		size_t i_bufsize = sizeof(p_sys->presence.sz_state);

		if (p_sys->settings.b_show_artist && p_sys->metadata.sz_artist[0] != '\0' &&
			p_sys->settings.b_show_album && p_sys->metadata.sz_album[0] != '\0')
		{
			snprintf(p_sys->presence.sz_state, i_bufsize, "%s - %s",
					 p_sys->metadata.sz_artist, p_sys->metadata.sz_album);
		}
		else if (p_sys->settings.b_show_artist && p_sys->metadata.sz_artist[0] != '\0')
		{
			snprintf(p_sys->presence.sz_state, i_bufsize, "%s",
					 p_sys->metadata.sz_artist);
		}
		else if (p_sys->settings.b_show_album && p_sys->metadata.sz_album[0] != '\0')
		{
			snprintf(p_sys->presence.sz_state, i_bufsize, "%s",
					 p_sys->metadata.sz_album);
		}

		snprintf(p_sys->presence.sz_details, sizeof(p_sys->presence.sz_details), "%s", p_sys->metadata.sz_title);
	}
	else
	{
		snprintf(p_sys->presence.sz_large_image, sizeof(p_sys->presence.sz_large_image), PLUGIN_IMAGE_LARGE_DEFAULT);
		snprintf(p_sys->presence.sz_large_text, sizeof(p_sys->presence.sz_large_text), "VLC Media Player");

		snprintf(p_sys->presence.sz_details, sizeof(p_sys->presence.sz_details), "Idling");
	}

	// bool b_result = p_sys->ipc.pf_set_presence(&p_sys->ipc, p_sys->presence);
	vlc_mutex_unlock(&p_sys->lock);

	return true;
}

/**
 * @brief Closes the Discord IPC connection and joins the worker thread.
 * @param self  Pointer to the VLC Discord plugin instance.
 * @return true if the connection was closed successfully, false otherwise.
 */
static bool Impl_Close(vlc_discord_t *self)
{
	if (!self || !self->p_sys)
		return false;
	vlc_discord_internal_data_t *p_sys = (vlc_discord_internal_data_t *)self->p_sys;

	if (p_sys->b_run)
	{
		p_sys->b_run = false;
		vlc_join(p_sys->thread, NULL);
	}

	vlc_mutex_destroy(&p_sys->lock);

	return true;
}

/**
 * @brief Destroys the VLC Discord plugin instance.
 * @param self  Pointer to the VLC Discord plugin instance.
 * @return true if the instance was destroyed successfully, false otherwise.
 */
static bool Impl_Destroy(vlc_discord_t *self)
{
	if (!self || !self->p_sys)
		return false;
	vlc_discord_internal_data_t *p_sys = (vlc_discord_internal_data_t *)self->p_sys;

	free(p_sys);
	self->p_sys = NULL;

	return true;
}

static bool Impl_SetEnabled(vlc_discord_t *self, bool b_enable)
{
	if (!self || !self->p_sys)
		return false;
	vlc_discord_internal_data_t *p_sys = (vlc_discord_internal_data_t *)self->p_sys;

	p_sys->settings.b_enabled = b_enable;

	if (b_enable)
	{
		if (!p_sys->b_run)
		{
			p_sys->b_run = true;
			if (vlc_clone(&p_sys->thread, Discord_Callbacks, self, VLC_THREAD_PRIORITY_LOW))
			{
				p_sys->b_run = false;
				return false;
			}
		}
	}
	else
	{
		if (p_sys->b_run)
		{
			p_sys->b_run = false;
			vlc_join(p_sys->thread, NULL);
		}
	}

	return true;
}

bool Discord_CreateInstance(vlc_discord_t *discord, vlc_discord_settings_t stgs, intf_thread_t *p_intf)
{
	if (!discord)
	{
		return false;
	}

	discord->pf_initialize_presence = Impl_InitializePresence;
	discord->pf_update = Impl_Update;
	discord->pf_close = Impl_Close;
	discord->pf_destroy = Impl_Destroy;
	discord->pf_set_enabled = Impl_SetEnabled;
	discord->p_sys = calloc(1, sizeof(vlc_discord_internal_data_t));

	if (!discord->p_sys)
	{
		return false;
	}

	((vlc_discord_internal_data_t *)discord->p_sys)->p_intf = p_intf;
	((vlc_discord_internal_data_t *)discord->p_sys)->settings = stgs;

	vlc_mutex_init(&((vlc_discord_internal_data_t *)discord->p_sys)->lock);

	return true;
}