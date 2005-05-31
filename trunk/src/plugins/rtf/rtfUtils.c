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
 * internal functions for RTF plugin
 */

#include "p_rtf.h"
#include <string.h>
#include <unicode/ustring.h>


int getText(struct doc_descriptor *desc, UChar *out, int size) {
  struct rtfState *state = (struct rtfState *)(desc->myState);
  UErrorCode err;
  int l;
  int fini;
  char tmp[2];

  memset(out, '\x00', size);
  fini = 0;
  l = 0;

  /* main loop */
  while(!fini && state->len > 0 && l < size - 2) {

    /* group delimiters */
    if(!strncmp(state->buf + state->cursor, "{", 1)) {
      state->cursor++;
      if(state->cursor >= state->len) {
	state->len = read(desc->fd, state->buf, BUFSIZE);
	state->cursor = 0;
      }

    } else if(!strncmp(state->buf + state->cursor, "}", 1)) {
      state->isMeta = 0;
      state->cursor++;
      if(state->cursor >= state->len) {
	state->len = read(desc->fd, state->buf, BUFSIZE);
	state->cursor = 0;
      }

      /* keyword */
    } else if(!strncmp(state->buf + state->cursor, "\\", 1)) {
      if(doKeyword(desc, out, size, &l) == -1) {
	fini = 1;
      }

      /* space or new line */
    } else if(isspace(state->buf[state->cursor])) {
      memcpy(out + l/2, "\x20\x00", 2);
      l += 2;
      state->cursor++;
      if(state->cursor >= state->len) {
	state->len = read(desc->fd, state->buf, BUFSIZE);
	state->cursor = 0;
      }

      /* text */
    } else {
      if(!state->isMeta) {
	memset(tmp, '\x00', 2);
	strncpy(tmp, state->buf + state->cursor, 1);

	/* converting to UTF-16 */
	err = U_ZERO_ERROR;
	ucnv_toUChars(desc->conv, out + l/2, size - l, tmp, 1, &err);
	if (U_FAILURE(err)) {
	  fprintf(stderr, "Unable to convert buffer\n");
	  return ERR_ICU;
	}
	l += 2;
      }
      state->cursor++;
      if(state->cursor >= state->len) {
	state->len = read(desc->fd, state->buf, BUFSIZE);
	state->cursor = 0;
      }
    }

  }
  if(state->len == 0 && l == 0) {
    return NO_MORE_DATA;
  } else {
    return l;
  }
}


int skipBloc(struct doc_descriptor *desc) {
  struct rtfState *state = (struct rtfState *)(desc->myState);
  int nbopened;

  nbopened = 0;
  for( ; nbopened || strncmp(state->buf + state->cursor, "}", 1); state->cursor++) {
    if(state->cursor >= state->len) {
      state->len = read(desc->fd, state->buf, BUFSIZE);
      state->cursor = 0;
    }
    
    if(!strncmp(state->buf + state->cursor, "{", 1)) {
      nbopened++;
    } else if(!strncmp(state->buf + state->cursor, "}", 1)) {
      nbopened--;
    }
  }
  state->cursor++;
  if(state->cursor >= state->len) {
    state->len = read(desc->fd, state->buf, BUFSIZE);
    state->cursor = 0;
  }
  
  return OK;
}


