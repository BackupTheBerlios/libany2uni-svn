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
}

void XMLCALL startElement(void *user_data, const char *name, const char **attrs) {
  char *ch, *ch2;
  int i;
  UErrorCode err;
  int namelen, valuelen, gotonext;
  UChar *uname, *uvalue;
  struct meta *meta;

  /* text items */
  if (strcmp(name, "ITEXT") == 0) {
    i = 0;
    while (attrs[i] != NULL && strncmp(attrs[i], "CH", 2) != 0) {
      i +=2;
    }
    i++;
    if (attrs[i] != NULL && strlen(attrs[i]) > 0) {

      /* adjusting begining byte for progression calcul */
      if ((((struct ParserState *)user_data)->size_adjusted) == 0) {
	((struct ParserState *)user_data)->size_adjusted = 1;
	((struct ParserState *)user_data)->begin_byte =
	  XML_GetCurrentByteIndex(*((struct ParserState *)user_data)->pparser);
      }

      /* filling text buffer */
      ch = (char *) malloc(strlen(attrs[i]) + 1);
      strncpy(ch, attrs[i], strlen(attrs[i]));
      strncpy(ch + strlen(attrs[i]), "\0", 1);
      if (((struct ParserState *)user_data)->chlen + strlen(ch) >= INTERNAL_BUFSIZE) {
	strncpy(ch + INTERNAL_BUFSIZE - ((struct ParserState *)user_data)->chlen, "\0", 1);
      }

      sprintf(((struct ParserState *)user_data)->ch +
	      ((struct ParserState *)user_data)->chlen, "%s", ch);
      ((struct ParserState *)user_data)->chlen += strlen(ch);
      free(ch);
    }
  
    /* new page */
  } else if (strcmp(name, "PAGEOBJECT") == 0) {
    /* nothing for now */

    /* handling metadata */
  } else if (strcmp(name, "DOCUMENT") == 0) {
    
    gotonext = 0;

    /* initializing new metadata structure */
    ((struct ParserState *)user_data)->meta = (struct meta *) malloc(sizeof(struct meta)); 
    ((struct ParserState *)user_data)->meta->next = NULL;
    meta = ((struct ParserState *)user_data)->meta;

    for (i = 0; attrs[i] != NULL; i += 2) {

      /* finding relevant metadata */
      if (strcmp(attrs[i], "COMMENTS") == 0 ||
	  strcmp(attrs[i], "TITLE") == 0 ||
	  strcmp(attrs[i], "AUTHOR") == 0 ) {

	if(strlen(attrs[i+1]) > 0) {

	  /* next structure of the linked list */
	  if (gotonext) {
	    meta->next = (struct meta *) malloc(sizeof(struct meta));
	    meta = meta->next;
	  }
	  
	  /* copying name */
	  ch = (char *) malloc(strlen(attrs[i]));
	  strncpy(ch, attrs[i], strlen(attrs[i]));
	  strncpy(ch + strlen(attrs[i]), "\0", 1);

	  /*converting name to UTF-16 */
	  err = U_ZERO_ERROR;
	  uname = (UChar *) malloc(2*strlen(ch)+1);
	  namelen = 2 * ucnv_toUChars(((struct ParserState *)user_data)->cnv,
				      uname, 2*strlen(ch)+1, ch, strlen(ch), &err);
	  if (U_FAILURE(err)) {
	    printf("error icu\n");
	  }
	  
	  /* copying value */
	  ch2 = (char *) malloc(strlen(attrs[i+1]));
	  strncpy(ch2, attrs[i+1], strlen(attrs[i+1]));
	  strncpy(ch2 + strlen(attrs[i+1]), "\0", 1);

	  /*converting value to UTF-16 */
	  err = U_ZERO_ERROR;
	  uvalue = (UChar *) malloc(2*strlen(ch2)+1);
	  valuelen = 2 * ucnv_toUChars(((struct ParserState *)user_data)->cnv,
				       uvalue, 2*strlen(ch2)+1, ch2, strlen(ch2), &err);
	  if (U_FAILURE(err)) {
	    printf("error icu\n");
	  }
	  
	  /* filling metadata structure */
	  meta->name = uname;
	  meta->name_length = u_strlen(uname);
	  meta->value = uvalue;
	  meta->value_length = u_strlen(uvalue);
	  gotonext = 1;

	  free(ch);
	  free(ch2);
	}
      }
    }
    
    /* suspending parsing for metadata handling */
    if(((struct ParserState *)user_data)->meta != NULL) {
      ((struct ParserState *)user_data)->meta_suspended = 1;
      XML_StopParser( *((struct ParserState *)user_data)->pparser, 1);
    }
  }
}

void XMLCALL endElement(void *user_data, const char *name) {
  if (strcmp(name, "ITEXT") == 0) {
    ((struct ParserState *)user_data)->suspended = 1;
    XML_StopParser( *((struct ParserState *)user_data)->pparser, 1);
  }
}
