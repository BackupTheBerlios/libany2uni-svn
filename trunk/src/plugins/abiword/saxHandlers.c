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
  UErrorCode err;
  UChar *uvalue;
  int valuelen;


  /* handling metadata value */
  if(((struct ParserState *)user_data)->isMeta == 1) {

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
      printf("error icu\n");
    }

    /* filling metadata structure */
    ((struct ParserState *)user_data)->meta->value = uvalue;
    ((struct ParserState *)user_data)->meta->value_length = u_strlen(uvalue);

    free(ch2);
  }

  /* handling textual content of paragraph (blank lines are skipped) */
  if(((struct ParserState *)user_data)->isTextContent == 1
     && strncmp(ch, "\n", 1) != 0 ) {


    if ((((struct ParserState *)user_data)->size_adjusted) == 0) {
      ((struct ParserState *)user_data)->size_adjusted = 1;
      ((struct ParserState *)user_data)->begin_byte =
	XML_GetCurrentByteIndex(*((struct ParserState *)user_data)->pparser);
    }

    /* special handling for notes */
    if(((struct ParserState *)user_data)->isNote == 2) {

      /* copying value and adding parentheses */
      ch2 = (char *) malloc(len + 5);
      strncpy(ch2, " (", 2);
      strncpy(ch2+2, ch, len);
      strncpy(ch2+len+2, ") \0", 3);

      /* adding the string to current buffer */
      if (((struct ParserState *)user_data)->chlen + strlen(ch2) >= INTERNAL_BUFSIZE) {
	strncpy(ch2 + INTERNAL_BUFSIZE - ((struct ParserState *)user_data)->chlen, "\0", 1);
      }
      sprintf(((struct ParserState *)user_data)->ch + ((struct ParserState *)user_data)->chlen, "%s", ch2);
      ((struct ParserState *)user_data)->chlen += strlen(ch2);

      free(ch2);

    } else {
      /* textual content */

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
}

void XMLCALL startElement(void *user_data, const char *name, const char **attrs) {
  char *ch;
  UErrorCode err;
  int namelen;
  UChar *uname;

  /* handling metadata name */
  if(strcmp(name, "m") == 0 &&
     strncmp(attrs[0], "key", 3) == 0 &&
     strncmp(attrs[1], "dc", 2) == 0) {

    /* initializing new metadata structure */
    ((struct ParserState *)user_data)->meta = (struct meta *) malloc(sizeof(struct meta)); 
    ((struct ParserState *)user_data)->meta->next = NULL;

    /* copying name */
    ch = (char *) malloc(strlen(attrs[1]));
    strncpy(ch, attrs[1]+3, strlen(attrs[1])-3);
    strncpy(ch + strlen(attrs[1])-3, "\0", 1);

    /*converting name to UTF-16 */
    err = U_ZERO_ERROR;
    uname = (UChar *) malloc(2*strlen(ch)+1);
    namelen = 2 * ucnv_toUChars(((struct ParserState *)user_data)->cnv,
				uname, 2*strlen(ch)+1, ch, strlen(ch), &err);
    if (U_FAILURE(err)) {
      printf("error icu\n");
    }
    
    /* filling metadata structure */
    ((struct ParserState *)user_data)->meta->name = uname;
    ((struct ParserState *)user_data)->meta->name_length = u_strlen(uname);

    /* indicating that next characters are metadata */
    ((struct ParserState *)user_data)->isMeta = 1;

    free(ch);

    /* begining of a paragraph */
  } else if(strcmp(name, "p") == 0) {
    ((struct ParserState *)user_data)->isTextContent = 1;

    /* begining of a footnote */
  } else if(strcmp(name, "foot") == 0) {
    ((struct ParserState *)user_data)->isNote = 1;

    /* begining of the textual content of a note */
  } else if(((struct ParserState *)user_data)->isNote == 1 && strcmp(name, "c") == 0) {
    ((struct ParserState *)user_data)->isNote = 2;
  }
}

void XMLCALL endElement(void *user_data, const char *name) {

  /* end of current metadata */
  if(((struct ParserState *)user_data)->isMeta == 1 && strcmp(name, "m") == 0) {
    ((struct ParserState *)user_data)->isMeta = 0;

    /* suspending parsing for metadata handling */
    ((struct ParserState *)user_data)->meta_suspended = 1;
    XML_StopParser( *((struct ParserState *)user_data)->pparser, 1);

    /*end of paragraph */
  } else if(strcmp(name, "p") == 0) {
    ((struct ParserState *)user_data)->isTextContent = 0;

    /* suspending parsing if a non empty paragraph has been parsed */
    if (((struct ParserState *)user_data)->chlen > 0) {
      ((struct ParserState *)user_data)->suspended = 1;
      XML_StopParser( *((struct ParserState *)user_data)->pparser, 1);
    }

    /* end of footnote */
  } else if(strncmp(name, "foot", 4) == 0) {
    ((struct ParserState *)user_data)->isNote = 0;
  }
}
