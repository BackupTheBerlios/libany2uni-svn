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
 * internal functions for excel plugin
 */

#include "p_excel.h"
#include <string.h>
#include <stdio.h>
#include <unicode/ustring.h>


int read_next(struct doc_descriptor *desc, UChar *out, int size) {
  struct oleState* state = (struct oleState *)(desc->myState);
  int recordType;
  int recordlen;
  int sstIndex;
  int l, lastLen;

  memset(out, '\x00', size);
  while(1) {
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
    recordType = recordlen = 0;
    memcpy(&recordType, state->buf + state->cur, 2);
    memcpy(&recordlen, state->buf + state->cur + 2, 2);
    state->cur += 4;

    /* LABELSST */
    if(recordType == 0xFD) {
      state->cur += 6;
      while(state->cur >= state->len) {
	state->cur -= state->len;
	state->len = readOLE(desc, state->buf);
	if(state->len < 0) {
	  return state->len;
	}
      }
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

      /* get index of string in SST */
      sstIndex = 0;
      memcpy(&sstIndex, state->buf + state->cur, 4);
      state->cur += 4;
      memcpy(out, state->SST[sstIndex], 2*u_strlen(state->SST[sstIndex]));
      l = 2*u_strlen(state->SST[sstIndex]);
      return l;

    } else {
      /* skip record */
      state->cur += recordlen;
    }

    while(state->cur >= state->len) {
      state->cur -= state->len;
      state->len = readOLE(desc, state->buf);
      if(state->len < 0) {
	return state->len;
      }
    }

  }

  return NO_MORE_DATA;
}


