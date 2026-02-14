/*****************************************************************************
 * settings.c: Discord Rich Presence plugin for VLC
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

#include "settings.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>
#include <vlc_variables.h>

void DiscordRPC_LoadSettings(vlc_discord_settings_t *p_stgs, void *p_data)
{
	intf_thread_t *p_intf = (intf_thread_t *)p_data;
    
    memset(p_stgs, 0, sizeof(vlc_discord_settings_t));

    char *psz_client_id = var_InheritString(p_intf, ID_RPC_CLIENT_ID);
    
    if (psz_client_id != NULL)
    {
        char *p_endptr;
        size_t i_len = strlen(psz_client_id);
        
        p_stgs->i_client_id = strtoull(psz_client_id, &p_endptr, 10);

        if (psz_client_id == p_endptr || *p_endptr != '\0' || i_len < 17 || i_len > 20)
            p_stgs->i_client_id = strtoull(DEFAULT_CLIENT_ID, NULL, 10);

        free(psz_client_id);
    }
    else
    {
        p_stgs->i_client_id = strtoull(DEFAULT_CLIENT_ID, NULL, 10);
    }

    p_stgs->b_enabled     = var_InheritBool(p_intf, ID_RPC_ENABLED);
    p_stgs->b_show_album   = var_InheritBool(p_intf, ID_RPC_SHOW_ALBUM);
    p_stgs->b_show_artist  = var_InheritBool(p_intf, ID_RPC_SHOW_ARTIST);
}