/*
   Copyright (C)  2000    Daniel A. Atkinson  <DanAtk@aol.com>
   Copyright (C)  2004    Ivano Primi  <ivprimi@libero.it>    

   This file is part of the HPA Library.

   The HPA Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The HPA Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the HPA Library; if not, write to the Free
   Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
   02110-1301 USA.
*/

#include <iostream>
#include <cctype>
using namespace std;

int
hpa_read_item (istream& is, char* buff, unsigned size)
{
  if (size == 0 || !buff)
    return 0;
  else /* size > 0 */
    {
      unsigned i, n;
      char ch;

      i = 0;
      while ((is.get (ch)))
	{
	  if (!isspace (ch))
	    {
	      i = 1;
	      break;
	    }
	}
      if (i == 0)
	return 0;
      else
	{
	  i = n = 0;
	  do
	    {
	      if(n<size-1)
		buff[n++]=ch;
	      i++;
	    } while ((is.get(ch)) && !isspace (ch));
	  buff[n] = '\0';
	  return i;
	}
    }
}
