/*
** Copyright (C) 2001-2011 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 2.1 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include	"sfconfig.h"

#include	<stdio.h>
#include	<string.h>
#include	<math.h>

#include	"sndfile.h"
#include	"common.h"


int
psf_store_string (SF_PRIVATE *psf, int str_type, const char *str)
{	char	new_str [128] ;
	size_t	len_remaining, str_len ;
	int		k, str_flags ;

	if (str == NULL)
		return SFE_STR_BAD_STRING ;

	str_len = strlen (str) ;

	/* A few extra checks for write mode. */
	if (psf->file.mode == SFM_WRITE || psf->file.mode == SFM_RDWR)
	{	if ((psf->str_flags & SF_STR_ALLOW_START) == 0)
			return SFE_STR_NO_SUPPORT ;
		if (psf->have_written && (psf->str_flags & SF_STR_ALLOW_END) == 0)
			return SFE_STR_NO_SUPPORT ;
		/* Only allow zero length strings for software. */
		if (str_type != SF_STR_SOFTWARE && str_len == 0)
			return SFE_STR_BAD_STRING ;
		} ;

	/* Find the next free slot in table. */
	for (k = 0 ; k < SF_MAX_STRINGS ; k++)
	{	/* If we find a matching entry clear it. */
		if (psf->strings [k].type == str_type)
			psf->strings [k].type = -1 ;

		if (psf->strings [k].type == 0)
			break ;
		} ;

	/* Determine flags */
	str_flags = SF_STR_LOCATE_START ;
	if (psf->file.mode == SFM_RDWR || psf->have_written)
	{	if ((psf->str_flags & SF_STR_ALLOW_END) == 0)
			return SFE_STR_NO_ADD_END ;
		str_flags = SF_STR_LOCATE_END ;
		} ;

	/* More sanity checking. */
	if (k >= SF_MAX_STRINGS)
		return SFE_STR_MAX_COUNT ;

	if (k == 0 && psf->str_end != NULL)
		return SFE_STR_WEIRD ;

	if (k != 0 && psf->str_end == NULL)
		return SFE_STR_WEIRD ;

	/* Special case for the first string. */
	if (k == 0)
		psf->str_end = psf->str_storage ;

	switch (str_type)
	{
		case SF_STR_TITLE :
		case SF_STR_COPYRIGHT :
		case SF_STR_ARTIST :
		case SF_STR_COMMENT :
		case SF_STR_DATE :
		case SF_STR_ALBUM :
		case SF_STR_LICENSE :
		case SF_STR_TRACKNUMBER :
		case SF_STR_GENRE :
				break ;

		default :
			return SFE_STR_BAD_TYPE ;
		} ;

	str_len = strlen (str) ;

	len_remaining = SIGNED_SIZEOF (psf->str_storage) - (psf->str_end - psf->str_storage) ;

	if (len_remaining < str_len + 2)
		return SFE_STR_MAX_DATA ;

	psf->strings [k].type = str_type ;
	psf->strings [k].str = psf->str_end ;
	psf->strings [k].flags = str_flags ;

	memcpy (psf->str_end, str, str_len + 1) ;
	/* Plus one to catch string terminator. */
	psf->str_end += str_len + 1 ;

	psf->str_flags |= str_flags ;

	return 0 ;
} /* psf_store_string */

int
psf_set_string (SF_PRIVATE *psf, int str_type, const char *str)
{	if (psf->file.mode == SFM_READ)
		return SFE_STR_NOT_WRITE ;

	return psf_store_string (psf, str_type, str) ;
} /* psf_set_string */

const char*
psf_get_string (SF_PRIVATE *psf, int str_type)
{	int k ;

	for (k = 0 ; k < SF_MAX_STRINGS ; k++)
		if (str_type == psf->strings [k].type)
			return psf->strings [k].str ;

	return NULL ;
} /* psf_get_string */

int
psf_location_string_count (const SF_PRIVATE * psf, int location)
{	int k, count = 0 ;

	for (k = 0 ; k < SF_MAX_STRINGS ; k++)
		if (psf->strings [k].type > 0 && psf->strings [k].flags & location)
			count ++ ;

	return count ;
} /* psf_location_string_count */
