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
 * internal functions for powerpoint plugin
 */

#include "p_powerpoint.h"
#include <string.h>
#include <stdio.h>
#include <unicode/ustring.h>


int read_next(struct doc_descriptor *desc, UChar *out, int size) {
  struct oleState* state = (struct oleState *)(desc->myState);
  UErrorCode err;
  UChar tmp;
  char src[516];
  int recordInstance;
  int recordType;
  int recordlen;
  int l, lastLen;
  int bytesToCopy;

  memset(out, '\x00', size);
  l = 0;
  if(state->nbBytesRead >= state->fileSize || state->len <= 0) {
    return NO_MORE_DATA;
  }
  while(1) {
    if(state->cur > state->len - 2) {
      memcpy(state->buf, state->buf + state->cur, state->len - state->cur);
      lastLen = state->len;
      state->len = readOLE(desc, state->buf + state->len - state->cur);
      if(state->len < 0) {
	return state->len;
      }
      state->len += lastLen - state->cur;
      state->cur = 0;
    }

    recordType = recordlen = recordInstance = 0;
    if(state->readerState == outOfString) {
      if(state->cur > state->len - 4) {
	memcpy(state->buf, state->buf + state->cur, state->len - state->cur);
	lastLen = state->len;
	state->len = readOLE(desc, state->buf + state->len - state->cur);
	if(state->len < 0) {
	  return state->len;
	}
	state->len += lastLen - state->cur;
	state->cur = 0;
      }
      memcpy(&recordInstance, state->buf + state->cur, 2);
      memcpy(&recordType, state->buf + state->cur + 2, 2);
      state->cur += 4;
      state->nbBytesRead += 4;
      if(state->cur > state->len - 4) {
	memcpy(state->buf, state->buf + state->cur, state->len - state->cur);
	lastLen = state->len;
	state->len = readOLE(desc, state->buf + state->len - state->cur);
	if(state->len < 0) {
	  return state->len;
	}
	state->len += lastLen - state->cur;
	state->cur = 0;
      }
      memcpy(&recordlen, state->buf + state->cur, 4);
      state->strrecordlen = recordlen;
      state->cur += 4;
      state->nbBytesRead += 4;
      if(state->nbBytesRead >= state->fileSize) {
	return NO_MORE_DATA;
      }
      if(state->cur > state->len - 2) {
	memcpy(state->buf, state->buf + state->cur, state->len - state->cur);
	lastLen = state->len;
	state->len = readOLE(desc, state->buf + state->len - state->cur);
	if(state->len < 0) {
	  return state->len;
	}
	state->len += lastLen - state->cur;
	state->cur = 0;
      }
    }

    /* unicode string */
    if(state->readerState == inUnicode || recordType == 0x0FA0 ||recordType == 0x0FBA) {
      state->readerState = inUnicode;
      for( ; state->strrecordlen > 0; state->strrecordlen -= 2) {
	memcpy(&tmp, state->buf + state->cur, 2);
	state->cur += 2;
	state->nbBytesRead += 2;
	if(u_isdefined(tmp)) {
	  memcpy(out + l/2, &tmp, 2);
	  l += 2;
	}
	if(l >= size - 2) {
	  if(state->strrecordlen == 0) {
	    memcpy(out + l/2, "\x20\x00",2);
	    l += 2;
	  }
	  return l;
	}
	if(state->nbBytesRead >= state->fileSize) {
	  state->readerState = outOfString;
	  if(l > 0) {
	    memcpy(out + l/2, "\x20\x00", 2);
	    l += 2;
	    return l;
	  } else {
	    return NO_MORE_DATA;
	  }
	}
	if(state->cur > state->len - 2) {
	  memcpy(state->buf, state->buf + state->cur, state->len - state->cur);
	  lastLen = state->len;
	  state->len = readOLE(desc, state->buf + state->len - state->cur);
	  if(state->len < 0) {
	    state->readerState = outOfString;
	    memcpy(out + l/2, "\x20\x00",2);
	    l += 2;
	    return l;
	  }
	  state->len += lastLen - state->cur;
	  state->cur = 0;
	}
      }
      state->readerState = outOfString;
      if(l > 0) {
	if(state->strrecordlen == 0) {
	  memcpy(out + l/2, "\x20\x00",2);
	  l += 2;
	}
	return l;
      }

      /* cp1252 string */
    } else if(state->readerState == inString ||recordType == 0x0FA8) {
      state->readerState = outOfString;
      while(state->strrecordlen > 0) {
	bytesToCopy = min(state->strrecordlen, state->len - state->cur);
	bytesToCopy = min((size - l)/2 - 1, bytesToCopy);

	memset(src, '\x00', 516);
	memcpy(src, state->buf + state->cur, bytesToCopy);
	state->cur += bytesToCopy;
	state->nbBytesRead += bytesToCopy;
	state->strrecordlen -= bytesToCopy;

	/* converting to UTF-16 */
	err = U_ZERO_ERROR;
	l += 2*ucnv_toUChars(desc->conv, out + l/2, size - l, src, bytesToCopy, &err);
	if (U_FAILURE(err)) {
	  fprintf(stderr, "Unable to convert buffer\n");
	  return ERR_ICU;
	}

	if(l >= size - 2) {
	  if(state->strrecordlen > 0) {
	    state->readerState = inString;
	  } else {
	    memcpy(out + l/2, "\x20\x00",2);
	    l += 2;
	  }
	  return l;
	}
	if(state->nbBytesRead >= state->fileSize) {
	  state->readerState = outOfString;
	  if(l > 0) {
	    memcpy(out + l/2, "\x20\x00", 2);
	    l += 2;
	    return l;
	  } else {
	    return NO_MORE_DATA;
	  }
	}
	if(state->cur > state->len - 2) {
	  memcpy(state->buf, state->buf + state->cur, state->len - state->cur);
	  lastLen = state->len;
	  state->len = readOLE(desc, state->buf + state->len - state->cur);
	  if(state->len < 0) {
	    state->readerState = outOfString;
	    memcpy(out + l/2, "\x20\x00",2);
	    l += 2;
	    return l;
	  }
	  state->len += lastLen - state->cur;
	  state->cur = 0;
	}
      }
      if(l > 0) {
	if(state->strrecordlen == 0) {
	  memcpy(out + l/2, "\x20\x00",2);
	  l += 2;
	}
	return l;
      }

    } else if(recordInstance != 0x000F) {
      state->readerState = outOfString;
      state->cur += recordlen;
      state->nbBytesRead += recordlen;
      while(state->cur >= state->len) {
	state->cur -= state->len;
	state->len = readOLE(desc, state->buf);
	if(state->len < 0) {
	  return state->len;
	}
      }
      if(state->nbBytesRead >= state->fileSize) {
	return NO_MORE_DATA;
      }

    } else {
      state->readerState = outOfString;
    }

  }


  return NO_MORE_DATA;
}


