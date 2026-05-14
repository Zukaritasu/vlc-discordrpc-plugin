/*****************************************************************************
 * cover.c: Discord Rich Presence plugin for VLC
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

#include "cover.h"

#include <vlc_common.h>
#include <vlc_threads.h>
#include <vlc_url.h>
#include <vlc_arrays.h>
#include <vlc_fs.h>
#include <vlc_image.h>
#include <vlc_block.h>
#include <vlc_picture.h>

#include <stdint.h>
#include <sys/stat.h>

#include "cover_api.h"

#define MAX_COVER_URL_SIZE 512
#define MAX_FILE_HASH_SIZE 32

#define MURMURHASH3_SEED   42

/**
 *
 */
typedef struct
{
    char *psz_url;      /* cover url */
    char *psz_filepath; /* Absolute path */
    char *psz_uri;      /* URI filepath */
    char *psz_hash;     /* hash from file */
} cover_data_t;

/**
 * @brief Wait event structure.
 */
typedef struct
{
    vlc_cond_t cond;  /* condition variable */
    bool b_triggered; /* event triggered flag */
} wait_event_t;

/**
 * @brief Plugin API response structure.
 */
typedef struct
{
    char *psz_message; /* API message */
    char *psz_status;  /* API status */
} plugin_api_response_t;

/**
 * @brief Cover handle structure.
 */
struct cover_handle
{
    bool b_running;          /* Plugin running flag */
    bool b_cancel;           /* Cancel flag */

    intf_thread_t *p_intf;   /* Interface thread */

    vlc_thread_t thread;     /* Thread */
    vlc_mutex_t lock;        /* Thread lock */
    wait_event_t event;      /* Event */

    vlc_dictionary_t covers; /* Dictionary of covers */

    char *psz_curr_uri;      /* Current URI */
};

/**
 * @brief Computes the 32-bit murmurhash3 hash of a block of data.
 *
 * @param key Pointer to the data to hash.
 * @param len Length of the data in bytes.
 * @param seed Seed value for the hash.
 * @return 32-bit murmurhash3 hash.
 */
static uint32_t murmurhash3_32(const void *key, size_t len, uint32_t seed)
{
    const uint8_t *data = (const uint8_t *)key;
    const int nblocks = len / 4;
    uint32_t h1 = seed;

    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    const uint32_t *blocks = (const uint32_t *)(data + nblocks * 4);

    for (int i = -nblocks; i; i++)
    {
        uint32_t k1 = blocks[i];

        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> 17);
        k1 *= c2;

        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> 19);
        h1 = h1 * 5 + 0xe6546b64;
    }

    const uint8_t *tail = (const uint8_t *)(data + nblocks * 4);
    uint32_t k1 = 0;

    switch (len & 3)
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
    case 3:
        k1 ^= tail[2] << 16;
    case 2:
        k1 ^= tail[1] << 8;
    case 1:
        k1 ^= tail[0];
        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> 17);
        k1 *= c2;
        h1 ^= k1;
#pragma GCC diagnostic pop
    };

    h1 ^= len;
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
}

static bool get_normalized_image(intf_thread_t *p_intf, cover_data_t* p_ref, block_t **p_block)
{
    image_handler_t *p_image_handler;
    picture_t*       p_pic = NULL;
    picture_t*       p_converted_pic = NULL;
    block_t*         p_converted_block = NULL;
    
    video_format_t   fmt_in;
    video_format_Init(&fmt_in, 0);

    video_format_t   fmt_out;
    video_format_Init(&fmt_out, VLC_CODEC_I420);

    p_image_handler = image_HandlerCreate(p_intf);
    if (!p_image_handler) 
        return false;

    p_pic = image_ReadUrl(p_image_handler, p_ref->psz_uri, &fmt_in, &fmt_out);
    if (!p_pic) 
    {
        msg_Dbg(p_intf, "Failed to read image from URI %s\n", p_ref->psz_uri);
        goto error;
    }

    if (p_pic->format.i_visible_width > p_pic->format.i_visible_height)
    {
        int offset = (p_pic->format.i_visible_width - p_pic->format.i_visible_height) / 2;
        p_pic->format.i_x_offset += offset;
        p_pic->format.i_visible_width = p_pic->format.i_visible_height;
    }
    else if (p_pic->format.i_visible_height > p_pic->format.i_visible_width)
    {
        int offset = (p_pic->format.i_visible_height - p_pic->format.i_visible_width) / 2;
        p_pic->format.i_y_offset += offset;
        p_pic->format.i_visible_height = p_pic->format.i_visible_width;
    }

    video_format_t fmt_resized;
    video_format_Init(&fmt_resized, VLC_CODEC_I420); 
    fmt_resized.i_width = 512;
    fmt_resized.i_height = 512;
    fmt_resized.i_visible_width = 512;
    fmt_resized.i_visible_height = 512;

    p_converted_pic = image_Convert(p_image_handler, p_pic, &p_pic->format, &fmt_resized);
    if (!p_converted_pic)
    {
        msg_Dbg(p_intf, "Failed to resize image\n");
        goto error;
    }

    video_format_t fmt_jpeg;
    video_format_Init(&fmt_jpeg, VLC_CODEC_JPEG);
    fmt_jpeg.i_width = 512;
    fmt_jpeg.i_height = 512;
    fmt_jpeg.i_visible_width = 512;
    fmt_jpeg.i_visible_height = 512;

    p_converted_block = image_Write(p_image_handler, p_converted_pic, &fmt_resized, &fmt_jpeg);
    if (!p_converted_block)
    {
        msg_Dbg(p_intf, "Failed to encode image to JPEG\n");
        goto error;
    }

    *p_block = p_converted_block;

    picture_Release(p_pic);
    picture_Release(p_converted_pic);
    image_HandlerDelete(p_image_handler);

    return true;

error:
    if (p_pic)
        picture_Release(p_pic);
    if (p_converted_pic)
        picture_Release(p_converted_pic);

    image_HandlerDelete(p_image_handler);

    msg_Dbg(p_intf, "Failed to normalize image for URI %s\n", p_ref->psz_uri);

    return false;
}

