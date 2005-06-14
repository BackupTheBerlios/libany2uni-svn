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
 *  XML plugin
 */

#include "p_xml.h"
#include "saxHandlers.h"
#include <unicode/ustring.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#define BUFSIZE 2048

/* params : desc : the document descriptor
 * return : an error code
 *
 * initializes the plugin by creating an xmlreader and
 * storing it in desc
 * This function also positions the meatreader at the beginning
 * of the metadata
 */
int initPlugin(struct doc_descriptor *desc) {
  UErrorCode err;

  desc->fd = open(desc->filename, O_RDONLY);
  desc->parser = XML_ParserCreate(NULL);
  desc->myState = (struct ParserState *) malloc(sizeof(struct ParserState));
  XML_SetUserData(desc->parser, desc->myState);
  XML_SetElementHandler(desc->parser, startElement, endElement);
  XML_SetCharacterDataHandler(desc->parser, characters);
  ((struct ParserState *)(desc->myState))->isTextContent = 0;
  ((struct ParserState *)(desc->myState))->suspended = 0;
  ((struct ParserState *)(desc->myState))->pparser = &(desc->parser);

  /* initialize converter ( content is utf8 ) */
  err = U_ZERO_ERROR;
  desc->conv = ucnv_open("utf8", &err);
  if (U_FAILURE(err)) {
    fprintf(stderr, "Unable to open ICU converter\n");
    return ERR_ICU;
  }
  ((struct ParserState *)(desc->myState))->cnv = desc->conv;
  

  return OK;
}


/* params : desc : the document descriptor
 * return : an error code
 *
 * closes the plugin by freeing the xmlreader
 */
int closePlugin(struct doc_descriptor *desc) {
  free(desc->myState);
  ucnv_close(desc->conv);
  XML_ParserFree(desc->parser);
  close(desc->fd);
  return OK;
}

/* params : desc : the document descriptor
 *          out  : destination buffer for the paragraph
 * return : the length of the paragraph
 *          NO_MORE_DATA if there is no more paragraph
 *          or an error code
 *
 * parses the next paragraph
 */
int parse(struct doc_descriptor* desc, char *out) {
  char buf[BUFSIZE];
  XML_ParsingStatus status;

  /* initializing next paragraph container */
  ((struct ParserState *)(desc->myState))->ch = out;
  ((struct ParserState *)(desc->myState))->chlen = 0;

  /* continuing to next paragraph*/
  if (((struct ParserState *)(desc->myState))->suspended) {
    ((struct ParserState *)(desc->myState))->suspended = 0;
    XML_ResumeParser(desc->parser);
  }

  /* filling a new buffer if the last one has been consumed */
  if (!((struct ParserState *)(desc->myState))->suspended) {
    ((struct ParserState *)(desc->myState))->buflen = read(desc->fd, buf, BUFSIZE);
  }

  while (!((struct ParserState *)(desc->myState))->suspended
	 && ((struct ParserState *)(desc->myState))->buflen > 0) {
    /* processing data until a whole paragraph has been parse
       or end of file is reached */

    /* parsing buffer */
    if (XML_Parse(desc->parser, buf,
		  ((struct ParserState *)(desc->myState))->buflen, 0) == XML_STATUS_ERROR) {
      fprintf(stderr, "Parsing error : %s\n",
	      XML_ErrorString(XML_GetErrorCode(desc->parser)));
      return SAX_ERROR;
    }

    /* filling new buffer if the last one has been consumed */
    XML_GetParsingStatus(desc->parser, &status);
    if (status.parsing != XML_SUSPENDED) {
      ((struct ParserState *)(desc->myState))->buflen = read(desc->fd, buf, BUFSIZE);
    }
  }
  
  /* end of file has been reached */
  if (((struct ParserState *)(desc->myState))->buflen == 0) {

    /* resuming parsing if needed (this shouldn't happen) */
    if (((struct ParserState *)(desc->myState))->suspended) {
      XML_ResumeParser(desc->parser);
    }

    /* signaling the end to the parser */
    if (XML_Parse(desc->parser, buf, 0, 1) == XML_STATUS_ERROR) {
      fprintf(stderr, "Parsing error : %s\n",
	      XML_ErrorString(XML_GetErrorCode(desc->parser)));
      return SAX_ERROR;
    }
    return NO_MORE_DATA;
  }

  return ((struct ParserState *)(desc->myState))->chlen;
}


/* params : desc : the document descriptor
 *          buf  : destination buffer for UTF-16 data
 * return : the length of the paragraph
 *          NO_MORE_DATA if there is no more paragraph
 *          ERR_STREAMFILE if an error occured
 *
 * reads the next paragraph and converts to UTF-16
 */
int p_read_content(struct doc_descriptor *desc, UChar *buf) {
  char *outputbuf, *src;
  UChar *dest;
  int len;
  UErrorCode err;

  len = 0;

  outputbuf = (char *) malloc(INTERNAL_BUFSIZE);

  /* reading the next paragraph */
  len = parse(desc, outputbuf);
  
  if (len > 0) {
    (desc->nb_par_read) += 1;

    /* converting to UTF-16 */
    err = U_ZERO_ERROR;
    dest = buf;
    src = outputbuf;
    ucnv_toUnicode(desc->conv, &dest, dest + 2*INTERNAL_BUFSIZE,
		   &src, outputbuf + strlen(outputbuf), NULL, FALSE, &err);
    len = 2*(dest - buf);
    if (U_FAILURE(err)) {
      fprintf(stderr, "Unable to convert buffer\n");
      return ERR_ICU;
    }

  }

  if(outputbuf != NULL) {
    free(outputbuf);
  }

  return len;
}


/* params : desc : the document descriptor
 * return : a name-value couple in UTF-16 for the next metadata available
 *          NULL if none
 *
 * reads the next metadata available 
 */
int p_read_meta(struct doc_descriptor *desc, struct meta *meta) {

  return NO_MORE_META;
}


/* params : desc : the document descriptor
 * return : an indicator of the progression in the processing
 */
int p_getProgression(struct doc_descriptor *desc) {
  if(desc->size > 0) {
    return (100 * XML_GetCurrentByteIndex(desc->parser)) / desc->size;
  } else {
    return 0;
  }
}
