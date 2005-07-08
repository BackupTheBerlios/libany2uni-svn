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

#include "p_mht.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#define BUFSIZE 2048

int initReader(struct doc_descriptor *desc) {
  struct mhtState *state = (struct mhtState *)(desc->myState);
  UErrorCode err;
  char encoding[40];
  int i;

  /* search Content-Type */
  state->len = read(desc->fd, state->buf, BUFSIZE);
  state->cursor = 0;
  while(strncmp(state->buf + state->cursor, "Content-Type", 12)) {

    if(getNextLine(desc)) {
      fprintf(stderr, "Unexpected end of file\n");
      return INIT_ERROR;
    }

    if(state->cursor >= state->len - 12) {
      strncpy(state->buf, state->buf + state->cursor,
	      state->len - state->cursor);
      state->len = read(desc->fd, state->buf + state->len - state->cursor,
			BUFSIZE - state->len + state->cursor)
	+ state->len - state->cursor;
      if(state->len == 0) {
	fprintf(stderr, "Unexpected end of file\n");
	return INIT_ERROR;
      }
      state->cursor = 0;
    }
  }
  state->cursor += 12;
  if(state->cursor >= state->len - 12) {
    strncpy(state->buf, state->buf + state->cursor,
	    state->len - state->cursor);
    state->len = read(desc->fd, state->buf + state->len - state->cursor,
		      BUFSIZE - state->len + state->cursor)
      + state->len - state->cursor;
    if(state->len == 0) {
      fprintf(stderr, "Unexpected end of file\n");
      return INIT_ERROR;
    }
    state->cursor = 0;
  }
  state->cursor += 2;
  state->monopart = 0;
  if(!strncmp(state->buf + state->cursor, "text/html", 9)) {

    /* unique html file */
    state->monopart = 1;

    /* get encoding */
    for( ; strncmp(state->buf + state->cursor, "charset=\"", 9) &&
	   strncmp(state->buf + state->cursor, "Content-", 8);
	 state->cursor++) {
      if(state->cursor >= state->len - 9) {
	strncpy(state->buf, state->buf + state->cursor,
		state->len - state->cursor);
	state->len = read(desc->fd, state->buf + state->len - state->cursor,
			  BUFSIZE - state->len + state->cursor)
	  + state->len - state->cursor;
	if(state->len == 0) {
	  fprintf(stderr, "Unexpected end of file\n");
	  return INIT_ERROR;
	}
	state->cursor = 0;
      }
    }  
    if(strncmp(state->buf + state->cursor, "charset=\"", 9)) {
      fprintf(stderr, "no charset found\n");
      return ERR_KEYWORD;
    }
    state->cursor += 9;
    
    memset(encoding, '\x00', 40);
    for(i = 0; strncmp(state->buf + state->cursor, "\"", 1);
	i++, state->cursor++) {
      if(state->cursor >= state->len) {
	state->len = read(desc->fd, state->buf, BUFSIZE);
	if(state->len == 0) {
	  fprintf(stderr, "Unexpected end of file\n");
	  return INIT_ERROR;
	}
	state->cursor = 0;
      }    
      strncpy(encoding + i, state->buf + state->cursor, 1);
    }

    /* initialize converter */
    err = U_ZERO_ERROR;
    desc->conv = ucnv_open(encoding, &err);
    if (U_FAILURE(err)) {
      fprintf(stderr, "Unable to open ICU converter\n");
      return ERR_ICU;
    }
    
    /* go to html beginning */
    if(state->cursor >= state->len - 14) {
      strncpy(state->buf, state->buf + state->cursor,
	      state->len - state->cursor);
      state->len = read(desc->fd, state->buf + state->len - state->cursor,
			BUFSIZE - state->len + state->cursor)
	+ state->len - state->cursor;
      if(state->len == 0) {
	fprintf(stderr, "Unexpected end of file\n");
	return INIT_ERROR;
      }
      state->cursor = 0;
    }
    for( ; strncmp(state->buf + state->cursor, "<!DOCTYPE HTML", 14);
	 state->cursor++) {
      if(state->cursor >= state->len - 14) {
	strncpy(state->buf, state->buf + state->cursor,
		state->len - state->cursor);
	state->len = read(desc->fd, state->buf + state->len - state->cursor,
			  BUFSIZE - state->len + state->cursor)
	  + state->len - state->cursor;
	if(state->len == 0) {
	  fprintf(stderr, "Unexpected end of file\n");
	  return INIT_ERROR;
	}
	state->cursor = 0;
      }
    }    
    
    /* search 'test/html' part */
  } else if(getNextHTMLpart(desc)) {
    return INIT_ERROR;
  }

  return OK;
}


