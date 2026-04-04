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

#include "format.h"

static bool is_separator(char c)
{
    return c == '-' || c == '/' || c == '~' || c == '|';
}

static void trim_string(char *psz_string)
{
    size_t i_start = 0;
    
    while (psz_string[i_start] != '\0' && isspace((unsigned char)psz_string[i_start]))
    {
        i_start++;
    }
    
    if (psz_string[i_start] == '\0')
    {
        psz_string[0] = '\0';
        return;
    }
    
    size_t i_end = strlen(psz_string) - 1;
    
    while (i_end > i_start && isspace((unsigned char)psz_string[i_end]))
    {
        i_end--;
    }
    
    size_t new_len = i_end - i_start + 1;
    if (i_start > 0)
    {
        memmove(psz_string, psz_string + i_start, new_len);
    }
    psz_string[new_len] = '\0';
}

size_t DiscordRPC_Format(char *psz_buffer, size_t i_buffer_size, const char* psz_format, 
    vlc_discord_metadata_t *p_md, vlc_dictionary_t *p_dict)
{
    if (psz_buffer == NULL || i_buffer_size == 0 || psz_format == NULL || p_md == NULL || p_dict == NULL)
    {
        return 0;
    }
    
    size_t i_pos = 0;
    size_t i_fmt_len = strlen(psz_format);

    bool b_copy_token = false;
    bool b_is_truncated = false;

    char sz_token_buffer[64];
    size_t i_token_pos = 0;
    
    for (size_t i = 0; i < i_fmt_len; i++)
    {
        if (psz_format[i] == ' ' && (i > 0 && psz_format[i - 1] == ' '))
            continue;

        if (psz_format[i] == '\\' && i + 1 < i_fmt_len)
        {
            if (i_pos < i_buffer_size - 1)
                psz_buffer[i_pos++] = psz_format[++i];
            continue;
        }
        
        if (psz_format[i] == '$' && i + 1 < i_fmt_len && psz_format[i + 1] == '{')
        {
            i++; // Skip the '$' and '{'
            i_token_pos = 0;
            b_copy_token = true;
            continue;
        }
        
        if (b_copy_token)
        {
            if (psz_format[i] == '}')
            {
                b_copy_token = false;
                bool b_key_found = false;

                const char *psz_value = (const char *)vlc_dictionary_value_for_key(p_dict, sz_token_buffer);
                if (psz_value)
                {
                    int i_written = snprintf(psz_buffer + i_pos, i_buffer_size - i_pos, "%s", psz_value);
                    if (i_written != -1)
                    {
                        if ((size_t)i_written >= i_buffer_size - i_pos)
                        {
                            b_is_truncated = true;
                            break;
                        }
                        else
                        {
                            i_pos += i_written;
                            b_key_found = i_written > 0;
                        }
                    }
                }

                if (!b_key_found)
                {
                    while (i_pos > 0 && (isspace((unsigned char)psz_buffer[i_pos - 1]) || 
                        is_separator(psz_buffer[i_pos - 1])))
                        i_pos--;

                    psz_buffer[i_pos] = '\0';
                }
            }
            else
            {
                if (i_token_pos < sizeof(sz_token_buffer) - 1)
                {
                    sz_token_buffer[i_token_pos++] = psz_format[i];
                    sz_token_buffer[i_token_pos] = '\0';
                }
                else
                {
                    // Invalid token because it is too long
                    while(i < i_fmt_len && psz_format[i] != '}')
                        i++;

                    b_copy_token = false;
                    sz_token_buffer[0] = '\0';
                }
            }
        }
        else
        {
            if (i_pos < i_buffer_size - 1)
            {
                psz_buffer[i_pos++] = psz_format[i];
                psz_buffer[i_pos] = '\0';
            }
            else
            {
                b_is_truncated = true;
                break;
            }
        }
    }

    if (i_pos > 0)
    {
        if (b_is_truncated)
        {
            i_pos = i_buffer_size - 1;
            psz_buffer[i_pos] = '\0';

            size_t temp_i = i_pos;
            while (temp_i > 0 && (psz_buffer[temp_i - 1] & 0xC0) == 0x80)
                temp_i--;
            
            if (temp_i > 0 && (psz_buffer[temp_i - 1] & 0x80))
            {
                int i_expected;
                unsigned char c = (unsigned char)psz_buffer[temp_i - 1];
                if ((c & 0xE0) == 0xC0) i_expected = 2;
                else if ((c & 0xF0) == 0xE0) i_expected = 3;
                else if ((c & 0xF8) == 0xF0) i_expected = 4;
                else i_expected = 1;

                if (i_pos - (temp_i - 1) < (size_t)i_expected)
                    i_pos = temp_i - 1;
            }
        }

        trim_string(psz_buffer);
        i_pos = strlen(psz_buffer);

        if (b_is_truncated)
        {
            i_pos = i_pos >= 3 ? i_pos - 3 : 0;
            
            psz_buffer[i_pos++] = '.';
            psz_buffer[i_pos++] = '.';
            psz_buffer[i_pos++] = '.';
            psz_buffer[i_pos] = '\0';
        }
    }

    return i_pos;
}