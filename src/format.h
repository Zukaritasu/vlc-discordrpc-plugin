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

 #ifndef FORMAT_H
 #define FORMAT_H

 #include <stdint.h>
 #include "metadata.h"

 /**
  * @brief Formats a string based on the provided format and metadata.
  * * @param psz_buffer   Pointer to the buffer where the formatted string will be stored.
  * @param i_bufferSize The size of the buffer.
  * @param psz_format   The format string.
  * @param p_md         Pointer to the metadata structure.
  * @param p_dict       Pointer to the dictionary structure.
  * @return The number of characters written to the buffer.
  */

 size_t DiscordRPC_Format(char *psz_buffer, size_t i_buffer_size, const char* psz_format, 
    vlc_discord_metadata_t *p_md, vlc_dictionary_t *p_dict);

 #endif // FORMAT_H