int doKeyword(struct doc_descriptor *desc, UChar *out, int size, int *l) {
  struct rtfState *state = (struct rtfState *)(desc->myState);
  UErrorCode err;
  char keyword[32], cparam[8], tmp[20];
  int i, j, iparam, v;

  if(strncmp(state->buf + state->cursor, "\\", 1)) {
    fprintf(stderr, "Not a keyword, operation aborted\n");
    return -2;
  }

  state->cursor++;
  if(state->cursor >= state->len) {
    state->len = read(desc->fd, state->buf, BUFSIZE);
    state->cursor = 0;
  }

  memset(keyword, '\x00', 32);
  memset(cparam, '\x00', 10);
  for(i = 0; isalpha(state->buf[state->cursor]); i++) {
    strncpy(keyword + i, state->buf + state->cursor, 1);
    state->cursor++;
    if(state->cursor >= state->len) {
      state->len = read(desc->fd, state->buf, BUFSIZE);
      state->cursor = 0;
    }
  }

  if(i) {
    if(!strncmp(state->buf + state->cursor, " ", 1)) {
      state->cursor++;
      if(state->cursor >= state->len) {
	state->len = read(desc->fd, state->buf, BUFSIZE);
	state->cursor = 0;
      }
    } else {
      for(j = 0; isdigit(state->buf[state->cursor]); j++) {
	strncpy(cparam + j, state->buf + state->cursor, 1);
	state->cursor++;
	if(state->cursor >= state->len) {
	  state->len = read(desc->fd, state->buf, BUFSIZE);
	  state->cursor = 0;
	}
      }
    }
    if(j) {
      iparam = atoi(cparam);
    }
    
    if(!strncmp(keyword, "par", max(i, 3))) {
      memcpy(out + (*l)/2, "\x20\x00", 2);
      (*l) += 2;
      return -1;

      /* groups to skip */
    } else if(!strncmp(keyword, "fonttbl", max(i, 7)) ||
	      !strncmp(keyword, "filetbl", max(i, 7)) ||
	      !strncmp(keyword, "colortbl", max(i, 8)) ||
	      !strncmp(keyword, "field", max(i, 5)) ||
	      !strncmp(keyword, "stylesheet", max(i, 10))) {
      skipBloc(desc);

      
      /* unicode character */
    } else if(!strncmp(keyword, "u", i) && j) {
      if(iparam) {
	out[(*l)/2] = iparam;
	(*l) += 2;
      }
      state->cursor++;
      if(state->cursor >= state->len) {
	state->len = read(desc->fd, state->buf, BUFSIZE);
	state->cursor = 0;
      }
      state->cursor++;
      if(state->cursor >= state->len) {
	state->len = read(desc->fd, state->buf, BUFSIZE);
	state->cursor = 0;
      }
      
      /* metadata */
    } else if(!strncmp(keyword, "info", max(i, 4))) {
      state->isMeta = 1;

    } else if(!strncmp(keyword, "title", max(i, 5))) {
      doMeta(desc, "title", string, 0);

    } else if(!strncmp(keyword, "subject", max(i, 7))) {
      doMeta(desc, "subject", string, 0);
      
    } else if(!strncmp(keyword, "author", max(i, 6))) {
      doMeta(desc, "author", string, 0);
      
    } else if(!strncmp(keyword, "manager", max(i, 7))) {
      doMeta(desc, "manager", string, 0);
      
    } else if(!strncmp(keyword, "company", max(i, 7))) {
      doMeta(desc, "company", string, 0);
      
    } else if(!strncmp(keyword, "operator", max(i, 8))) {
      doMeta(desc, "operator", string, 0);

    } else if(!strncmp(keyword, "category", max(i, 8))) {
      doMeta(desc, "category", string, 0);

    } else if(!strncmp(keyword, "keywords", max(i, 8))) {
      doMeta(desc, "keywords", string, 0);

    } else if(!strncmp(keyword, "comment", max(i, 7))) {
      doMeta(desc, "comment", string, 0);

    } else if(!strncmp(keyword, "creatim", max(i, 7))) {
      doMeta(desc, "creation", date, 0);

    } else if(!strncmp(keyword, "revtim", max(i, 6))) {
      doMeta(desc, "revision", date, 0);

    } else if(!strncmp(keyword, "nofpages", max(i, 8))) {
      doMeta(desc, "number of pages", number, iparam);

    } else if(!strncmp(keyword, "nofwords", max(i, 8))) {
      doMeta(desc, "number of words", number, iparam);

    } else if(!strncmp(keyword, "yr", max(i, 2))) {
      memset(tmp, '\x00', 20);
      sprintf(tmp, "year %d ", iparam);
      /* converting to UTF-16 */
      err = U_ZERO_ERROR;
      ucnv_toUChars(desc->conv, out + (*l)/2, size - (*l)/2, tmp, strlen(tmp), &err);
      if (U_FAILURE(err)) {
	fprintf(stderr, "Unable to convert buffer\n");
	return ERR_ICU;
      }
      (*l) += 2*strlen(tmp);

    } else if(!strncmp(keyword, "mo", max(i, 2))) {
      memset(tmp, '\x00', 20);
      sprintf(tmp, "month %d ", iparam);
      /* converting to UTF-16 */
      err = U_ZERO_ERROR;
      ucnv_toUChars(desc->conv, out + (*l)/2, size - (*l)/2, tmp, strlen(tmp), &err);
      if (U_FAILURE(err)) {
	fprintf(stderr, "Unable to convert buffer\n");
	return ERR_ICU;
      }
      (*l) += 2*strlen(tmp);

    } else if(!strncmp(keyword, "dy", max(i, 2))) {
      memset(tmp, '\x00', 20);
      sprintf(tmp, "day %d ", iparam);
      /* converting to UTF-16 */
      err = U_ZERO_ERROR;
      ucnv_toUChars(desc->conv, out + (*l)/2, size - (*l)/2, tmp, strlen(tmp), &err);
      if (U_FAILURE(err)) {
	fprintf(stderr, "Unable to convert buffer\n");
	return ERR_ICU;
      }
      (*l) += 2*strlen(tmp);

    } else if(!strncmp(keyword, "hr", max(i, 2))) {
      memset(tmp, '\x00', 20);
      sprintf(tmp, "%d h ", iparam);
      /* converting to UTF-16 */
      err = U_ZERO_ERROR;
      ucnv_toUChars(desc->conv, out + (*l)/2, size - (*l)/2, tmp, strlen(tmp), &err);
      if (U_FAILURE(err)) {
	fprintf(stderr, "Unable to convert buffer\n");
	return ERR_ICU;
      }
      (*l) += 2*strlen(tmp);

    } else if(!strncmp(keyword, "min", max(i, 3))) {
      memset(tmp, '\x00', 20);
      sprintf(tmp, "%d min ", iparam);
      /* converting to UTF-16 */
      err = U_ZERO_ERROR;
      ucnv_toUChars(desc->conv, out + (*l)/2, size - (*l)/2, tmp, strlen(tmp), &err);
      if (U_FAILURE(err)) {
	fprintf(stderr, "Unable to convert buffer\n");
	return ERR_ICU;
      }
      (*l) += 2*strlen(tmp);

    } else if(!strncmp(keyword, "sec", max(i, 3))) {
      memset(tmp, '\x00', 20);
      sprintf(tmp, "%d sec", iparam);
      /* converting to UTF-16 */
      err = U_ZERO_ERROR;
      ucnv_toUChars(desc->conv, out + (*l)/2, size - (*l)/2, tmp, strlen(tmp), &err);
      if (U_FAILURE(err)) {
	fprintf(stderr, "Unable to convert buffer\n");
	return ERR_ICU;
      }
      (*l) += 2*strlen(tmp);

    }else if(state->isMeta) {
      skipBloc(desc);

      /* text */
    } else if(!strncmp(keyword, "tab", max(i, 3))) {
      memcpy(out + (*l)/2, "\x20\x00", 2);
      (*l) += 2;

    } else if(!strncmp(keyword, "emdash", max(i, 6))
	      || !strncmp(keyword, "endash", max(i, 6))) {
      memcpy(out + (*l)/2, "\x2D\x00", 2);
      (*l) += 2;

    } else if(!strncmp(keyword, "emspace", max(i, 7))
	      || !strncmp(keyword, "enspace", max(i, 7))
	      || !strncmp(keyword, "qmspace", max(i, 7))) {
      memcpy(out + (*l)/2, "\x20\x00", 2);
      (*l) += 2;

    } else if(!strncmp(keyword, "lquote", max(i, 6))) {
      memcpy(out + (*l)/2, "\x27\x00", 2);
      (*l) += 2;

    } else if(!strncmp(keyword, "rquote", max(i, 6))) {
      memcpy(out + (*l)/2, "\x27\x00", 2);
      (*l) += 2;

    } else if(!strncmp(keyword, "ldblquote", max(i, 9))) {
      memcpy(out + (*l)/2, "\x22\x00", 2);
      (*l) += 2;

    } else if(!strncmp(keyword, "rdblquote", max(i, 9))) {
      memcpy(out + (*l)/2, "\x22\x00", 2);
      (*l) += 2;

      /* footnotes */
    } else if(!strncmp(keyword, "footnote", max(i, 8))) {
      memcpy(out + (*l)/2, "\x2A\x00", 2);
      (*l) += 2;
    }

  } else {
    /* not an alpha keyword */
    if(!strncmp(state->buf + state->cursor, "~", 1)) {
      memcpy(out + (*l)/2, "\x20\x00", 2);
      (*l) += 2;
      state->cursor++;
      if(state->cursor >= state->len) {
	state->len = read(desc->fd, state->buf, BUFSIZE);
	state->cursor = 0;
      }

    } else if(!strncmp(state->buf + state->cursor, "{", 1)) {
      memcpy(out + (*l)/2, "\x7B\x00", 2);
      (*l) += 2;
      state->cursor++;
      if(state->cursor >= state->len) {
	state->len = read(desc->fd, state->buf, BUFSIZE);
	state->cursor = 0;
      }

    } else if(!strncmp(state->buf + state->cursor, "}", 1)) {
      memcpy(out + (*l)/2, "\x7D\x00", 2);
      (*l) += 2;
      state->cursor++;
      if(state->cursor >= state->len) {
	state->len = read(desc->fd, state->buf, BUFSIZE);
	state->cursor = 0;
      }

    } else if(!strncmp(state->buf + state->cursor, "*", 1)) {
      /* skip group */
      skipBloc(desc);

    } else if(!strncmp(state->buf + state->cursor, "\\", 1)) {
      memcpy(out + (*l)/2, "\x5C\x00", 2);
      (*l) += 2;
      state->cursor++;
      if(state->cursor >= state->len) {
	state->len = read(desc->fd, state->buf, BUFSIZE);
	state->cursor = 0;
      }

    } else if(!strncmp(state->buf + state->cursor, "'", 1)) {
      state->cursor++;
      if(state->cursor >= state->len) {
	state->len = read(desc->fd, state->buf, BUFSIZE);
	state->cursor = 0;
      }
      v = 0;
      for(j = 0; j < 2; j++) {
	v *= 16;
	if(isdigit(state->buf[state->cursor])) {
	  v += state->buf[state->cursor] - '0';
	} else if(islower(state->buf[state->cursor])) {
	  v += state->buf[state->cursor] - 'a' + 10;
	} else {
	  v += state->buf[state->cursor] - 'A' + 10;
	}
	state->cursor++;
	if(state->cursor >= state->len) {
	  state->len = read(desc->fd, state->buf, BUFSIZE);
	  state->cursor = 0;
	}
      }
      out[(*l)/2] = v;
      (*l) += 2;
    }

  }

  return OK;
}


