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
 *  LATEX plugin
 */

#include "p_txt.h"
#include <unicode/ustring.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>


/* params : desc : the document descriptor
 * return : an error code
 *
 * initializes the plugin by creating an xmlreader and
 * storing it in desc
 * This function also positions the meatreader at the beginning
 * of the metadata
 */
int initPlugin(struct doc_descriptor *desc) {
  int err;

  desc->fd = open(desc->filename, O_RDONLY);

  err = initTxt(desc);
  if(err) {
    close(desc->fd);
    return err;
  }

  return OK;
}


/* params : desc : the document descriptor
 * return : an error code
 *
 */
int closePlugin(struct doc_descriptor *desc) {
  ucnv_close(desc->conv);
  close(desc->fd);
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
  char *outputbuf, *src;
  UChar *dest;
  int len;
  UErrorCode err;

  len = 0;

  outputbuf = (char *) malloc(INTERNAL_BUFSIZE);

  /* reading the next paragraph */
  memset(outputbuf, '\x00', INTERNAL_BUFSIZE);
  len = getText(desc, outputbuf, INTERNAL_BUFSIZE);
  
  if (len > 0) {
    (desc->nb_par_read) += 1;

    /* converting to UTF-16 */
    err = U_ZERO_ERROR;
    dest = buf;
    src = outputbuf;
    ucnv_toUnicode(desc->conv, &dest, dest + 2*INTERNAL_BUFSIZE,
		   &src, outputbuf + len, NULL, FALSE, &err);
    len = 2*(dest - buf);
    if (U_FAILURE(err)) {
      free(outputbuf);
      outputbuf = NULL;
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
  if (desc->size > 0) {
    return ( (100 * lseek(desc->fd, 0, SEEK_CUR)) / desc->size);
  }
  return 0;
}
