/*  XFce 4 - Netload Plugin
 *    Copyright (c) 2003 Bernhard Walle <bernhard.walle@gmx.de>
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <locale.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef BUFSIZ
#define BUFSIZ 512
#endif


char* format_with_thousandssep( char* string, int stringsize, double number, int digits )
{
    char* str = string;
    char buffer[BUFSIZ], formatstring[BUFSIZ];
    char* bufptr = buffer;
    unsigned int i;
    int numberOfIntegerChars, count;
    struct lconv* localeinfo = localeconv();
    int grouping = (int)localeinfo->grouping[0] == 0 ? INT_MAX : (int)localeinfo->grouping[0];
    
    
    /* sensible value for digits */
    if( digits < 0 || digits >= 10 )
    {
        digits = 2;
    }
    
    snprintf( formatstring, BUFSIZ, "%%.%df", digits );
    snprintf( buffer, BUFSIZ, formatstring, number );
    
    /* get the number of integer characters */
    count = numberOfIntegerChars = ( digits > 0
                        ? ( strstr( buffer, localeinfo->decimal_point ) - buffer )
                        :   strlen( buffer ) );
    
    
    /* check for length */
    if( numberOfIntegerChars / grouping + (int)strlen( buffer ) > stringsize )
    {
        return NULL;
    }
    
    
    /* insert the thousands separator */
    while( *bufptr != 0 && *bufptr != localeinfo->decimal_point[0] )
    {
        if( count % grouping == 0 && count != numberOfIntegerChars )
        {
            for( i = 0; i < strlen( localeinfo->thousands_sep ); i++ )
            {
                *str++ = localeinfo->thousands_sep[i];
            }
        }
        *str++ = *bufptr++;
        count--;
    }
    
    /* Copy the rest */
    while( digits > 0 && *bufptr != 0 )
    {
        *str++ = *bufptr++;
    }
    
    /* terminate with 0 finally */
    *str = 0;
    
    return string;
}
