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
 *  WORD plugin
 */

#include "p_word.h"
#include <unicode/ustring.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

extern FILE *fdopen(int fildes, const char *mode);

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
  enum wType type;
  long int length, textstart;
  char buf[32];

  desc->myState = (struct ParserState *) malloc(sizeof(struct ParserState));
  
  desc->fd = open(desc->filename, O_RDONLY);
  desc->filedes = fdopen(desc->fd, "r");

  type = identify_version(desc->filedes);
  switch(type) {
  case unknown:
    fprintf(stderr, "Not MS Word format\n");
    return ERR_UNKNOWN_FORMAT;
    break;

  case oldword:
    fseek(desc->filedes, 0, SEEK_SET);
    fread(buf, 1, 32, desc->filedes);
    memcpy(&length, buf + 28, 4);
    memcpy(&textstart, buf + 24, 4);
    desc->size = length - textstart;
    ((struct ParserState *)(desc->myState))->begin_byte = textstart;
    fseek(desc->filedes, textstart, SEEK_SET);
    break;

  case ole:
    desc->size = (off_t) seek_textstart(desc);
    break;
  }

  /* initialize converter */
  err = U_ZERO_ERROR;
  desc->conv = ucnv_open("cp1252", &err);
  if (U_FAILURE(err)) {
    fprintf(stderr, "unable to open ICU converter\n");
    return ERR_ICU;
  }

  return OK;
}


/* params : desc : the document descriptor
 * return : an error code
 *
 */
int closePlugin(struct doc_descriptor *desc) {
  free(desc->myState);
  ucnv_close(desc->conv);
  fclose(desc->filedes);
  return OK;
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
  char *outputbuf;
  int len;
  UErrorCode err;

  len = 0;
  outputbuf = (char *) malloc(INTERNAL_BUFSIZE);

  /* reading the next paragraph */
  len = read_next(desc, outputbuf, INTERNAL_BUFSIZE);

  if (len > 0) {
    (desc->nb_par_read) += 1;

    /* converting to UTF-16 */
    err = U_ZERO_ERROR;
    len = 2 * ucnv_toUChars(desc->conv, buf, 2*INTERNAL_BUFSIZE,
			    outputbuf, strlen(outputbuf), &err);
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
  struct meta *pre;

  if(desc->meta == NULL) {
    return NO_MORE_META;
  } else {

    /* copying content of desc->meta in meta */
    meta->name = desc->meta->name;
    meta->name_length = desc->meta->name_length;
    meta->value = desc->meta->value;
    meta->value_length = desc->meta->value_length;

    /* switching to next metadata in descriptor
     (the current one is lost) */
    pre = desc->meta;
    desc->meta = desc->meta->next;
    free(pre);
  }
  return OK;
}


/* params : desc : the document descriptor
 * return : an indicator of the progression in the processing
 */
int p_getProgression(struct doc_descriptor *desc) {
  if (desc->size > 0) {
    return ( (100 * (ftell(desc->filedes) -
		     ((struct ParserState *)(desc->myState))->begin_byte))
	     / desc->size);
  }
  return 0;
}
