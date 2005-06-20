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


int getText(struct doc_descriptor *desc, char *buf, int size) {
  struct mhtState *state = (struct mhtState *)(desc->myState);
  char esc[1], tmp[2];
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
      state->cursor+=9;
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
	strncpy(buf + l, " ", 1);
	l++;
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

      strncpy(buf + l, " ", 1);
      l++;
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
	sprintf(buf + l, "%c", v);
	l++;
	if(l >= size - 2) {
	  strncpy(buf + l, " ", 1);
	  l++;
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
      state->cursor += escapeChar(state->buf + state->cursor, esc);
      strncpy(buf + l, esc, 1);
      l++;
      if(l >= size - 2) {
	strncpy(buf + l, " ", 1);
	l++;
	return l;
      }

      /* text content */
    } else {
      if(strncmp(state->buf + state->cursor, "\x0A", 1) &&
	 strncmp(state->buf + state->cursor, "\x0D", 1)) {
	strncpy(buf + l, state->buf + state->cursor, 1);
	l++;
	if(l >= size - 2) {
	  strncpy(buf + l, " ", 1);
	  l++;
	  return l;
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
    }
  }

  if(l > 0) {
    strncpy(buf + l, " ", 1);
    l++;
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


int escapeChar(char *buf, char *res) {
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
    strncpy(res, "&", 1);
    return 1;
  }

  /* identifying token */
  if (!strncmp(token, "&amp;", 5)) {
    strncpy(res, "&", 1);
    return 5;
  } else if (!strncmp(token, "&lt;", 4)) {
    strncpy(res, "<", 1);
    return 4;
  } else if (!strncmp(token, "&gt;", 4)) {
    strncpy(res, ">", 1);
    return 4;
  } else if (!strncmp(token, "&quot;", 6)) {
    strncpy(res, "\x22", 1);
    return 6;
  } else if (!strncmp(token, "&eacute;", 8)) {
    strncpy(res, "\xe9", 1);
    return 8;
  } else if (!strncmp(token, "&Eacute;", 8)) {
    strncpy(res, "\xc9", 1);
    return 8;
  } else if (!strncmp(token, "&egrave;", 8)) {
    strncpy(res, "\xe8", 1);
    return 8;
  } else if (!strncmp(token, "&ecirc;", 7)) {
    strncpy(res, "\xea", 1);
    return 7;
  } else if (!strncmp(token, "&agrave;", 8)) {
    strncpy(res, "\xe0", 1);
    return 8;
  } else if (!strncmp(token, "&iuml;", 6)) {
    strncpy(res, "\xef", 1);
    return 6;
  } else if (!strncmp(token, "&ccedil;", 8)) {
    strncpy(res, "\xe7", 1);
    return 8;
  } else if (!strncmp(token, "&ntilde;", 8)) {
    strncpy(res, "\xf1", 1);
    return 8;
  } else if (!strncmp(token, "&copy;", 6)) {
    strncpy(res, "\xa9", 1);
    return 6;
  } else if (!strncmp(token, "&#169;", 6)) {
    strncpy(res, "\xa9", 1);
    return 6;
  } else if (!strncmp(token, "&reg;", 5)) {
    strncpy(res, "\xae", 1);
    return 5;
  } else if (!strncmp(token, "&#174;", 6)) {
    strncpy(res, "\xae", 1);
    return 6;
  } else if (!strncmp(token, "&deg;", 5)) {
    strncpy(res, "\xb0", 1);
    return 5;
  } else if (!strncmp(token, "&#176;", 6)) {
    strncpy(res, "\xb0", 1);
    return 6;
  } else if (!strncmp(token, "&ordm;", 6)) {
    strncpy(res, "\xba", 1);
    return 6;
  } else if (!strncmp(token, "&laquo;", 7)) {
    strncpy(res, "\xab", 1);
    return 7;
  } else if (!strncmp(token, "&#171;", 6)) {
    strncpy(res, "\xab", 1);
    return 6;
  } else if (!strncmp(token, "&raquo;", 7)) {
    strncpy(res, "\xbb", 1);
    return 7;
  } else if (!strncmp(token, "&#187;", 6)) {
    strncpy(res, "\xbb", 1);
    return 6;
  } else if (!strncmp(token, "&micro;", 7)) {
    strncpy(res, "\xb5", 1);
    return 7;
  } else if (!strncmp(token, "&#181;", 6)) {
    strncpy(res, "\xb5", 1);
    return 6;
  } else if (!strncmp(token, "&para;", 6)) {
    strncpy(res, "\xb6", 1);
    return 6;
  } else if (!strncmp(token, "&#182;", 6)) {
    strncpy(res, "\xb6", 1);
    return 6;
  } else if (!strncmp(token, "&frac14;", 8)) {
    strncpy(res, "\xbc", 1);
    return 8;
  } else if (!strncmp(token, "&#188;", 6)) {
    strncpy(res, "\xbc", 1);
    return 6;
  } else if (!strncmp(token, "&frac12;", 8)) {
    strncpy(res, "\xbd", 1);
    return 8;
  } else if (!strncmp(token, "&#189;", 6)) {
    strncpy(res, "\xbd", 1);
    return 6;
  } else if (!strncmp(token, "&frac34;", 8)) {
    strncpy(res, "\xbe", 1);
    return 8;
  } else if (!strncmp(token, "&#190;", 6)) {
    strncpy(res, "\xbe", 1);
    return 6;
  } else if (!strncmp(token, "&#156;", 6)) {
    strncpy(res, "\x9c", 1);
    return 6;
  } else {
    strncpy(res, " ", 1);
    return i+1;
  }
}
