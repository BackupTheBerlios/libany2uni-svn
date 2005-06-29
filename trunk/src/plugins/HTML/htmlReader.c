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

#include "p_html.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BUFSIZE 2048


int escapeChar(struct doc_descriptor *desc, char *buf, UChar *res) {
  char token[9];
  int i;

  /* copying token into local buffer */
  i = 0;
  while(i<=8 && strncmp(buf + i, ";", 1)) {
    strncpy(token + i, buf + i, 1);
    i++;
  }
  if (strncmp(buf + i, ";\0", 2)) {
    strncpy(token + i, buf + i, 1);   
  } else {

    /* if it does not seem to be a token, result is '&' */
    memcpy(res, "\x26\x00", 2);
    return 1;
  }

  /* identifying token */
  if (!strncmp(token, "&amp;", 5)) {
    memcpy(res, "\x26\x00", 2);
    return 5;
  } else if (!strncmp(token, "&lt;", 4)) {
    memcpy(res, "\x3C\x00", 2);
    return 4;
  } else if (!strncmp(token, "&gt;", 4)) {
    memcpy(res, "\x3E\x00", 2);
    return 4;
  } else if (!strncmp(token, "&quot;", 6)) {
    memcpy(res, "\x22\x00", 2);
    return 6;
  } else if (!strncmp(token, "&eacute;", 8)) {
    memcpy(res, "\xE9\x00", 2);
    return 8;
  } else if (!strncmp(token, "&Eacute;", 8)) {
    memcpy(res, "\xC9\x00", 2);
    return 8;
  } else if (!strncmp(token, "&egrave;", 8)) {
    memcpy(res, "\xE8\x00", 2);
    return 8;
  } else if (!strncmp(token, "&Egrave;", 8)) {
    memcpy(res, "\xC8\x00", 2);
    return 8;
  } else if (!strncmp(token, "&ecirc;", 7)) {
    memcpy(res, "\xEA\x00", 2);
    return 7;
  } else if (!strncmp(token, "&agrave;", 8)) {
    memcpy(res, "\xE0\x00", 2);
    return 8;
  } else if (!strncmp(token, "&iuml;", 6)) {
    memcpy(res, "\xEF\x00", 2);
    return 6;
  } else if (!strncmp(token, "&ccedil;", 8)) {
    memcpy(res, "\xE7\x00", 2);
    return 8;
  } else if (!strncmp(token, "&ntilde;", 8)) {
    memcpy(res, "\xF1\x00", 2);
    return 8;
  } else if (!strncmp(token, "&copy;", 6)) {
    memcpy(res, "\xA9\x00", 2);
    return 6;
  } else if (!strncmp(token, "&reg;", 5)) {
    memcpy(res, "\xAE\x00", 2);
    return 5;
  } else if (!strncmp(token, "&deg;", 5)) {
    memcpy(res, "\xB0\x00", 2);
    return 5;
  } else if (!strncmp(token, "&ordm;", 6)) {
    memcpy(res, "\xBA\x00", 2);
    return 6;
  } else if (!strncmp(token, "&laquo;", 7)) {
    memcpy(res, "\xAB\x00", 2);
    return 7;
  } else if (!strncmp(token, "&raquo;", 7)) {
    memcpy(res, "\xBB\x00", 2);
    return 7;
  } else if (!strncmp(token, "&micro;", 7)) {
    memcpy(res, "\xB5\x00", 2);
    return 7;
  } else if (!strncmp(token, "&para;", 6)) {
    memcpy(res, "\xB6\x00", 2);
    return 6;
  } else if (!strncmp(token, "&frac14;", 8)) {
    memcpy(res, "\xBC\x00", 2);
    return 8;
  } else if (!strncmp(token, "&frac12;", 8)) {
    memcpy(res, "\xBD\x00", 2);
    return 8;
  } else if (!strncmp(token, "&frac34;", 8)) {
    memcpy(res, "\xBE\x00", 2);
    return 8;
  } else if (!strncmp(token, "&#", 2)) {
    res[0] = atoi(token + 2);
    return 6;
  } else {
    memcpy(res, "\x20\x00", 2);
    return i+1;
  }
}