int doMeta(struct doc_descriptor *desc, char *name, enum metaType type, int param) {
  struct rtfState *state = (struct rtfState *)(desc->myState);
  struct meta *meta = NULL;
  UErrorCode err;
  char metastring[1024];
  UChar datestring[1024];
  int j;

  if(desc->meta == NULL) {
    desc->meta = (struct meta *) malloc(sizeof(struct meta));
    meta = desc->meta;
  } else {
    for(meta = desc->meta; meta->next != NULL; meta = meta->next) {}
    meta->next = (struct meta *) malloc(sizeof(struct meta));
    meta = meta->next;
  }
  meta->next = NULL;
  memset(metastring, '\x00', 1024);
  meta->name = (UChar *) malloc(2*strlen(name) + 2);
      
  /*converting to UTF-16 */
  err = U_ZERO_ERROR;
  ucnv_toUChars(desc->conv, meta->name, 2*strlen(name) + 2, name, strlen(name), &err);
  if (U_FAILURE(err)) {
    fprintf(stderr, "Unable to convert buffer\n");
  }
  meta->name_length = u_strlen(meta->name);

  /* get value */
  memset(metastring, '\x00', 1024);
  switch(type) {

  case string:
    for(j = 0 ; strncmp(state->buf + state->cursor, "}", 1); j++) {
      strncpy(metastring + j, state->buf + state->cursor, 1);
      state->cursor++;
      if(state->cursor >= state->len) {
	state->len = read(desc->fd, state->buf, BUFSIZE);
	state->cursor = 0;
      }
    }
    meta->value = (UChar *) malloc(2*j + 2);
    
    /*converting to UTF-16 */
    err = U_ZERO_ERROR;
    ucnv_toUChars(desc->conv, meta->value, 2*j + 2, metastring, j, &err);
    if (U_FAILURE(err)) {
      fprintf(stderr, "Unable to convert buffer\n");
    }
    meta->value_length = u_strlen(meta->value);
    
    break;

  case number:
    sprintf(metastring, "%d", param);
    j = strlen(metastring);
    meta->value = (UChar *) malloc(2*j + 2);
    
    /*converting to UTF-16 */
    err = U_ZERO_ERROR;
    ucnv_toUChars(desc->conv, meta->value, 2*j + 2, metastring, j, &err);
    if (U_FAILURE(err)) {
      fprintf(stderr, "Unable to convert buffer\n");
    }
    meta->value_length = u_strlen(meta->value);
    
    break;

  case date:
    j = 0;
    memset(datestring, '\x00', 1024);
    while(strncmp(state->buf + state->cursor, "}", 1)) {
      if(!strncmp(state->buf + state->cursor, "\\", 1)) {
	doKeyword(desc, datestring, 1024, &j);
      } else {
	state->cursor++;
	if(state->cursor >= state->len) {
	  state->len = read(desc->fd, state->buf, BUFSIZE);
	  state->cursor = 0;
	}
      }
    }
    meta->value = (UChar *) malloc(j + 2);
    meta->value_length = u_strlen(datestring);
    memcpy(meta->value, datestring, j);
    break;

  }

  state->cursor++;
  if(state->cursor >= state->len) {
    state->len = read(desc->fd, state->buf, BUFSIZE);
    state->cursor = 0;
  }

  return OK;
}


int max(int a, int b) {
  return (a < b) ? b : a;
}
