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
#include <ctype.h>

int getText(struct doc_descriptor *desc, UChar *out, int size) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  char buf[BUFSIZE];
  int len, i;
  int nbopened;

  if(state->currentStream == -1) {
    return NO_MORE_DATA;
  }

  /* enter current page */
  gotoRef(desc, state->currentPage);
  len = readObject(desc, buf, BUFSIZE);

  /* search dictionary */
  for (i = 0; strncmp(buf + i, "<<", 2); i++) {
    if (i >= len - 2) {
      strncpy(buf, buf + i + 1, 1);
      len = readObject(desc, buf + 1, BUFSIZE - 1) + 1;
      i = 0;
    }
  }
  /* search content */
  nbopened = 0;
  while ((nbopened || strncmp(buf + i, ">>", 2)) && strncmp(buf + i, "/Contents", 9)) {

    if(!strncmp(buf + i, "<<", 2)) {
      nbopened++;
    } else if(!strncmp(buf + i, ">>", 2)) {
      nbopened--;
    }
    i++;
    if (i >= len - 9) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
  }

  /* if content is found */
  if (!strncmp(buf + i, "/Contents", 9)) {
    if (state->currentStream == 0) {
      
      i += 9;
      while(!strncmp(buf + i, " ", 1) ||
	    !strncmp(buf + i, "\x0A", 1) ||
	    !strncmp(buf + i, "\x0D", 1)) {
	i++;
	if (i >= len) {
	  len = readObject(desc, buf, BUFSIZE);
	  i = 0;
	}
      }
      if(!strncmp(buf + i, "[", 1)) {
	/* this is an array of streams */
	
	/* get first stream */
	i++;
	if (i >= len) {
	  len = readObject(desc, buf, BUFSIZE);
	  i = 0;
	}
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if (i >= len) {
	    len = readObject(desc, buf, BUFSIZE);
	    i = 0;
	  }
	}
	if (i <= len - 10) {
	  strncpy(buf, buf + i, len - i);
	  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	  i = 0;
	}
	state->currentStream = getNumber(buf + i);

      } else {
	/* unique stream */
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if (i >= len) {
	    len = readObject(desc, buf, BUFSIZE);
	    i = 0;
	  }
	}
	if (i <= len - 10) {
	  strncpy(buf, buf + i, len - i);
	  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	  i = 0;
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
      state->currentStream = -1;
      return NO_MORE_DATA;
    }
  }

  state->currentStream = -1;
  return NO_MORE_DATA;
}


