/*****************************************************************************
 * path.h: Discord Rich Presence plugin for VLC
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

#ifndef PATH_H
#define PATH_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <wchar.h>

#define PLUGIN_FOLDER_NAME "vlcrc"
#define WIDE_IMPL(x)	   L##x
#define WIDE(x)			   WIDE_IMPL(x)

#ifdef _WIN32
#include <Windows.h>

#ifndef OS_SEPARATOR
#define OS_SEPARATOR "\\"
#endif

#ifndef PLUGIN_FOLDER_PATH
#define PLUGIN_FOLDER_PATH OS_SEPARATOR PLUGIN_FOLDER_NAME
#endif

// C:\Users\USER\AppData\Roaming\vlcrc
#ifndef PLUGIN_FOLDER_PATH_ALT
#define PLUGIN_FOLDER_PATH_ALT                                                                     \
	OS_SEPARATOR "AppData" OS_SEPARATOR "Roaming" OS_SEPARATOR PLUGIN_FOLDER_NAME
#endif
#else
#ifndef OS_SEPARATOR
#define OS_SEPARATOR "/"
#endif

#ifndef PLUGIN_FOLDER_PATH
#define PLUGIN_FOLDER_PATH OS_SEPARATOR PLUGIN_FOLDER_NAME
#endif

// $HOME/.config/vlc/vlcrc
#ifndef PLUGIN_FOLDER_PATH_ALT
#define PLUGIN_FOLDER_PATH_ALT                                                                     \
	OS_SEPARATOR ".config" OS_SEPARATOR "vlc" OS_SEPARATOR PLUGIN_FOLDER_NAME
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Obtain the absolute base directory where plugin directory should be depending on the
 * platform.
 *
 * @return If success, returns the pointer of the first character of the absolute base directory
 * (null-terminated), NULL otherwise.
 */
static inline char *get_base_plugin_directory(void)
{
	const char *p_initial_base_directory = NULL;
	char	   *p_result_base_directory	 = NULL;
	size_t		i_new_path_size			 = 0;
	bool		b_fallback				 = false;

#ifdef _WIN32
	p_initial_base_directory = getenv("APPDATA");
#else
	p_initial_base_directory = getenv("XDG_CONFIG_HOME");
#endif

	if (p_initial_base_directory == NULL)
	{
#ifdef _WIN32
		p_initial_base_directory = getenv("USERPROFILE");
#else
		p_initial_base_directory = getenv("HOME");
#endif
		b_fallback = true;
	}

	if (!p_initial_base_directory)
	{
		return NULL;
	}

	i_new_path_size = strlen(p_initial_base_directory);

	if (b_fallback)
	{
		i_new_path_size += strlen(PLUGIN_FOLDER_PATH_ALT);
	}
	else
	{
		i_new_path_size += strlen(PLUGIN_FOLDER_PATH);
	}

	p_result_base_directory =
		(char *)malloc(sizeof(char) * (i_new_path_size + 1)); // +1 for null character

	if (!p_result_base_directory)
	{
		return NULL;
	}

	strcpy(p_result_base_directory, p_initial_base_directory);

	if (b_fallback)
	{
		strcat(p_result_base_directory, PLUGIN_FOLDER_PATH_ALT);
	}
	else
	{
		strcat(p_result_base_directory, PLUGIN_FOLDER_PATH);
	}

	return p_result_base_directory;
}

/**
 * @brief Appends a variable number of `wchar_t *` arguments to the plugin base directory.
 * @warning 1. The function's behaviour is undefined if:
 * @warning - The `i_length` argument is greater than the amount of variadic arguments you provided.
 * @warning - Any variadic argument has a data type different from `wchar_t *`.
 * @note 1. If `i_length` is 0, the function will return NULL.
 *
 * @param p_path Pointer to the first character of the wide null-terminated string.
 * @param i_length The amount of variadic arguments.
 * @param ... Variable amount of pointer to wchar_t arguments with null character.
 * @return If success, returns a pointer to wchar_t of the resulting wide null-terminated string
 * path starting from the base plugin directory, NULL otherwise.
 */
