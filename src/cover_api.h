/*****************************************************************************
 * cover_api.h: Discord Rich Presence plugin for VLC
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

#ifndef COVER_API_H
#define COVER_API_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_interface.h>

/* API server url */
#define COVER_API_SERVER_BASE_URL        "?API_URL?"

/* Error messages */
#define MSG_ERROR_HASH_MISSING           "hash_missing"
#define MSG_ERROR_EMPTY_BODY             "empty_body"
#define MSG_ERROR_IMAGE_NOT_FOUND        "image_not_found"
#define MSG_ERROR_INVALID_ARGUMENTS      "invalid_arguments"

/* Success messages */
#define MSG_SUCCESS_IMAGE_ALREADY_EXISTS "image_already_exists"
#define MSG_SUCCESS_IMAGE_UPLOADED       "image_uploaded"

/**
 * @brief Performs a GET request to the server to obtain the cover URL.
 *
 * @param psz_hash The image hash.
 * @param ppsz_url Pointer where the URL will be stored (must be freed with free()).
 * @return true if the URL was obtained successfully, false otherwise.
 */
bool cover_api_get_response(intf_thread_t *p_intf,
                            const char *psz_hash,
                            char **ppsz_url);

/**
 * @brief Uploads a binary image to the server via a POST request.
 *
 * @param psz_hash The image hash.
 * @param p_data The binary data of the image.
 * @param i_size The size of the data in bytes.
 * @param pb_abort Pointer to a boolean flag to abort the upload.
 * @return true if the upload was successful, false otherwise.
 */
bool cover_api_upload_binary(intf_thread_t *p_intf, 
                             const char *psz_hash,
                             const void *p_data,
                             uint64_t i_size,
                             const bool *pb_abort);

#endif // COVER_API_H
