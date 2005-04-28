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
 * internal functions for PDF plugin
 */

#include "p_pdf.h"
		
int getText(struct doc_descriptor *desc, char *out, int size) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  char buf[BUFSIZE];
  int len, i;

  /* enter current page */
  gotoRef(desc, state->xref, state->currentPage);
  len = read(desc->fd, buf, BUFSIZE);
  
  /* search dictionary */
  for (i = 0; strncmp(buf + i, "<<", 2); i++) {
    if (i == len - 2) {
      strncpy(buf, buf + i + 1, 1);
      len = read(desc->fd, buf + 1, BUFSIZE - 1) + 1;
      i = 0;
    }
  }
  /* search content */
  while (strncmp(buf + i, ">>", 2) && strncmp(buf + i, "/Contents ", 10)) {
    i++;
    if (i == len - 10) {
      strncpy(buf, buf + i, len - i);
      len = read(desc->fd, buf + len - i, BUFSIZE - len + i);
      i = 0;
    }
  }
  /* if content is found */
  if (!strncmp(buf + i, "/Contents ", 10)) {
    if (state->currentStream == 0) {
      
      i += 10;
      if(!strncmp(buf + i, "[", 1)) {
	/* this is an array of streams */
	
	/* get first stream */
	i++;
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if (i == len) {
	    len = read(desc->fd, buf, BUFSIZE);
	  }
	}
	state->currentStream = getNumber(buf + i);

      } else {
	/* unique stream */
	while(!strncmp(buf + i, " ", 1)) {
	  i++;
	  if (i == len) {
	    len = read(desc->fd, buf, BUFSIZE);
	  }
	}
	state->currentStream = getNumber(buf + i);
      }
    }

    return procedeStream(desc, out, size);
    
  } else {
    /* if no content in page, goto next page */
    
    if(getNextPage(desc) != -1) {
      state->currentStream = 0;
      return getText(desc, out, size);
    } else {
      return NO_MORE_DATA;
    }
  }

  return NO_MORE_DATA;
}


int getNextStream(struct doc_descriptor *desc) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  char buf[BUFSIZE], tmp[20];
  int len, i;

  gotoRef(desc, state->xref, state->currentPage);
  len = read(desc->fd, buf, BUFSIZE);
  for(i = 0; i <= len - 10 && strncmp(buf + i, "/Contents ", 10); i++) {}
  i += 10;
  if (!strncmp(buf + i, "[", 1)) {
    strncmp(tmp, "\x00\x00\x00\x00\x00\x00", 6);
    sprintf(tmp, "%d ", state->currentStream);
    while(strncmp(buf + i, tmp, strlen(tmp))) {
      i++;
    }
    i +=  strlen(tmp);
    for( ; !strncmp(buf + i, " ", 1) ||
	   !strncmp(buf + i, "\x0A", 1) ||
	   !strncmp(buf + i, "\x0D", 1); i++) {}
    if(strncmp(buf + i, "]", 1)) {
      state->currentStream = getNumber(buf + i);
      state->currentOffset = 0;
      return state->currentStream;
    }
  }
  
  /* next stream is in another page */

  do {
    if (getNextPage(desc) == -1) {
      return NO_MORE_DATA;
    }
    gotoRef(desc, state->xref, state->currentPage);
    len = read(desc->fd, buf, BUFSIZE);
    
    /* search dictionary */
    for (i = 0; strncmp(buf + i, "<<", 2); i++) {}
    
    /* search content */
    for( ;strncmp(buf + i, ">>", 2) && strncmp(buf + i, "/Contents ", 10); i++) {}
    
  } while(strncmp(buf + i, "/Contents ", 10));

  i += 10;
  if(!strncmp(buf + i, "[", 1)) {
    /* this is an array of streams */
    
    /* get first stream */
    i++;
    for( ;!strncmp(buf + i, " ", 1) ||
	   !strncmp(buf + i, "\x0A", 1) ||
	   !strncmp(buf + i, "\x0D", 1); i++) {}
    state->currentStream = getNumber(buf + i);
    state->currentOffset = 0;
    return state->currentStream;
    
  } else {
    /* unique stream */
    for( ;!strncmp(buf + i, " ", 1); i++) {}
    state->currentStream = getNumber(buf + i);
    state->currentOffset = 0;
    return state->currentStream;
  }   
}

