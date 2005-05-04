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
  } else if (!memcmp(extension, "sxd", 3)) {
    return OODRAW;
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



