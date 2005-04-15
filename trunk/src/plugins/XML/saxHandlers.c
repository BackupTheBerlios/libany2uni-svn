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

  if (strncmp(ch, "\n", 1) && strncmp(ch, "\t", 1) && strncmp(ch, " ", 1)){
    ch2 = (char *) malloc(len + 1);
    strncpy(ch2, ch, len);
    strncpy(ch2 + len, "\0", 1);
    if (((struct ParserState *)user_data)->chlen + strlen(ch2) >= INTERNAL_BUFSIZE) {
      strncpy(ch2 + INTERNAL_BUFSIZE - ((struct ParserState *)user_data)->chlen, "\0", 1);
    }
    sprintf(((struct ParserState *)user_data)->ch +
	    ((struct ParserState *)user_data)->chlen, "%s", ch2);
    ((struct ParserState *)user_data)->chlen += strlen(ch2);

    free(ch2);
  }
}

void XMLCALL startElement(void *user_data, const char *name, const char **attrs) {

}

void XMLCALL endElement(void *user_data, const char *name) {
  if( ((struct ParserState *)user_data)->chlen > 0) {
    ((struct ParserState *)user_data)->suspended = 1;
    XML_StopParser( *((struct ParserState *)user_data)->pparser, 1);
  }

}
