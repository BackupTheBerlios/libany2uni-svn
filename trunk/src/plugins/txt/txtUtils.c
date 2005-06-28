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
 * internal functions for txt plugin
 */

#include "p_txt.h"
#include <string.h>
#include <unicode/ustring.h>


/*
 * extracts text
 */
int getText(struct doc_descriptor *desc, char *out, int size) {
  char buf[BUFSIZE];
  int len, i, l;

  l = 0;
  len = read(desc->fd, buf, BUFSIZE);
  i = 0;
  memset(out, '\x00', size);
  while(len > 0 && strncmp(buf + i, "\n", 1) && strncmp(buf + i, "\r", 1)) {
    strncpy(out + l, buf + i, 1);
    l++;
    i++;
    if(i >= len) {
      len = read(desc->fd, buf, BUFSIZE);
      i = 0;
      if(!len) {
	if(l > 0) {
	  return l;
	} else {
	  return NO_MORE_DATA;
	}
      }
    }
    if(l >= size - 2) {
      lseek(desc->fd, i - len, SEEK_CUR);
      return l;
    }
  }

  if(l > 0) {
    lseek(desc->fd, i - len, SEEK_CUR);
    return l;
  } else if(len > 0 && (!strncmp(buf + i, "\n", 1) ||
			!strncmp(buf + i, "\r", 1))){
    lseek(desc->fd, i - len + 1, SEEK_CUR);
    return 0;
  } else {
    return NO_MORE_DATA;
  }
}


int initTxt(struct doc_descriptor *desc) {
  UErrorCode err;
  char *encoding = NULL;
  int len, BOMlength = 0;
  char buf[BUFSIZE];
  UChar outbuf[4*BUFSIZE];


  lseek(desc->fd, 0, SEEK_SET);
  len = read(desc->fd, buf, BUFSIZE);

  /* detect BOM */
  err = U_ZERO_ERROR;
  encoding = ucnv_detectUnicodeSignature(buf, BUFSIZE, &BOMlength, &err);
  if(encoding != NULL) {
    lseek(desc->fd, BOMlength, SEEK_SET);

    /* initialize converter to encoding */
    err = U_ZERO_ERROR;
    desc->conv = ucnv_open(encoding, &err);
    if (U_FAILURE(err)) {
      fprintf(stderr, "unable to open ICU converter\n");
      return ERR_ICU;
    }
    
  } else {
    /* initialize converter to UTF-8 */
    err = U_ZERO_ERROR;
    desc->conv = ucnv_open("utf8", &err);
    if (U_FAILURE(err)) {
      fprintf(stderr, "unable to open ICU converter\n");
      return ERR_ICU;
    }

    /* check the first 2048 bytes */
    err = U_ZERO_ERROR;
    ucnv_setToUCallBack(desc->conv, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL, &err);
    if (U_FAILURE(err)) {
      fprintf(stderr, "error setToUCallback\n");
      return ERR_ICU;
    }
    err = U_ZERO_ERROR;
    ucnv_toUChars(desc->conv, outbuf, 4 * BUFSIZE, buf, len, &err);
    if (U_FAILURE(err)) {
      fprintf(stderr, "Unknown encoding\n");
      return ERR_ICU;
    }
    lseek(desc->fd, 0, SEEK_SET);
  }

  return OK;
}
