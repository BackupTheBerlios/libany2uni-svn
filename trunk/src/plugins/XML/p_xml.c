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
  XML_SetUserData(desc->parser, &(desc->myState));
  XML_SetElementHandler(desc->parser, startElement, endElement);
  XML_SetCharacterDataHandler(desc->parser, characters);
  (desc->myState).isTextContent = 0;
  (desc->myState).suspended = 0;
  (desc->myState).pparser = &(desc->parser);

  /* initialize converter ( content is utf8 ) */
  err = U_ZERO_ERROR;
  desc->conv = ucnv_open("utf8", &err);
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
  desc->myState.ch = out;
  desc->myState.chlen = 0;

  /* continuing to next paragraph*/
  if ((desc->myState).suspended) {
    (desc->myState).suspended = 0;
    XML_ResumeParser(desc->parser);
  }

  /* filling a new buffer if the last one has been consumed */
  if (!(desc->myState).suspended) {
    desc->myState.buflen = read(desc->fd, buf, BUFSIZE);
  }

  while (!(desc->myState).suspended && desc->myState.buflen > 0) {
    /* processing data until a whole paragraph has been parse
       or end of file is reached */

    /* parsing buffer */
    if (XML_Parse(desc->parser, buf, desc->myState.buflen, 0) == XML_STATUS_ERROR) {
      return -2;
    }

    /* filling new buffer if the last one has been consumed */
    XML_GetParsingStatus(desc->parser, &status);
    if (status.parsing != XML_SUSPENDED) {
      desc->myState.buflen = read(desc->fd, buf, BUFSIZE);
    }
  }
  
  /* end of file has been reached */
  if (desc->myState.buflen == 0) {

    /* resuming parsing if needed (this shouldn't happen) */
    if ((desc->myState).suspended) {
      XML_ResumeParser(desc->parser);
    }

    /* signaling the end to the parser */
    if (XML_Parse(desc->parser, buf, 0, 1) == XML_STATUS_ERROR) {
      return -2;
    }
    return NO_MORE_DATA;
  }

  return desc->myState.chlen;
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
  len = parse(desc, outputbuf);
  
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
  if(desc->size > 0) {
    return (100 * XML_GetCurrentByteIndex(desc->parser)) / desc->size;
  } else {
    return 0;
  }
}
