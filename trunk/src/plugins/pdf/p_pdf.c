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
 *  PDF plugin
 */

#include "p_pdf.h"


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
  desc->myState = (struct pdfState *) malloc(sizeof(struct pdfState));
  desc->nb_pages_read = 0;
  desc->pageCount = 1;

  ((struct pdfState *)(desc->myState))->version = version(desc->fd);
  if(((struct pdfState *)(desc->myState))->version < 0) {
    fprintf(stderr, "not PDF file\n");
    return ERR_UNKNOWN_FORMAT;
  }

  /* initialize converter */
  err = U_ZERO_ERROR;
  desc->conv = ucnv_open("latin1", &err);
  if (U_FAILURE(err)) {
    fprintf(stderr, "unable to open ICU converter\n");
    return ERR_ICU;
  }

  if (initReader(desc) < 0) {
    return INIT_ERROR;
  };

  
  return OK;
}


/* params : desc : the document descriptor
 * return : an error code
 *
 */
int closePlugin(struct doc_descriptor *desc) {
  if(((struct pdfState *)(desc->myState))->glyphfile != -1) {
    close(((struct pdfState *)(desc->myState))->glyphfile);
  }
  freeCMapList(((struct pdfState *)(desc->myState))->cmaplist);
  freeEncodingTable(((struct pdfState *)(desc->myState))->encodings);
  freeFilterStruct(((struct pdfState *)(desc->myState))->filter);
  freeXRefStruct(((struct pdfState *)(desc->myState))->XRef);
  free(desc->myState);
  ucnv_close(desc->conv);
  close(desc->fd);
  return OK;
}

/* params : desc : the document descriptor
 *          buf  : destination buffer for UTF-16 data
 * return : the length of the paragraph
 *
 * reads the next paragraph and converts to UTF-16
 */
int p_read_content(struct doc_descriptor *desc, UChar *buf) {
  int len;

  len = 0;

  /* reading the next paragraph */
  len = getText(desc, buf, 2*INTERNAL_BUFSIZE);

  if (len > 0) {
    (desc->nb_par_read) += 1;
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
  if(desc->pageCount > 0) {
    return 100 * desc->nb_pages_read / desc->pageCount;
  } else {
    return 0;
  }
}
