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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


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
  int fd;
  int len;
  char buf[1024];

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
  } else if (!memcmp(extension, "doc", 3) ||
	     !memcmp(extension, "rtf", 3)) {

    /* check file header */
    fd = open(filename, O_RDONLY);
    len = read(fd, buf, 1024);
    if((len >= 2 && !strncmp(buf, "\xDB\xA5", 2)) ||
       (len >= 8 && !strncmp(buf, "\xD0\xCF\x11\xE0\xA1\xB1\x1A\xE1", 8))) {
      return MSWORD;
    } else if(len >= 5 && !strncmp(buf, "{\\rtf", 5)) {
      return RTFDOC;
    } else {
      return UNKNOWN;
    }
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
  } else if (!memcmp(extension, "txt", 3)) {
    return TXT;
  } else if (!memcmp(extension, "mht", 3)) {
    return MHT;
  }
  return UNKNOWN;
}