int getText(struct doc_descriptor *desc, UChar *buf, int size) {
  struct mhtState *state = (struct mhtState *)(desc->myState);
  UErrorCode err;
  char *src;
  UChar *dest, esc[3];
  char tmp[2];
  int l, i, v;

  l = 0;
  memset(buf, '\x00', size);
  if(state->cursor >= state->len - 7) {
    strncpy(state->buf, state->buf + state->cursor,
	    state->len - state->cursor);
    state->len = read(desc->fd, state->buf + state->len - state->cursor,
		      BUFSIZE - state->len + state->cursor)
      + state->len - state->cursor;
    if(state->len == 0) {
      fprintf(stderr, "Unexpected end of file\n");
      return INIT_ERROR;
    }
    state->cursor = 0;
  }
  if(!strncmp(state->buf + state->cursor, "</HTML>", 7) ||
     !strncmp(state->buf + state->cursor, "</html>", 7)) {
    if(state->monopart || getNextHTMLpart) {
      return NO_MORE_DATA;
    }
  }

  while(strncmp(state->buf + state->cursor, "</HTML>", 7) &&
	strncmp(state->buf + state->cursor, "</html>", 7)) {

    /* skip javascript */
    if(!strncmp(state->buf + state->cursor, "<SCRIPT", 7) ||
       !strncmp(state->buf + state->cursor, "<script", 7)) {
      while(strncmp(state->buf + state->cursor, "</SCRIPT>", 9) &&
	    strncmp(state->buf + state->cursor, "</script>", 9)) {
	state->cursor++;
	if(state->cursor >= state->len - 9) {
	  strncpy(state->buf, state->buf + state->cursor,
		  state->len - state->cursor);
	  state->len = read(desc->fd, state->buf + state->len - state->cursor,
			    BUFSIZE - state->len + state->cursor)
	    + state->len - state->cursor;
	  if(state->len == 0) {
	    fprintf(stderr, "Unexpected end of file\n");
	    return INIT_ERROR;
	  }
	  state->cursor = 0;
	}
      }
      state->cursor += 9;
      if(state->cursor >= state->len - 7) {
	strncpy(state->buf, state->buf + state->cursor,
		state->len - state->cursor);
	state->len = read(desc->fd, state->buf + state->len - state->cursor,
			  BUFSIZE - state->len + state->cursor)
	  + state->len - state->cursor;
	if(state->len == 0) {
	  fprintf(stderr, "Unexpected end of file\n");
	  return INIT_ERROR;
	}
	state->cursor = 0;
      }

      /* end of paragraph */
    } else if(!strncmp(state->buf + state->cursor, "<P ", 3) ||
	      !strncmp(state->buf + state->cursor, "<p ", 3)) {
      while(strncmp(state->buf + state->cursor, ">", 1)) {
	state->cursor++;
	if(state->cursor >= state->len - 7) {
	  strncpy(state->buf, state->buf + state->cursor,
		  state->len - state->cursor);
	  state->len = read(desc->fd, state->buf + state->len - state->cursor,
			    BUFSIZE - state->len + state->cursor)
	    + state->len - state->cursor;
	  if(state->len == 0) {
	    fprintf(stderr, "Unexpected end of file\n");
	    return INIT_ERROR;
	  }
	  state->cursor = 0;
	}
      }
      state->cursor++;
      if(state->cursor >= state->len - 7) {
	strncpy(state->buf, state->buf + state->cursor,
		state->len - state->cursor);
	state->len = read(desc->fd, state->buf + state->len - state->cursor,
			  BUFSIZE - state->len + state->cursor)
	  + state->len - state->cursor;
	if(state->len == 0) {
	  fprintf(stderr, "Unexpected end of file\n");
	  return INIT_ERROR;
	}
	state->cursor = 0;
      }
      if(l > 0) {
	memcpy(buf + l/2, "\x20\x00", 1);
	l += 2;
	return l;
      }

      /* new line becomes space */
    } else if(!strncmp(state->buf + state->cursor, "<BR", 3) ||
	      !strncmp(state->buf + state->cursor, "<br", 3)) {
      while(strncmp(state->buf + state->cursor, ">", 1)) {
	state->cursor++;
	if(state->cursor >= state->len - 7) {
	  strncpy(state->buf, state->buf + state->cursor,
		  state->len - state->cursor);
	  state->len = read(desc->fd, state->buf + state->len - state->cursor,
			    BUFSIZE - state->len + state->cursor)
	    + state->len - state->cursor;
	  if(state->len == 0) {
	    fprintf(stderr, "Unexpected end of file\n");
	    return INIT_ERROR;
	  }
	  state->cursor = 0;
	}
      }
      state->cursor++;
      if(state->cursor >= state->len - 7) {
	strncpy(state->buf, state->buf + state->cursor,
		state->len - state->cursor);
	state->len = read(desc->fd, state->buf + state->len - state->cursor,
			  BUFSIZE - state->len + state->cursor)
	  + state->len - state->cursor;
	if(state->len == 0) {
	  fprintf(stderr, "Unexpected end of file\n");
	  return INIT_ERROR;
	}
	state->cursor = 0;
      }

      memcpy(buf + l/2, "\x20\x00", 2);
      l += 2;
      if(l >= size - 2) {
	return l;
      }


      /* skip markups */
    } else if(!strncmp(state->buf + state->cursor, "<", 1)) {
      while(strncmp(state->buf + state->cursor, ">", 1)) {
	state->cursor++;
	if(state->cursor >= state->len - 7) {
	  strncpy(state->buf, state->buf + state->cursor,
		  state->len - state->cursor);
	  state->len = read(desc->fd, state->buf + state->len - state->cursor,
			    BUFSIZE - state->len + state->cursor)
	    + state->len - state->cursor;
	  if(state->len == 0) {
	    fprintf(stderr, "Unexpected end of file\n");
	    return INIT_ERROR;
	  }
	  state->cursor = 0;
	}
      }
      state->cursor++;
      if(state->cursor >= state->len - 7) {
	strncpy(state->buf, state->buf + state->cursor,
		state->len - state->cursor);
	state->len = read(desc->fd, state->buf + state->len - state->cursor,
			  BUFSIZE - state->len + state->cursor)
	  + state->len - state->cursor;
	if(state->len == 0) {
	  fprintf(stderr, "Unexpected end of file\n");
	  return INIT_ERROR;
	}
	state->cursor = 0;
      }
      
      /* unicode character */
    } else if(!strncmp(state->buf + state->cursor, "=", 1)) {
      state->cursor++;
      if(isalnum(state->buf[state->cursor])) {
	for(v = 0, i = 0; i < 2; i++) {
	  v *= 16;
	  strncpy(tmp, state->buf + state->cursor, 1);
	  strncpy(tmp + 1, "\0", 1);
	  state->cursor++;
	  if(islower(tmp[0])) {
	    v += tmp[0] - 87;
	  } else if(isupper(tmp[0])) {
	    v += tmp[0] - 55;
	  } else {
	    v += atoi(tmp);
	  }
	}
	buf[l/2] = v;
	l += 2;
	if(l >= size - 2) {
	  memcpy(buf + l, "\x20\x00", 2);
	  l += 2;
	  return l;
	}

      } else {
	state->cursor++;
      }

      /* escape character */
    } else if(!strncmp(state->buf + state->cursor, "&", 1)) {
      if(state->cursor >= state->len - 8) {
	strncpy(state->buf, state->buf + state->cursor,
		state->len - state->cursor);
	state->len = read(desc->fd, state->buf + state->len - state->cursor,
			  BUFSIZE - state->len + state->cursor)
	  + state->len - state->cursor;
	if(state->len == 0) {
	  fprintf(stderr, "Unexpected end of file\n");
	  return INIT_ERROR;
	}
	state->cursor = 0;
      }
      memset(esc, '\x00', 6);
      state->cursor += escapeChar(state->buf + state->cursor, esc);
      memcpy(buf + l/2, esc, 2*u_strlen(esc));
      l += 2*u_strlen(esc);
      if(l >= size - 2) {
	memcpy(buf + l, "\x20\x00", 2);
	l += 2;
	return l;
      }

      /* text content */
    } else {
      if(strncmp(state->buf + state->cursor, "\x0A", 1) &&
	 strncmp(state->buf + state->cursor, "\x0D", 1)) {

	dest = buf + l/2;
	src = state->buf + state->cursor;
	err = U_ZERO_ERROR;
	ucnv_toUnicode(desc->conv, &dest, buf + size/2,
		       &src, state->buf + state->cursor + 1,
		       NULL, FALSE, &err);
	if (U_FAILURE(err)) {
	  fprintf(stderr, "Unable to convert buffer\n");
	  return ERR_ICU;
	}
	l+= 2*(dest - buf - l/2);

	if(l >= size - 2) {
	  memcpy(buf + l, "\x20\x00", 2);
	  l += 2;
	  return l;
	}
      } else {
	memcpy(buf + l, "\x20\x00", 2);
	l += 2;
      }
      state->cursor++;
      if(state->cursor >= state->len - 7) {
	strncpy(state->buf, state->buf + state->cursor,
		state->len - state->cursor);
	state->len = read(desc->fd, state->buf + state->len - state->cursor,
			  BUFSIZE - state->len + state->cursor)
	  + state->len - state->cursor;
	if(state->len == 0) {
	  fprintf(stderr, "Unexpected end of file\n");
	  return INIT_ERROR;
	}
	state->cursor = 0;
      }
    }
  }

  if(l > 0) {
    memcpy(buf + l, "\x20\x00", 2);
    l += 2;
  }
  return l;
}