int getNextPage(struct doc_descriptor *desc) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  int len, i, t;
  char buf[BUFSIZE], tmp[20];
  int parentRef;

  /* search parent */
  gotoRef(desc, state->xref, state->currentPage);
  len = read(desc->fd, buf, BUFSIZE);
  strncpy(tmp, buf + 29, 8);
  for(i = 0; i <= len - 8 && strncmp(buf + i, "/Parent ", 8); i++) {
    if(!strncmp(buf + i, ">>", 2)) {
      /* end of document */
      return -1;
    }
  }
       
  i += 8;
  parentRef = getNumber(buf + i);
  gotoRef(desc, state->xref, parentRef);
  
  /* search current page in kids */
  len = read(desc->fd, buf, BUFSIZE);
  for(i = 0; i <= len - 6 && strncmp(buf + i, "/Kids ", 6); i++) {}
  strncpy(tmp, "\x00\x00\x00\x00\x00\x00", 6);
  sprintf(tmp, "%d ", state->currentPage);

  for( ; i <= len - strlen(tmp) && strncmp(buf + i, tmp, strlen(tmp)); i++) {}
  i += strlen(tmp);

  /* skip end of reference */
  for( ; strncmp(buf + i, "R", 1); i++) {}
  i ++;
  for( ; !strncmp(buf + i, " ", 1) ||
	 !strncmp(buf + i, "\x0A", 1) ||
	 !strncmp(buf + i, "\x0D", 1); i++) {}

  /* get next page */
  if(strncmp(buf + i, "]", 1)) {
    state->currentPage = getNumber(buf + i);
    gotoRef(desc, state->xref, state->currentPage);
    len = read(desc->fd, buf, BUFSIZE);
    strncpy(tmp, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 10);

    if (getValue(desc, buf, len, "Type", tmp)) {
	fprintf(stderr, "Can't find Type\n");
	return -2;
    }
    while(!strncmp(tmp, "Pages", 5)) {
      i = 0;
      
      while(i <= len - 6 && strncmp(buf + i, "/Kids ", 6)) {
	if (i == len - 6) {
	  strncpy(buf, buf + i, 6);
	  len = read(desc->fd, buf + 6, BUFSIZE - 6) + 6;
	  i = 0;
	} else {
	  i++;
	}
      }
      i += 7;
      while (!strncmp(buf + i, " ", 1)) {
	i++;
	if (i == len) {
	  len = read(desc->fd, buf, BUFSIZE);
	  i = 0;
	}
      }
      t = 0;
      while (strncmp(buf + i, " ", 1)) {
	strncpy(tmp + t, buf + i, 1);
	t++;
	i++;
	if (i == len) {
	  len = read(desc->fd, buf, BUFSIZE);
	  i = 0;
	}
      }
      strncpy(tmp + t, "\0", 1);
      gotoRef(desc, state->xref, atoi(tmp));
      state->currentPage = atoi(tmp);

      len = read(desc->fd, buf, BUFSIZE);
      strncpy(tmp, "\x00\x00\x00\x00\x00\x00", 6);
      if (getValue(desc, buf, len, "Type", tmp) < 0) {
	fprintf(stderr, "Can't find Type\n");
	return -2;
      }
    }
    state->currentStream = 0;
    state->currentOffset = 0;
    return state->currentPage;

    /* search in parent */
  } else {
    state->currentPage = parentRef;
    return getNextPage(desc);
  }
  
}


enum filter identifyFilter(char *buf) {

  if(!strncmp(buf, "LZWDecode", 9)) {
    return lzw;
  } else if (!strncmp(buf, "FlateDecode", 11)) {
    return flateDecode;
  } else if (!strncmp(buf, "Crypt", 5)) {
    return crypt;
  } else {
    return none;
  }
}