static inline wchar_t *wide_append_to_path(const wchar_t *p_path, const size_t i_length, ...)
{
	if (!p_path || i_length == 0)
	{
		return NULL;
	}

	const wchar_t *next;
	wchar_t		  *p_wide_result_path = NULL;
	size_t		   i_result_size	  = 0;
	va_list		   args1, args2;

	va_start(args1, i_length);
	va_copy(args2, args1);

	i_result_size = wcslen(p_path);
	{
		int i = 0;
		do
		{
			next = va_arg(args1, const wchar_t *);

			if (next != NULL)
			{
				i_result_size += wcslen(next) + 1; // +1 for initial OS separator
			}
			i++;
		} while (i < i_length);
	}

	va_end(args1);

	p_wide_result_path =
		(wchar_t *)malloc(sizeof(wchar_t) * (i_result_size + 1)); // +1 for null character

	if (!p_wide_result_path)
	{
		return NULL;
	}

	wcscpy(p_wide_result_path, p_path);

	{
		int i = 0;
		do
		{
			next = va_arg(args2, const wchar_t *);
			if (next != NULL)
			{
				wcscat(p_wide_result_path, WIDE(OS_SEPARATOR));
				wcscat(p_wide_result_path, next);
			}
			i++;
		} while (i < i_length);
	}

	va_end(args2);

	return p_wide_result_path;
}

/**
 * @brief Appends a variable number of `char *` arguments to the plugin base directory.
 * @warning 1. The function's behaviour is undefined if:
 * @warning - The `i_length` argument is greater than the amount of variadic arguments you provided.
 * @warning - Any variadic argument has a data type different from `char *`.
 * @note 1. If `i_length` is 0, the function will return NULL.
 *
 * @param p_path Pointer to the first character of the null-terminated string.
 * @param i_length The amount of variadic arguments.
 * @param ... Variable amount of pointer to char arguments with null character.
 * @return If success, returns a pointer to char of the resulting null-terminated string path
 * starting from the base plugin directory, NULL otherwise.
 */
static inline char *multibyte_append_to_path(const char *p_path, const size_t i_length, ...)
{
	if (!p_path || i_length == 0)
	{
		return NULL;
	}

	const char *next;
	char	   *p_result_path = NULL;
	size_t		i_result_size = 0;
	va_list		args1, args2;

	va_start(args1, i_length);
	va_copy(args2, args1);

	i_result_size = strlen(p_path);
	{
		int i = 0;
		do
		{
			next = va_arg(args1, const char *);

			if (next != NULL)
			{
				i_result_size += strlen(next) + 1; // +1 for initial OS separator
			}
			i++;
		} while (i < i_length);
	}

	va_end(args1);

	p_result_path = (char *)malloc(sizeof(char) * (i_result_size + 1)); // +1 for null character

	if (!p_result_path)
	{
		return NULL;
	}

	strcpy(p_result_path, p_path);

	{
		int i = 0;
		do
		{
			next = va_arg(args2, const char *);
			if (next != NULL)
			{
				strcat(p_result_path, OS_SEPARATOR);
				strcat(p_result_path, next);
			}
			i++;
		} while (i < i_length);
	}

	va_end(args2);

	return p_result_path;
}

/**
 * @brief Appends a variable number of `wchar_t *` arguments to the plugin base directory.
 * @warning 1. The function's behaviour is undefined if:
 * @warning - The `i_length` argument is greater than the amount of variadic arguments you provided.
 * @warning - Any variadic argument has a data type different from `wchar_t *`.
 * @note 1. If `i_length` is 0, the return will be the absolute path of the base plugin directory,
 * which is the same as calling `get_base_plugin_directory()`. For that reason, if you're not
 * planning to add directories to the path, please use `get_base_plugin_directory()` instead.
 *
 * @param i_length The amount of variadic arguments.
 * @param ... Variable amount of pointer to wchar_t arguments with null character.
 * @return If success, returns a pointer to char of the resulting wide null-terminated string path
 * starting from the base plugin directory, NULL otherwise.
 */
