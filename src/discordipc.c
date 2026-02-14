/*****************************************************************************
 * discordipc.c: Discord Rich Presence plugin for VLC
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

#include "discordipc.h"

#include <stdlib.h>
#include <vlc_rand.h>

#define MAX_PIPE_ATTEMPTS 10

#if defined(_WIN32)

#include <windows.h>

typedef HANDLE pipe_t;
#define INVALID_PIPE INVALID_HANDLE_VALUE
#define get_pid() GetCurrentProcessId()

#elif defined(__linux__) || defined(__APPLE__)

#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

typedef int pipe_t;
#define INVALID_PIPE -1
#define get_pid() getpid()

#endif // defined(_WIN32) || defined(__linux__) || defined(__APPLE__)

#define PIPE_WRITE_TIMEOUT_MS 2000
#define PIPE_READ_TIMEOUT_MS 3000
#define MAX_MESSAGE_SIZE 16384

/**
 * @struct vlc_discord_ipc_data_t
 * @brief Internal private data for the Discord IPC instance.
 */
typedef struct
{
    intf_thread_t *p_intf;      /**< Pointer to VLC interface for logging */
    bool           b_connected; /**< Connection status flag */
    pipe_t         handle;      /**< OS-specific pipe/socket handle */
    vlc_mutex_t    lock;        /**< Mutex to ensure thread-safe IPC access */
    DiscordIPCException pf_err; /**< Callback for internal error reporting */
} vlc_discord_ipc_data_t;

/**
 * @brief Discord IPC packet header.
 * * This structure matches the binary format required by Discord's IPC 
 * handshake and frame transmission.
 */
typedef struct
{
    uint32_t i_opcode; /**< Operation code (0=Handshake, 1=Frame, 2=Close) */
    uint32_t i_length; /**< Length of the following JSON payload */
} vlc_discord_ipc_header_t;

/**
 * @brief Generates a pseudo-random nonce for Discord JSON requests.
 * Uses vlc_mrand48 for cross-platform compliant randomness.
 */
static void GenerateNonce(char *psz_dest, size_t i_size)
{
    if (psz_dest && i_size > 0)
    {
        uint32_t i_nonce = ((uint32_t)vlc_mrand48() % 900000) + 100000;
        snprintf(psz_dest, i_size, "%u", i_nonce);
    }
}

/**
 * @brief Escapes characters for JSON compliance.
 * Prevents injection/malformed JSON when track titles contain quotes or backslashes.
 */
static void JsonEscape(char *psz_dest, const char *psz_src, size_t i_max)
{
    size_t j = 0;
    for (size_t i = 0; psz_src[i] != '\0' && j < i_max - 2; i++)
    {
        if (psz_src[i] == '\"' || psz_src[i] == '\\')
        {
            psz_dest[j++] = '\\';
        }
        psz_dest[j++] = psz_src[i];
    }
    psz_dest[j] = '\0';
}

/**
 * @brief Thread-safe write operation with timeout.
 */
static bool WriteAll(vlc_discord_ipc_data_t *p_sys, const void *p_buffer, size_t i_size, bool *bp_errpipe)
{
#ifdef _WIN32
	OVERLAPPED ov = {.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)};
	if (!ov.hEvent)
	{
		return false;
	}

	DWORD i_written = 0;
	bool b_ok = false;
	if (!WriteFile(p_sys->handle, p_buffer, (DWORD)i_size, &i_written, &ov))
	{
		DWORD last_error = GetLastError();
		switch (last_error)
		{
		case ERROR_BROKEN_PIPE:
		case ERROR_NO_DATA:
		case ERROR_PIPE_NOT_CONNECTED:
			{
				if (bp_errpipe) *bp_errpipe = true;
				goto cleanup;
			}
		case ERROR_IO_PENDING:
			{
				if (WaitForSingleObject(ov.hEvent, PIPE_WRITE_TIMEOUT_MS) == WAIT_OBJECT_0)
				{
					GetOverlappedResult(p_sys->handle, &ov, &i_written, FALSE);
					b_ok = (i_written == i_size);
				}
				else
				{
					CancelIo(p_sys->handle);
				}
			}
			break;
		default:
			break;
		}
	}
	else
	{
		b_ok = (i_written == i_size);
	}

cleanup:
	CloseHandle(ov.hEvent);
	return b_ok;
