/*
** ANSI FIX for toasters environments ;D
** By D_Skywalk (c) 2006
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef LIBREADINI_ANSI_H
#define LIBREADINI_ANSI_H

/*
** Defines
*/

#ifndef stricmp
    #define stricmp strcasecmp
#endif

char *reverse( char *s)
{
  char c, *p, *q;
  if (s!=NULL && *s!=0) /* No empty strings, please! */
  {
    q = s;
    while (*(++q)) ; /* points q at '0' terminator; */
    for (p=s; p < --q; p++)  /* ignores middle character when strlen is odd */
    {
      c = *p;
      *p = *q;
      *q = c;
    }
  }
  return s;
}

char* itoa( int value, char* result, int base ) {
	
	char* out = result;
	
	int quotient = value;

	/* check that the base if valid */
	
	if (base < 2 || base > 16) { *result = 0; return result; }
	

	

	
	do {
	
		*out = "0123456789abcdef"[ abs( quotient % base ) ];
	
		++out;
	
		quotient /= base;
	
	} while ( quotient );
	

	
	/* Only apply negative sign for base 10 */
	
	if ( value < 0 && base == 10) *out++ = '-';
	

	
	result = reverse( out );
	
	*out = 0;
	
	return result;
	
}

#endif

