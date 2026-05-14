/*****************************************************************************
 * cover_api.c: Discord Rich Presence plugin for VLC
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

#include "cover_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct MemoryStruct
{
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL)
        return 0;

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

static int ProgressCallback(void *clientp, curl_off_t dltotal, curl_off_t dlnow,
                            curl_off_t ultotal, curl_off_t ulnow)
{
    (void)dltotal;
    (void)dlnow;
    (void)ultotal;
    (void)ulnow;
    const bool *pb_abort = (const bool *)clientp;
    
    return pb_abort && *pb_abort;
}

static char *parse_json_field(const char *json, const char *field)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":", field);

    const char *start = strstr(json, search);
    if (!start)
        return NULL;

    start = strchr(start + strlen(search), '"');
    if (!start)
        return NULL;
    start++;

    const char *end = strchr(start, '"');
    if (!end)
        return NULL;

    size_t len = end - start;
    char *value = malloc(len + 1);
    if (value)
    {
        memcpy(value, start, len);
        value[len] = '\0';
    }
    return value;
}

bool cover_api_get_response(intf_thread_t *p_intf, const char *psz_hash, char **ppsz_url)
{
    if (!psz_hash || !ppsz_url)
        return false;

    CURL *curl;
    CURLcode res;
    bool success = false;
    struct MemoryStruct chunk = {.memory = malloc(1), .size = 0};

    char url[512];
    snprintf(url, sizeof(url), COVER_API_SERVER_BASE_URL "/cover?hash=%s", psz_hash);

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1000L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);

        res = curl_easy_perform(curl);

        if (res == CURLE_OK)
        {
            char *status = parse_json_field(chunk.memory, "status");
            if (status)
            {
                if (strcmp(status, "ok") == 0)
                {
                    *ppsz_url = parse_json_field(chunk.memory, "message");
                    if (*ppsz_url)
                        success = true;
                }
                else
                {
                    char *msg = parse_json_field(chunk.memory, "message");
                    if (msg && strcmp(msg, MSG_ERROR_IMAGE_NOT_FOUND) == 0)
                    {
                        *ppsz_url = msg;
                        success = true;
                    }
                    else
                    {
                        msg_Err(p_intf, "API Error: %s\n", chunk.memory);
                        free(msg);
                    }
                }
                free(status);
            }
            else
                msg_Dbg(p_intf, "API Response: %s\n", chunk.memory);
        }
        else
            msg_Dbg(p_intf, "HTTP Request failed: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curl);
    }

    free(chunk.memory);
    return success;
}

bool cover_api_upload_binary(intf_thread_t *p_intf, const char *psz_hash, 
                             const void *p_data,
                             uint64_t i_size,
                             const bool *pb_abort)
{
    if (!psz_hash || !p_data || i_size == 0)
        return false;

    CURL *curl;
    CURLcode res;
    bool success = false;
    struct MemoryStruct chunk = {.memory = malloc(1), .size = 0};

    char url[512];
    snprintf(url, sizeof(url), COVER_API_SERVER_BASE_URL "/cover/upload?hash=%s",
             psz_hash);

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, p_data);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)i_size);
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, (void *)pb_abort);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        res = curl_easy_perform(curl);

        if (res == CURLE_OK)
        {
            char *status = parse_json_field(chunk.memory, "status");
            if (status)
            {
                if (strcmp(status, "ok") == 0)
                    success = true;
                else
                    msg_Err(p_intf, "API Error: %s\n", chunk.memory);
                free(status);
            }
            else
                msg_Dbg(p_intf, "API Response: %s\n", chunk.memory);
        }
        else
            msg_Dbg(p_intf, "HTTP Request failed: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curl);
    }

    free(chunk.memory);
    return success;
}