#else
	
	size_t i_sent = 0;
    const char *p_ptr = (const char *)p_buffer;

    while (i_sent < i_size)
    {
        struct pollfd pfd = {.fd = p_sys->handle, .events = POLLOUT};
        int i_ret = poll(&pfd, 1, PIPE_WRITE_TIMEOUT_MS);
        if (i_ret <= 0) return false;

		if (pfd.revents & (POLLHUP | POLLERR)) 
		{
			if (bp_errpipe) *bp_errpipe = true; 
			return false;
		}

        ssize_t i_bytes = send(p_sys->handle, p_ptr + i_sent, i_size - i_sent, MSG_NOSIGNAL);
        if (i_bytes <= 0)
		{
			if (errno == EPIPE && bp_errpipe) *bp_errpipe = true;
			return false;
		}
        
        i_sent += (size_t)i_bytes;
    }
    return true;
#endif
}

/**
 * @brief Thread-safe read operation with timeout.
 */
static bool ReadAll(vlc_discord_ipc_data_t *p_sys, void *p_buffer, size_t i_size, bool *bp_errpipe)
{
#ifdef _WIN32
	OVERLAPPED ov = {.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL)};
	if (!ov.hEvent)
	{
		return false;
	}

	DWORD read = 0;
	bool b_ok = false;
	if (!ReadFile(p_sys->handle, p_buffer, (DWORD)i_size, &read, &ov))
	{
		DWORD last_error = GetLastError();
		switch (last_error)
		{
		case ERROR_BROKEN_PIPE:
		case ERROR_PIPE_NOT_CONNECTED:
			{
				if (bp_errpipe) *bp_errpipe = true;
				goto cleanup;
			}
		case ERROR_IO_PENDING:
			{
				if (WaitForSingleObject(ov.hEvent, PIPE_READ_TIMEOUT_MS) == WAIT_OBJECT_0)
				{
					GetOverlappedResult(p_sys->handle, &ov, &read, FALSE);
					b_ok = (read == i_size);
				}
				else
				{
					CancelIo(p_sys->handle);
				}
			}
			break;
		default:
			break;
		}
	}
	else
	{
		b_ok = (read == i_size);
	}

cleanup:
	CloseHandle(ov.hEvent);
	return b_ok;
#else

	size_t i_received = 0;
    while (i_received < i_size)
    {
        struct pollfd pfd = {.fd = p_sys->handle, .events = POLLIN};
        if (poll(&pfd, 1, PIPE_READ_TIMEOUT_MS) <= 0) return false;

		if (pfd.revents & (POLLHUP | POLLERR)) 
		{ 
			if (bp_errpipe) *bp_errpipe = true;
			return false;
		}

        ssize_t i_bytes = recv(p_sys->handle, (char*)p_buffer + i_received, i_size - i_received, 0);
        if (i_bytes == 0)
		{
			if (bp_errpipe) *bp_errpipe = true;
			return false;
		}

		if (i_bytes < 0)
		{
			if (errno == EINTR) continue;
			if (errno == EAGAIN || errno == EWOULDBLOCK) 
				return true;
			if (errno == ECONNRESET && bp_errpipe) *bp_errpipe = true;
			return false;
		}
		
        i_received += i_bytes;
    }
    return true;
#endif
}

/**
 * @brief Sends a synchronous message to Discord and validates the response.
 */