int procedeStream(struct doc_descriptor *desc, char *out, int size) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  enum filter filter, tfilter;
  int streamlength, beginByte;
  int len, i, j, l, fini, v;
  uLongf  destlen;
  char buf[BUFSIZE], tmp[20];
  char stream[50*BUFSIZE], *buf2;

  streamlength = 0;

  gotoRef(desc, state->xref, state->currentStream);
  len = read(desc->fd, buf, BUFSIZE);
  
  /* search dictionary */
  for (i = 0; strncmp(buf + i, "<<", 2); i++) {
    if (i == len - 2) {
      strncpy(buf, buf + i + 1, 1);
      len = read(desc->fd, buf + 1, BUFSIZE - 1) + 1;
      i = 0;
    }
  }
  /* search length and Filter */
  filter = none;
  while (strncmp(buf + i, ">>", 2)) {

    /* get Length */
    if(!strncmp(buf + i, "/Length ", 8)) {
      i += 8;
      if (i > len - 8) {
	strncpy(buf, buf + i, len - i);
	len = read(desc->fd, buf + len - i, BUFSIZE - len + i);
	i = 0;
      }

      for(j = 0; strncmp(buf + i + j, " ", 1) &&
	    strncmp(buf + i + j, "\x0A", 1) &&
	    strncmp(buf + i + j, "\x0D", 1); j++) {}
      j++;
      if(!strncmp(buf + i + j, "/", 1)) {
	/* Length is an integer */
	streamlength = getNumber(buf + i);

	/* Length is a reference */
      } else {
	v = lseek(desc->fd, 0, SEEK_CUR);
	gotoRef(desc, state->xref, getNumber(buf + i));
	l = read(desc->fd, stream, BUFSIZE);
	j = getNextLine(stream, l);
	while(!strncmp(stream + j, " ", 1)) {
	  j++;
	}
	streamlength = getNumber(stream + j);
	lseek(desc->fd, v, SEEK_SET);
      }
    }
    
    /* get Filters */
    if(!strncmp(buf + i, "/Filter ", 8)) {
      i += 8;
      if (i > len - 8) {
	strncpy(buf, buf + i, len - i);
	len = read(desc->fd, buf + len - i, BUFSIZE - len + i);
	i = 0;
      }
      
      /* array of filters */
      if(!strncmp(buf + i, "[", 1)) {

	while(strncmp(buf + i, "]", 1)) {

	  if (!strncmp(buf + i, "/", 1)) {
	    j = 0;
	    while(strncmp(buf + i, " ", 1)) {
	      strncpy(tmp + j, buf + i, 1);
	      j++;
	      i++;
	      if (i == len) {
		len = read(desc->fd, buf, BUFSIZE);
	      }
	    }
	    strncpy(tmp + j, "\0", 1);
	    tfilter = identifyFilter(tmp);
	    switch (tfilter) {
	    case flateDecode:
	      filter = tfilter;
	      break;
	    case lzw:
	      fprintf(stderr, "LZW desompression not implemented\n");
	      return NO_MORE_DATA;
	      break;
	    case crypt:
	      fprintf(stderr, "Encrypted document : operation aborted\n");
	      return NO_MORE_DATA;
	      break;
	    default:
	      break;
	    }
	  }

	  i++;
	  if (i == len) {
	    len = read(desc->fd, buf, BUFSIZE);
	  }
	}

	/* unique filter */
      } else {
	i++;
	j = 0;
	while(strncmp(buf + i, " ", 1) &&
	      strncmp(buf + i, "\x0A", 1) &&
	      strncmp(buf + i, "\x0D", 1)) {
	  strncpy(tmp + j, buf + i, 1);
	  i++;
	  j++;
	  if (i == len) {
	    len = read(desc->fd, buf, BUFSIZE);
	  }
	}
	strncpy(tmp + j, "\0", 1);
	filter = identifyFilter(tmp);
	switch (filter) {
	case lzw:
	  fprintf(stderr, "LZW desompression not implemented\n");
	  return NO_MORE_DATA;
	  break;
	case crypt:
	  fprintf(stderr, "Encrypted document : operation aborted\n");
	  return NO_MORE_DATA;
	  break;
	default:
	  break;
	}
      }
    }
    i++;
    if (i == len - 8) {
      strncpy(buf, buf + i, len - i);
      len = read(desc->fd, buf + len - i, BUFSIZE - len + i);
      i = 0;
    }
  }

  /* search stream begining */
  while(strncmp(buf + i, "stream", 6)) {
    i++;
    if (i == len - 6) {
      strncpy(buf, buf + i, len - i);
      len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
  }
  i += getNextLine(buf + i, len - i);
  beginByte = lseek(desc->fd, i - len, SEEK_CUR);

  /* uncompress or copy stream */
  buf2 = (char *) malloc(streamlength);
  len = read(desc->fd, buf2, streamlength);

  for(j = 0; j < 50*BUFSIZE; j++) {
    strncpy(stream + j, "\x00", 1);
  }

  if(filter == flateDecode) {
    destlen = 50*BUFSIZE;
    uncompress(stream, &destlen, buf2, len);
    destlen = strlen(stream);

  } else {
    strncpy(stream, buf2, len);
    destlen = len;
  }

  /* go to current offset in stream */
  i = state->currentOffset;

  /* get next paragraph */
  fini = 0;
  l = 0;
  while (!fini) {

    /* get normal string */
    if(l < size && !strncmp(stream + i, "(", 1)) {
      i++;
      state->currentOffset++;
      while (strncmp(stream + i, ")", 1)) {
	strncpy(out + l, stream + i, 1);
	l++;
	i++;
	state->currentOffset++;
      }
    
      /* get hex string */
    } else if (!strncmp(stream + i, "<", 1)) {
      i++;
      state->currentOffset++;
      j = 0;
      v = 0;
      while(l < size && strncmp(stream + i, ">", 1)) {
	strncpy(tmp, stream + i, 1);
	strncpy(tmp + 1, "\0", 1);
	v *= 16;
	if(!strncmp(tmp, "a", 1) || !strncmp(tmp, "A", 1)) {
	  v += 10;
	} else if(!strncmp(tmp, "b", 1) || !strncmp(tmp, "B", 1)) {
	  v += 11;
	} else if(!strncmp(tmp, "c", 1) || !strncmp(tmp, "C", 1)) {
	  v += 12;
	} else if(!strncmp(tmp, "d", 1) || !strncmp(tmp, "D", 1)) {
	  v += 13;
	} else if(!strncmp(tmp, "e", 1) || !strncmp(tmp, "E", 1)) {
	  v += 14;
	} else if(!strncmp(tmp, "f", 1) || !strncmp(tmp, "F", 1)) {
	  v += 15;
	} else {
	  v += atoi(tmp);
	}
	i++;
	state->currentOffset++;
      }
      sprintf(out + l, "%c", v);
      l++;
    }

    /* release output */
    if (l >= size || !strncmp(stream + i, "ET", 2)) {
      i ++;
      state->currentOffset ++;
      fini = 1;
      free(buf2);
      return l;
    }

    i++;
    state->currentOffset++;

    /* end of stream */
    if(i >= destlen) {
      fini = 1;
      free(buf2);
      if(getNextStream(desc) == -1) {
	return NO_MORE_DATA;
      }
      return l;
    }
  }

  free(buf2);
  return NO_MORE_DATA;
  
}

