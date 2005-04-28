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

  if(((struct ParserState *)user_data)->isTextContent > 0
     && strncmp(ch, "\n", 1) != 0 ) {
    if ((((struct ParserState *)user_data)->size_adjusted) == 0) {
      ((struct ParserState *)user_data)->size_adjusted = 1;
      ((struct ParserState *)user_data)->begin_byte =
	XML_GetCurrentByteIndex(*((struct ParserState *)user_data)->pparser);
    }

    /* copying value */
    ch2 = (char *) malloc(len + 1);
    strncpy(ch2, ch, len);
    strncpy(ch2+len, "\0", 1);

    /* adding value to current buffer */
    if (((struct ParserState *)user_data)->chlen + strlen(ch2) >= INTERNAL_BUFSIZE) {
      strncpy(ch2 + INTERNAL_BUFSIZE - ((struct ParserState *)user_data)->chlen, "\0", 1);
    }
    sprintf(((struct ParserState *)user_data)->ch + ((struct ParserState *)user_data)->chlen, "%s", ch2);
    ((struct ParserState *)user_data)->chlen += strlen(ch2);
    
    free(ch2);
  }  
}

void XMLCALL startElement(void *user_data, const char *name, const char **attrs) {
  if(!strcmp(name, "text:p")) {
    ((struct ParserState *)user_data)->isTextContent += 1;

  } else if(((struct ParserState *)user_data)->chlen > 0 &&
	    !strcmp(name, "text:tab-stop")) {
    sprintf(((struct ParserState *)user_data)->ch + 
	     ((struct ParserState *)user_data)->chlen, " ");
    ((struct ParserState *)user_data)->chlen += 1;
    
  }

}

void XMLCALL endElement(void *user_data, const char *name) {

  if(!strcmp(name, "text:p")) {
    ((struct ParserState *)user_data)->isTextContent -= 1;

        /* suspending parsing if a non empty paragraph has been parsed */
    if (((struct ParserState *)user_data)->chlen > 0) {
      ((struct ParserState *)user_data)->suspended = 1;
      XML_StopParser( *((struct ParserState *)user_data)->pparser, 1);
    }
  }
}

void XMLCALL metaCharacters(void *user_data, const char *ch, int len) {
  char *ch2;
  UErrorCode err;
  UChar *uvalue;
  int valuelen;

  if( !((struct ParserState *)user_data)->isMeta ) {

    /* copying value */
    ch2 = (char *) malloc(len+1);
    strncpy(ch2, ch, len);
    strncpy(ch2 + len, "\0", 1);
    
    /*converting value to UTF-16 */
    err = U_ZERO_ERROR;
    uvalue = malloc(2*strlen(ch2)+1);
    valuelen = 2 * ucnv_toUChars(((struct ParserState *)user_data)->cnv,
				 uvalue, 2*strlen(ch2)+1, ch2, strlen(ch2), &err);
    if (U_FAILURE(err)) {
      fprintf(stderr, "Unable to convert buffer\n");
    }
    
    /* filling metadata structure */
    ((struct ParserState *)user_data)->meta->value = uvalue;
    ((struct ParserState *)user_data)->meta->value_length = u_strlen(uvalue);
    
    free(ch2);
  }

}

void XMLCALL metaEndElement(void *user_data, const char *name) {
  if( !strncmp(name, "dc", 2)) {
    ((struct ParserState *)user_data)->isMeta = 1;
    ((struct ParserState *)user_data)->meta_suspended = 1;
    XML_StopParser( *((struct ParserState *)user_data)->pparser, 1);
  }
}

void XMLCALL metaStartElement(void *user_data, const char *name,  const char **attrs) {
  char *ch;
  UErrorCode err;
  UChar *uname;
  int namelen;

  if( !strncmp(name, "dc", 2)) {

    ((struct ParserState *)user_data)->isMeta = 0;

    /* initializing new metadata structure */
    ((struct ParserState *)user_data)->meta = (struct meta *) malloc(sizeof(struct meta));
    ((struct ParserState *)user_data)->meta->next = NULL;

    ch = (char *) malloc(strlen(name) - 2 );
    strncpy(ch, name + 3, strlen(name) - 3);
    strncpy(ch + strlen(name) - 3, "\0", 1);

    /*converting value to UTF-16 */
    err = U_ZERO_ERROR;
    uname = malloc(2*strlen(ch)+1);
    namelen = 2 * ucnv_toUChars(((struct ParserState *)user_data)->cnv,
				uname, 2*strlen(ch)+1, ch, strlen(ch), &err);
    if (U_FAILURE(err)) {
      fprintf(stderr, "Unable to convert buffer\n");
    }

    /* filling metadata structure */
    ((struct ParserState *)user_data)->meta->name = uname;
    ((struct ParserState *)user_data)->meta->name_length = u_strlen(uname);

    free(ch);
  }
}
