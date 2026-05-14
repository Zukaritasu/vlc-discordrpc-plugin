/*****************************************************************************
 * cover.h: Discord Rich Presence plugin for VLC
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

#ifndef COVER_H
#define COVER_H

#include <stdint.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>

struct cover_handle;

typedef struct cover_handle cover_handle_t;

/**
 * @brief Obtains the cover URL from the API.
 * 
 * @param p_handle The cover handle.
 * @param psz_artworkurl The artwork URL.
 * @param psz_url The URL buffer to store the cover URL.
 * @param i_cbuf The size of the URL buffer.
 */
void DiscordRPC_GetCoverURL(cover_handle_t* p_handle,
                            const char *psz_artworkurl,
                            char *psz_url,
                            size_t i_cbuf);

/**
 * @brief Creates a new cover handle.
 *
 * @param p_intf The interface thread.
 * @return The cover handle.
 */
cover_handle_t *DiscordRPC_CreateCoverHandle(intf_thread_t *p_intf);

/**
 * @brief Closes the cover handle.
 *
 * @param p_handle The cover handle.
 */
void DiscordRPC_CloseCoverHandle(cover_handle_t *p_handle);

#endif // COVER_H