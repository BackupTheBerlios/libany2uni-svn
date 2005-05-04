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
#include <math.h>
		
int getText(struct doc_descriptor *desc, char *out, int size) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  char buf[BUFSIZE];
  int len, i;
  int nbopened;

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
  nbopened = 0;
  while ((nbopened || strncmp(buf + i, ">>", 2)) && strncmp(buf + i, "/Contents", 9)) {
    if(!strncmp(buf + i, "<<", 2)) {
      nbopened++;
    }
    if(strncmp(buf + i, ">>", 2)) {
      nbopened--;
    }
    i++;
    if (i == len - 9) {
      strncpy(buf, buf + i, len - i);
      len = read(desc->fd, buf + len - i, BUFSIZE - len + i);
      i = 0;
    }
  }
  /* if content is found */
  if (!strncmp(buf + i, "/Contents", 9)) {
    if (state->currentStream == 0) {
      
      i += 9;
      while(!strncmp(buf + i, " ", 1)) {
	i++;
      }
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
  int nbopened;

  gotoRef(desc, state->xref, state->currentPage);
  len = read(desc->fd, buf, BUFSIZE);
  for(i = 0; i <= len - 9 && strncmp(buf + i, "/Contents", 9); i++) {}
  i += 9;
  while(!strncmp(buf + i, " ", 1)) {
    i++;
  }
  if (!strncmp(buf + i, "[", 1)) {
    strncmp(tmp, "\x00\x00\x00\x00\x00\x00", 6);
    sprintf(tmp, "%d ", state->currentStream);
    while(strncmp(buf + i, tmp, strlen(tmp))) {
      i++;
    }
    i +=  strlen(tmp);
    for( ; strncmp(buf + i, "R", 1); i++) {}
    i++;
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
    nbopened = 0;
    for( ;strncmp(buf + i, ">>", 2) && strncmp(buf + i, "/Contents", 9); i++) {
      if(!strncmp(buf + i, "<<", 2)) {
	nbopened++;
      }
      if(strncmp(buf + i, ">>", 2)) {
	nbopened--;
      }
    }
    
  } while(strncmp(buf + i, "/Contents", 9));

  i += 9;
  while(!strncmp(buf + i, " ", 1)) {
    i++;
  }
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
  int nbopened;

  /* search parent */
  gotoRef(desc, state->xref, state->currentPage);
  len = read(desc->fd, buf, BUFSIZE);
  strncpy(tmp, buf + 29, 8);
  nbopened = 0;
  for(i = 0; i <= len - 7 && strncmp(buf + i, "/Parent", 7); i++) {
    if(!strncmp(buf + i, "<<", 2)) {
      nbopened++;
    }
    if(!strncmp(buf + i, ">>", 2)) {
      nbopened--;

      if(!nbopened) {
	/* end of document */
	return -1;
      }
    }
  }
       
  i += 7;
  while(!strncmp(buf + i, " ", 1)) {
    i++;
  }
  parentRef = getNumber(buf + i);
  gotoRef(desc, state->xref, parentRef);

  /* search current page in kids */
  len = read(desc->fd, buf, BUFSIZE);
  for(i = 0; i <= len - 5 && strncmp(buf + i, "/Kids", 5); i++) {}
  while(!strncmp(buf + i, " ", 1)) {
    i++;
  }
  strncpy(tmp, "\x00\x00\x00\x00\x00\x00", 5);
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
    /*getEncodings(desc);*/
    len = read(desc->fd, buf, BUFSIZE);
    strncpy(tmp, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 10);

    if (getValue(desc, buf, len, "Type", tmp)) {
	fprintf(stderr, "Can't find Type\n");
	return -2;
    }
    while(!strncmp(tmp, "Pages", 5)) {
      i = 0;
      
      while(i <= len - 5 && strncmp(buf + i, "/Kids", 5)) {
	if (i == len - 5) {
	  strncpy(buf, buf + i, 5);
	  len = read(desc->fd, buf + 5, BUFSIZE - 5) + 5;
	  i = 0;
	} else {
	  i++;
	}
      }
      i += 5;
      while (!strncmp(buf + i, " ", 1)) {
	i++;
	if (i == len) {
	  len = read(desc->fd, buf, BUFSIZE);
	  i = 0;
	}
      }
      i++;
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
      /*getEncodings(desc);*/

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
  } else if (!strncmp(buf, "ASCII85Decode", 13)) {
    return ascii85Decode;
  } else if (!strncmp(buf, "Crypt", 5)) {
    return crypt;
  } else {
    return none;
  }
}


int procedeStream(struct doc_descriptor *desc, char *out, int size) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  enum filter  tfilter;
  struct pdffilter *filter;
  int beginByte;
  int escaped;
  int len, i, j, k, l, fini, v;
  char buf[BUFSIZE], tmp[20];
  char *buf2, buf3[BUFSIZE];
  char fontName[12];

  buf2 = NULL;
  escaped = 0;

  if(state->stream == NULL) {
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
    while (strncmp(buf + i, ">>", 2)) {

      /* get Length */
      if(!strncmp(buf + i, "/Length", 7)) {
	i += 7;
	while(!strncmp(buf + i, " ", 1)) {
	  i++;
	}
	if (i > len - 7) {
	  strncpy(buf, buf + i, len - i);
	  len = read(desc->fd, buf + len - i, BUFSIZE - len + i);
	  i = 0;
	}
	
	for(j = 0; strncmp(buf + i + j, " ", 1) &&
	      strncmp(buf + i + j, "/", 1) &&
	      strncmp(buf + i + j, "\x0A", 1) &&
	      strncmp(buf + i + j, "\x0D", 1); j++) {}
	if(strncmp(buf + i + j, "/", 1) && strncmp(buf + i + j, ">>", 2)) {
	  j++;
	}
	if(!strncmp(buf + i + j, "/", 1) || !strncmp(buf + i + j, ">>", 2)) {

	  /* Length is an integer */
	  state->length = getNumber(buf + i);
	  
	  /* Length is a reference */
	} else {
	  v = lseek(desc->fd, 0, SEEK_CUR);
	  gotoRef(desc, state->xref, getNumber(buf + i));
	  l = read(desc->fd, buf3, BUFSIZE);
	  j = getNextLine(buf3, l);
	  while(!strncmp(buf3 + j, " ", 1)) {
	    j++;
	  }
	  state->length = getNumber(buf3 + j);
	  lseek(desc->fd, v, SEEK_SET);
	}
      }

      /* get Filters */
      if(!strncmp(buf + i, "/Filter", 7)) {

	i += 7;
	if (i > len - 7) {
	  strncpy(buf, buf + i, len - i);
	  len = read(desc->fd, buf + len - i, BUFSIZE - len + i);
	  i = 0;
	}
	while(!strncmp(buf + i, " ", 1)) {
	  i++;
	}

	/* array of filters */
	if(!strncmp(buf + i, "[", 1)) {

	  while(strncmp(buf + i, "]", 1)) {
	    
	    if (!strncmp(buf + i, "/", 1)) {
	      i++;
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
	      case lzw:
		fprintf(stderr, "LZW desompression not implemented\n");
		return NO_MORE_DATA;
		break;
	      case crypt:
		fprintf(stderr, "Encrypted document : operation aborted\n");
		return NO_MORE_DATA;
		break;
	      default:
		if(state->filter == NULL) {
		  state->filter = (struct pdffilter *) malloc(sizeof(struct pdffilter));
		  filter = state->filter;
		} else {
		  filter = state->filter;
		  while(filter->next != NULL) {
		    filter = filter->next;
		  }
		  filter->next = (struct pdffilter *) malloc(sizeof(struct pdffilter));
		  filter = filter->next;
		}
		filter->next = NULL;
		filter->filtercode = tfilter;
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

	  /* search end of filters linked list in state */
	  if(state->filter == NULL) {
	    state->filter = (struct pdffilter *) malloc(sizeof(struct pdffilter));
	    filter = state->filter;
	  } else {
	    filter = state->filter;
	    while(filter->next != NULL) {
	      filter = filter->next;
	    }
	    filter->next = (struct pdffilter *) malloc(sizeof(struct pdffilter));
	    filter = filter->next;
	  }
	  filter->next = NULL;
	  
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
	  tfilter = identifyFilter(tmp);
	  switch (tfilter) {
	  case lzw:
	    fprintf(stderr, "LZW desompression not implemented\n");
	    return NO_MORE_DATA;
	    break;
	  case crypt:
	    fprintf(stderr, "Encrypted document : operation aborted\n");
	    return NO_MORE_DATA;
	    break;
	  default:
	    filter->filtercode = tfilter;
	    break;
	  }
	}
      }
      i++;

      if (i >= len - 8) {
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
    buf2 = (char *) malloc(state->length);
    state->stream = (char *) malloc(20*state->length);

    len = read(desc->fd, buf2, state->length);

    for(j = 0; j < 20*state->length; j++) {
      strncpy(state->stream + j, "\x00", 1);
    }

    if(state->filter != NULL) {
      filter = state->filter;
      applyFilter(desc, filter->filtercode, buf2, len);
      filter = filter->next;
      while(filter != NULL) {
	applyFilter(desc, filter->filtercode, state->stream, state->streamlength);
	filter = filter->next;
      }
    } else {
      strncpy(state->stream, buf2, len);
      state->streamlength = len;
    }
    free(buf2);
    buf2 = NULL;
  }

  /* go to current offset in stream */
  i = state->currentOffset;

  /* get next paragraph */
  fini = 0;
  l = 0;
  while (!fini) {

    /* font changed */
    if(!state->inString && !strncmp(state->stream + i, "/", 1)) {
      i++;
      state->currentOffset++;
      for(k = 0; strncmp(state->stream + i, " ", 1)
	     && strncmp(state->stream + i, "\x0A", 1)
	     && strncmp(state->stream + i, "\x0D", 1); i++) {
	strncpy(fontName + k, state->stream + i, 1);
	k++;
      }
      strncpy(fontName + k, "\0", 1);
/*      if(strncmp(state->currentFont, fontName, strlen(fontName))) {
	if (!setEncoding(desc, fontName)) {
	  strncpy(state->currentFont, fontName, strlen(fontName));
	  strncpy(state->currentFont + strlen(fontName), "\0", 1);
	}
      }*/
    }

    /* end of array ( might be an end of line ) */
    if(l < size - 1 && !state->inString && !strncmp(state->stream + i, "]", 1)) {
      strncpy(out + l, " ", 1);
      l++;
      i++;
      state->currentOffset++;
    }

    /* get normal string */
    if(l < size - 1 && (state->inString || !strncmp(state->stream + i, "(", 1))) {

      if(!strncmp(state->stream + i, "(", 1)) {
	state->inString = 1;
	i++;
	state->currentOffset++;
      }
      while (l < size - 1 && strncmp(state->stream + i, ")", 1)) {

	/* skip multiple spaces */
	while(!strncmp(state->stream + i, "  ", 2)){
	  i++;
	  state->currentOffset++;
	}

	/* escape characters */
	if(!strncmp(state->stream + i, "\\", 1)) {
	  i++;
	  state->currentOffset++;
	  if(strncmp(state->stream + i, "(", 1) &&
	     strncmp(state->stream + i, ")", 1) &&
	     strncmp(state->stream + i, "\\", 1)) {

	    if (!strncmp(state->stream + i, "n", 1) ||
		!strncmp(state->stream + i, "r", 1) ||
		!strncmp(state->stream + i, "t", 1) ||
		!strncmp(state->stream + i, "b", 1) ||
		!strncmp(state->stream + i, "f", 1)) {
	      i++;
	      state->currentOffset++;
	    } else {

	      /* octal code */
	      v = 0;
	      for(j = 0; j < 3; j++) {
		strncpy(tmp, state->stream + i + j, 1);
		strncpy(tmp + 1, "\0", 1);
		v += atoi(tmp) * pow(8, 2 - j);
	      }
	      sprintf(out + l, "%c", v);
	      l++;
	      i += 2;
	      state->currentOffset += 2;
	      escaped = 1;
	    }
	  }
	}
	/* skip useless characters */
	if(!escaped &&
	   strncmp(state->stream + i, "\x0A", 1) &&
	   strncmp(state->stream + i, "\x0D", 1) &&
	   strncmp(state->stream + i, "\x0C", 1) &&
	   strncmp(state->stream + i, "\x0B", 1)) {

	  /* charset mapping */
	  /*-------------------------------------------------------------*/

	  strncpy(out + l, state->stream + i, 1);
	  l++;
	}
	escaped = 0;
	i++;
	state->currentOffset++;
      }
      if(!strncmp(state->stream + i, ")", 1)) {
	state->inString = 0;
      }
    
      /* get hex string */
    } else if (!strncmp(state->stream + i, "<", 1)) {
      i++;
      state->currentOffset++;
      j = 0;
      v = 0;
      while(l < size - 1 && strncmp(state->stream + i, ">", 1)) {
	strncpy(tmp, state->stream + i, 1);
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
    if (l >= size - 1 || !strncmp(state->stream + i, "ET", 2)) {
      i ++;
      state->currentOffset ++;
      fini = 1;
      if (l >= size-1) {
	l = size-1;
      }
      strncpy(out + l, "\0", 1);
      return l;
    }

    i++;
    state->currentOffset++;

    /* end of stream */
    if(i >= state->streamlength) {
      fini = 1;
      free(state->stream);
      state->stream = NULL;
      state->filter = NULL;
      state->streamlength = 0;
      state->currentOffset = 0;
      if(getNextStream(desc) == -1) {
	return NO_MORE_DATA;
      }
      strncpy(out + l, "\0", 1);
      return l;
    }
  }
  
  free(state->stream);
  return NO_MORE_DATA;
  
}

int getNumber(char *buf) {
  char tmp[20];
  int t = 0;

  while (strncmp(buf + t, " ", 1) &&
	 strncmp(buf + t, "/", 1) &&
	 strncmp(buf + t, "\x0A", 1) &&
	 strncmp(buf + t, "\x0D", 1)) {
    strncpy(tmp + t, buf + t, 1);
    t++;
  }
  strncpy(tmp + t, "\0", 1);
  return atoi(tmp);
}


int applyFilter(struct doc_descriptor *desc, enum filter filter, char *buf, int buflen) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  char *inbuf;
  char _85[5], ascii[4];
  int i, j;
  unsigned long long int val;

  inbuf = (char *) malloc(buflen);
  memcpy(inbuf, buf, buflen);

  switch(filter) {

  case flateDecode:
    state->streamlength = 20*state->length;
    uncompress(state->stream, &(state->streamlength), inbuf, buflen);
    break;

  case ascii85Decode:
    i = 0;
    state->streamlength = 0;

    while(i < buflen) {

      if(!strncmp(inbuf + i, "\x7A", 1)) {
	memcpy(state->stream + state->streamlength, "\x00\x00\x00\x00", 4);
	state->streamlength += 4;
	i++;
      }
      if(inbuf[i]>=33 && buf[i]<=117) {
	val = 0;
	strncpy(_85, inbuf + i, 5);
	i += 5;
	for(j = 0; j < 5; j++) {
	  val += (_85[4 - j] - 33) * pow(85, j);
	}
	for(j = 0; j < 4; j++) {
	  ascii[3 - j] =  val % 256;
	  val /= 256;
	}
	memcpy(state->stream + state->streamlength, ascii, 4);
	state->streamlength += 4;
      } else {
	i++;
      }
    }
    break;

  default:
    memcpy(state->stream, inbuf, buflen);
    state->streamlength = buflen;
    break;

  }

  free(inbuf);
  return 0;
}


int getValue(struct doc_descriptor *desc, char *buf, int size, char *name, char *value) {
  int i, j, len, nbopened;
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
  for(j=0; j<21; j++) {
    strncpy(token + j, "\x00", 1);
  }
  sprintf(token, "/%s", name);

  nbopened = 0;
  for( ; i <= len - strlen(token) &&
	 strncmp(buf + i, token, strlen(token)) &&
	 (nbopened || strncmp(buf + i, ">>", 2)); i++) {
    if(!strncmp(buf + i, "<<", 2)) {
      nbopened++;
    }
    if(!strncmp(buf + i, ">>", 2)) {
      nbopened--;
    }
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

  while(!strncmp(buf + i, " ", 1)) {
    i++;
    if (i == len) {
      len = read(desc->fd, buf, BUFSIZE);
      i = 0;
    }
  }

  if(strncmp(buf + i, "/", 1)) {
    return -1;
  }
  i++;

  /* filling value string */
  for(j = 0; strncmp(buf + i, " ", 1)
	&& strncmp(buf + i, "/", 1)
	&& strncmp(buf + i, "\x0A", 1)
	&& strncmp(buf + i, "\x0D", 1); j++) {
    strncpy(value + j, buf + i, 1);
    i++;
    if (i == len) {
      len = read(desc->fd, buf, BUFSIZE);
      i = 0;
    }
  }
  strncpy(value + j, "\0", 1);

  return 0;
}


int initReader(struct doc_descriptor *desc) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  char buf[BUFSIZE], tmp[20], keyword[20];
  int len, i, found, t;

  state->currentStream = state->currentOffset = 0;
  state->length = 0;
  state->stream = NULL;
  state->inString = 0;
  state->encodings = NULL;
  strncpy(state->currentFont, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 10);
  state->filter = NULL;

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
      if(i == len - 5) {
	strncpy(buf, buf + i + 1, 4);
	len = read(desc->fd, buf + 4, BUFSIZE - 4) + 4;
	i = 0;
      }
    }
    if(!found) {
      fprintf(stderr, "Unable to find Root entry in trailer\n");
      return -2;
    }

    /* getting Root (catalog) reference */
    i += 4;
    while (!strncmp(buf + i, " ", 1)) {
      i++;
    }
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
    /*getEncodings(desc);*/
    len = read(desc->fd, buf, BUFSIZE);
    if (getValue(desc, buf, len, "Type", tmp) < 0) {
	fprintf(stderr, "Can't find Type\n");
	return -2;
    }

    /* finding first page */
    while(!strncmp(tmp, "Pages", 5)) {
      i = 0;

      while(i <= len - 5 && strncmp(buf + i, "/Kids", 5)) {
	if (i == len - 5) {
	  strncpy(buf, buf + i, 5);
	  len = read(desc->fd, buf + 5, BUFSIZE - 5) + 5;
	  i = 0;
	} else {
	  i++;
	}
      }
      i += 5;
      while(!strncmp(buf + i, " ", 1)) {
	i++;
	if (i == len) {
	  len = read(desc->fd, buf, BUFSIZE);
	  i = 0;
	}
      }
      i++;
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
      /*getEncodings(desc);*/
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
      while (!strncmp(buf + i, " ", 1)) {
	i++;
	if (i == len) {
	  len = read(desc->fd, buf, BUFSIZE);
	  i = 0;
	}
      }
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
  if(len - i < 11) {
    strncpy(buf, buf + i, len - i);
    len = read(desc->fd, buf + len - i, 10);
    i = 0;
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
	 && strncmp(input + i, "\x0D", 1)
	 && strncmp(input + i, "\\", 1); i++) {
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


int getEncodings(struct doc_descriptor *desc) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  int len, len2, i, j, prevPosition;
  char buf[BUFSIZE], buf2[BUFSIZE];
  char name[10], value[20];
  struct encodingTable *encoding = state->encodings;
  int nbopened;

  prevPosition = lseek(desc->fd, 0, SEEK_CUR);

  /* search fonts in current object */
  len = read(desc->fd, buf, BUFSIZE);

  /* search dictionary */
  for (i = 0; i < len - 1 && strncmp(buf + i, "<<", 2); i++) {}
  i += 2;

  nbopened = 0;
  for ( ; i < len - 9 && (nbopened || strncmp(buf + i, ">>", 2)) && strncmp(buf + i, "/Resources", 10); i++) {
    if(!strncmp(buf + i, "<<", 2)) {
      nbopened++;
    }
    if(strncmp(buf + i, ">>", 2)) {
      nbopened--;
    }
  }

  /* if no ressources available */
  if (strncmp(buf + i, "/Resources", 10)) {
    lseek(desc->fd, prevPosition, SEEK_SET);
    return 0;
  }


  i += 10;
  while (!strncmp(buf + i, " ", 1)) {
    i++;
  }
  
  /* enter ressources */
  gotoRef(desc, state->xref, getNumber(buf + i));
  len = read(desc->fd, buf, BUFSIZE);

  /* search dictionary */
  for (i = 0; i < len - 1 && strncmp(buf + i, "<<", 2); i++) {}
  i += 2;

  nbopened = 0;
  for ( ; i < len - 4 && (nbopened || strncmp(buf + i, ">>", 2)) && strncmp(buf + i, "/Font", 5); i++) {
    if(!strncmp(buf + i, "<<", 2)) {
      nbopened++;
    }
    if(strncmp(buf + i, ">>", 2)) {
      nbopened--;
    }
  }

  /* if no font available */
  if(strncmp(buf + i, "/Font", 5)) {
    lseek(desc->fd, prevPosition, SEEK_SET);
    return 0;
  }
  i += 5;
  while (!strncmp(buf + i, " ", 1)) {
    i++;
  }
  
  /* enter fonts */
  if(strncmp(buf + i, "<<", 2)) {
    gotoRef(desc, state->xref, getNumber(buf + i));
    len = read(desc->fd, buf, BUFSIZE);
    i = 0;
  }

  /* search dictionary */
  for ( ; i < len - 1 && strncmp(buf + i, "<<", 2); i++) {}
  i += 2;

  nbopened = 0;  
  for ( ; i < len - 1 && (nbopened || strncmp(buf + i, ">>", 2)); i++) {
    if(!strncmp(buf + i, "<<", 2)) {
      nbopened++;
    }
    if(strncmp(buf + i, ">>", 2)) {
      nbopened--;
    }

    if(!strncmp(buf + i, "/", 1)) {
      i++;
      strncpy(name, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 10);
      getKeyword(buf + i, name);

     /* is the font already registered */
      if(encoding == NULL) {
	state->encodings = (struct encodingTable *) malloc(sizeof(struct encodingTable));
	encoding = state->encodings;
	encoding->fontName = NULL;
      } else {
	for ( ; encoding->next != NULL && strncmp(encoding->fontName, name, strlen(name));
	      encoding = encoding->next) {}
	if (strncmp(encoding->fontName, name, strlen(name))) {

	  encoding->next = (struct encodingTable *) malloc(sizeof(struct encodingTable));
	  encoding = encoding->next;
	  encoding->fontName = NULL;
	}
      }
      if (encoding->fontName == NULL) {
	encoding->next = NULL;
	encoding->fontName = (char *) malloc(strlen(name) + 1);
	strncpy(encoding->fontName, name, strlen(name));
	strncpy((encoding->fontName) + strlen(name), "\0", 1);
	
	/* get font encoding */
	i += strlen(name) + 1;
	gotoRef(desc, state->xref, getNumber(buf + i));
	len2 = read(desc->fd, buf2, BUFSIZE);

	/* search dictionary */
	for (j = 0; j < len2 - 1 && strncmp(buf2 + j, "<<", 2); j++) {}
	j += 2;

	nbopened = 0;
	for ( ; j < len2 - 8 && strncmp(buf2 + j, "/Encoding", 9)
		&& (nbopened || strncmp(buf2 + j, ">>", 2)); j++) {
	  if(!strncmp(buf2 + j, "<<", 2)) {
	    nbopened++;
	  }
	  if(strncmp(buf2 + j, ">>", 2)) {
	    nbopened--;
	  }
	}

	if (!strncmp(buf2 + j, "/Encoding", 9)) {
	  j += 9;
	  while (!strncmp(buf + i, " ", 1)) {
	    i++;
	  }

	  /* is it a name or a dictionary ? */
	  if(!strncmp(buf2 + j, "/", 1)) {
	    j++;
	    strncpy(value, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 20);
	    getKeyword(buf2 + j, value);

	    if(!strncmp(value, "WinAnsiEncoding", 15)) {
	      encoding->encoding = (char *) malloc(7);
	      strncpy(encoding->encoding, "latin1\0", 7);

	    } else if(!strncmp(value, "MacRomanEncoding", 16)) {
	      encoding->encoding = (char *) malloc(7);
	      strncpy(encoding->encoding, "latin1\0", 7);

	    } else if(!strncmp(value, "MacExpertEncoding", 17)) {
	      encoding->encoding = (char *) malloc(7);
	      strncpy(encoding->encoding, "latin1\0", 7);

	    } else if(!strncmp(value, "PDFDocEncoding", 14)) {
	      encoding->encoding = (char *) malloc(7);
	      strncpy(encoding->encoding, "latin1\0", 7);

	    } else {
	      encoding->encoding = (char *) malloc(7);
	      strncpy(encoding->encoding, "latin1\0", 7);
	    }

	  } else {
	    encoding->encoding = (char *) malloc(7);
	    strncpy(encoding->encoding, "latin1\0", 7);	    
	  }
	}
      }
    }
  }
  lseek(desc->fd, prevPosition, SEEK_SET);
  return OK;
}


int setEncoding(struct doc_descriptor *desc, char *fontName) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  struct encodingTable *encoding = state->encodings;
  UErrorCode err;

  /* search font in table */
  while(encoding != NULL && strcmp(encoding->fontName, fontName)) {
    encoding = encoding->next;
  }

  /* update ICU converter encoding */
  if(encoding != NULL) {
    ucnv_close(desc->conv);
    err = U_ZERO_ERROR;
    desc->conv = ucnv_open(encoding->encoding, &err);
    if (U_FAILURE(err)) {
      fprintf(stderr, "unable to open ICU converter\n");
      return ERR_ICU;
    }
  } else {
    return -1;
  }

  return OK;
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
