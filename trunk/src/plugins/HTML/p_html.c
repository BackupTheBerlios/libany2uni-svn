/*
 *  HTML plugin
 */

#include "p_html.h"
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
  char encoding[20];

  desc->fd = open(desc->filename, O_RDONLY);

  /* initialize converter ( content is utf8 ) */
  err = U_ZERO_ERROR;
  getEncoding(desc->fd, encoding);
  desc->conv = ucnv_open(encoding, &err);
  if (U_FAILURE(err)) {
    return ERR_ICU;
  }
  (desc->myState).cnv = desc->conv;
  

  return OK;
}


/* params : desc : the document descriptor
 * return : an error code
 *
 * closes the plugin by freeing the xmlreader
 */
int closePlugin(struct doc_descriptor *desc) {
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
  char *outputbuf;
  int len;
  UErrorCode err;

  len = 0;

  outputbuf = (char *) malloc(INTERNAL_BUFSIZE);

  /* reading the next paragraph */
  while (len == 0) {
    len = getText(desc, outputbuf);
  }
  if (len > 0) {
    (desc->nb_par_read) += 1;

    /* converting to UTF-16 */
    err = U_ZERO_ERROR;
    len = 2 * ucnv_toUChars(desc->conv, buf, 2*INTERNAL_BUFSIZE,
			    outputbuf, strlen(outputbuf), &err);
    if (U_FAILURE(err)) {
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
  int current_pos;

  current_pos = lseek(desc->fd, 0, SEEK_CUR);
  return ( 100 * current_pos ) / desc->size;
}
