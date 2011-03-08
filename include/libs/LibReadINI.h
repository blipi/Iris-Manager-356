/*
 * LibReadINI by D_Skywalk
 *
 * Copyright (c) 2006 David Colmenero Aka D_Skywalk
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
 *
 *
 *   Version 0.8 (Stable)
 * 
 * 0.8 - Initial Public version
 *
**/

#ifndef LIBREADINI_H
#define LIBREADINI_H

#define     MS_STYLE            1
#define     NOT_MS_STYLE        2

/*
 * NEW fuction prototypes
 */

/*
 * Opens the config file
 */
int cfgOpen(const char * Filename,const int Style);

/*
 * Selects (& reads) current open config file
 * Note: This function use a Case Sensitive string comparison...
 */
int cfgSelectSection(const char * SectionName);

/*
 * Returns an: int, text or bool variable from opened config file
 * if not is found returns default
 * Note: These functions use a NO Case Sensitive string comparison...
 */
int cfgReadInt(const char * VariableName,const int Default);
char * cfgReadText(const char * VariableName,const char * Default);
bool cfgReadBool(const char * VariableName, const bool Default);


/*
 * Close the config file
 */
void cfgClose( void );
void cfgFree( void );

#endif  /* LIBREADINI_H */