int initOLE(struct doc_descriptor *desc) {
  struct oleState* state = (struct oleState *)(desc->myState);
  char name[64];
  int j, namelen;
  int propStart;

  state->readerState = outOfString;
  state->fileSize = state->nbBytesRead = 0;
  state->len = read(desc->fd, state->buf, BBSIZE);
  if(strncmp(state->buf, "\xD0\xCF\x11\xE0\xA1\xB1\x1A\xE1", 8)) {
    fprintf(stderr, "Not an OLE file\n");
    return -2;
  }

  memcpy(&(state->nbOfBBD), state->buf+ 0x2C, 4);
  memcpy(&(state->rootStart), state->buf + 0x30, 4);
  memcpy(&(state->sbdStart), state->buf + 0x3C, 4);
  memcpy(&(state->bbdList), state->buf + 0x4C, 4);
  state->BBD = NULL;
  getBBD(desc);

  /* go to root entry */
  lseek(desc->fd, (state->rootStart + 1) * BBSIZE, SEEK_SET);
  state->currentBBlock = state->rootStart;
  state->bigSize = 1;
  state->len = read(desc->fd, state->buf, BBSIZE);
  state->cur = -0x80;

  /* search for PowerPoint Document */
  do{
    state->cur += 0x80;
    if(state->cur > state->len - 0x80) {
      /* goto next block */
      getNextBlock(desc);
      lseek(desc->fd, (state->currentBBlock + 1) * BBSIZE, SEEK_SET);
      state->len = read(desc->fd, state->buf, BBSIZE);
      state->cur = 0;
    }
    memset(name, '\x00', 64);
    namelen = 0;
    memcpy(&namelen, state->buf + state->cur + 0x40, 2);
    for(j = 0; j < namelen; j += 2) {
      name[j/2] = state->buf[state->cur + j];
    }

    /* get small blocks file start block */
    if(!strncmp(name, "Root Entry", 10)) {
      memcpy(&(state->SBFileStart), state->buf + state->cur + 0x74, 4);
    }

  } while(state->len > 0 && strncmp(name, "PowerPoint Document", 19));

  if(!state->len) {
    fprintf(stderr, "Not a Powerpoint (97 or higher) file\n");
    return -2;
  }


  memcpy(&(desc->size), state->buf + state->cur + 0x78, 4);
  memcpy(&propStart, state->buf + state->cur + 0x74, 4);
  memcpy(&(state->fileSize), state->buf + state->cur + 0x78, 4);

  /* if the file is contained in SBDs : goto sbd */
  if(desc->size < 0x1000) {
    state->bigSize = 0;
    lseek(desc->fd,
	  (state->SBFileStart + 1) * BBSIZE + state->currentSBlock * SBSIZE,
	  SEEK_SET);
    state->len = read(desc->fd, state->buf, SBSIZE);

  } else {
    /* goto first big block of file */
    state->bigSize = 1;
    state->currentBBlock = propStart;
    lseek(desc->fd, (state->currentBBlock + 1) * BBSIZE, SEEK_SET);
    state->len = read(desc->fd, state->buf, BBSIZE);
  }

  return OK;
}