int getNumber(char *buf) {
  char tmp[20];
  int t = 0;

  while (strncmp(buf + t, " ", 1) &&
	 strncmp(buf + t, "\x0A", 1) &&
	 strncmp(buf + t, "\x0D", 1)) {
    strncpy(tmp + t, buf + t, 1);
    t++;
  }
  strncpy(tmp + t, "\0", 1);
  return atoi(tmp);
}


int getValue(struct doc_descriptor *desc, char *buf, int size, char *name, char *value) {
  int i, j, len;
  char token[21];

  /* finding begining of dictionary */
  len = size;
  for(i = 0; i < len - 1 && strncmp(buf + i, "<<", 2); i++) {
    if (i == len - 2) {
      strncpy(buf, buf + i, 2);
      len = read(desc->fd, buf + 2, BUFSIZE - 2) + 2;
      i = 0;
    }
  }
  i += 2;
  
  /* finding desired field */
  sprintf(token, "/%s /", name);
  for( ; i <= len - strlen(token) &&
	 strncmp(buf + i, token, strlen(token)) &&
	 strncmp(buf + i, ">>", 2); i++) {
    if (i == len - strlen(token)) {
      strncpy(buf, buf + i, strlen(token));
      len = read(desc->fd, buf + strlen(token), BUFSIZE - strlen(token)) + strlen(token);
      i = 0;
    }
  }
  if (!strncmp(buf + i, ">>", 2)) {
    return -1;
  }
  i += strlen(token);
  
  /* filling value string */
  for(j = 0; strncmp(buf + i, " ", 1); j++) {
    strncpy(value + j, buf + i, 1);
    i++;
    if (i == len) {
      len = read(desc->fd, buf, BUFSIZE);
      i = 0;
    }
  }

  return 0;
}


