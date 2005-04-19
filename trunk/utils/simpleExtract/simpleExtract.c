/*
 * tool for text extraction in utf8
 * usage : ./simpleExtract <document>
 */

#include "../../src/userInterface.h"
#include <unicode/utypes.h>
#include <unicode/ustring.h>
#include <string.h>

#define METADATA 1
#define DEFAULT  0

void usage() {
  printf("wrong command\nusage : ./simpleExtract -[option] <document>\n");
  printf("   options :     -m : get metadata\n");
}

int main(int argc, char *argv[]) {
  struct doc_descriptor d;
  UChar ubuf[10000];
  UErrorCode err;
  char *out;
  int32_t size;
  int nb_options, mode, first;
  struct meta meta;

  mode = DEFAULT;

  if (argc < 2) {
    usage();
    exit(0);
  }

  nb_options = 0;

  if(strncmp(argv[1], "-", 1) == 0) {
    if (strcmp(argv[1], "-m") == 0) {
      nb_options++;
      mode = METADATA;
    } else {
      usage();
      exit(0);
    }
  }

  if(argc != 2 + nb_options) {
    usage();
    exit(0);
  }

  if (openDocument(argv[nb_options + 1], &d)) {
    printf("error openDocument\n");
    exit(0);
  }


  if(mode == DEFAULT) {

    /* read next paragraph */
    while (read_content(&d, ubuf) >= 0) {
      
      /* get the needed size for output string */
      u_strToUTF8(NULL, 0, &size, ubuf, u_strlen(ubuf), &err);
      /* allocate output string with 1 more character for termination */
      out = (char *) malloc(size+1);
      
      /* convert utf16 to utf8 */
      err = U_ZERO_ERROR;
      u_strToUTF8(out, size+1, NULL, ubuf, u_strlen(ubuf), &err);
      if (U_FAILURE(err)) {
	printf("error ICU %d \n", err);
	exit(0);
      }
      
      /* printing result to standard output */
      printf("%s ", out);
      
      free(out);    
    }
  } else if (mode == METADATA) {
    
    /* reading the whole file first to ensure that
       all present metadata are found */
    while (read_content(&d, ubuf) >= 0) {}


    first = 1;
    /* printing metadata to standard output */
    while(read_meta(&d, &meta) != NO_MORE_META) {
      if( !first) {
	printf(",");
      }
      first = 0;

      /* get the needed size for output string */
      u_strToUTF8(NULL, 0, &size, meta.name, meta.name_length, &err);

      /* allocate output string with 1 more character for termination */
      out = (char *) malloc(size+1);
      
      /* convert utf16 to utf8 */
      err = U_ZERO_ERROR;
      u_strToUTF8(out, size+1, NULL, meta.name, meta.name_length, &err);
      if (U_FAILURE(err)) {
	printf("error ICU %d \n", err);
	exit(0);
      }
      
      /* printing name to standard output */
      printf("%s", out);
      
      free(out);    

      /* get the needed size for output string */
      u_strToUTF8(NULL, 0, &size, meta.value, meta.value_length, &err);

      /* allocate output string with 1 more character for termination */
      out = (char *) malloc(size+1);
      
      /* convert utf16 to utf8 */
      err = U_ZERO_ERROR;
      u_strToUTF8(out, size+1, NULL, meta.value, meta.value_length, &err);
      if (U_FAILURE(err)) {
	printf("error ICU %d \n", err);
	exit(0);
      }
      
      /* printing result to standard output */
      printf(":%s", out);
      
      free(out);    
      
    }

  }

  if (closeDocument(&d)) {
    printf("error closeDocument\n");
  }

  return 0;
}