int getNextBlock(struct doc_descriptor *desc) {
  struct oleState* state = (struct oleState *)(desc->myState);
  struct BBD *BBD;
  int len;
  int current;
  char buf[BBSIZE];

  if(state->bigSize) {
    current = state->currentBBlock;
    for(BBD = state->BBD; BBD != NULL && BBD->bbd != current; BBD = BBD->next) {}
    if(BBD == NULL) {
      fprintf(stderr, "Error in BBD\n");
      return -2;
    }
    state->currentBBlock = BBD->nextbbd;
    
    /* is it the end of workbook ? */
    if(state->currentBBlock == -2) {
      return NO_MORE_DATA;
    } else if(state->currentBBlock == -3) {
      fprintf(stderr, "Unexpected block type\n");
      return -2;
    }

  } else {
    lseek(desc->fd, (state->sbdStart + 1) * BBSIZE, SEEK_SET);
    len = read(desc->fd, buf, BBSIZE);
    current = state->currentSBlock;
    state->currentSBlock = 0;
    memcpy(&(state->currentSBlock), buf + current * 4, 4);
    if(state->currentSBlock == -2) {
      return NO_MORE_DATA;
    }
  }

  return OK;
}


int readOLE(struct doc_descriptor *desc, char *out) {
  struct oleState* state = (struct oleState *)(desc->myState);
  int err;

  err = getNextBlock(desc);
  if(err < 0) {
    return err;
  }
  if(state->bigSize) {
    lseek(desc->fd, (state->currentBBlock + 1) * BBSIZE, SEEK_SET);
    return read(desc->fd, out, BBSIZE);

  } else {
    lseek(desc->fd,
	  (state->SBFileStart + 1) * BBSIZE + state->currentSBlock * SBSIZE,
	  SEEK_SET);
    return read(desc->fd, out, SBSIZE);
  }

}


int getBBD(struct doc_descriptor *desc) {
  struct oleState* state = (struct oleState *)(desc->myState);
  struct BBD *BBD = NULL;
  char buf[BBSIZE];
  int currentBB;
  int BBDnb;
  int len, i,j;

  for(currentBB= 0, j = 0; j < state->nbOfBBD; j++) {
    memcpy(&BBDnb, state->buf + 0x4C + 4*j, 4);

    /* read next BBD */
    lseek(desc->fd, (BBDnb + 1) * BBSIZE, SEEK_SET);
    len = read(desc->fd, buf, BBSIZE);

    for(i = 0;  4 * i < len; i++, currentBB++) {
      
      if(state->BBD == NULL) {
	state->BBD = (struct BBD *) malloc(sizeof(struct BBD));
	BBD = state->BBD;
      } else {
	BBD->next = (struct BBD *) malloc(sizeof(struct BBD));
	BBD = BBD->next;
      }
      BBD->next = NULL;
      BBD->bbd = currentBB;
      memcpy(&(BBD->nextbbd), buf + 4 * i, 4);

    }

  }

  return OK;
}

 int min(int a, int b) {
   return (a < b) ? a : b;
 }

int freeBBD(struct BBD *BBD) {
  struct BBD *tmpBBD;

  while(BBD != NULL) {
    tmpBBD = BBD;
    BBD = BBD->next;
    free(tmpBBD);
  }
  return OK;
}