int getText(struct doc_descriptor *desc, UChar *buf, int size) {
  char buf2[BUFSIZE];
  UErrorCode err;
  char *src;
  UChar *dest, esc[3];;
  int len, i, isMarkup, isJavascript, l;
  int dangerousCut, fini, r, offset, endOfFile, space_added;

  space_added = 0;
  l = 0;
  fini = 0;
  endOfFile = 0;
  isJavascript = 0;
  dangerousCut = 0;
  isMarkup = 0;
  len = read(desc->fd, buf2, BUFSIZE);
  while (!fini && len > 0 && l < size - 2){

    /* consuming buffer */
    for (i = 0; l < size - 2 && i < len && !dangerousCut && !fini; i++) {

      /* end of buffer are possible points of failure
	 if a markup or a token is cut, it will not be
	 parsed. */
      if (!endOfFile && i > len - 9 && ( !strncmp(buf2 + i, "\x3c", 1) ||
					 !strncmp(buf2 + i, "\x26", 1) )) {
	dangerousCut = 1;
	break;
      }
	
      /* detecting end of javascript */
      if (isJavascript && (!strncmp(buf2 + i, "</script>", 9))) {
	isJavascript = 0;
	i += 9;
      }

      /* detecting new paragraph */
      if(l > 0 && !isJavascript && (!strncmp(buf2 + i, "<p", 2))) {
	fini = 1;
	i+=2;
	while( strncmp(buf2 + i, ">", 1) ) {
	  i++;
	}
	lseek(desc->fd, i - len, SEEK_CUR);
	break;
      }

      /* detecting begining of markup */
      if(!isJavascript && !isMarkup && !strncmp(buf2 + i, "\x3c", 1)) {

	/* detecting begining of javascript */
	if (!strncmp(buf2 + i, "<script", 7)) {
	  isJavascript = 1;
	} else {

	  isMarkup = 1;
	}
      }

      /* detecting end of markup */
      if (!isJavascript && isMarkup && !strncmp(buf2 + i, "\x3e", 1)) {
	if(!space_added && l > 0) {
	  memcpy(buf + l/2, "\x20\x00", 2);
	  l+=2;
	  space_added = 1;
	}
	isMarkup = 0;
      }

      /* handling text */
      if (!isJavascript && !isMarkup && strncmp(buf2 + i, "\x3e", 1)) {

	/* skipping blancs */
	if (strncmp(buf2 + i, "\n", 1) &&
	    strncmp(buf2 + i, "\t", 1) ) {
	  
	  /* converting tokens */
	  if (!isJavascript && !isMarkup && !strncmp(buf2 + i, "\x26", 1)) {
	    memset(esc, '\x00', 6);
	    offset = escapeChar(desc, buf2 + i, esc);
	    if (memcmp(esc, "\x20\x00", u_strlen(esc))) {
	      memcpy(buf + l/2, esc, 2*u_strlen(esc));
	      l+=2*u_strlen(esc);
	      space_added = 0;
	    }
	    i += (offset - 1);
	  } else {

	    /* filling output buffer */
	    dest = buf + l/2;
	    src = buf2 + i;
	    err = U_ZERO_ERROR;
	    ucnv_toUnicode(desc->conv, &dest, buf + size/2,
			    &src, buf2 + i + 1, NULL, FALSE, &err);
	    if (U_FAILURE(err)) {
	      fprintf(stderr, "Unable to convert buffer\n");
	      return ERR_ICU;
	    }
	    l+= 2*(dest - buf - l/2);
	    space_added = 0;
	  }
	}
      }
    }

    /* filling new buffer correctly */
    if (!fini) {
      if (dangerousCut) {
	r = len -i;
	strncpy(buf2, buf2 + i, r);
	len = read(desc->fd, buf2 + r, BUFSIZE - r) + r;
	if ( len < 9 ) {
	  endOfFile = 1;
	}
	dangerousCut = 0;
      } else {
	len = read(desc->fd, buf2, BUFSIZE);
      }
    }

  }

  /* ending buffer properly */
  if ( l > 0 ) {
    memcpy(buf + l, "\x00\x00", 2);
  return l;
  }

  if(len == 0) {
    return NO_MORE_DATA;
  }

  return l;
}

int getEncoding(int fd, char *encoding) {
  int i, len, none, r;
  char buf[2048];
  
  none = 0;
  i = 0;

  /* search for 'charset' in header */
  len = read(fd, buf, BUFSIZE);
  while(len > 0 && !none && strncmp(buf + i, "charset=", 8)
	&& strncmp(buf + i, "CHARSET=", 8)) {
    if (len - i < 8) {
      r = len - i;
      strncpy(buf, buf + i, r);
      len = read(fd, buf + r, BUFSIZE - r) + r;
      i = 0;
      break;
    }
    if (!strncmp(buf + i, "<body", 5) || !strncmp(buf + i, "<BODY", 5)) {
      none = 1;
    }
    i++;
  }
  if (!none && len > 0) {
    len = 0;
    i += 8;

    /* copy charset */
    while ( strncmp(buf + i + len, "\x22", 1)) {
      len ++;
    }
    strncpy(encoding, buf + i, len);
    strncpy(encoding + len, "\0", 1);

    /* default charset is US-ASCII */
  } else {
    strncpy(encoding, "US-ASCII\0", 9);
  }

  lseek(fd, 0, SEEK_SET);

  return OK;
}