static bool SendDiscordMessageSync(vlc_discord_ipc_data_t *p_sys, uint32_t i_opcode, const char *psz_handshake, bool *bp_errpipe)
{
	if (p_sys->handle == INVALID_PIPE)
	{
		if (p_sys->pf_err)
			p_sys->pf_err(p_sys->p_intf, "Pipe is invalid or disconnected.");
		return false;
	}

	size_t json_len = strlen(psz_handshake);
	if (json_len > MAX_MESSAGE_SIZE)
	{
		if (p_sys->pf_err)
			p_sys->pf_err(p_sys->p_intf, "Message size exceeds maximum limit.");
		return false;
	}

	vlc_discord_ipc_header_t header = {.i_opcode = i_opcode, .i_length = (int32_t)json_len};
	if (!WriteAll(p_sys, &header, sizeof(header), bp_errpipe))
	{
		if (p_sys->pf_err)
			p_sys->pf_err(p_sys->p_intf, "Failed to write header to Discord pipe.");
		return false;
	}

	if (!WriteAll(p_sys, psz_handshake, json_len, bp_errpipe))
	{
		if (p_sys->pf_err)
			p_sys->pf_err(p_sys->p_intf, "Failed to write JSON payload to Discord pipe.");
		return false;
	}

	vlc_discord_ipc_header_t resp_header;
	if (!ReadAll(p_sys, &resp_header, sizeof(resp_header), bp_errpipe))
	{
		if (p_sys->pf_err)
			p_sys->pf_err(p_sys->p_intf, "Failed to read response header (Timeout or disconnected).");
		return false;
	}

	if (resp_header.i_length > MAX_MESSAGE_SIZE)
	{
		if (p_sys->pf_err)
			p_sys->pf_err(p_sys->p_intf, "Discord response is too large.");
		return false;
	}

	if (resp_header.i_length > 0)
	{
		char *response = malloc(resp_header.i_length + 1);
		if (!response)
		{
			return false;
		}

		if (!ReadAll(p_sys, response, resp_header.i_length, bp_errpipe))
		{
			if (p_sys->pf_err)
				p_sys->pf_err(p_sys->p_intf, "Failed to read response body.");
			free(response);
			return false;
		}
		response[resp_header.i_length] = '\0';

		bool b_success = (strstr(response, "\"evt\":\"READY\"") ||
						strstr(response, "\"cmd\":\"SET_ACTIVITY\"") ||
						strstr(response, "\"code\":0"));

		if (!b_success)
		{
			char *sz_msg_pos = strstr(response, "\"message\":\"");
			if (sz_msg_pos && p_sys->pf_err)
			{
				sz_msg_pos += 11; // Skip "\"message\":\""
				char *sz_end_quote = strchr(sz_msg_pos, '\"');
				if (sz_end_quote)
				{
					*sz_end_quote = '\0'; // Null terminate at the closing quote
					p_sys->pf_err(p_sys->p_intf, sz_msg_pos);
				}
				else
				{
					p_sys->pf_err(p_sys->p_intf, "Unknown Discord error occurred.");
				}
			}
			else if (p_sys->pf_err)
			{
				p_sys->pf_err(p_sys->p_intf, "Unrecognized Discord response or protocol error.");
			}
		}

		free(response);
		
		return b_success;
	}

	return true;
}

static bool Impl_Close(vlc_discord_ipc_t *p_self)
{
	if (!p_self || !p_self->p_sys)
		return false;
	vlc_discord_ipc_data_t *p_sys = (vlc_discord_ipc_data_t *)p_self->p_sys;

	vlc_mutex_lock(&p_sys->lock);

	if (p_sys->handle == INVALID_PIPE)
	{
		vlc_mutex_unlock(&p_sys->lock);
		return true;
	}

	char psz_nonce[16];
	char psz_clear_activity[512];

	GenerateNonce(psz_nonce, sizeof(psz_nonce));

	snprintf(psz_clear_activity, sizeof(psz_clear_activity),
			 "{\"cmd\":\"SET_ACTIVITY\",\"args\":{\"pid\":%lu,\"activity\":null},\"nonce\":\"%s\"}",
			 (unsigned long)get_pid(),
			 psz_nonce);

	SendDiscordMessageSync(p_sys, 1, psz_clear_activity, NULL);

#if defined(_WIN32)
	CloseHandle(p_sys->handle);
#elif defined(__linux__) || defined(__APPLE__)
	close(p_sys->handle);
#else
	#error “Platform not supported for this Discord plugin”
#endif // defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
	p_sys->handle = INVALID_PIPE;
	p_sys->b_connected = false;

	vlc_mutex_unlock(&p_sys->lock);
	
	return true;
}

static bool Impl_Destroy(vlc_discord_ipc_t *p_self)
{
	if (!p_self || !p_self->p_sys)
		return false;
	vlc_discord_ipc_data_t *p_sys = (vlc_discord_ipc_data_t *)p_self->p_sys;

	vlc_mutex_destroy(&p_sys->lock);

	free(p_sys);
	p_self->p_sys = NULL;
	
	return true;
}