int initReader(struct doc_descriptor *desc) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  char buf[BUFSIZE], tmp[20], keyword[20];
  int len, i, found, t;

  state->currentStream = state->currentOffset = 0;

  if(state->version < 5) {
    /* pdf version < 1.5 */

    /* looking for 'startxref' */
    lseek(desc->fd, -40, SEEK_END);
    len = read(desc->fd, buf, 40);
    if (len != 40) {
      fprintf(stderr, "file corrupted\n");
    }
    for(i = 30; i > 0 && strncmp(buf + i, "startxref", 9); i--) {}

    if(strncmp(buf + i, "startxref", 9)) {
      fprintf(stderr, "Unable to find xref reference\n");
      return -2;
    }

    /* getting xref offset */
    i += getNextLine(buf + i, len - i);
    t = 0;
    while (strncmp(buf + i + t, "\x0A", 1) && strncmp(buf + i + t, "\x0D", 1)) {
      strncpy(tmp + t, buf + i + t, 1);
      t++;
    }
    strncpy(tmp + t, "\0", 1);
    state->xref = atoi(tmp);

    /* going to xref table */
    lseek(desc->fd, state->xref, SEEK_SET);
    len = read(desc->fd, buf, BUFSIZE);

    /* looking for Root in trailer (just after xref) */
    found = 0;
    for( i = 0; i < len - 4 && !found; i++) {
      if(!strncmp(buf + i, "/", 1)) {
	i++;
	getKeyword(buf + i, keyword);
	if (!strncmp(keyword, "Root", 4)) {
	  found = 1;
	}
      }
    }
    if(!found) {
      fprintf(stderr, "Unable to find Root entry in trailer\n");
      return -2;
    }

    /* getting Root (catalog) reference */
    i += 4;
    state->catalogRef = getNumber(buf + i);

    /* going to catalog */
    gotoRef(desc, state->xref, state->catalogRef);
    len = read(desc->fd, buf, BUFSIZE);

    /* looking for page tree reference */
    found = 0;
    for( i = 0; i < len - 5 && !found; i++) {
      if(!strncmp(buf + i, "/", 1)) {
	i++;
	getKeyword(buf + i, keyword);
	if (!strncmp(keyword, "Pages", 5)) {
	  found = 1;
	}
      }
    }
    if (!found) {
      fprintf(stderr, "Unable to find Pages in catalog\n");
    }
    i += 5;
    state->pagesRef = getNumber(buf + i);

    /* going to page tree */
    gotoRef(desc, state->xref, state->pagesRef);
    len = read(desc->fd, buf, BUFSIZE);
    if (getValue(desc, buf, len, "Type", tmp) < 0) {
	fprintf(stderr, "Can't find Type\n");
	return -2;
    }

    /* finding first page */
    while(!strncmp(tmp, "Pages", 5)) {
      i = 0;

      while(i <= len - 6 && strncmp(buf + i, "/Kids ", 6)) {
	if (i == len - 6) {
	  strncpy(buf, buf + i, 6);
	  len = read(desc->fd, buf + 6, BUFSIZE - 6) + 6;
	  i = 0;
	} else {
	  i++;
	}
      }
      i += 7;
      while (!strncmp(buf + i, " ", 1)) {
	i++;
	if (i == len) {
	  len = read(desc->fd, buf, BUFSIZE);
	  i = 0;
	}
      }
      t = 0;
      while (strncmp(buf + i, " ", 1)) {
	strncpy(tmp + t, buf + i, 1);
	t++;
	i++;
	if (i == len) {
	  len = read(desc->fd, buf, BUFSIZE);
	  i = 0;
	}
      }
      strncpy(tmp + t, "\0", 1);
      gotoRef(desc, state->xref, atoi(tmp));
      state->currentPage = atoi(tmp);

      len = read(desc->fd, buf, BUFSIZE);
      strncpy(tmp, "\x00\x00\x00\x00\x00\x00", 6);
      if (getValue(desc, buf, len, "Type", tmp) < 0) {
	fprintf(stderr, "Can't find Type\n");
	return -2;
      }
    }

  } else {
    fprintf(stderr, "PDF version >= 1.5 : not yet implemented\n");
    return -2;
  }

  return OK;
}