int getNextStream(struct doc_descriptor *desc) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  char buf[BUFSIZE], tmp[20];
  int len, i;
  int nbopened;

  gotoRef(desc, state->currentPage);
  len = readObject(desc, buf, BUFSIZE);
  for(i = 0; i <= len - 9 && strncmp(buf + i, "/Contents", 9); i++) {
    if (i >= len- 9) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
  }
  i += 9;
  if (i >= len) {
    len = readObject(desc, buf, BUFSIZE);
    i = 0;
  }
  while(!strncmp(buf + i, " ", 1) ||
	!strncmp(buf + i, "\x0A", 1) ||
	!strncmp(buf + i, "\x0D", 1)) {
    i++;
    if (i >= len) {
      len = readObject(desc, buf, BUFSIZE);
      i = 0;
    }
  }

  if (!strncmp(buf + i, "[", 1)) {
    strncpy(tmp, "\x00\x00\x00\x00\x00\x00", 6);
    sprintf(tmp, "%d ", state->currentStream);
    while(strncmp(buf + i, tmp, strlen(tmp))) {
      i++;
      if (i >= len- 6) {
	strncpy(buf, buf + i, len - i);
	len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	i = 0;
      }
    }
    i +=  strlen(tmp);
    if (i >= len) {
      len = readObject(desc, buf, BUFSIZE);
      i = 0;
    }
    for( ; strncmp(buf + i, "R", 1); i++) {
      if (i >= len) {
	len = readObject(desc, buf, BUFSIZE);
	i = 0;
      }
    }
    i++;
    if (i >= len) {
      len = readObject(desc, buf, BUFSIZE);
      i = 0;
    }
    for( ; !strncmp(buf + i, " ", 1) ||
	   !strncmp(buf + i, "\x0A", 1) ||
	   !strncmp(buf + i, "\x0D", 1); i++) {
      if (i >= len) {
	len = readObject(desc, buf, BUFSIZE);
	i = 0;
      }
    }
    if(strncmp(buf + i, "]", 1)) {
      if (i >= len - 10) {
	strncpy(buf, buf + i, len - i);
	len = readObject(desc, buf + len - i, BUFSIZE - len  + i) + len - i;
	i = 0;
      }
      state->currentStream = getNumber(buf + i);
      state->currentOffset = 0;
      return state->currentStream;
    }
  }
  
  /* next stream is in another page */
  state->newPage = 1;
  do {
    if (getNextPage(desc) < 0) {
      state->currentStream = -1;
      return NO_MORE_DATA;
    }
    gotoRef(desc, state->currentPage);
    len = readObject(desc, buf, BUFSIZE);

    /* search dictionary */
    for (i = 0; strncmp(buf + i, "<<", 2); i++) {
      if (i >= len - 2) {
	strncpy(buf, buf + i, len - i);
	len = readObject(desc, buf + len - i, BUFSIZE - len  + i) + len - i;
	i = 0;
      }
    }
    
    /* search content */
    nbopened = 0;
    for( ;(nbopened || strncmp(buf + i, ">>", 2))
	   && strncmp(buf + i, "/Contents", 9); i++) {
      if (i >= len - 9) {
	strncpy(buf, buf + i, len - i);
	len = readObject(desc, buf + len - i, BUFSIZE - len  + i) + len - i;
	i = 0;
      }
      if(!strncmp(buf + i, "<<", 2)) {
	nbopened++;
      } else if(!strncmp(buf + i, ">>", 2)) {
	nbopened--;
      }
    }
     
  } while(strncmp(buf + i, "/Contents", 9));

  i += 9;
  if (i >= len ) {
    len = readObject(desc, buf, BUFSIZE);
    i = 0;
  }
  while(!strncmp(buf + i, " ", 1) ||
	!strncmp(buf + i, "\x0A", 1) ||
	!strncmp(buf + i, "\x0D", 1)) {
    i++;
    if (i >= len ) {
      len = readObject(desc, buf, BUFSIZE);
      i = 0;
    }
  }
  if(!strncmp(buf + i, "[", 1)) {
    /* this is an array of streams */
    
    /* get first stream */
    i++;
    if (i >= len - 10) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len  + i) + len - i;
      i = 0;
    }
    for( ;!strncmp(buf + i, " ", 1) ||
	   !strncmp(buf + i, "\x0A", 1) ||
	   !strncmp(buf + i, "\x0D", 1); i++) {
      if (i <= len - 10) {
	strncpy(buf, buf + i, len - i);
	len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	i = 0;
      }
    }
    if (i <= len - 10) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
    state->currentStream = getNumber(buf + i);
    state->currentOffset = 0;
    return state->currentStream;
    
  } else {
    /* unique stream */
    for( ;!strncmp(buf + i, " ", 1) ||
	   !strncmp(buf + i, "\x0A", 1) ||
	   !strncmp(buf + i, "\x0D", 1); i++) {
      if (i >= len - 9) {
	strncpy(buf, buf + i, len - i);
	len = readObject(desc, buf + len - i, BUFSIZE - len  + i) + len - i;
	i = 0;
      }
    }
    if (i <= len - 10) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
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
  gotoRef(desc, state->currentPage);
  len = readObject(desc, buf, BUFSIZE);
  strncpy(tmp, buf + 29, 8);
  nbopened = 0;
  for(i = 0; strncmp(buf + i, "/Parent", 7); i++) {
    if (i >= len - 7) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len  + i) + len - i;
      i = 0;
    }
    if(!strncmp(buf + i, "<<", 2)) {
      nbopened++;
    }
    if(!strncmp(buf + i, ">>", 2)) {
      nbopened--;
      if(!nbopened) {
	/* end of document */
	desc->nb_pages_read++;
	return NO_MORE_DATA;
      }
    }
  }

  if (i >= len - 7) {
    strncpy(buf, buf + i, len - i);
    len = readObject(desc, buf + len - i, BUFSIZE - len  + i) + len - i;
    i = 0;
  }
       
  i += 7;
  if (i >= len) {
    len = readObject(desc, buf, BUFSIZE);
    i = 0;
  }
  while(!strncmp(buf + i, " ", 1) ||
	!strncmp(buf + i, "\x0A", 1) ||
	!strncmp(buf + i, "\x0D", 1)) {
    i++;
    if (i >= len - 10) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len  + i) + len - i;
      i = 0;
    }
  }
  if (i <= len - 10) {
    strncpy(buf, buf + i, len - i);
    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
    i = 0;
  }
  parentRef = getNumber(buf + i);
  gotoRef(desc, parentRef);

  /* search current page in kids */
  len = readObject(desc, buf, BUFSIZE);
  for(i = 0; i <= len - 5 && strncmp(buf + i, "/Kids", 5); i++) {
    if (i >= len - 5) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len  + i) + len - i;
      i = 0;
    }
  }
  while(!strncmp(buf + i, " ", 1) ||
	!strncmp(buf + i, "\x0A", 1) ||
	!strncmp(buf + i, "\x0D", 1)) {
    i++;
    if (i >= len - 7) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len  + i) + len - i;
      i = 0;
    }
  }
  strncpy(tmp, "\x00\x00\x00\x00\x00\x00", 6);
  sprintf(tmp, "%d ", state->currentPage);

  for(i-- ; i <= len - strlen(tmp) - 1 &&
	 (isdigit(buf[i]) || strncmp(buf + i + 1, tmp, strlen(tmp))); i++) {
    if (i >= len - 7) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len  + i) + len - i;
      i = 0;
    }
  }
  i += strlen(tmp);
  if (i >= len) {
    len = readObject(desc, buf, BUFSIZE);
    i = 0;
  }

  /* skip end of reference */
  for( ; strncmp(buf + i, "R", 1); i++) {
    if (i >= len) {
      len = readObject(desc, buf, BUFSIZE);
      i = 0;
    }
  }
  i++;
  for( ; !strncmp(buf + i, " ", 1) ||
	 !strncmp(buf + i, "\x0A", 1) ||
	 !strncmp(buf + i, "\x0D", 1); i++) {
    if (i >= len - 10) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len  + i) + len - i;
      i = 0;
    }    
  }
  if (i >= len - 10) {
    strncpy(buf, buf + i, len - i);
    len = readObject(desc, buf + len - i, BUFSIZE - len  + i) + len - i;
    i = 0;
  }    

  /* get next page */
  if(strncmp(buf + i, "]", 1)) {
    if (i <= len - 10) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
    state->currentPage = getNumber(buf + i);
    gotoRef(desc, state->currentPage);
    getEncodings(desc);
    gotoRef(desc, state->currentPage);
    len = readObject(desc, buf, BUFSIZE);
    strncpy(tmp, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 10);
    i = 0;
    if (getValue(desc, buf, len, "Type", tmp)) {
      fprintf(stderr, "Can't find Type\n");
      return ERR_DICTIONARY;
    }
    while(!strncmp(tmp, "Pages", 5)) {
      i = 0;
      
      while(i <= len - 5 && strncmp(buf + i, "/Kids", 5)) {
	if (i >= len - 5) {
	  strncpy(buf, buf + i, 5);
	  len = readObject(desc, buf + 5, BUFSIZE - 5) + 5;
	  i = 0;
	} else {
	  i++;
	}
      }
      i += 5;
      while (!strncmp(buf + i, " ", 1) ||
	     !strncmp(buf + i, "\x0A", 1) ||
	     !strncmp(buf + i, "\x0D", 1)) {
	i++;
	if (i >= len) {
	  len = readObject(desc, buf, BUFSIZE);
	  i = 0;
	}
      }
      i++;
      while (!strncmp(buf + i, " ", 1) ||
	     !strncmp(buf + i, "\x0A", 1) ||
	     !strncmp(buf + i, "\x0D", 1)) {
	i++;
	if (i >= len) {
	  len = readObject(desc, buf, BUFSIZE);
	  i = 0;
	}
      }
      t = 0;
      while (strncmp(buf + i, " ", 1) &&
	     strncmp(buf + i, "\x0A", 1) &&
	     strncmp(buf + i, "\x0D", 1)) {
	strncpy(tmp + t, buf + i, 1);
	t++;
	i++;
	if (i >= len) {
	  len = readObject(desc, buf, BUFSIZE);
	  i = 0;
	}
      }
      strncpy(tmp + t, "\0", 1);
      gotoRef(desc, atoi(tmp));
      state->currentPage = atoi(tmp);
      getEncodings(desc);
      gotoRef(desc, state->currentPage);

      len = readObject(desc, buf, BUFSIZE);
      strncpy(tmp, "\x00\x00\x00\x00\x00\x00", 6);
      if (i >= len - 6) {
	strncpy(buf, buf + i, len - i);
	len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	i = 0;
      }
      if (getValue(desc, buf, len, "Type", tmp) < 0) {
	fprintf(stderr, "Can't find Type\n");
	return ERR_DICTIONARY;
      }
    }
    state->currentStream = 0;
    state->currentOffset = 0;
    desc->nb_pages_read++;
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


int procedeStream(struct doc_descriptor *desc, UChar *out, int size) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  enum filter  tfilter;
  struct pdffilter *filter;
  int beginByte;
  int escaped;
  int len, i, j, k, l, fini, v;
  char buf[BUFSIZE], tmp[20];
  char buf3[BUFSIZE];
  char fontName[12];

  escaped = 0;
  if(!state->stream) {
    state->stream = 1;
    gotoRef(desc, state->currentStream);
    len = read(desc->fd, buf, BUFSIZE);

    /* search dictionary */
    for (i = 0; strncmp(buf + i, "<<", 2); i++) {
      if (i >= len - 2) {
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
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if (i >= len - 7) {
	    strncpy(buf, buf + i, len - i);
	    len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
	    i = 0;
	  }
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
	  if (i <= len - 10) {
	    strncpy(buf, buf + i, len - i);
	    len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
	    i = 0;
	  }
	  state->length = state->streamlength = getNumber(buf + i);

	  /* Length is a reference */
	} else {
	  v = lseek(desc->fd, 0, SEEK_CUR);
	  if (i <= len - 10) {
	    strncpy(buf, buf + i, len - i);
	    len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
	    i = 0;
	  }

	  gotoRef(desc, getNumber(buf + i));
	  l = read(desc->fd, buf3, BUFSIZE);
	  j = getNextLine(buf3, l);
	  while(!strncmp(buf3 + j, " ", 1) ||
		!strncmp(buf3 + j, "\x0A", 1) ||
		!strncmp(buf3 + j, "\x0D", 1)) {
	    j++;
	  }
	  state->length = state->streamlength = getNumber(buf3 + j);
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
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	}

	/* array of filters */
	if(!strncmp(buf + i, "[", 1)) {

	  while(strncmp(buf + i, "]", 1)) {
   
	    if (!strncmp(buf + i, "/", 1)) {
	      i++;
	      j = 0;
	      while(strncmp(buf + i, " ", 1) &&
		    strncmp(buf + i, "\x0A", 1) &&
		    strncmp(buf + i, "\x0D", 1)) {
		strncpy(tmp + j, buf + i, 1);
		j++;
		i++;
		if (i >= len) {
		  len = read(desc->fd, buf, BUFSIZE);
		  i = 0;
		}
	      }
	      strncpy(tmp + j, "\0", 1);

	      tfilter = identifyFilter(tmp);
	      switch (tfilter) {
	      case lzw:
		fprintf(stderr, "LZW decompression not implemented\n");
		state->currentStream = -1;
		return NO_MORE_DATA;
		break;
	      case crypt:
		fprintf(stderr, "Encrypted document : operation aborted\n");
		state->currentStream = -1;
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
	    if (i >= len) {
	      len = read(desc->fd, buf, BUFSIZE);
	      i = 0;
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
	  if (i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	  while(strncmp(buf + i, " ", 1) &&
		strncmp(buf + i, ">", 1) &&
		strncmp(buf + i, "\x0A", 1) &&
		strncmp(buf + i, "\x0D", 1)) {
	    strncpy(tmp + j, buf + i, 1);
	    i++;
	    j++;
	    if (i >= len) {
	      len = read(desc->fd, buf, BUFSIZE);
	      i = 0;
	    }
	  }
	  i--;
	  strncpy(tmp + j, "\0", 1);
	  tfilter = identifyFilter(tmp);
	  switch (tfilter) {
	  case lzw:
	    fprintf(stderr, "LZW decompression not implemented\n");
	    state->currentStream = -1;
	    return NO_MORE_DATA;
	    break;
	  case crypt:
	    fprintf(stderr, "Encrypted document : operation aborted\n");
	    state->currentStream = -1;
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
      if (i >= len - 6) {
	strncpy(buf, buf + i, len - i);
	len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
	i = 0;
      }
    }
    i += getNextLine(buf + i, len - i);
    beginByte = lseek(desc->fd, i - len, SEEK_CUR);
  }
  state->objectStream = state->currentStream;
  state->offsetInStream = state->currentOffset;
  state->first = 0;
  len = readObject(desc, buf, BUFSIZE);
  state->objectStream = state->currentStream;

  /* get next paragraph */
  i = 0;
  fini = 0;
  l = 0;
  memset(out, '\x00', size);
  /* end of stream */
  if(len <= 0) {
    fini = 1;
    state->stream = 0;
    freeFilterStruct(state->filter);
    state->filter = NULL;
    state->currentOffset = 0;
    getNextStream(desc);
    if(state->newPage) {
      memcpy(out + l, "\x20\x00", 2);
      l += 2;
      state->newPage = 0;
    }
    memcpy(out + l, "\x00\x00", 2);
    return l;
  }

  while (!fini) {

    /* font changed */
    if(!state->inString && !strncmp(buf + i, "/", 1)) {

      for(k = 0; strncmp(buf + i, " ", 1)
	    && strncmp(buf + i, "\x0A", 1)
	    && strncmp(buf + i, "\x0D", 1); ) {
	i++;
	state->currentOffset++;
	state->offsetInStream = state->currentOffset;
	if(i >= len - 2) {
	  strncpy(buf, buf + i, len - i);
	  len = readObject(desc, buf + len - i , BUFSIZE - len + i) + len - i;
	  state->objectStream = state->currentStream;
	  i = 0;
	}
	/* end of stream */
	if(len <= 0) {
	  fini = 1;
	  state->stream = 0;
	  freeFilterStruct(state->filter);
	  state->filter = NULL;
	  state->currentOffset = 0;
	  if(state->last_available) {
	    out[l/2] = state->last;
	    l += 2;
	    state->last = 0;
	    state->last_available = 0;
	  }
	  getNextStream(desc);
	  if(state->newPage) {
	    memcpy(out + l, "\x20\x00", 2);
	    l += 2;
	    state->newPage = 0;
	  }
	  memcpy(out + l, "\x00\x00", 2);
	  return l;
	}
	strncpy(fontName + k, buf + i, 1);
	k++;
      }
      strncpy(fontName + k, "\0", 1);

      if(state->currentEncoding == NULL ||
	 strncmp(state->currentEncoding->fontName, fontName, strlen(fontName))) {
	setEncoding(desc, fontName);
      }
    }

    /* get normal string */
    if(l < size - 6 && (state->inString || !strncmp(buf + i, "(", 1))) {
      if(!state->inString && !strncmp(buf + i, "(", 1)) {
	state->inString = 1;
	i++;
	state->currentOffset++;
	state->offsetInStream = state->currentOffset;
	if(i >= len - 2) {
	  strncpy(buf, buf + i, len - i);
	  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	  state->objectStream = state->currentStream;
	  i = 0;
	}
	/* end of stream */
	if(len <= 0) {
	  fini = 1;
	  state->stream = 0;
	  state->inString = 0;
	  freeFilterStruct(state->filter);
	  state->filter = NULL;
	  state->currentOffset = 0;
	  if(state->last_available) {
	    out[l/2] = state->last;
	    l += 2;
	    state->last = 0;
	    state->last_available = 0;
	  }
	  getNextStream(desc);
	  if(state->newPage) {
	    memcpy(out + l, "\x20\x00", 2);
	    l += 2;
	    state->newPage = 0;
	  }
	  memcpy(out + l, "\x00\x00", 2);
	  return l;
	}
      }
      while (l < size - 6 && strncmp(buf + i, ")", 1)) {

	/* skip multiple spaces */
	while(!strncmp(buf + i, "  ", 2)){
	  i++;
	  state->currentOffset++;
	  state->offsetInStream = state->currentOffset;
	  if(i >= len - 2) {
	    strncpy(buf, buf + i, len - i);
	    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	    state->objectStream = state->currentStream;
	    i = 0;
	  }
	  /* end of stream */
	  if(len <= 0) {
	    fini = 1;
	    state->stream = 0;
	    state->inString = 0;
	    freeFilterStruct(state->filter);
	    state->filter = NULL;
	    state->currentOffset = 0;
	    if(state->last_available) {
	      out[l/2] = state->last;
	      l += 2;
	      state->last = 0;
	      state->last_available = 0;
	    }
	    getNextStream(desc);
	    if(state->newPage) {
	      memcpy(out + l, "\x20\x00", 2);
	      l += 2;
	      state->newPage = 0;
	    }
	    memcpy(out + l, "\x00\x00", 2);
	    return l;
	  }
	}

	/* escape characters */
	if(!strncmp(buf + i, "\\", 1)) {
	  i++;
	  state->currentOffset++;
	  state->offsetInStream = state->currentOffset;
	  if(i >= len - 2) {
	    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	    state->objectStream = state->currentStream;
	    i = 0;
	  }
	  if(len <= 0) {
	    fini = 1;
	    state->stream = 0;
	    state->inString = 0;
	    freeFilterStruct(state->filter);
	    state->filter = NULL;
	    state->currentOffset = 0;
	    if(state->last_available) {
	      out[l/2] = state->last;
	      l += 2;
	      state->last = 0;
	      state->last_available = 0;
	    }
	    getNextStream(desc);
	    if(state->newPage) {
	      memcpy(out + l, "\x20\x00", 2);
	      l += 2;
	      state->newPage = 0;
	    }
	    memcpy(out + l, "\x00\x00", 2);
	    return l;
	  }
	  
	  if(strncmp(buf + i, "(", 1) &&
	     strncmp(buf + i, ")", 1) &&
	     strncmp(buf + i, "\\", 1)) {

	    if (!strncmp(buf + i, "n", 1) ||
		!strncmp(buf + i, "r", 1) ||
		!strncmp(buf + i, "t", 1) ||
		!strncmp(buf + i, "b", 1) ||
		!strncmp(buf + i, "f", 1)) {
	      memcpy(out + l, "\x20\x00", 2);
	      l+=2;
	      if(l >= size - 2) {
		fini = 1;
		state->stream = 0;
		freeFilterStruct(state->filter);
		state->filter = NULL;
		state->currentOffset = 0;
		if(state->last_available) {
		  out[l/2] = state->last;
		  l += 2;
		  state->last = 0;
		  state->last_available = 0;
		}
		memcpy(out + l, "\x00\x00", 2);
 		return l;
	      }
	      escaped = 1;

	    } else {

	      /* octal code */
	      v = 0;
	      for(j = 0; j < 3; j++) {
		strncpy(tmp, buf + i + j, 1);
		strncpy(tmp + 1, "\0", 1);
		v += atoi(tmp) * pow(8, 2 - j);
	      }
	      mapCharset(desc, v, out, &l);
	      i += 2;
	      state->currentOffset += 2;
	      state->offsetInStream = state->currentOffset;
	      if(i >= len - 2) {
		strncpy(buf, buf + i, len - i);
		len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
		state->objectStream = state->currentStream;
		i = 0;
	      }
	      escaped = 1;
	    }
	  }
	}
	/* skip useless characters */
	if(!escaped &&
	   strncmp(buf + i, "\x0A", 1) &&
	   strncmp(buf + i, "\x0D", 1) &&
	   strncmp(buf + i, "\x0C", 1) &&
	   strncmp(buf + i, "\x0B", 1)) {


	  v = 0;
	  memcpy(&v, buf + i, 1);
	  mapCharset(desc, v, out, &l);

	}
	escaped = 0;
	i++;
	state->currentOffset++;
	state->offsetInStream = state->currentOffset;
	if(len > 0 && i >= len - 2) {
	  strncpy(buf, buf + i, len - i);
	  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	  state->objectStream = state->currentStream;
	  i = 0;
	}
      }
      if(!strncmp(buf + i, ")", 1)) {
	state->inString = 0;
      }
    
      /* get hex string */
    } else if (!strncmp(buf + i, "<", 1)) {
      i++;
      state->currentOffset++;
      state->offsetInStream = state->currentOffset;
      if(i >= len - 2) {
	strncpy(buf, buf + i, len - i);
	len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	state->objectStream = state->currentStream;
	i = 0;
      }
      j = 0;
      while(l < size - 6 && strncmp(buf + i, ">", 1)) {
	v = 0;
	for(j = 0; j < 2; j++) {
	  strncpy(tmp, buf + i, 1);
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
	  } else if(!strncmp(tmp, ">", 1)) {
	    v += 0;
	  } else {
	    v += atoi(tmp);
	  }
	  if(strncmp(tmp, ">", 1)) {
	    i++;
	    state->currentOffset++;
	    state->offsetInStream = state->currentOffset;
	    if(i >= len - 2) {
	      strncpy(buf, buf + i, len - i);
	      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	      if(len <= 0) {
		fini = 1;
		state->stream = 0;
		state->inString = 0;
		freeFilterStruct(state->filter);
		state->filter = NULL;
		state->currentOffset = 0;
		if(state->last_available) {
		  out[l/2] = state->last;
		  l += 2;
		  state->last = 0;
		  state->last_available = 0;
		}
		getNextStream(desc);
		if(state->newPage) {
		  memcpy(out + l, "\x20\x00", 2);
		  l += 2;
		  state->newPage = 0;
		}
		memcpy(out + l, "\x00\x00", 2);
		return l;
	      }
	      state->objectStream = state->currentStream;
	      i = 0;
	    }
	  }
	}
	mapCharset(desc, v, out, &l);
      }
    }

    /* release output */
    if (l >= size - 6 || !strncmp(buf + i, "ET", 2)) {
      i++;
      state->currentOffset++;
      state->offsetInStream = state->currentOffset;
      if(state->last_available && l < size - 6) {
	out[l/2] = state->last;
	l += 2;
	state->last = 0;
	state->last_available = 0;
      }
      if(i >= len - 2) {
	strncpy(buf, buf + i, len - i);
	len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	state->objectStream = state->currentStream;
	i = 0;
      }

      memcpy(out + l, "\x00\x00", 2);

      /* end of stream */
      if(len <= 0) {
	fini = 1;
	state->stream = 0;
	freeFilterStruct(state->filter);
	state->filter = NULL;
	state->currentOffset = 0;
	if(state->last_available) {
	  out[l/2] = state->last;
	  l += 2;
	  state->last = 0;
	  state->last_available = 0;
	}
	getNextStream(desc);
	if(state->newPage) {
	  memcpy(out + l, "\x20\x00", 2);
	  l += 2;
	  state->newPage = 0;
	}
	memcpy(out + l, "\x00\x00", 2);
	return l;
      }
      fini = 1;
      return l;
    }

    i++;
    state->currentOffset++;
    state->offsetInStream = state->currentOffset;
    if(i >= len) {
      len = readObject(desc, buf, BUFSIZE);
      state->objectStream = state->currentStream;
      i = 0;
    }

    /* end of stream */
    if(len <= 0) {
      fini = 1;
      state->stream = 0;
      freeFilterStruct(state->filter);
      state->filter = NULL;
      state->currentOffset = 0;
      if(state->last_available) {
	out[l/2] = state->last;
	l += 2;
	state->last = 0;
	state->last_available = 0;
      }
      getNextStream(desc);
      if(state->newPage) {
	memcpy(out + l, "\x20\x00", 2);
	l += 2;
	state->newPage = 0;
      }
      memcpy(out + l, "\x00\x00", 2);
      return l;
    }
  }
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


int getValue(struct doc_descriptor *desc, char *buf, int size, char *name, char *value) {
  int i, j, len, nbopened;
  char token[21];
  char buf2[BUFSIZE];

  strncpy(buf2, buf, size);

  /* finding begining of dictionary */
  len = size;
  for(i = 0; i < len - 1 && strncmp(buf2 + i, "<<", 2); i++) {
    if(i >= len - 1) {
      strncpy(buf2, buf2 + i, len - i);
      len = readObject(desc, buf2 + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
  }
  i += 2;

  if(i >= len - strlen(token)) {
    strncpy(buf2, buf2 + i, len - i);
    len = readObject(desc, buf2 + len - i, BUFSIZE - len + i) + len - i;
    i = 0;
  }
  /* finding desired field */
  for(j=0; j<21; j++) {
    strncpy(token + j, "\x00", 1);
  }
  sprintf(token, "/%s", name);

  nbopened = 0;
  for( ; i <= len - strlen(token) &&
	 strncmp(buf2 + i, token, strlen(token)) &&
	 (nbopened || strncmp(buf2 + i, ">>", 2)); i++) {
    if(i >= len - strlen(token)) {
      strncpy(buf2, buf2 + i, len - i);
      len = readObject(desc, buf2 + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }

    if(!strncmp(buf2 + i, "<<", 2)) {
      nbopened++;
    } else if(!strncmp(buf2 + i, ">>", 2)) {
      nbopened--;
    }
  }

  if (!strncmp(buf2 + i, ">>", 2)) {
    return ERR_DICTIONARY;
  }

  if(i >= len - strlen(token)) {
    strncpy(buf2, buf2 + i, len - i);
    len = readObject(desc, buf2 + len - i, BUFSIZE - len + i) + len - i;
    i = 0;
  }

  i += strlen(token);

  if(i >= len - 20) {
    strncpy(buf2, buf2 + i, len - i);
    len = readObject(desc, buf2 + len - i, BUFSIZE - len + i) + len - i;
    i = 0;
  }

  while(!strncmp(buf2 + i, " ", 1) ||
	!strncmp(buf2 + i, "\x0A", 1) ||
	!strncmp(buf2 + i, "\x0D", 1)) {
    i++;
    if(i >= len - 20) {
      strncpy(buf2, buf2 + i, len - i);
      len = readObject(desc, buf2 + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
  }

  if(strncmp(buf2 + i, "/", 1)) {
    return ERR_DICTIONARY;
  }
  i++;
  if(i >= len - 20) {
    strncpy(buf2, buf2 + i, len - i);
    len = readObject(desc, buf2 + len - i, BUFSIZE - len + i) + len - i;
    i = 0;
  }

  /* filling value string */
  for(j = 0; strncmp(buf2 + i, " ", 1)
	&& strncmp(buf2 + i, "/", 1)
	&& strncmp(buf2 + i, "\x0A", 1)
	&& strncmp(buf2 + i, "\x0D", 1); j++) {
    strncpy(value + j, buf2 + i, 1);
    i++;
    if(i >= len - 20) {
      strncpy(buf2, buf2 + i, len - i);
      len = readObject(desc, buf2 + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
  }
  strncpy(value + j, "\0", 1);

  return OK;
}


int initReader(struct doc_descriptor *desc) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  char buf[BUFSIZE], tmp[20], keyword[20];
  int len, i, found, t;
  int infoRef, nbopened;

  state->currentStream = state->currentOffset = 0;
  state->glyphfile = -1;
  state->newPage = 0;
  state->cmaplist = NULL;
  state->last = 0;
  state->last_available = 0;
  state->currentEncoding = NULL;
  state->length = 0;
  state->stream = 0;
  state->inString = 0;
  state->encodings = NULL;
  state->filter = NULL;
  state->XRef = NULL;
  state->predictor = 0;
  state->columns = 0;
  infoRef = -1;

  /* looking for 'startxref' */
  lseek(desc->fd, -1024, SEEK_END);
  len = read(desc->fd, buf, 1024);

  for(i = 1014; i > 0 && strncmp(buf + i, "startxref", 9); i--) {}
  
  if(strncmp(buf + i, "startxref", 9)) {
    fprintf(stderr, "Unable to find xref reference\n");
    return ERR_UNKNOWN_FORMAT;
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

  /* get xref */
  lseek(desc->fd, state->xref, SEEK_SET);
  if(getXRef(desc)) {
    return ERR_XREF;
  }

  /* going to xref table or stream */
  lseek(desc->fd, state->xref, SEEK_SET);
  len = read(desc->fd, buf, BUFSIZE);
  
  /* looking for Root in trailer or stream dictionary */
  for(i = 0 ; strncmp(buf + i, "<<", 2); i++) {
    if(i >= len - 2) {
      strncpy(buf, buf + i, len - i);
      len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
  }
  i += 2;
  if(i >= len - 2) {
    strncpy(buf, buf + i, len - i);
    len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
    i = 0;
  }
  found = 0;
  nbopened = 0;
  for( ; nbopened || strncmp(buf + i, ">>", 2); i++) {
    if(i >= len - 5) {
      strncpy(buf, buf + i, len - i);
      len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
    if(!strncmp(buf + i, "<<", 2)) {
      nbopened++;
    } else if(!strncmp(buf + i, ">>", 2)) {
      nbopened--;
    }
    if(!strncmp(buf + i, "/", 1)) {
      i++;
      getKeyword(buf + i, keyword);
      if (!strncmp(keyword, "Root", 4)) {
	found = 1;
	/* getting Root (catalog) reference */
	i += 4;
	if(i >= len - 10) {
	  strncpy(buf, buf + i, len - i);
	  len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
	  i = 0;
	}
	while (!strncmp(buf + i, " ", 1) ||
	       !strncmp(buf + i, "\x0A", 1) ||
	       !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	}
	state->catalogRef = getNumber(buf + i);

      } else if(!strncmp(keyword, "Info", 4)) {
	/* getting Info dictionary reference */
	i += 4;
	if(i >= len - 10) {
	  strncpy(buf, buf + i, len - i);
	  len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
	  i = 0;
	}
	while (!strncmp(buf + i, " ", 1) ||
	       !strncmp(buf + i, "\x0A", 1) ||
	       !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	}
	infoRef = getNumber(buf + i);
      } else if(!strncmp(keyword, "Encrypt", 7)) {
	fprintf(stderr, "encrypted document\n");
	return ENCRYPTED_FILE;
      }
    }
  }
  if(!found) {
    fprintf(stderr, "Unable to find Root entry in trailer\n");
    return ERR_UNKNOWN_FORMAT;
  }

  /* get metadata */
  if(infoRef != -1) {
    getMetadata(desc, infoRef);
  }


  /* going to catalog */
  gotoRef(desc, state->catalogRef);
  len = readObject(desc, buf, BUFSIZE);
  
  /* looking for page tree reference */
  found = 0;
  for( i = 0; i < len - 5 && !found; i++) {
    if (i >= len - 7) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
    if(!strncmp(buf + i, "/", 1)) {
      i++;
      if (i >= len - 5) {
	strncpy(buf, buf + i, len - i);
	len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	i = 0;
      }
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
  if (i >= len - 10) {
    strncpy(buf, buf + i, len - i);
    len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
    i = 0;
  }
  state->pagesRef = getNumber(buf + i);
  
  /* going to page tree */
  gotoRef(desc, state->pagesRef);
  getEncodings(desc);
  gotoRef(desc, state->pagesRef);
  len = readObject(desc, buf, BUFSIZE);
  if (getValue(desc, buf, len, "Type", tmp) < 0) {
    fprintf(stderr, "Can't find Type\n");
    return ERR_DICTIONARY;
  }

  /* get page count */
  for(i = 0; strncmp(buf + i, "/Count", 6); i++) {}
  i += 6;
  for( ; !strncmp(buf + i, " ", 1) ||
	 !strncmp(buf + i, "\x0A", 1) ||
	 !strncmp(buf + i, "\x0D", 1); i++) {}
  desc->pageCount = getNumber(buf + i);

  gotoRef(desc, state->pagesRef);
  len = readObject(desc, buf, BUFSIZE);

  /* finding first page */
  while(!strncmp(tmp, "Pages", 5)) {
    i = 0;
    
    while(i <= len - 5 && strncmp(buf + i, "/Kids", 5)) {
      if (i >= len - 5) {
	strncpy(buf, buf + i, 5);
	len = readObject(desc, buf + 5, BUFSIZE - 5) + 5;
	i = 0;
      } else {
	i++;
	if (i >= len - 5) {
	  strncpy(buf, buf + i, len - i);
	  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	  i = 0;
	}
      }
    }
    i += 5;
    if (i >= len - 5) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
    while(!strncmp(buf + i, " ", 1) ||
	  !strncmp(buf + i, "\x0A", 1) ||
	  !strncmp(buf + i, "\x0D", 1)) {
      i++;
      if (i >= len) {
	len = readObject(desc, buf, BUFSIZE);
	i = 0;
      }
    }
    i++;
    if (i >= len - 5) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
    while (!strncmp(buf + i, " ", 1) ||
	   !strncmp(buf + i, "\x0A", 1) ||
	   !strncmp(buf + i, "\x0D", 1)) {
      i++;
      if (i >= len) {
	len = readObject(desc, buf, BUFSIZE);
	i = 0;
      }
    }
    t = 0;
    
    while (strncmp(buf + i, " ", 1) &&
	   strncmp(buf + i, "\x0A", 1) &&
	   strncmp(buf + i, "\x0D", 1)) {
      strncpy(tmp + t, buf + i, 1);
      t++;
      i++;
      if (i >= len) {
	len = readObject(desc, buf, BUFSIZE);
	i = 0;
      }
    }
    strncpy(tmp + t, "\0", 1);
    gotoRef(desc, atoi(tmp));
    state->currentPage = atoi(tmp);
    getEncodings(desc);
    gotoRef(desc, state->currentPage);
    
    len = readObject(desc, buf, BUFSIZE);
    strncpy(tmp, "\x00\x00\x00\x00\x00\x00", 6);
    if (getValue(desc, buf, len, "Type", tmp) < 0) {
      fprintf(stderr, "Can't find Type\n");
      return ERR_DICTIONARY;
    }
  }
  return OK;
}


int gotoRef(struct doc_descriptor *desc, int ref){
  struct pdfState *state = (struct pdfState *)(desc->myState);
  struct xref *XRef;
  struct pdffilter *tmpfilter, *filter = NULL;
  char buf[BUFSIZE], buf2[BUFSIZE], tmp[20], rest[10];
  int len, len2, i, j, v, l, found, restlen;
  int nbopened;
  z_stream z;
  char prediction[10];

  memset(state->prediction, '\x00', 10);

  /* finding reference in table */
  XRef = state->XRef;
  if(XRef == NULL) {
    fprintf(stderr, "XRef is empty\n");
    return ERR_XREF;
  }
  while(XRef != NULL && XRef->object_number != ref) {
    XRef = XRef->next;
  }
  if(XRef == NULL) {
    fprintf(stderr, "cannot find reference to object %d\n", ref);
    return ERR_XREF;
  }

  if(!XRef->isInObjectStream) {

    /* setting file cursor */
    state->objectStream = -1;
    lseek(desc->fd, XRef->offset_or_index, SEEK_SET);

  } else {
    /* object is in object stream */
    state->objectStream = XRef->object_stream;
    state->predictor = state->columns = 1;

    /* position on object stream */
    for(XRef = state->XRef; XRef != NULL &&
	  XRef->object_number != state->objectStream; XRef = XRef->next) {}
    if(XRef == NULL) {
      fprintf(stderr, "cannot find object stream number %d\n", state->objectStream);
      return ERR_XREF;
    }
    lseek(desc->fd, XRef->offset_or_index, SEEK_SET);
    len = read(desc->fd, buf, BUFSIZE);

    /* search dictionary */
    for(i = 0; strncmp(buf + i, "<<", 2); i++) {}
    i += 2;

    /* get Length, First and filters */
    nbopened = 0;
    while(nbopened || strncmp(buf + i, ">>", 2)) {
      if(!strncmp(buf + i, "<<", 2)) {
	nbopened++;
      } else if(!strncmp(buf + i, ">>", 2)) {
	nbopened--;
      }
      
      /* get Length */
      if(!strncmp(buf + i, "/Length", 7)) {
	i += 7;
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
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
	  state->streamlength = state->length = getNumber(buf + i);
	  
	  /* Length is a reference */
	} else {
	  v = lseek(desc->fd, 0, SEEK_CUR);
	  gotoRef(desc, getNumber(buf + i));
	  l = read(desc->fd, buf2, BUFSIZE);
	  j = getNextLine(buf2, l);
	  while(!strncmp(buf2 + j, " ", 1) ||
		!strncmp(buf2 + j, "\x0A", 1) ||
		!strncmp(buf2 + j, "\x0D", 1)) {
	    j++;
	  }
	  state->streamlength = state->length = getNumber(buf2 + j);
	  lseek(desc->fd, v, SEEK_SET);
	}
      }

      /* get First */
      if(!strncmp(buf + i, "/First", 3)) {
	i += 6;
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	}
	state->first = getNumber(buf + i);
      }

      /* get decodeParms */
      if(!strncmp(buf + i, "/DecodeParms", 12)) {
	for( ; strncmp(buf + i, "<<", 2); i++) {}
	i += 2;
	for( ; strncmp(buf + i, ">>", 2); i++) {

	  if(!strncmp(buf + i, "/Columns", 8)) {
	    i += 8;
	    for( ; !strncmp(buf + i, " ", 1) ||
		   !strncmp(buf + i, "\x0A", 1) ||
		   !strncmp(buf + i, "\x0D", 1); i++) {}
	    state->columns = getNumber(buf + i);
	  }
	  if(!strncmp(buf + i, "/Predictor", 10)) {
	    i += 10;
	    for( ; !strncmp(buf + i, " ", 1) ||
		   !strncmp(buf + i, "\x0A", 1) ||
		   !strncmp(buf + i, "\x0D", 1); i++) {}
	    state->predictor = getNumber(buf + i);
	  }

	}
	if(i >= len - 12) {
	  strncpy(buf, buf + i, len - i);
	  len = read(desc->fd, buf + len - i, BUFSIZE -len + i) + len - i;
	  i = 0;
	}
	i += 2;
      }

      /* get filters */
      if(!strncmp(buf + i, "/Filter", 7)) {
	i += 7;
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	}
	
	/* array of filters */
	if(!strncmp(buf + i, "[", 1)) {
	  
	  while(strncmp(buf + i, "]", 1)) {
	    
	    if (!strncmp(buf + i, "/", 1)) {
	      i++;
	      j = 0;
	      while(strncmp(buf + i, " ", 1) &&
		    strncmp(buf + i, "\x0A", 1) &&
		    strncmp(buf + i, "\x0D", 1)) {
		strncpy(tmp + j, buf + i, 1);
		j++;
		i++;
	      }
	      strncpy(tmp + j, "\0", 1);
	      if(filter == NULL) {
		filter = (struct pdffilter *) malloc(sizeof(struct pdffilter));
		filter->next = NULL;
		tmpfilter = filter;
	      } else {
		for(tmpfilter = filter; tmpfilter->next != NULL;
		    tmpfilter = tmpfilter->next) {}
	      }
	      tmpfilter->filtercode = identifyFilter(tmp);
	    }
	  }

	  /* unique filter */
	} else {

	  /* search end of filters linked list in state */
	  filter = (struct pdffilter *) malloc(sizeof(struct pdffilter));
	  filter->next = NULL;
	  
	  i++;
	  j = 0;
	  while(strncmp(buf + i, " ", 1) &&
		strncmp(buf + i, "\x0A", 1) &&
		strncmp(buf + i, "\x0D", 1)) {
	    strncpy(tmp + j, buf + i, 1);
	    i++;
	    j++;
	  }
	  strncpy(tmp + j, "\0", 1);
	  filter->filtercode = identifyFilter(tmp);
	}
      }

      i++;
    }

    /* search stream beginning */
    for( ; strncmp(buf + i, "stream", 6); i++) {}
    i += 6;
    i += getNextLine(buf + i, len - i);
    lseek(desc->fd, i - len, SEEK_CUR);

    /* inflate stream beginning and find offset of desired object */
    found = 0;
    len = read(desc->fd, buf, (state->length < BUFSIZE) ? state->length : BUFSIZE);
    state->length -= len;
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    z.next_in = buf;
    z.avail_in = len;
    z.avail_out = BUFSIZE;
    z.next_out = buf2;
    inflateInit(&z);
    inflate(&z, Z_SYNC_FLUSH);
    len2 = BUFSIZE - z.avail_out;

    /* reverse prediction filter */
    memset(rest, '\x00', 7);
    restlen = 0;
    if(state->predictor >= 10) {
      len = 0;
      for(i = 1; i < BUFSIZE - z.avail_out - state->columns + 1; i += state->columns + 1) {
	for(j = 0; j < state->columns; j++) {
	  buf[len] = (prediction[j] + buf2[i + j]);
	  len++;
	}
	memcpy(prediction, buf + len - state->columns, state->columns);
      }
      memcpy(rest, buf2 + i - 1, BUFSIZE - z.avail_out  - i + 1);
      restlen = BUFSIZE - z.avail_out  - i + 1;
      memcpy(buf2, buf, len);
    }

    for(i = 0; !found && i < len2; ) {
      if(getNumber(buf2 + i) == ref) {
	found = 1;
      }

      for( ; strncmp(buf2 + i, " ", 1) &&
	     strncmp(buf2 + i, "\x0A", 1) &&
	     strncmp(buf2 + i, "\x0D", 1); i++) {}
      for( ; !strncmp(buf2 + i, " ", 1) ||
	     !strncmp(buf2 + i, "\x0A", 1) ||
	     !strncmp(buf2 + i, "\x0D", 1); i++) {}
      if(found) {
	state->offsetInStream = getNumber(buf2 + i);

      } else {
	for( ; strncmp(buf2 + i, " ", 1) &&
	       strncmp(buf2 + i, "\x0A", 1) &&
	       strncmp(buf2 + i, "\x0D", 1); i++) {}
	for( ; !strncmp(buf2 + i, " ", 1) ||
	       !strncmp(buf2 + i, "\x0A", 1) ||
	       !strncmp(buf2 + i, "\x0D", 1); i++) {}
      }
    }
    
    inflateEnd(&z);
    freeFilterStruct(filter);
  }
  return 0;
}


int getKeyword(char *input, char *output){
  int i;

  for (i = 0; i < 20 && strncmp(input + i, " ", 1)
	 && strncmp(input + i, "\x0A", 1)
	 && strncmp(input + i, "\x0D", 1)
	 && strncmp(input + i, "(", 1)
	 && strncmp(input + i, "<", 1)
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


int getXRef(struct doc_descriptor *desc) {
  struct pdfState *state = (struct pdfState *)(desc->myState);
  struct xref *XRef = NULL;
  struct pdffilter *filter;
  struct pdffilter *tmpfilter;
  int len, i, t, len2, v;
  int xinf, xsup, nbopened, inflateFinished, restlen;
  char buf[BUFSIZE], tmp[20], rest[10];
  char *decoded, *decoded2;
  int size, length, prev, cols, predictor;
  long int dictionaryBegin, currentObject;
  int field1Size, field2Size, field3Size;
  char prediction[10];
  z_stream z;

  cols = predictor = 1;
  len = read(desc->fd, buf, BUFSIZE);

  /* case of a cross reference table */
  if (!strncmp(buf, "xref", 4)) {
    i = getNextLine(buf, len);
    
    /* get range of references in table */
    t = 0;
    while (strncmp(buf + i, " ", 1) &&
	   strncmp(buf + i, "\x0A", 1) &&
	   strncmp(buf + i, "\x0D", 1)) {
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
    i += getNextLine(buf + i, len - i);
    if(i >= len - 20) {
      strncpy(buf, buf + i, len - i);
      len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }

    if(state->XRef == NULL) {
      XRef = (struct xref *) malloc(sizeof(struct xref));
      state->XRef = XRef;
      XRef->next = NULL;
    } else {
      XRef = state->XRef;
      while(XRef->next != NULL) {
	XRef = XRef->next;
      }
      XRef->next = (struct xref *) malloc(sizeof(struct xref));
      XRef = XRef->next;
      XRef->next = NULL;
    }

    /* copy each entry in state->XRef */
    for(t = xinf; t <= xsup; t++) {
      strncpy(tmp, buf + i, 10);
      strncpy(tmp + 10, "\0", 1);
      i += 20;
      XRef->object_number = t;
      XRef->offset_or_index = atoi(tmp);
      XRef->isInObjectStream = 0;

      if(t < xsup) {
	XRef->next = (struct xref *) malloc(sizeof(struct xref));
	XRef = XRef->next;
	XRef->next = NULL;
      }

      if( i >= len - 20) {
	strncpy(buf, buf + i, len - i);
	len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
	i = 0;
      }
    }

    /* procede previous xref if any */

    /* search trailer dictionary */
    while(strncmp(buf + i, "<<", 2)) {
      i++;
      if(i >= len - 4) {
	strncpy(buf, buf + i, len - i);
	len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
	i = 0;
      }
    }
    i += 2;

    /* search prev entry */
    nbopened = 0;
    while(strncmp(buf + i, "/Prev", 5) &&
	  (nbopened || strncmp(buf + i, ">>", 2))) {

      if(!strncmp(buf + i, "<<", 2)) {
	nbopened++;
      } else if (!strncmp(buf + i, ">>", 2)) {
	nbopened--;
      }
      i++;
      if(i >= len - 5) {
	strncpy(buf, buf + i, len - i);
	len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
	i = 0;
      }
    }

    /* previous xref found */
    if(!strncmp(buf + i, "/Prev", 5)) {
      i += 5;
      if(i >= len - 10) {
	strncpy(buf, buf + i, len - i);
	len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
	i = 0;
      }

      /* get previous xref */
      while(!strncmp(buf + i, " ", 1) ||
	    !strncmp(buf + i, "\x0A", 1) ||
	    !strncmp(buf + i, "\x0D", 1)) {
	i++;
	if(i >= len - 10) {
	  strncpy(buf, buf + i, len - i);
	  len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
	  i = 0;
	}
      }
      lseek(desc->fd, getNumber(buf + i), SEEK_SET);
      return getXRef(desc);      
    }
      

  } else {
    /* case of a cross reference stream */

    size = 0;
    field1Size = field2Size = field3Size = 0;
    /*search dictionary */
    for(i = 0; strncmp(buf + i, "<<", 2); i++) {}
    i += 2;
    dictionaryBegin = lseek(desc->fd, 0, SEEK_CUR);

    /* analyze dictionary */
    nbopened = 0;
    prev = length = xsup =xinf = -1;
    filter = NULL;
    while( nbopened || strncmp(buf + i, ">>", 2)) {
      if(!strncmp(buf + i, "<<", 2)) {
	nbopened++;
      } else if(!strncmp(buf + i, ">>", 2)) {
	nbopened--;
      }
	
      /* get size */
      if(!strncmp(buf + i, "/Size", 5)) {
	i += 5;
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	t = 0;
	while(strncmp(buf + i, " ", 1) &&
	      strncmp(buf + i, "/", 1) &&
	      strncmp(buf + i, "\x0A", 1) &&
	      strncmp(buf + i, "\x0D", 1)) {
	  strncpy(tmp + t, buf + i, 1);
	  t++;
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	strncpy(tmp + t, "\0", 1);
	size = atoi(tmp);
      }
	
      /* get length */
      if(!strncmp(buf + i, "/Length", 7)) {
	i += 7;
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	t = 0;
	while(strncmp(buf + i, " ", 1) &&
	      strncmp(buf + i, "/", 1) &&
	      strncmp(buf + i, "\x0A", 1) &&
	      strncmp(buf + i, "\x0D", 1)) {
	  strncpy(tmp + t, buf + i, 1);
	  t++;
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	strncpy(tmp + t, "\0", 1);
	length = atoi(tmp);
      }
	
      /* get filter */
      if(!strncmp(buf + i, "/Filter", 7)) {
	i += 7;
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	if(!strncmp(buf + i, "[", 1)) {
	  /* multiple filters */
	    
	  filter = (struct pdffilter *) malloc(sizeof(struct pdffilter));
	  filter->next = NULL;
	  filter->filtercode = -1;
	  tmpfilter = filter;
	    
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	  while(strncmp(buf + i, "]", 1)) {
	    while(!strncmp(buf + i, " ", 1) ||
		  !strncmp(buf + i, "\x0A", 1) ||
		  !strncmp(buf + i, "\x0D", 1)) {
	      i++;
	      if(i >= len) {
		len = read(desc->fd, buf, BUFSIZE);
		i = 0;
	      }
	    }
	    if(strncmp(buf + i, "]", 1)) {
	      if(tmpfilter->filtercode != -1) {
		tmpfilter->next = (struct pdffilter *) malloc(sizeof(struct pdffilter));
		tmpfilter = tmpfilter->next;
		tmpfilter->next = NULL;
	      }
	      i++;
	      t = 0;
	      while(strncmp(buf + i, " ", 1) &&
		    strncmp(buf + i, "/", 1) &&
		    strncmp(buf + i, "\x0A", 1) &&
		    strncmp(buf + i, "\x0D", 1)) {
		strncpy(tmp + t, buf + i, 1);
		t++;
		i++;
		if(i >= len) {
		  len = read(desc->fd, buf, BUFSIZE);
		  i = 0;
		}
	      }
	      strncpy(tmp + t, "\0", 1);
	      tmpfilter->filtercode = identifyFilter(tmp);
	    }
	  }
	} else {
	  /* unique filter */
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }

	  filter = (struct pdffilter *) malloc(sizeof(struct pdffilter));
	  filter->next = NULL;

	  t = 0;
	  while(strncmp(buf + i, " ", 1) &&
		strncmp(buf + i, "/", 1) &&
		strncmp(buf + i, "\x0A", 1) &&
		strncmp(buf + i, "\x0D", 1)) {
	    strncpy(tmp + t, buf + i, 1);
	    t++;
	    i++;
	    if(i >= len) {
	      len = read(desc->fd, buf, BUFSIZE);
	      i = 0;
	    }
	  }
	  strncpy(tmp + t, "\0", 1);
	  filter->filtercode = identifyFilter(tmp);
	}
      }

      /* get previous xref offset */
      if(!strncmp(buf + i, "/Prev", 5)) {
	i += 5;
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	t = 0;
	while(strncmp(buf + i, " ", 1) &&
	      strncmp(buf + i, "/", 1) &&
	      strncmp(buf + i, "\x0A", 1) &&
	      strncmp(buf + i, "\x0D", 1)) {
	  strncpy(tmp + t, buf + i, 1);
	  t++;
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	strncpy(tmp + t, "\0", 1);
	prev = atoi(tmp);

      }

      /* get decodeParms */
      if(!strncmp(buf + i, "/DecodeParms", 12)) {
	for( ; strncmp(buf + i, "<<", 2); i++) {
	  if(i >= len - 12) {
	    strncpy(buf, buf + i, len - i);
	    len = read(desc->fd, buf + len - i, BUFSIZE -len + i) + len - i;
	    i = 0;
	  }
	}
	if(i >= len - 12) {
	  strncpy(buf, buf + i, len - i);
	  len = read(desc->fd, buf + len - i, BUFSIZE -len + i) + len - i;
	  i = 0;
	}
	i += 2;
	for( ; strncmp(buf + i, ">>", 2); i++) {
	  if(i >= len - 12) {
	    strncpy(buf, buf + i, len - i);
	    len = read(desc->fd, buf + len - i, BUFSIZE -len + i) + len - i;
	    i = 0;
	  }
	  if(!strncmp(buf + i, "/Columns", 8)) {
	    i += 8;
	    if(i >= len - 12) {
	      strncpy(buf, buf + i, len - i);
	      len = read(desc->fd, buf + len - i, BUFSIZE -len + i) + len - i;
	      i = 0;
	    }
	    for( ; !strncmp(buf + i, " ", 1) ||
		   !strncmp(buf + i, "\x0A", 1) ||
		   !strncmp(buf + i, "\x0D", 1); i++) {
	      if(i >= len - 12) {
		strncpy(buf, buf + i, len - i);
		len = read(desc->fd, buf + len - i, BUFSIZE -len + i) + len - i;
		i = 0;
	      }
	    }
	    cols = getNumber(buf + i);
	  }
	  if(!strncmp(buf + i, "/Predictor", 10)) {
	    i += 10;
	    if(i >= len - 12) {
	      strncpy(buf, buf + i, len - i);
	      len = read(desc->fd, buf + len - i, BUFSIZE -len + i) + len - i;
	      i = 0;
	    }
	    for( ; !strncmp(buf + i, " ", 1) ||
		   !strncmp(buf + i, "\x0A", 1) ||
		   !strncmp(buf + i, "\x0D", 1); i++) {
	      if(i >= len - 12) {
		strncpy(buf, buf + i, len - i);
		len = read(desc->fd, buf + len - i, BUFSIZE -len + i) + len - i;
		i = 0;
	      }
	    }
	    predictor = getNumber(buf + i);
	  }

	}
	if(i >= len - 12) {
	  strncpy(buf, buf + i, len - i);
	  len = read(desc->fd, buf + len - i, BUFSIZE -len + i) + len - i;
	  i = 0;
	}
	i += 2;
      }

      /* get index */
      if(!strncmp(buf + i, "/Index", 6)) {

	/* get first entry number */
	while(strncmp(buf + i, "[", 1)) {
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	i++;
	if(i >= len) {
	  len = read(desc->fd, buf, BUFSIZE);
	  i = 0;
	}
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	t = 0;
	while(strncmp(buf + i, " ", 1) &&
	      strncmp(buf + i, "/", 1) &&
	      strncmp(buf + i, "]", 1) &&
	      strncmp(buf + i, "\x0A", 1) &&
	      strncmp(buf + i, "\x0D", 1)) {
	  strncpy(tmp + t, buf + i, 1);
	  t++;
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	strncpy(tmp + t, "\0", 1);
	xinf = atoi(tmp);

	/* get last entry number */
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	t = 0;
	while(strncmp(buf + i, " ", 1) &&
	      strncmp(buf + i, "/", 1) &&
	      strncmp(buf + i, "]", 1) &&
	      strncmp(buf + i, "\x0A", 1) &&
	      strncmp(buf + i, "\x0D", 1)) {
	  strncpy(tmp + t, buf + i, 1);
	  t++;
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	strncpy(tmp + t, "\0", 1);
	xsup = xinf + atoi(tmp) - 1;
      }

      /* get xref fields sizes */
      if(!strncmp(buf + i, "/W", 2)) {

	/* get first field size */
	while(strncmp(buf + i, "[", 1)) {
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	i++;
	if(i >= len) {
	  len = read(desc->fd, buf, BUFSIZE);
	  i = 0;
	}
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	t = 0;
	while(strncmp(buf + i, " ", 1) &&
	      strncmp(buf + i, "/", 1) &&
	      strncmp(buf + i, "\x0A", 1) &&
	      strncmp(buf + i, "\x0D", 1)) {
	  strncpy(tmp + t, buf + i, 1);
	  t++;
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	strncpy(tmp + t, "\0", 1);
	field1Size = atoi(tmp);

	/* get second field size */
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	t = 0;
	while(strncmp(buf + i, " ", 1) &&
	      strncmp(buf + i, "/", 1) &&
	      strncmp(buf + i, "\x0A", 1) &&
	      strncmp(buf + i, "\x0D", 1)) {
	  strncpy(tmp + t, buf + i, 1);
	  t++;
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	strncpy(tmp + t, "\0", 1);
	field2Size = atoi(tmp);

	/* get third field size */
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	t = 0;
	while(strncmp(buf + i, " ", 1) &&
	      strncmp(buf + i, "/", 1) &&
	      strncmp(buf + i, "]", 1) &&
	      strncmp(buf + i, "\x0A", 1) &&
	      strncmp(buf + i, "\x0D", 1)) {
	  strncpy(tmp + t, buf + i, 1);
	  t++;
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	strncpy(tmp + t, "\0", 1);
	field3Size = atoi(tmp);
      }

      i++;
      if(i >= len - 12) {
	strncpy(buf, buf + i, len - i);
	len = read(desc->fd, buf + len - i, BUFSIZE -len + i) + len - i;
	i = 0;
      }
    }
    
    /* set xref bounds (if no Index) */
    if(xinf == -1) {
      xinf = 0;
      xsup = size - 1;
    }

    if(i >= len - 6) {
      strncpy(buf, buf + i, len - i);
      len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }

    /* get stream */
    while(strncmp(buf + i, "stream", 6)) {
      i++;
      if(i >= len - 6) {
	strncpy(buf, buf + i, len - i);
	len = read(desc->fd, buf + len - i, BUFSIZE - len + i) + len - i;
	i = 0;
      }
    }
    i+=6;
    i += getNextLine(buf + i, len - i);
    lseek(desc->fd, i - len, SEEK_CUR);

    /* apply filters */
    memset(prediction, '\x00', cols);
    memset(rest, '\x00', cols + 2);
    restlen = 0;
    inflateFinished = 0;
    currentObject = xinf;
    z.zalloc = Z_NULL;
    z.zfree = Z_NULL;
    z.next_in = buf;
    z.avail_in = (length < BUFSIZE) ? length : BUFSIZE;
    inflateInit(&z);
    z.avail_in = 0;

    do {
      if(z.avail_in == 0) {
	len = read(desc->fd, buf, (length < BUFSIZE ) ? length : BUFSIZE);
	length -= len;
	z.next_in = buf;
	z.avail_in += len;
      }
      z.avail_out = 6000 - restlen;
      decoded = (char *) malloc(6000);
      memcpy(decoded, rest, restlen);
      z.next_out = decoded + restlen;
      inflateFinished = inflate(&z, Z_SYNC_FLUSH);
      len2 = 6000 - z.avail_out;

      /* reverse prediction filter */
      if(predictor >= 10) {
	decoded2 = (char *) malloc(5000);
	len2 = 0;
	for(i = 1; i < 6000 - z.avail_out - cols + 1; i += cols + 1) {
	  for(t = 0; t < cols; t++) {
	    decoded2[len2] = (prediction[t] + decoded[i + t]);
	    len2++;
	  }
	  memcpy(prediction, decoded2 + len2 - cols, cols);
	}
	memcpy(rest, decoded + i - 1, 6000 - z.avail_out  - i + 1);
	restlen = 6000 - z.avail_out  - i + 1;
	memcpy(decoded, decoded2, len2);
	free(decoded2);
      }

      /* procede xref table */
      if(state->XRef == NULL) {
	XRef = (struct xref *) malloc(sizeof(struct xref));
	state->XRef = XRef;
	XRef->next = NULL;
      } else {
	XRef = state->XRef;
	while(XRef->next != NULL) {
	  XRef = XRef->next;
	}
	XRef->next = (struct xref *) malloc(sizeof(struct xref));
	XRef = XRef->next;
	XRef->next = NULL;
      }
      i = 0;
      while(i < len2 && currentObject <= xsup) {
	memcpy(tmp, decoded + i, field1Size);
	memcpy(tmp + field1Size, "\0", 1);

	if( tmp[0] == 0 ) {
	  /* skip free object entries */
	
	} else if( tmp[0] == 1) {
	  /* standard xref table entry */
	  v = 0;
	  for(t = 0; t < field2Size; t++) {
	    memcpy(((char *)&v) + t,
		   decoded + i + field1Size + field2Size - t - 1, 1);
	  }
	  XRef->offset_or_index = v;
	  XRef->object_number = currentObject;
	  XRef->isInObjectStream = 0;
	  if(i < len2 - 6 && currentObject < xsup) {
	    XRef->next = (struct xref *) malloc(sizeof(struct xref));
	    XRef = XRef->next;
	    XRef->next = NULL;
	  }
	
	
	} else if( tmp[0] == 2) {
	  /* compressed object entry */
	  v = 0;
	  for(t = 0; t < field2Size; t++) {
	    memcpy(((char *)&v) + t,
		   decoded + i + field1Size + field2Size - t - 1, 1);
	  }
	  XRef->object_stream = v;
	  v = 0;
	  for(t = 0; t < field3Size; t++) {
	    memcpy(((char *)&v) + t,
		   decoded + i + field1Size + field2Size  + field3Size
		   - t - 1, 1);
	  }
	  XRef->offset_or_index = v;
	  XRef->object_number = currentObject;
	  XRef->isInObjectStream = 1;
	  if(i < len2 - 6 && currentObject < xsup) {
	    XRef->next = (struct xref *) malloc(sizeof(struct xref));
	    XRef = XRef->next;
	    XRef->next = NULL;
	  }
	
	} else {
	  /* unknown type (future pdf versions) */
	  fprintf(stderr, "Unknown entry type in xref stream : %d\n", tmp[0]);
	  return ERR_XREF;
	}
      
	i += field1Size + field2Size + field3Size;
	currentObject++;
      }
    
      free(decoded);
    } while(inflateFinished == 0);

    inflateEnd(&z);
    freeFilterStruct(filter);

    /* procede previous xref */
    if(prev != -1) {
      lseek(desc->fd, prev, SEEK_SET);
      return getXRef(desc);
    }
  }
  return 0;
}


int version(int fd) {
  char buf[8], tmp[2];

  /* reading header */
  lseek(fd, 0, SEEK_SET);
  if( read(fd, buf, 8) != 8) {
    fprintf(stderr, "not a document file\n");
    return ERR_UNKNOWN_FORMAT;
  }

  /* checking header */
  if( strncmp(buf, "%PDF-1.", 7)){
    fprintf(stderr, "not a PDF file\n");
    return ERR_UNKNOWN_FORMAT;
  }

  /* get version number */
  strncpy(tmp, buf + 7, 1);
  strncpy(tmp + 1, "\0", 1);

  return atoi(tmp);
}

int freeFilterStruct(struct pdffilter *filter) {
  struct pdffilter *tmp;

  while(filter != NULL) {
    tmp = filter;
    filter = filter->next;
    free(tmp);
  }
  return 0;
}

int freeXRefStruct(struct xref *xref) {
  struct xref *tmp;

  while(xref != NULL) {
    tmp = xref;
    xref = xref->next;
    free(tmp);
  }
  return 0;
}


int readObject(struct doc_descriptor *desc, void *buf, size_t buflen) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  z_stream z;
  char srcbuf[BUFSIZE], outbuf[BUFSIZE], buf2[BUFSIZE+4], rest[10];
  int len, i, len2, j, finish, size, restlen, total;
  int ascii85, flate, destlen;

  /* object is not inside a stream object */
  if(state->objectStream == -1) {
    return read(desc->fd, buf, buflen);
  }

  /* object is inside a stream object */
  state->length = state->streamlength;
  ascii85 = 0;
  flate = 0;
  gotoRef(desc, state->objectStream);
  len = read(desc->fd, srcbuf, BUFSIZE);

  /* get filters */
  for(i = 0; strncmp(srcbuf + i, "/Filter", 7)
	&& strncmp(srcbuf + i, "stream", 6); i++) {
    if(!strncmp(srcbuf + i, "%", 1)) {
      i += getNextLine(srcbuf + i, len - i);
    }
  }

  if(!strncmp(srcbuf + i, "/Filter", 7)) {
    i += 7;
    for( ; !strncmp(srcbuf + i, " ", 1) ||
	   !strncmp(srcbuf + i, "\x0A", 1) ||
	   !strncmp(srcbuf + i, "\x0D", 1); i++) {}

    if(!strncmp(srcbuf + i, "[", 1)) {

      /* multiple filters */
      i++;
      while(strncmp(srcbuf + i, "]", 1)) {
	if(!strncmp(srcbuf + i, "/", 1)) {
	  i++;
	  if(identifyFilter(srcbuf + i) == flateDecode) {
	    flate = 1;
	  }
	  if(identifyFilter(srcbuf + i) == ascii85Decode) {
	    if(!flate) {
	      ascii85 = 1;
	    } else {
	      ascii85 = 2;
	    }
	  }
	}
	i++;
      }

    } else {

      /* unique filter */
      i++;
      if(identifyFilter(srcbuf + i) == ascii85Decode) {
	ascii85 = 1;
      }
    }
  }

  /* go to stream beginning */
  for( ; strncmp(srcbuf + i, "stream", 6); i++) {}
  i += 6;
  i += getNextLine(srcbuf + i, len - i);
  lseek(desc->fd, i - len, SEEK_CUR);
  i = 0;
  len2 = 0;

  /* prepare stream inflating */
  z.zalloc = Z_NULL;
  z.zfree = Z_NULL;
  z.next_in = srcbuf;
  z.avail_in = (state->length < BUFSIZE) ? state->length : BUFSIZE;
  inflateInit(&z);
  z.avail_in = 0;
  finish = 0;
  size = buflen;
  restlen = 0;
  total = 0;
  
  /* find object */
  do {
    if(z.avail_in == 0) {
      len = read(desc->fd, srcbuf, BUFSIZE);
      if(ascii85 == 1) {
	lseek(desc->fd, -decodeASCII85(srcbuf, len, outbuf, &destlen), SEEK_CUR);
	strncpy(srcbuf, outbuf, destlen);
	len = destlen;
      }
      if(len == 0) {
	inflateEnd(&z);
	return 0;
      }
      state->length -= len;
      z.avail_in = len;
      z.next_in = srcbuf;
    }
    z.avail_out = BUFSIZE;
    z.next_out = outbuf;
    finish = inflate(&z, Z_SYNC_FLUSH);

    if(ascii85 == 2) {
      strncpy(buf2 + restlen, outbuf, BUFSIZE - z.avail_out);
      restlen = decodeASCII85(buf2, BUFSIZE - z.avail_out + restlen, buf, &destlen);
      strncpy(buf2, outbuf + BUFSIZE - z.avail_out - restlen, restlen);
      strncpy(outbuf, buf, destlen);
    } else {
      destlen = BUFSIZE - z.avail_out;
    }
    total += destlen;

  } while(!finish && total <= state->offsetInStream + state->first);

  i = state->offsetInStream + state->first - total + destlen;
  if(i > destlen) {
    inflateEnd(&z);
    return 0;
  }
  
  /* copy output in buf */
  len2 = 0;
  strncpy(buf + len2, outbuf + i,
	  (destlen - i < size) ? destlen - i : size);
  len2 += (destlen - i < size) ? destlen - i : size;
  size -= (destlen - i < size) ? destlen - i : size;
  
  while(!finish && len2 < buflen) {
    if(z.avail_in == 0) {
      len = read(desc->fd, srcbuf, BUFSIZE);
      if(ascii85 == 1) {
	lseek(desc->fd, -decodeASCII85(srcbuf, len, outbuf, &destlen), SEEK_CUR);
	strncpy(srcbuf, outbuf, destlen);
	len = destlen;
      }
      if(len == 0) {
	inflateEnd(&z);

	/* reverse prediction filter */
	if(len2 && state->predictor >= 10) {
	  memset(rest, '\x00', state->columns + 2);
	  restlen = 0;
	  len = 0;
	  for(i = 1; i < len2 - state->columns + 1; i += state->columns + 1) {
	    for(j = 0; j < state->columns; j++) {
	      buf2[len] = (state->prediction[j] + ((char *)(buf))[i + j]);
	      len++;
	    }
	    memcpy(state->prediction, buf2 + len - state->columns, state->columns);
	  }
	  memcpy(rest, buf + i - 1, len2  - i + 1);
	  restlen = len2  - i + 1;
	  memcpy(buf, buf2, len);
	  len2 = len;
	}

	state->offsetInStream += len2;

	return len2;
      }
      state->length -= len;
      z.avail_in = len;
      z.next_in = srcbuf;
    }
    z.avail_out = BUFSIZE;
    z.next_out = outbuf;
    finish = inflate(&z, Z_SYNC_FLUSH);
    destlen = (BUFSIZE - z.avail_out < size) ? BUFSIZE - z.avail_out : size;

    if(ascii85 == 2) {
      strncpy(buf2 + restlen, outbuf, BUFSIZE - z.avail_out);
      strncpy(rest, outbuf + BUFSIZE - z.avail_out - 5, 5);
      restlen = decodeASCII85(buf2, BUFSIZE - z.avail_out + restlen, outbuf, &destlen);
      strncpy(buf2, rest + 5 - restlen, restlen);
      if(destlen > size) {
	destlen = size;
      }
    }
    strncpy(buf + len2, outbuf, destlen);
    len2 += destlen;
    size -= destlen;
  }
  inflateEnd(&z);
  
  /* reverse prediction filter */
  if(state->predictor >= 10) {
    memset(rest, '\x00', 10);
    restlen = 0;
    len = 0;
    for(i = 1; i < len2 - state->columns + 1; i += state->columns + 1) {
      for(j = 0; j < state->columns; j++) {
	buf2[len] = (state->prediction[j] + ((char *)(buf))[i + j]);
	len++;
      }
      memcpy(state->prediction, buf2 + len - state->columns, state->columns);
    }
    memcpy(rest, buf + i - 1, len2  - i + 1);
    restlen = len2  - i + 1;
    memcpy(buf, buf2, len);
    len2 = len;
  }

  state->offsetInStream += len2;

  return len2;
}


int decodeASCII85(char *src, int srclen, char *dest, int *destlen){
  char _85[5], ascii[4];
  int val, i, j;

  i = 0;
  *destlen = 0;

  while(i < srclen - 4) {

    if(!strncmp(src + i, "\x7A", 1)) {
      memcpy(dest + *destlen, "\x00\x00\x00\x00", 4);
      *destlen += 4;
      i++;
    } else if(src[i]>=33 && src[i]<=117) {
      val = 0;
      strncpy(_85, src + i, 5);
      i += 5;
      for(j = 0; j < 5; j++) {
	val += (_85[4 - j] - 33) * pow(85, j);
      }
      for(j = 0; j < 4; j++) {
	ascii[3 - j] =  val % 256;
	val /= 256;
      }
      memcpy(dest + *destlen, ascii, 4);
      *destlen += 4;
    } else {
      i++;
    }
  }

  return srclen - i;
}


int freeCMapList(struct CMapList *cmaplist) {
  struct CMapList *tmplist;
  struct ToUnicodeCMap *cmap, *tmp;

  while(cmaplist != NULL) {
    tmplist = cmaplist;
    cmaplist = cmaplist->next;
    cmap = tmplist->cmap;
    while(cmap != NULL) {
      tmp = cmap;
      cmap = cmap->next;
      free(tmp->value);
      free(tmp);
    }
    free(tmplist);
  }

  return OK;
}


int freeEncodingTable(struct encodingTable *table) {
  struct encodingTable *tmpTable;
  struct diffTable *diff, *tmpdiff;

  while(table != NULL) {
    tmpTable = table;
    table = table->next;
    if(tmpTable->diff != NULL) {
      diff = tmpTable->diff;
      while(diff != NULL) {
	tmpdiff = diff;
	diff = diff->next;
	free(tmpdiff->name);
	free(tmpdiff);
      }
    }
    free(tmpTable->fontName);
    if(tmpTable->encoding != NULL) {
      free(tmpTable->encoding);
    }
    free(tmpTable);
  }

  return OK;
}

int getMetadata(struct doc_descriptor *desc, int infoRef) {
  struct meta *meta = NULL;
  char buf[BUFSIZE], name[20], value[128], tmp[1];
  int len, i, k;
  int v, j = 0;
  UErrorCode err;
  UChar *uvalue, *uname;
  int valuelen, namelen;

  desc->meta = NULL;

  /* goto Info dictionary */
  gotoRef(desc, infoRef);
  len = readObject(desc, buf, BUFSIZE);
  for(i = 0; strncmp(buf + i, "<<", 2); i++) {
    if(i >= len - 2) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
  }
  i += 2;
  if(i >= len - 2) {
    strncpy(buf, buf + i, len - i);
    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
    i = 0;
  }
  for( ; strncmp(buf + i, ">>", 2); i++) {

    if(i >= len - 14) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
    if(!strncmp(buf + i, "/", 1)) {
      i++;
      getKeyword(buf + i, name);
      if(strncmp(name, "Trapped", 7)) {
	memset(value, '\x00', 128);
	i += strlen(name);
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	}
	if(!strncmp(buf + i, "(", 1)) {
	  /* standard text string */
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }
	  for(j = 0 ; j < 127 && strncmp(buf + i, ")", 1); j++) {
	    if(i >= len) {
	      len = read(desc->fd, buf, BUFSIZE);
	      i = 0;
	    }
	    if(!strncmp(buf + i, "\\", 1)) {
	      i++;
	      if(i >= len) {
		len = read(desc->fd, buf, BUFSIZE);
		i = 0;
	      }
	    }
	    strncpy(value + j, buf + i, 1);
	    i++;
	  }
	  strncpy(value + j, "\0", 1);

	} else if(!strncmp(buf + i, "<", 1)) {
	  /* hex string */
	  i++;
	  if(i >= len) {
	    len = read(desc->fd, buf, BUFSIZE);
	    i = 0;
	  }

	  for(j = 0; j < 127 && strncmp(buf + i, ">", 1); ) {
	    v = 0;
	    for(k = 0; k < 2; k++) {
	      strncpy(tmp, buf + i, 1);
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
	      } else if(!strncmp(tmp, ">", 1)) {
		/* nothing */
	      } else {
		v += atoi(tmp);
	      }
	      if(strncmp(tmp, ">", 1)) {
		i++;
		if(i >= len - 2) {
		  strncpy(buf, buf + i, len - i);
		  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
		  i = 0;
		}
	      }
	    }

	    memcpy(value + j, &v, 1);
	    j++;
	  }
	  strncpy(value + j, "\0", 1);
	}

	if(j){

	  /* convert name and value to UTF-16 */
	  uname = (UChar *) malloc(2*strlen(name) + 2);
	  err = U_ZERO_ERROR;
	  namelen = ucnv_toUChars(desc->conv, uname, 2*strlen(name)+2,
				      name, strlen(name), &err);
	  if(value[0] == -2) {
	    uvalue = (UChar *) malloc(j);
	    err = U_ZERO_ERROR;
	    desc->conv = ucnv_open("utf16be", &err);
	    err = U_ZERO_ERROR;
	    valuelen = ucnv_toUChars(desc->conv, uvalue, j,
				     (value+2), j-2, &err);

	    err = U_ZERO_ERROR;
	    desc->conv = ucnv_open("latin1", &err);

	  } else {
	    uvalue = (UChar *) malloc(2*strlen(value) + 2);
	    err = U_ZERO_ERROR;
	    valuelen = ucnv_toUChars(desc->conv, uvalue, 2*strlen(value)+2,
				     value, strlen(value), &err);
	  }
	  if(valuelen) {
	    /* create new metadata structure in the list */
	    if(desc->meta == NULL) {
	      desc->meta = (struct meta *) malloc(sizeof(struct meta));
	      meta = desc->meta;
	    } else {
	      meta->next = (struct meta *) malloc(sizeof(struct meta));
	      meta = meta->next;
	    }
	    meta->next = NULL;
	    meta->name = uname;
	    meta->name_length = namelen;
	    meta->value = uvalue;
	    meta->value_length = valuelen;
	  } else {
	    free(uname);
	    free(uvalue);
	  }
	}
      }
    }
  }

  return OK;
}