/**
 * @brief Frees the cover data.
 *
 * @param p_cover_data Pointer to the cover data.
 * @param unused Unused parameter.
 */
static void free_cover(void *p_cover_data, void *unused)
{
    (void)unused;

    cover_data_t *p_cdata = p_cover_data;
    if (!p_cdata)
        return;

    free(p_cdata->psz_url);
    free(p_cdata->psz_filepath);
    free(p_cdata->psz_uri);
    free(p_cdata->psz_hash);
}

/**
 * @brief Gets the binary file data.
 *
 * @param filepath Path to the file.
 * @param p_buffer Pointer to the buffer to store the file data.
 * @param i_size Pointer to the size of the file data.
 * @return true if the file was read successfully, false otherwise.
 */
static bool get_binary_file(const char *filepath, void **p_buffer, uint64_t *i_size)
{
    struct stat st;
    if (vlc_stat(filepath, &st) || st.st_size == 0)
        return false;

    FILE *file = vlc_fopen(filepath, "rb");
    if (!file)
        return false;

    void *p_file_buf = malloc(st.st_size);
    if (!p_file_buf)
    {
        fclose(file);
        return false;
    }

    size_t i_bytes_readed = fread(p_file_buf, 1, st.st_size, file);
    fclose(file);

    bool b_ok = i_bytes_readed > 0;
    if (!b_ok)
        free(p_file_buf);
    else
    {
        *p_buffer = p_file_buf;
        *i_size = i_bytes_readed;
    }

    return b_ok;
}

/**
 * @brief Computes the hash of a file.
 *
 * @param filepath Path to the file.
 * @return Hash of the file.
 */
static char *get_file_hash(const char *filepath)
{
    void *p_file_buf = NULL;
    uint64_t i_size = 0;

    if (!get_binary_file(filepath, &p_file_buf, &i_size))
        return NULL;

    uint32_t hash = murmurhash3_32(p_file_buf, (size_t)i_size, MURMURHASH3_SEED);
    free(p_file_buf);

    char sz_hash[MAX_FILE_HASH_SIZE];
    snprintf(sz_hash, sizeof(sz_hash), "%08x", hash);

    return strdup(sz_hash);
}

/**
 * @brief Gets the cover data.
 *
 * @param p_handle Pointer to the cover handle.
 * @param p_cd Pointer to the cover data.
 * @return true if the cover data was retrieved successfully, false otherwise.
 */
static bool get_cover_data(cover_handle_t *p_handle, cover_data_t **p_cd)
{
    *p_cd = calloc(1, sizeof(cover_data_t));
    cover_data_t* p_ref = *p_cd;

    if (!p_ref)
        return false;

    vlc_mutex_lock(&p_handle->lock);
    p_ref->psz_uri = strdup(p_handle->psz_curr_uri);
    vlc_mutex_unlock(&p_handle->lock);

    if (!p_ref->psz_uri)
        goto out;

    p_ref->psz_filepath = vlc_uri2path(p_ref->psz_uri);
    if (!p_ref->psz_filepath)
        goto out;

    p_ref->psz_hash = get_file_hash(p_ref->psz_filepath);
    if (!p_ref->psz_hash)
        goto out;

    /* The API response may be the expected URL or an error message;
       therefore, an error message may indicate that the image was not
       found on the server.
    */
    char *psz_response = NULL;

    if (!cover_api_get_response(p_handle->p_intf, p_ref->psz_hash, &psz_response) || !psz_response)
        goto out;

    msg_Dbg(p_handle->p_intf, "API Response: %s\n", psz_response);

    if (strncmp(psz_response, "https", 5) == 0)
    {
        p_ref->psz_url = psz_response;
        return true;
    }

    bool b_not_found = strcmp(psz_response, MSG_ERROR_IMAGE_NOT_FOUND) == 0;
    free(psz_response);

    if (b_not_found)
    {
        block_t *p_converted_block = NULL;

        if (!get_normalized_image(p_handle->p_intf, p_ref, &p_converted_block))
            goto out;

        bool b_uploaded = cover_api_upload_binary(p_handle->p_intf, p_ref->psz_hash,
                                                  p_converted_block->p_buffer, 
                                                  (uint64_t)p_converted_block->i_buffer, 
                                                  &p_handle->b_cancel);
        block_Release(p_converted_block);

        if (b_uploaded)
        {
            /* Create URL for cover image
               We do this so that it works even if the server response does not contain the URL.
            */
            char sz_buffer[MAX_COVER_URL_SIZE];
            snprintf(sz_buffer, sizeof(sz_buffer), COVER_API_SERVER_BASE_URL "/static/%s",
                     p_ref->psz_hash);

            if ((p_ref->psz_url = strdup(sz_buffer)))
                return true;
        }
        else
        {
            msg_Dbg(p_handle->p_intf, "Failed to upload cover image for hash %s\n", 
                p_ref->psz_hash);
        }
    }

out:
    free(p_ref->psz_filepath);
    free(p_ref->psz_hash);
    free(p_ref->psz_uri);
    free(p_ref);

    *p_cd = NULL;

    return false;
}