static inline wchar_t *wide_append_to_plugin_path(const size_t i_length, ...)
{
	const wchar_t *next;
	char		  *p_base_directory		 = get_base_plugin_directory();
	wchar_t		  *p_wide_base_directory = NULL;
	wchar_t		  *p_wide_result_path	 = NULL;
	size_t		   i_result_size		 = 0;
	va_list		   args1, args2;

	if (!p_base_directory)
	{
		return NULL;
	}

	i_result_size = strlen(p_base_directory);

	p_wide_base_directory = (wchar_t *)malloc(sizeof(wchar_t) * (i_result_size + 1));

	if (!p_wide_base_directory)
	{
		return NULL;
	}

	mbstowcs(p_wide_base_directory, p_base_directory, i_result_size + 1);

	free(p_base_directory);

	if (i_length == 0)
	{
		return p_wide_base_directory;
	}

	va_start(args1, i_length);
	va_copy(args2, args1);

	{
		int i = 0;
		do
		{
			next = va_arg(args1, const wchar_t *);

			if (next != NULL)
			{
				i_result_size += wcslen(next) + 1; // +1 for initial OS separator
			}
			i++;
		} while (i < i_length);
	}

	va_end(args1);

	p_wide_result_path = (wchar_t *)malloc(sizeof(wchar_t) * (i_result_size + 1));

	if (!p_wide_result_path)
	{
		return NULL;
	}

	wcscpy(p_wide_result_path, p_wide_base_directory);

	{
		int i = 0;
		do
		{
			next = va_arg(args2, const wchar_t *);
			if (next != NULL)
			{
				wcscat(p_wide_result_path, WIDE(OS_SEPARATOR));
				wcscat(p_wide_result_path, next);
			}
			i++;
		} while (i < i_length);
	}

	va_end(args2);

	free(p_wide_base_directory);

	return p_wide_result_path;
}

/**
 * @brief Appends a variable number of `char *` arguments to the plugin base directory.
 * @warning 1. The function's behaviour is undefined if:
 * @warning - The `i_length` argument is greater than the amount of variadic arguments you provided.
 * @warning - Any variadic argument has a data type different from `char *`.
 * @note 1. If `i_length` is 0, the return will be the absolute path of the base plugin directory,
 * which is the same as calling `get_base_plugin_directory()`. For that reason, if you're not
 * planning to add directories to the path, please use `get_base_plugin_directory()` instead.
 *
 * @param i_length The amount of variadic arguments.
 * @param ... Variable amount of pointer to char arguments with null character.
 * @return If success, returns a pointer to char of the resulting null-terminated string path
 * starting from the base plugin directory, NULL otherwise.
 */
static inline char *multibyte_append_to_plugin_path(const size_t i_length, ...)
{
	const char *next;
	char	   *p_base_directory = get_base_plugin_directory();
	char	   *p_result_path	 = NULL;
	size_t		i_result_size	 = 0;
	va_list		args1, args2;

	if (!p_base_directory)
	{
		return NULL;
	}

	if (i_length == 0)
	{
		return p_base_directory;
	}

	va_start(args1, i_length);
	va_copy(args2, args1);

	i_result_size = strlen(p_base_directory);
	{
		int i = 0;
		do
		{
			next = va_arg(args1, const char *);

			if (next != NULL)
			{
				i_result_size += strlen(next) + 1; // +1 for initial OS separator
			}
			i++;
		} while (i < i_length);
	}

	va_end(args1);

	p_result_path = (char *)malloc(sizeof(char) * (i_result_size + 1)); // +1 for null character

	if (!p_result_path)
	{
		return NULL;
	}

	strcpy(p_result_path, p_base_directory);

	{
		int i = 0;
		do
		{
			next = va_arg(args2, const char *);
			if (next != NULL)
			{
				strcat(p_result_path, OS_SEPARATOR);
				strcat(p_result_path, next);
			}
			i++;
		} while (i < i_length);
	}

	va_end(args2);

	free(p_base_directory);

	return p_result_path;
}

#ifdef __cplusplus
}
#endif

#endif // PATH_H