int getNextLine(struct doc_descriptor *desc) {
  struct mhtState *state = (struct mhtState *)(desc->myState);

  /* skipping non EOL characters */
  for(; strncmp(state->buf + state->cursor, "\x0A", 1) &&
	strncmp(state->buf + state->cursor, "\x0D", 1);
      state->cursor++) {
    if(state->cursor >= state->len) {
      state->len = read(desc->fd, state->buf, BUFSIZE);
      if(state->len == 0) {
	return NO_MORE_DATA;
      }
      state->cursor = 0;
    }
  }

  /* skipping EOL characters */
  for( ;(!strncmp(state->buf + state->cursor, "\x0A", 1) ||
	 !strncmp(state->buf + state->cursor, "\x0D", 1));
       state->cursor++){
    if(state->cursor >= state->len) {
      state->len = read(desc->fd, state->buf, BUFSIZE);
      if(state->len == 0) {
	return NO_MORE_DATA;
      }
      state->cursor = 0;
    }
  }

  return OK;

}


int getNextHTMLpart(struct doc_descriptor *desc) {
  struct mhtState *state = (struct mhtState *)(desc->myState);
  UErrorCode err;
  int i;
  char encoding[40];

  do {
    if(state->cursor >= state->len - 15) {
      strncpy(state->buf, state->buf + state->cursor,
	      state->len - state->cursor);
      state->len = read(desc->fd, state->buf + state->len - state->cursor,
			BUFSIZE - state->len + state->cursor)
	+ state->len - state->cursor;
      if(state->len == 0) {
	return NO_MORE_DATA;
      }
      state->cursor = 0;
    }    

    /* search next part */
    for( ; strncmp(state->buf + state->cursor, "----=_NextPart_", 15);
	 state->cursor++){
      if(state->cursor >= state->len - 15) {
	strncpy(state->buf, state->buf + state->cursor,
		state->len - state->cursor);
	state->len = read(desc->fd, state->buf + state->len - state->cursor,
			  BUFSIZE - state->len + state->cursor)
	  + state->len - state->cursor;
	if(state->len == 0) {
	  return NO_MORE_DATA;
	}
	state->cursor = 0;
      }    
    }

    /* check content-type */
    if(getNextLine(desc)) {
      return NO_MORE_DATA;
    }
    if(state->cursor >= state->len - 14) {
      strncpy(state->buf, state->buf + state->cursor,
	      state->len - state->cursor);
      state->len = read(desc->fd, state->buf + state->len - state->cursor,
			BUFSIZE - state->len + state->cursor)
	+ state->len - state->cursor;
      if(state->len == 0) {
	return NO_MORE_DATA;
      }
      state->cursor = 0;
    }    
    while(strncmp(state->buf + state->cursor, "Content-Type: ", 14)) {
      if(getNextLine(desc)) {
	return NO_MORE_DATA;
      }
      if(state->cursor >= state->len - 14) {
	strncpy(state->buf, state->buf + state->cursor,
		state->len - state->cursor);
	state->len = read(desc->fd, state->buf + state->len - state->cursor,
			  BUFSIZE - state->len + state->cursor)
	  + state->len - state->cursor;
	if(state->len == 0) {
	  return NO_MORE_DATA;
	}
	state->cursor = 0;
      }        
    }
    state->cursor += 14;

    if(state->cursor >= state->len - 9) {
      strncpy(state->buf, state->buf + state->cursor,
	      state->len - state->cursor);
      state->len = read(desc->fd, state->buf + state->len - state->cursor,
			BUFSIZE - state->len + state->cursor)
	+ state->len - state->cursor;
      if(state->len == 0) {
	return NO_MORE_DATA;
      }
      state->cursor = 0;
    }        
  } while(state->len && strncmp(state->buf + state->cursor, "text/html", 9));
  if(!state->len) {
    return NO_MORE_DATA;
  }

  /* get encoding */
  if(getNextLine(desc)) {
    return NO_MORE_DATA;
  }
  for( ; strncmp(state->buf + state->cursor, "charset=\"", 9) &&
	 strncmp(state->buf + state->cursor, "Content-", 8);
       state->cursor++) {
    if(state->cursor >= state->len - 9) {
      strncpy(state->buf, state->buf + state->cursor,
	      state->len - state->cursor);
      state->len = read(desc->fd, state->buf + state->len - state->cursor,
			BUFSIZE - state->len + state->cursor)
	+ state->len - state->cursor;
      if(state->len == 0) {
	return NO_MORE_DATA;
      }
      state->cursor = 0;
    }
  }  
  if(strncmp(state->buf + state->cursor, "charset=\"", 9)) {
    fprintf(stderr, "no charset found\n");
    return ERR_KEYWORD;
  }
  state->cursor += 9;

  memset(encoding, '\x00', 40);
  for(i = 0; strncmp(state->buf + state->cursor, "\"", 1);
      i++, state->cursor++) {
    if(state->cursor >= state->len) {
      state->len = read(desc->fd, state->buf, BUFSIZE);
      if(state->len == 0) {
	return NO_MORE_DATA;
      }
      state->cursor = 0;
    }    
    strncpy(encoding + i, state->buf + state->cursor, 1);
  }

  /* initialize converter */
  err = U_ZERO_ERROR;
  desc->conv = ucnv_open(encoding, &err);
  if (U_FAILURE(err)) {
    fprintf(stderr, "Unable to open ICU converter\n");
    return ERR_ICU;
  }

  /* go to html beginning */
  if(state->cursor >= state->len - 14) {
    strncpy(state->buf, state->buf + state->cursor,
	    state->len - state->cursor);
    state->len = read(desc->fd, state->buf + state->len - state->cursor,
		      BUFSIZE - state->len + state->cursor)
      + state->len - state->cursor;
    if(state->len == 0) {
      return NO_MORE_DATA;
    }
    state->cursor = 0;
  }
  for( ; strncmp(state->buf + state->cursor, "<!DOCTYPE HTML", 14);
       state->cursor++) {
    if(state->cursor >= state->len - 14) {
      strncpy(state->buf, state->buf + state->cursor,
	      state->len - state->cursor);
      state->len = read(desc->fd, state->buf + state->len - state->cursor,
			BUFSIZE - state->len + state->cursor)
	+ state->len - state->cursor;
      if(state->len == 0) {
	return NO_MORE_DATA;
      }
      state->cursor = 0;
    }
  }

  return OK;
}


int escapeChar(char *buf, UChar *res) {
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
