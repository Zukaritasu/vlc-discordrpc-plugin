/*****************************************************************************
 * cover_cache.h: Discord Rich Presence plugin for VLC
 *****************************************************************************
 * Copyright (C) 2026 Zukaritasu
 *
 * Authors: Victor Barrientos <victorbarrientos.dev@gmail.com>
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

#ifndef COVER_CACHE_H
#define COVER_CACHE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct
{
	char *psz_url;		/* cover url */
	char *psz_filepath; /* Absolute path */
	char *psz_uri;		/* URI filepath */
	char *psz_hash;		/* hash from file */
} cover_data_t;

typedef struct cover_cache_t cover_cache_t;

/**
 * @brief Allocates a new cover cache in the heap.
 *
 * @return The cover cache.
 */
cover_cache_t *DiscordRPC_CreateCoverCache(void);

/**
 * @brief Returns the size of the specified cover cache.
 *
 * @param p_cache The cover cache.
 * @return The number of covers inside of the cover cache.
 */
size_t DiscordRPC_GetCoverCacheSize(const cover_cache_t *p_cache);

/**
 * @brief Deallocates a cover cache from the heap. Also deallocates all the nodes into the cache.
 *
 * @param p_cache The memory address of the cover cache that you want to free.
 */
void DiscordRPC_FreeCoverCache(cover_cache_t **p_cache);

/**
 * @brief Searches inside a cover cache for a cover data with the specified URI.
 *
 * @param p_cache The cover cache.
 * @param pcsz_uri The URI you want to search for the belonging cover data.
 * @return true if the cover data exists, false otherwise.
 */
bool DiscordRPC_ExistsCoverData(const cover_cache_t *p_cache, const char *pcsz_uri);

/**
 * @brief Obtains the cover data belonging to the specified URI.
 *
 * @param p_cache The cover cache.
 * @param pcsz_uri The URI of the belonging cover data.
 * @return A pointer to the cover data if it was found, NULL otherwise.
 */
cover_data_t *DiscordRPC_GetCoverData(const cover_cache_t *p_cache, const char *pcsz_uri);

/**
 * @brief Adds a cover data inside the specified cover cache.
 *
 * @param p_cache The cover cache.
 * @param p_cover A pointer to the cover data you want to add inside the cover cache.
 * @return true if the cover data was added successfully, false otherwise.
 */
bool DiscordRPC_AddCoverData(cover_cache_t *p_cache, const cover_data_t *p_cover);

/**
 * @brief Delete from the cover cache the cover data belonging to the specified URI.
 *
 * @param p_cache The cover cache.
 * @param pcsz_uri The URI of the cover data you want to delete.
 * @return true if the cover data was deleted successfully, false otherwise.
 */
bool DiscordRPC_RemoveCoverData(cover_cache_t *p_cache, const char *pcsz_uri);

#endif // COVER_CACHE_H