static bool Impl_SetPresence(vlc_discord_ipc_t *p_self, discord_presence_t presence)
{
	if (!p_self || !p_self->p_sys)
		return false;
	vlc_discord_ipc_data_t *p_sys = (vlc_discord_ipc_data_t *)p_self->p_sys;

	vlc_mutex_lock(&p_sys->lock);

	discord_presence_t* dp_presence = &presence;

	if (p_sys->handle == INVALID_PIPE)
	{
		vlc_mutex_unlock(&p_sys->lock);
		return false;
	}

	char s_state[256], s_details[256], s_l_text[256], s_s_text[256];
	JsonEscape(s_state, dp_presence->sz_state, sizeof(s_state));
	JsonEscape(s_details, dp_presence->sz_details, sizeof(s_details));
	JsonEscape(s_l_text, dp_presence->sz_large_text, sizeof(s_l_text));
	JsonEscape(s_s_text, dp_presence->sz_small_text, sizeof(s_s_text));

	char psz_nonce[16];
	GenerateNonce(psz_nonce, sizeof(psz_nonce));

	char *psz_json = malloc(2048);
	if (!psz_json)
	{
		vlc_mutex_unlock(&p_sys->lock);
		return false;
	}

	int offset = snprintf(psz_json, 2048,
						  "{\"cmd\":\"SET_ACTIVITY\",\"args\":{\"pid\":%lu,\"activity\":{",
						  (unsigned long)get_pid());

	bool b_need_comma = false;

	if (strlen(s_state) > 0)
	{
		offset += snprintf(psz_json + offset, 2048 - offset, "\"state\":\"%s\"", s_state);
		b_need_comma = true;
	}

	if (strlen(s_details) > 0)
	{
		offset += snprintf(psz_json + offset, 2048 - offset, "%s\"details\":\"%s\"", b_need_comma ? "," : "", s_details);
		b_need_comma = true;
	}

	if (dp_presence->i_start_time > 0)
	{
		offset += snprintf(psz_json + offset, 2048 - offset, "%s\"timestamps\":{\"start\":%lld", b_need_comma ? "," : "", dp_presence->i_start_time);
		if (dp_presence->i_end_time > 0)
			offset += snprintf(psz_json + offset, 2048 - offset, ",\"end\":%lld", dp_presence->i_end_time);
		offset += snprintf(psz_json + offset, 2048 - offset, "}");
		b_need_comma = true;
	}

	if (dp_presence->sz_large_image[0] || dp_presence->sz_small_image[0])
	{
		offset += snprintf(psz_json + offset, 2048 - offset, "%s\"assets\":{", b_need_comma ? "," : "");
		bool hasAsset = false;
		if (dp_presence->sz_large_image[0])
		{
			offset += snprintf(psz_json + offset, 2048 - offset, "\"large_image\":\"%s\"", dp_presence->sz_large_image);
			hasAsset = true;
		}
		if (s_l_text[0])
		{
			offset += snprintf(psz_json + offset, 2048 - offset, "%s\"large_text\":\"%s\"", hasAsset ? "," : "", s_l_text);
			hasAsset = true;
		}
		if (dp_presence->sz_small_image[0])
		{
			offset += snprintf(psz_json + offset, 2048 - offset, "%s\"small_image\":\"%s\"", hasAsset ? "," : "", dp_presence->sz_small_image);
			hasAsset = true;
		}
		if (s_s_text[0])
		{
			offset += snprintf(psz_json + offset, 2048 - offset, "%s\"small_text\":\"%s\"", hasAsset ? "," : "", s_s_text);
		}
		offset += snprintf(psz_json + offset, 2048 - offset, "}");
		b_need_comma = true;
	}

	snprintf(psz_json + offset, 2048 - offset, "}},\"nonce\":\"%s\"}", psz_nonce);

	bool b_errpipe = false;
	bool b_result = SendDiscordMessageSync(p_sys, 1, psz_json, &b_errpipe);

	if (b_errpipe)
	{
#if defined(_WIN32)
		CloseHandle(p_sys->handle);
#elif defined(__linux__) || defined(__APPLE__)
		close(p_sys->handle);
#else
	#error “Platform not supported for this Discord plugin”
#endif // defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
		p_sys->handle = INVALID_PIPE;
		p_sys->b_connected = false;
	}
	
	free(psz_json);

	vlc_mutex_unlock(&p_sys->lock);

	return b_result;
}