int initOLE(struct doc_descriptor *desc) {
  struct oleState* state = (struct oleState *)(desc->myState);
  char name[64];
  int j, namelen;
  int propStart, sstSize;
  int recordType, recordlen;
  int lastrecord, lastlen;

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
  state->len = read(desc->fd, state->buf, BBSIZE);
  state->cur = -0x80;

  /* search for Workbook */
  do{
    state->cur += 0x80;
    if(state->cur > state->len - 0x80) {
      state->len = read(desc->fd, state->buf, BBSIZE);
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

  } while(state->len > 0 && strncmp(name, "Workbook", 8));

  if(!state->len) {
    fprintf(stderr, "Not a BIFF8 (excel 97, 2000, XP) file\n");
    return -2;
  }


  memcpy(&(desc->size), state->buf + state->cur + 0x78, 4);
  memcpy(&propStart, state->buf + state->cur + 0x74, 4);

  /* if the file is contained in SBDs : get sbd */
  if(desc->size < 0x1000) {
    state->bigSize = 0;
    lseek(desc->fd,
	  (state->SBFileStart + 1) * BBSIZE + state->currentSBlock * SBSIZE,
	  SEEK_SET);
    state->len = read(desc->fd, state->buf, SBSIZE);
  } else {
    state->bigSize = 1;
    state->currentBBlock = propStart;
    lseek(desc->fd, (state->currentBBlock + 1) * BBSIZE, SEEK_SET);
    state->len = read(desc->fd, state->buf, BBSIZE);
  }

  /* check BIFF8 header */
  if(strncmp(state->buf, "\x09\x08", 2)) {
    fprintf(stderr, "Not a BIFF8 (excel 97, 2000, XP) file\n");
    return -2;
  }
  if(strncmp(state->buf + 4, "\x00\x06", 2)) {
    fprintf(stderr, "Not a BIFF8 (excel 97, 2000, XP) file\n");
    return -2;
  }

  /* read workbook globals */
  state->cur = 20;
  recordType = recordlen = 0;
  memcpy(&recordType, state->buf + state->cur, 2);
  memcpy(&recordlen, state->buf + state->cur + 2, 2);
  state->cur += 4;
  j = 0;
  state->sstSize = 0;
  lastrecord = 0;
  while(recordType != 0x0A) {
    /* SST */
    if(recordType == 0xFC) {
      lastrecord = recordType;
      if(state->cur > state->len - 8) {
	lastlen = state->len;
	memcpy(state->buf, state->buf + state->cur, state->len - state->cur);
	state->len = readOLE(desc, state->buf + state->len - state->cur);
	if(state->len < 0) {
	  return state->len;
	}
	state->len += lastlen - state->cur;
	state->cur = 0;
      }
      memcpy(&(desc->size), state->buf + state->cur, 4);
      memcpy(&(state->sstSize), state->buf + state->cur + 4, 4);
      state->cur += 8;
      recordlen -= 8;
      state->SST = (UChar **) malloc(state->sstSize * sizeof(UChar *));
      if(state->SST == NULL) {
	fprintf(stderr, "Memory allocation error : malloc failed\n");
	return -2;
      }
      for(j = 0; j < state->sstSize; j++) {
	if(getUnicodeString(desc, &(state->SST[j]))) {
	  fprintf(stderr, "Unable to read SST\n");
	  return -2;
	}
      }

      /* CONTINUE */
    } else if(lastrecord == 0xFC && recordType == 0x3C) {
      if(state->cur > state->len - 8) {
	lastlen = state->len;
	memcpy(state->buf, state->buf + state->cur, state->len - state->cur);
	state->len = readOLE(desc, state->buf + state->len - state->cur);
	if(state->len < 0) {
	  return state->len;
	}
	state->len += lastlen - state->cur;
	state->cur = 0;
      }
      sstSize = 0;
      memcpy(&sstSize, state->buf + state->cur + 4, 4);
      state->sstSize += sstSize;
      state->cur += 8;
      recordlen -= 8;
      state->SST = (UChar **) realloc(state->SST, state->sstSize * sizeof(UChar *));
      if(state->SST == NULL) {
	fprintf(stderr, "Memory allocation error : realloc failed\n");
	return -2;
      }
      for(; j < state->sstSize; j++) {
	if(getUnicodeString(desc, &(state->SST[j]))) {
	  fprintf(stderr, "Unable to read SST\n");
	  return -2;
	}
      }
      
    } else {
      state->cur += recordlen;
      lastrecord = recordType;
    }
    while(state->cur >= state->len) {
      state->cur -= state->len;
      state->len = readOLE(desc, state->buf);
      if(state->len < 0) {
	return state->len;
      }
    }

    if(state->cur > state->len - 4) {
      lastlen = state->len;
      memcpy(state->buf, state->buf + state->cur, state->len - state->cur);
      state->len = readOLE(desc, state->buf + state->len - state->cur);
      if(state->len < 0) {
	return state->len;
      }
      state->len += lastlen - state->cur;
      state->cur = 0;
    }
    recordType = recordlen = 0;
    memcpy(&recordType, state->buf + state->cur, 2);
    memcpy(&recordlen, state->buf + state->cur + 2, 2);
    state->cur += 4;
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


int getUnicodeString(struct doc_descriptor *desc, UChar **target) {
  struct oleState* state = ((struct oleState *)(desc->myState));
  UErrorCode err;
  int j, i, k;
  char str[2*BUFSIZE];
  int flags = 0;
  int strlen = 0;
  int charlen = 1;
  int rtSize, feSize;
  int lastlen;

  if(state->cur > state->len - 3) {
    lastlen = state->len;
    memcpy(state->buf, state->buf + state->cur, state->len - state->cur);
    state->len = readOLE(desc, state->buf + state->len - state->cur);
    if(state->len < 0) {
      return state->len;
    }
    state->len += lastlen - state->cur;
    state->cur = 0;
  }

  feSize = rtSize = 0;
  flags = 0;
  memcpy(&flags, state->buf + state->cur + 2, 1);
  if(flags != 0 && flags != 1 && flags != 8 && flags != 9 && flags != 4
     && flags != 5 && flags != 0x0C && flags != 0x0D) {

    /* string length is encoded with 1 byte */
    memcpy(&flags, state->buf + state->cur + 1, 1);
    memcpy(&strlen, state->buf + state->cur, 1);
    state->cur += 2;

  } else {
    /* string length is encoded with 2 byte */
    memcpy(&strlen, state->buf + state->cur, 2);
    state->cur += 3;
  }

  if(flags & 0x01) {
    charlen = 2;
  }

  if(flags & 0x08) {
    /* rich text info */
    if(state->cur > state->len - 2) {
      lastlen = state->len;
      memcpy(state->buf, state->buf + state->cur, state->len - state->cur);
      state->len = readOLE(desc, state->buf + state->len - state->cur);
      if(state->len < 0) {
	return state->len;
      }
      state->len += lastlen - state->cur;
      state->cur = 0;
    }
    memcpy(&rtSize, state->buf + state->cur, 2);
    state->cur += 2;
  }

  if(flags & 0x04) {
    /* Far-East info */
    if(state->cur > state->len - 4) {
      lastlen = state->len;
      memcpy(state->buf, state->buf + state->cur, state->len - state->cur);
      state->len = readOLE(desc, state->buf + state->len - state->cur);
      if(state->len < 0) {
	return state->len;
      }
      state->len += lastlen - state->cur;
      state->cur = 0;
    }
    memcpy(&feSize, state->buf + state->cur, 4);
    state->cur += 4;
  }

  for(j = 0; j < charlen * strlen; j += charlen) {
    if(state->cur > state->len - charlen) {
      lastlen = state->len;
      memcpy(state->buf, state->buf + state->cur, state->len - state->cur);
      state->len = readOLE(desc, state->buf + state->len - state->cur);
      if(state->len < 0) {
	return state->len;
      }
      state->len += lastlen - state->cur;
      state->cur = 0;
    }
    memcpy(str + j, state->buf + state->cur, charlen);
    state->cur += charlen;
  }
  memset(str + j, '\0', charlen);

  *target = (UChar*) malloc(2 * (j+1));
  if(*target == NULL) {
    fprintf(stderr, "Memory allocation error : malloc failed\n");
    return -2;
  }

  if(charlen == 2) {
    for(i = 0, k = 0; i < j; i++) {
      if(u_isdefined(str[i])) {
	memcpy(*target + 2*k, str + 2*i, 2);
	k++;
      }
    }
    memcpy(*target+ 2*k, "\x00\x00", 2);

  } else {
    /* converting to UTF-16 */
    err = U_ZERO_ERROR;
    ucnv_toUChars(desc->conv, *target, 2 * (j + 1), str, j + 1, &err);
    if (U_FAILURE(err)) {
      fprintf(stderr, "Unable to convert buffer\n");
      free((char *) str);
      return ERR_ICU;
    }
  }

  state->cur += 4*rtSize;
  state->cur += feSize;
  while(state->cur >= state->len) {
    state->cur -= state->len;
    state->len = readOLE(desc, state->buf);
    if(state->len < 0) {
      return state->len;
    }
  }

  return OK;
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
