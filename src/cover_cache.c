/*****************************************************************************
 * cover_cache.c: Discord Rich Presence plugin for VLC
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

#include "cover_cache.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct cover_node_t
{
	cover_data_t		 data;
	struct cover_node_t *next;
} cover_node_t;

typedef struct cover_cache_t
{
	cover_node_t *head;
	size_t		  size;
} cover_cache_t;

/*
Requerimientos:
- [x] Consultar si existe
- [x] Obtener cover
- [x] Agregar cover
- [x] Borrar cover
- [x] Borrar toda la lista

Clave: Cover URI (ruta local)
Valor: cover_data_t
*/

static cover_node_t *CreateCoverNode(const cover_data_t *data)
{
	cover_node_t *p_node = malloc(sizeof(cover_node_t));

	if (p_node == NULL)
		return NULL;

	p_node->data = *data;
	p_node->next = NULL;

	return p_node;
}

static bool AppendCoverNode(cover_cache_t *p_cache, cover_node_t *p_node)
{
	if (p_cache == NULL)
		return false;

	if (p_cache->head == NULL)
	{
		p_cache->head = p_node;
	}
	else
	{
		cover_node_t *p_actual_node = p_cache->head;

		while (p_actual_node->next != NULL)
		{
			p_actual_node = p_actual_node->next;
		}

		p_actual_node->next = p_node;
	}

	p_cache->size++;

	return true;
}

static void FreeCoverNode(cover_node_t *p_node) { free(p_node); }

cover_cache_t *DiscordRPC_CreateCoverCache(void)
{
	cover_cache_t *p_cache = malloc(sizeof(cover_cache_t));

	if (p_cache == NULL)
		return NULL;

	p_cache->head = NULL;
	p_cache->size = 0;

	return p_cache;
}

size_t DiscordRPC_GetCoverCacheSize(const cover_cache_t *p_cache)
{
	if (p_cache == NULL)
		return 0ULL;

	return p_cache->size;
}

void DiscordRPC_FreeCoverCache(cover_cache_t **p_cache)
{
	if ((*p_cache)->head != NULL)
	{
		cover_node_t *p_actual_node = (*p_cache)->head, *p_next_node = p_actual_node->next;

		/* Free all nodes from the cache before freeing the cache. */
		do
		{
			free(p_actual_node);

			p_actual_node = p_next_node;

			if (p_actual_node != NULL)
				p_next_node = p_actual_node->next;
		} while (p_actual_node != NULL);
	}

	free(*p_cache);
	*p_cache = NULL;
}

bool DiscordRPC_ExistsCoverData(const cover_cache_t *p_cache, const char *pcsz_uri)
{
	cover_node_t *p_node_target = p_cache->head;

	while (p_node_target != NULL)
	{
		if (p_node_target->data.psz_uri != NULL &&
			strcmp(p_node_target->data.psz_uri, pcsz_uri) == 0)
		{
			return true;
		}

		p_node_target = p_node_target->next;
	}

	return false;
}

cover_data_t *DiscordRPC_GetCoverData(const cover_cache_t *p_cache, const char *pcsz_uri)
{
	if (p_cache == NULL || pcsz_uri == NULL)
		return NULL;

	cover_node_t *p_node_target = p_cache->head;

	while (p_node_target != NULL)
	{
		if (p_node_target->data.psz_uri != NULL &&
			strcmp(p_node_target->data.psz_uri, pcsz_uri) == 0)
		{
			return &p_node_target->data;
		}

		p_node_target = p_node_target->next;
	}

	return NULL;
}

bool DiscordRPC_AddCoverData(cover_cache_t *p_cache, const cover_data_t *p_cover)
{
	if (p_cache == NULL)
		return false;

	cover_node_t *p_new_cover_node = CreateCoverNode(p_cover);

	if (p_new_cover_node == NULL)
		return false;

	return AppendCoverNode(p_cache, p_new_cover_node);
}

bool DiscordRPC_RemoveCoverData(cover_cache_t *p_cache, const char *pcsz_uri)
{
	if (p_cache->head == NULL)
		return false;

	cover_node_t *p_target_node = p_cache->head, *p_before_target_node = NULL;
	bool		  b_node_found = false;

	while (p_target_node != NULL)
	{
		if (p_target_node->data.psz_uri != NULL &&
			strcmp(p_target_node->data.psz_uri, pcsz_uri) == 0)
		{
			b_node_found = true;
			break;
		}

		p_before_target_node = p_target_node;
		p_target_node		 = p_target_node->next;
	}

	if (!b_node_found)
		return false;

	if (p_target_node == p_cache->head)
	{
		p_cache->head = p_target_node->next;
	}
	else
	{
		p_before_target_node->next = p_target_node->next;
	}

	FreeCoverNode(p_target_node);
	p_cache->size--;

	return true;
}