static bool Impl_Connect(vlc_discord_ipc_t *p_self, uint64_t id)
{
	if (!p_self || !p_self->p_sys)
		return false;
	vlc_discord_ipc_data_t *p_sys = (vlc_discord_ipc_data_t *)p_self->p_sys;

	vlc_mutex_lock(&p_sys->lock);

	char psz_handshake[256];
	snprintf(psz_handshake, sizeof(psz_handshake), "{\"v\":1,\"client_id\":\"%llu\"}", id);

#if defined(_WIN32)

	char psz_pipe_name[128];
	for (int i = 0; i < MAX_PIPE_ATTEMPTS; i++)
	{
		snprintf(psz_pipe_name, sizeof(psz_pipe_name), "\\\\.\\pipe\\discord-ipc-%d", i);
		if (WaitNamedPipeA(psz_pipe_name, 100))
		{
			p_sys->handle = CreateFileA(psz_pipe_name, GENERIC_READ | GENERIC_WRITE, 0, NULL,
									  OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
			if (p_sys->handle != INVALID_PIPE)
			{
				if (SendDiscordMessageSync(p_sys, 0, psz_handshake, NULL))
				{
					p_sys->b_connected = true;
					vlc_mutex_unlock(&p_sys->lock);
					return true;
				}
				CloseHandle(p_sys->handle);
				p_sys->handle = INVALID_PIPE;
			}
		}
	}
	
#elif defined(__linux__) || defined(__APPLE__)
	char psz_socket_path[128];
	const char *psz_temp_path = getenv("XDG_RUNTIME_DIR");
	const char* psz_fallback_path = "/tmp";

	if (!psz_temp_path)
		psz_temp_path = psz_fallback_path;

	for (int i = 0; i < MAX_PIPE_ATTEMPTS; i++)
	{
		p_sys->handle = socket(AF_UNIX, SOCK_STREAM, 0);
		if (p_sys->handle == INVALID_PIPE)
			continue;

		struct sockaddr_un addr;
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		snprintf(addr.sun_path, sizeof(addr.sun_path), "%s/discord-ipc-%d", psz_temp_path, i);

		if (connect(p_sys->handle, (struct sockaddr *)&addr, sizeof(addr)) == 0)
		{
			if (SendDiscordMessageSync(p_sys, 0, psz_handshake, NULL))
			{
				p_sys->b_connected = true;
				vlc_mutex_unlock(&p_sys->lock);
				return true;
			}
		}

		if (psz_temp_path != psz_fallback_path)
		{
			snprintf(addr.sun_path, sizeof(addr.sun_path), "%s/discord-ipc-%d", psz_fallback_path, i);
            if (connect(p_sys->handle, (struct sockaddr *)&addr, sizeof(addr)) == 0)
            {
                if (SendDiscordMessageSync(p_sys, 0, psz_handshake, NULL))
                {
                    p_sys->b_connected = true;
                    vlc_mutex_unlock(&p_sys->lock);
                    return true;
                }
            }
		}

		close(p_sys->handle);
	}
	
#else
	#error “Platform not supported for this Discord plugin”
#endif // defined(_WIN32) || defined(__linux__) || defined(__APPLE__)

	if (p_sys->pf_err)
		p_sys->pf_err(p_sys->p_intf, "Could not connect to Discord. Is Discord running?");

	vlc_mutex_unlock(&p_sys->lock);

	return false;
}

static bool Impl_IsConnected(const vlc_discord_ipc_t *p_self)
{
	if (!p_self || !p_self->p_sys)
		return false;
	vlc_discord_ipc_data_t *p_sys = (vlc_discord_ipc_data_t *)p_self->p_sys;

	return p_sys->b_connected;
}

bool DiscordRPC_CreateIPC(vlc_discord_ipc_t *p_ipc, intf_thread_t *p_intf, DiscordIPCException pf_err)
{
	if (!p_ipc || !p_intf)
		return false;

	p_ipc->pf_close = Impl_Close;
	p_ipc->pf_connect = Impl_Connect;
	p_ipc->pf_is_connected = Impl_IsConnected;
	p_ipc->pf_set_presence = Impl_SetPresence;
	p_ipc->pf_destroy = Impl_Destroy;

	vlc_discord_ipc_data_t *p_sys = (vlc_discord_ipc_data_t *)calloc(1, sizeof(vlc_discord_ipc_data_t));
	if (!p_sys)
	{
		return false;
	}

	p_sys->p_intf = p_intf;
	p_sys->pf_err = pf_err;
	p_sys->handle = INVALID_PIPE;

	vlc_mutex_init(&p_sys->lock);

	p_ipc->p_sys = p_sys;

	return true;
}