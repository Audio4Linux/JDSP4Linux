#ifndef FINDBINARY_H
#define FINDBINARY_H
// Copyright 2015 by Mark Whitis.  License=MIT style
#include <stdio.h>
#include <stdlib.h>
#if (!defined(_WIN32) || !defined(_WIN64))
#include <unistd.h>
#endif
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#ifdef _MSC_VER
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#endif

// "look deep into yourself, Clarice"  -- Hanibal Lector
char findyourself_save_pwd[PATH_MAX];
char findyourself_save_argv0[PATH_MAX];
char findyourself_save_path[PATH_MAX];
char findyourself_path_separator              = '/';
char findyourself_path_separator_as_string[2] = "/";
char findyourself_path_list_separator[8]      = ":"; // could be ":; "
char findyourself_debug                       = 0;

int  findyourself_initialized                 = 0;

void findyourself_init(char *argv0)
{
#if (!defined(_WIN32) || !defined(_WIN64))
	getcwd(findyourself_save_pwd, sizeof(findyourself_save_pwd));

	strncpy(findyourself_save_argv0, argv0,          sizeof(findyourself_save_argv0));
	findyourself_save_argv0[sizeof(findyourself_save_argv0) - 1] = 0;

	strncpy(findyourself_save_path,  getenv("PATH"), sizeof(findyourself_save_path));
	findyourself_save_path[sizeof(findyourself_save_path) - 1]   = 0;
	findyourself_initialized                                     = 1;
#endif
}

int find_yourself(char  *result,
                  size_t size_of_result)
{
#if (!defined(_WIN32) || !defined(_WIN64))
	char newpath[PATH_MAX + 256];
	char newpath2[PATH_MAX + 256];

	assert(findyourself_initialized);
	result[0] = 0;

	if (findyourself_save_argv0[0] == findyourself_path_separator)
	{
		if (findyourself_debug)
		{
			printf("  absolute path\n");
		}

		realpath(findyourself_save_argv0, newpath);

		if (findyourself_debug)
		{
			printf("  newpath=\"%s\"\n", newpath);
		}

		if (!access(newpath, F_OK))
		{
			strncpy(result, newpath, size_of_result);
			result[size_of_result - 1] = 0;
			return 0;
		}
		else
		{
			perror("access failed 1");
		}
	}
	else if ( strchr(findyourself_save_argv0, findyourself_path_separator))
	{
		if (findyourself_debug)
		{
			printf("  relative path to pwd\n");
		}

		strncpy(newpath2, findyourself_save_pwd, sizeof(newpath2));
		newpath2[sizeof(newpath2) - 1] = 0;
        strncat(newpath2, findyourself_path_separator_as_string, sizeof(newpath2) - strlen(newpath2) - 1);
		newpath2[sizeof(newpath2) - 1] = 0;
        strncat(newpath2, findyourself_save_argv0,               sizeof(newpath2) - strlen(newpath2) - 1);
		newpath2[sizeof(newpath2) - 1] = 0;
		realpath(newpath2, newpath);

		if (findyourself_debug)
		{
			printf("  newpath=\"%s\"\n", newpath);
		}

		if (!access(newpath, F_OK))
		{
			strncpy(result, newpath, size_of_result);
			result[size_of_result - 1] = 0;
			return 0;
		}
		else
		{
			perror("access failed 2");
		}
	}
	else
	{
		if (findyourself_debug)
		{
			printf("  searching $PATH\n");
		}

		char *saveptr;
		char *pathitem;

		for (pathitem = strtok_r(findyourself_save_path, findyourself_path_list_separator,  &saveptr); pathitem; pathitem = strtok_r(NULL, findyourself_path_list_separator, &saveptr))
		{
			if (findyourself_debug >= 2)
			{
				printf("pathitem=\"%s\"\n", pathitem);
			}

			strncpy(newpath2, pathitem, sizeof(newpath2));
			newpath2[sizeof(newpath2) - 1] = 0;
            strncat(newpath2, findyourself_path_separator_as_string, sizeof(newpath2) - strlen(newpath2) - 1);
			newpath2[sizeof(newpath2) - 1] = 0;
            strncat(newpath2, findyourself_save_argv0,               sizeof(newpath2) - strlen(newpath2) - 1);
			newpath2[sizeof(newpath2) - 1] = 0;
			realpath(newpath2, newpath);

			if (findyourself_debug)
			{
				printf("  newpath=\"%s\"\n", newpath);
			}

			if (!access(newpath, F_OK))
			{
				strncpy(result, newpath, size_of_result);
				result[size_of_result - 1] = 0;
				return 0;
			}
		} // end for

		perror("access failed 3");

	} // end else

	// if we get here, we have tried all three methods on argv[0] and still haven't succeeded.   Include fallback methods here.
#endif
	return 1;
}

#endif // FINDBINARY_H