static void *run(void *data)
{
    cover_handle_t *p_handle = data;
    while (true)
    {
        vlc_mutex_lock(&p_handle->lock);

        if (!p_handle->b_running)
        {
            vlc_mutex_unlock(&p_handle->lock);
            break;
        }

        /* wait for the signal to continue */
        while (p_handle->b_running && !p_handle->event.b_triggered)
            vlc_cond_wait(&p_handle->event.cond, &p_handle->lock);

        if (!p_handle->b_running)
        {
            vlc_mutex_unlock(&p_handle->lock);
            break;
        }

        p_handle->event.b_triggered = false;

        vlc_mutex_unlock(&p_handle->lock);

        cover_data_t *cover = NULL;
        if (get_cover_data(p_handle, &cover))
        {
            vlc_mutex_lock(&p_handle->lock);

            if (!vlc_dictionary_has_key(&p_handle->covers, cover->psz_uri))
                vlc_dictionary_insert(&p_handle->covers, cover->psz_uri, cover);
            else
            {
                free_cover(cover, NULL);
                free(cover);
            }

            vlc_mutex_unlock(&p_handle->lock);
        }
    }

    return NULL;
}

void DiscordRPC_GetCoverURL(cover_handle_t *p_handle, const char *psz_artworkurl, 
                            char *psz_url, size_t i_cbuf)
{
    if (!p_handle || !psz_artworkurl || psz_artworkurl[0] == '\0' ||
        !psz_url || i_cbuf == 0)
        return;

    vlc_mutex_lock(&p_handle->lock);

    psz_url[0] = '\0';

    if (!p_handle->b_running)
        goto unlock;

    const cover_data_t *p_cdata = vlc_dictionary_value_for_key(&p_handle->covers, psz_artworkurl);
    if (p_cdata)
    {
        snprintf(psz_url, i_cbuf, "%s", p_cdata->psz_url);
        goto unlock;
    }

    if (p_handle->psz_curr_uri && !strcmp(p_handle->psz_curr_uri, psz_artworkurl))
        goto unlock;

    /* set up the new path to access the cover URI */ 
    free(p_handle->psz_curr_uri);
    p_handle->psz_curr_uri = strdup(psz_artworkurl);
    if (!p_handle->psz_curr_uri)
        goto unlock;

    p_handle->event.b_triggered = true;
    vlc_cond_signal(&p_handle->event.cond);

unlock:
    vlc_mutex_unlock(&p_handle->lock);
}

cover_handle_t *DiscordRPC_CreateCoverHandle(intf_thread_t *p_intf)
{
    cover_handle_t *p_handle = calloc(1, sizeof(cover_handle_t));
    if (!p_handle)
    {
        return NULL;
    }

    vlc_dictionary_init(&p_handle->covers, 0);

    vlc_mutex_init(&p_handle->lock);
    vlc_cond_init(&p_handle->event.cond);

    p_handle->p_intf = p_intf;
    p_handle->b_running = true;
    
    if (vlc_clone(&p_handle->thread, run, p_handle, VLC_THREAD_PRIORITY_LOW))
    {
        vlc_cond_destroy(&p_handle->event.cond);
        vlc_mutex_destroy(&p_handle->lock);

        free(p_handle);
        p_handle = NULL;
    }

    return p_handle;
}

void DiscordRPC_CloseCoverHandle(cover_handle_t *p_handle)
{
    if (!p_handle)
        return;

    vlc_mutex_lock(&p_handle->lock);
    p_handle->b_running = false;
    p_handle->b_cancel = true;

    vlc_cond_signal(&p_handle->event.cond);
    vlc_mutex_unlock(&p_handle->lock);

    vlc_join(p_handle->thread, NULL);

    vlc_cond_destroy(&p_handle->event.cond);
    vlc_mutex_destroy(&p_handle->lock);

    /* free covers */
    vlc_dictionary_clear(&p_handle->covers, free_cover, NULL);

    /* others */
    free(p_handle->psz_curr_uri);
    free(p_handle);
}