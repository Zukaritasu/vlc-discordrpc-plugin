/*****************************************************************************
 * plugin.c: Discord Rich Presence plugin for VLC
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

#define MODULE_STRING "discord_rpc"

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>

#include <string.h>

#include "discord.h"
#include "settings.h"

/**
 * @brief Internal state for the Discord RPC interface.
 */
typedef struct {
    vlc_discord_t          discord;  /**< Discord RPC handle */
    vlc_discord_settings_t settings; /**< Plugin configuration settings */
    vlc_timer_t            timer;    /**< Timer for periodic presence updates */
} intdf_sys_t;

static int  Open (vlc_object_t *);
static void Close(vlc_object_t *);

vlc_module_begin()
    set_shortname("Discord RPC")
    set_description("Discord Rich Presence by Zukaritasu")
    set_category(CAT_INTERFACE)
    set_subcategory(SUBCAT_INTERFACE_CONTROL)
    set_capability("interface", 0)

    /** settings */
    add_string(ID_RPC_CLIENT_ID, DEFAULT_CLIENT_ID, "Discord Application ID", "Enter the Client ID obtained from the Discord Developer Portal.", false)
    add_bool(ID_RPC_ENABLED, true, "Enable Rich Presence", "Enable or disable Discord Rich Presence integration.", false)
    add_bool(ID_RPC_SHOW_ARTIST, true, "Show artist name", "Display the artist name in your Discord status.", false)
    add_bool(ID_RPC_SHOW_ALBUM, true, "Show album name", "Display the album title in your Discord status.", false)
    /** */

    set_callbacks(Open, Close)
vlc_module_end()

/**
 * @brief Timer callback function for periodic presence updates.
 * 
 * This function is called by the VLC timer at regular intervals to update
 * the Discord Rich Presence with the current playback information.
 * 
 * @param data Pointer to the interface thread structure
 */
static void OnTimer(void *data) {
    intf_thread_t *p_intf = (intf_thread_t *)data;
    intdf_sys_t *p_sys = (intdf_sys_t*)p_intf->p_sys;

    if (p_sys) p_sys->discord.pf_update(&p_sys->discord);
}

/**
 * @brief Open function for the Discord RPC interface.
 * 
 * This function is called when the plugin is loaded by VLC.
 * It initializes the Discord Rich Presence instance and sets up the timer
 * for periodic presence updates.
 * 
 * @param p_this Pointer to the interface thread structure
 * @return VLC_SUCCESS on success, VLC_ENOMEM on memory allocation failure,
 *         VLC_EGENERIC on other errors
 */
static int Open(vlc_object_t *p_this) {
    intf_thread_t *p_intf = (intf_thread_t *)p_this;

    intdf_sys_t *p_sys = malloc(sizeof(intdf_sys_t));

    if (!p_sys) return VLC_ENOMEM;

    p_intf->p_sys = (intf_sys_t*)p_sys;
    
    msg_Info(p_intf, "Starting Discord Rich Presence..");

    DiscordRPC_LoadSettings(&p_sys->settings, (void*)p_intf);
    
    memset(&p_sys->discord, 0, sizeof(vlc_discord_t));
    
    if (!Discord_CreateInstance(&p_sys->discord, p_sys->settings, p_intf))
    {
        msg_Err(p_intf, "An error occurred while creating the Discord instance");
        free(p_sys);
        return VLC_EGENERIC;
    }
    
    if (!p_sys->discord.pf_initialize_presence(&p_sys->discord))
    {
        msg_Err(p_intf, "An error occurred while initializing presence");
        free(p_sys);
        return VLC_EGENERIC;
    }

    if (vlc_timer_create(&p_sys->timer, OnTimer, p_intf) != 0)
    {
        p_sys->discord.pf_destroy(&p_sys->discord);
        free(p_sys);
        return VLC_ENOMEM;
    }
    
    vlc_timer_schedule(p_sys->timer, false, vlc_tick_from_sec(1), vlc_tick_from_sec(2));
    
    return VLC_SUCCESS;
}

/**
 * @brief Close function for the Discord RPC interface.
 * 
 * This function is called when the plugin is unloaded by VLC.
 * It cleans up the Discord Rich Presence instance and releases all resources.
 * 
 * @param p_this Pointer to the interface thread structure
 */
static void Close(vlc_object_t *p_this) {
    intf_thread_t *p_intf = (intf_thread_t *)p_this;
    intdf_sys_t *p_sys = (intdf_sys_t*)p_intf->p_sys;

    if (!p_sys) return;

    vlc_timer_destroy(p_sys->timer);
    
    if (p_sys->discord.pf_close) p_sys->discord.pf_close(&p_sys->discord);
    if (p_sys->discord.pf_destroy) p_sys->discord.pf_destroy(&p_sys->discord);
    
    free(p_sys);
}