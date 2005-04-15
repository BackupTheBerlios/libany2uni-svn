/*
 *
 */

#include <stdio.h>
#include "internals.h"

extern size_t strlen(const char *s);
extern int memcmp(const void *s1, const void *s2, size_t n);


/* params : filename : the name of the document file
 *                     (with or without path)
 * return : the file extension, NULL of none
 */
char *getextension(char *filename) {
  int i;
  char *result = NULL;

  /* detecting the beginning of the extension */
  for (i=strlen(filename)-2; i>=0 && memcmp(filename + i, ".", 1); i--) {}
  
  /* setting the result */
  if (i >= 0){
    i++;
    result = filename + i;
  }
  return result;
}


/* params : filename : the name of the document file
 *                     (with or without path)
 * return : a code for the file format
 */
int format_detection(char *filename) {
  char *extension = NULL;

  /* getting the extension string */
  extension = getextension(filename);

  /* matching the extension to its code */
  if (extension == NULL) {
    return UNKNOWN;
  }
  if (!memcmp(extension, "abw", 3)) {
    return ABIWORD;
  } else if (!memcmp(extension, "scd", 3)
	     || !memcmp(extension, "sla", 3)) {
    return SCRIBUS;
  } else if (!memcmp(extension, "kwd", 3)) {
    return KWORD;
  } else if (!memcmp(extension, "ksp", 3)) {
    return KSPREAD;
  } else if (!memcmp(extension, "kpr", 3)) {
    return KPRESENTER;
  } else if (!memcmp(extension, "sxw", 3)) {
    return OOWRITE;
  } else if (!memcmp(extension, "sxc", 3)) {
    return OOCALC;
  } else if (!memcmp(extension, "sxi", 3)) {
    return OOIMPRESS;
  } else if (!memcmp(extension, "xml", 3)) {
    return XMLDOC;
  } else if (!memcmp(extension, "htm", 3)
	     || !memcmp(extension, "html", 4)) {
    return HTMLDOC;
  } else if (!memcmp(extension, "tex", 3)) {
    return LATEX;
  } else if (!memcmp(extension, "rtf", 3)) {
    return RTFDOC;
  } else if (!memcmp(extension, "doc", 3)) {
    return MSWORD;
  } else if (!memcmp(extension, "xls", 3)) {
    return MSEXCEL;
  } else if (!memcmp(extension, "ppt", 3)
	     || !memcmp(extension, "pps", 3)) {
    return MSPPT;
  } else if (!memcmp(extension, "pdf", 3)) {
    return PDFDOC;
  } else if (!memcmp(extension, "wp", 2)) {
    return WPDOC;
  } else if (!memcmp(extension, "qxd", 3)) {
    return QXPRESS;
  }
  return UNKNOWN;
}



