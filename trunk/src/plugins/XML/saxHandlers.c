/*
  This file is part of the libany2uni project, an universal
  text extractor in unicode utf-16
  Copyright (C) 2005  Gwendal Dufresne

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the
  Free Software Foundation, Inc., 59 Temple Place - Suite 330,
  Boston, MA  02111-1307, USA.
*/

/*
 * callback functions for Expat SAX parser
 */

#include "saxHandlers.h"
#include "../../misc.h"
#include <unicode/ustring.h>
#include <stdio.h>
#include <string.h>


void XMLCALL characters(void *user_data, const char *ch, int len) {
  char *ch2;
  int i;

  for(i = 0; i < len && (!strncmp(ch + i, " ", 1) ||
			 !strncmp(ch + i, "\n", 1) ||
			 !strncmp(ch + i, "\t", 1)); i++) {}
  if(i != len) {
    ch2 = (char *) malloc(len + 1 - i);
    strncpy(ch2, ch + i, len - i);
    strncpy(ch2 + len - i, "\0", 1);
    if (((struct ParserState *)user_data)->chlen + strlen(ch2) >= INTERNAL_BUFSIZE) {
      strncpy(ch2 + INTERNAL_BUFSIZE - ((struct ParserState *)user_data)->chlen, "\0", 1);
    }
    sprintf(((struct ParserState *)user_data)->ch +
	    ((struct ParserState *)user_data)->chlen, "%s", ch2);
    ((struct ParserState *)user_data)->chlen += strlen(ch2);
    
    free(ch2);
  }
}

void XMLCALL startElement(void *user_data, const char *name, const char **attrs) {

}

void XMLCALL endElement(void *user_data, const char *name) {
  if( ((struct ParserState *)user_data)->chlen > 0) {
    ((struct ParserState *)user_data)->suspended = 1;
    XML_StopParser( *((struct ParserState *)user_data)->pparser, 1);
  }

}
