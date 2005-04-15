/*
 * KOFFICE plugin
 */

#include "p_koffice.h"
#include "saxHandlers.h"
#include "unzip.h"
#include <unicode/ustring.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#define BUFSIZE 2048

/*
 * fills metadata structures using the appropriate file in the archive
 */
int getMeta(struct doc_descriptor *desc) {
  char buf[BUFSIZE];
  struct meta *meta;

  /* opening file containing metadata */
  unzLocateFile(desc->unzFile, "documentinfo.xml", 2);
  unzOpenCurrentFile(desc->unzFile);

  /* filling buffer */
  (desc->myState).buflen = unzReadCurrentFile(desc->unzFile, buf, BUFSIZE);

  /* creating metadata sax parser */
  desc->parser = XML_ParserCreate(NULL);
  XML_SetUserData(desc->parser, &(desc->myState));
  XML_SetEndElementHandler(desc->parser, metaEndElement);
  XML_SetCharacterDataHandler(desc->parser, metaCharacters);
  (desc->myState).isMeta = 1;
  (desc->myState).isTextContent = 0;
  (desc->myState).meta_suspended = 0;
  (desc->myState).pparser = &(desc->parser);

  /* proceding until the end of metadata */
  while( (desc->myState).buflen > 0 && (desc->myState).isMeta) {
    (desc->myState).meta = NULL;
    
    if (XML_Parse(desc->parser, buf, desc->myState.buflen, 0) == XML_STATUS_ERROR) {
      return -2;
    }
    
    while((desc->myState).meta_suspended) {
      meta = desc->meta;

      /* adding the new metadata at the end of the list */
      if (meta == NULL) {
	desc->meta = (desc->myState).meta;
      } else {
	while(meta->next != NULL) {
	  meta = meta->next;
	}
	meta->next = (desc->myState).meta;
      }
      
      /* resuming parsing */
      (desc->myState).meta_suspended = 0;
      XML_ResumeParser(desc->parser);
    }
    
    /* filling a new buffer */
    if((desc->myState).buflen > 0) {
      (desc->myState).buflen = unzReadCurrentFile(desc->unzFile, buf, BUFSIZE);    
    }
  }
  
  XML_ParserFree(desc->parser);
  unzCloseCurrentFile(desc->unzFile);

  return OK;
}



/* params : desc : the document descriptor
 * return : an error code
 *
 * initializes the plugin by creating an xmlreader and
 * storing it in desc
 */
int initPlugin(struct doc_descriptor *desc) {
  UErrorCode err;
  unz_file_info info;

  /* initialize converter ( content is utf8 ) */
  err = U_ZERO_ERROR;
  desc->conv = ucnv_open("utf8", &err);
  if (U_FAILURE(err)) {
    return ERR_ICU;
  }
  (desc->myState).cnv = desc->conv;

  desc->unzFile = unzOpen(desc->filename);
  (desc->myState).meta_suspended = 0;
  (desc->myState).meta = NULL;
  getMeta(desc);

  unzLocateFile(desc->unzFile, "maindoc.xml", 2);
  unzOpenCurrentFile(desc->unzFile);
  unzGetCurrentFileInfo(desc->unzFile, &info, NULL, 0, NULL, 0, NULL, 0);
  desc->size = info.uncompressed_size;
  desc->parser = XML_ParserCreate(NULL);
  XML_SetUserData(desc->parser, &(desc->myState));
  XML_SetElementHandler(desc->parser, startElement, endElement);
  XML_SetCharacterDataHandler(desc->parser, characters);
  (desc->myState).isTextContent = 0;
  (desc->myState).isNote = 0;
  (desc->myState).isMeta = 0;
  (desc->myState).suspended = 0;
  (desc->myState).pparser = &(desc->parser);
  (desc->myState).begin_byte = 0;
  (desc->myState).size_adjusted = 0;

  

  return OK;
}


/* params : desc : the document descriptor
 * return : an error code
 *
 * closes the plugin by freeing the xmlreader
 */
int closePlugin(struct doc_descriptor *desc) {
  XML_ParserFree(desc->parser);
  unzCloseCurrentFile(desc->unzFile);
  unzClose(desc->unzFile);
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
    desc->myState.buflen = unzReadCurrentFile(desc->unzFile, buf, BUFSIZE);
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
      desc->myState.buflen = unzReadCurrentFile(desc->unzFile, buf, BUFSIZE);
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
    desc->meta = desc->meta->next;
  }
  return OK;
}


/* params : desc : the document descriptor
 * return : an indicator of the progression in the processing
 */
int p_getProgression(struct doc_descriptor *desc) {
  if(desc->size > 0) {
    return (100 * (XML_GetCurrentByteIndex(desc->parser)
		   - desc->myState.begin_byte))
      / (desc->size - desc->myState.begin_byte);
  } else {
    return 0;
  }
}