int gotoRef(struct doc_descriptor *desc, size_t xref, int ref){
  char buf[BUFSIZE], tmp[11], keyword[20];
  int i, len, xinf, xsup, t, found, prev;

  /* going to xref */
  if (lseek(desc->fd, xref, SEEK_SET) != xref) {
    fprintf(stderr, "Unable to find xref\n");
    return -2;
  }
  len = read(desc->fd, buf, BUFSIZE);
  if (!len) {
    fprintf(stderr, "Unable to read xref\n");
    return -2;
  }
  i = getNextLine(buf, len);
  
  /* get range of references in table */
  t = 0;
  while (strncmp(buf + i, " ", 1)) {
    strncpy(tmp + t, buf + i, 1);
    t++;
    i++;
  }
  strncpy(tmp + t, "\0", 1);
  xinf = atoi(tmp);
  t = 0;
  i++;
  while (strncmp(buf + i + t, " ", 1) &&
	 strncmp(buf + i + t, "\x0A", 1) &&
	 strncmp(buf + i + t, "\x0D", 1)) {
    strncpy(tmp + t, buf + i + t, 1);
    t++;
  }
  strncpy(tmp + t, "\0", 1);
  xsup = xinf + atoi(tmp) - 1;

  if (ref > xsup || xinf > ref) {

    /* looking for ref in another xref table */
    found = 0;
    for( ; i < len - 4 && !found; i++) {
      if (!strncmp(buf + i, "/", 1)) {
	i++;
	getKeyword(buf + i, keyword);
	if (!strncmp(keyword, "Prev", 4)) {
	  found = 1;
	}
      }
      if (!found && i == len - 5) {
	strncpy(buf, buf + i + 1, 5);
	len = read(desc->fd, buf + 5, len - 5) + 5;
	i = 0;
      }
    }
    i += 4;
    prev = getNumber(buf + i);
    return (gotoRef(desc, prev, ref));
  }

  /* getting offset string */
  i += getNextLine(buf + i, len - i);
  i += 20 * (ref - xinf);
  while (i >= len) {
    len = read(desc->fd, buf, BUFSIZE);
    i -= BUFSIZE;
  }
  strncpy(tmp, buf + i, 10);
  strncpy(tmp + 10, "\0", 1);
  /* setting file cursor */
  if (lseek(desc->fd, atoi(tmp), SEEK_SET) != atoi(tmp)) {
    fprintf(stderr, "Unable to reach offset %d\n", atoi(tmp));
    return -2;
  }

  return 0;
}


int getKeyword(char *input, char *output){
  int i;

  for (i = 0; i < 20 && strncmp(input + i, " ", 1)
	 && strncmp(input + i, "\x0A", 1)
	 && strncmp(input + i, "\x0D", 1); i++) {
    strncpy(output + i, input + i, 1);
  }
  strncpy(output + i, "\0", 1);
  return strlen(output);
}


int getNextLine(char *buf, int size) {
  int i = 0;

  /* skipping non EOL characters */
  for(i = 0; i < size && strncmp(buf + i, "\x0A", 1) &&
	strncmp(buf + i, "\x0D", 1); i++) {}

  /* skipping EOL characters */
  for( ;i < size && (!strncmp(buf + i, "\x0A", 1) ||
		     !strncmp(buf + i, "\x0D", 1)); i++){}

  return i;

}


int version(int fd) {
  char buf[8], tmp[2];

  /* reading header */
  lseek(fd, 0, SEEK_SET);
  if( read(fd, buf, 8) != 8) {
    fprintf(stderr, "not a document file\n");
    return -2;
  }

  /* checking header */
  if( strncmp(buf, "%PDF-1.", 7)){
    fprintf(stderr, "not a PDF file\n");
    return -2;
  }

  /* get version number */
  strncpy(tmp, buf + 7, 1);
  strncpy(tmp + 1, "\0", 1);

  return atoi(tmp);
}
