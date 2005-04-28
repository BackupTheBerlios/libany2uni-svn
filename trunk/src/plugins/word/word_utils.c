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
 * internal functions for word plugin
 */

#include "p_word.h"
#include <string.h>
#include <stdio.h>
#include <unicode/ustring.h>

/* to identifie the word document type ( ole or older ) */
enum wType identify_version(FILE *fd) {
  char buf[10];

  fseek(fd, 0, SEEK_SET);
  if (fread(buf, 1, 8, fd) != 8) {
    fprintf(stderr, "not word file\n");
    return unknown;
  }
  if (!strncmp(buf, "\xDB\xA5", 2)) {
    fseek(fd, 0, SEEK_SET);
    return oldword;
  }
  if (!strncmp(buf, "\xD0\xCF\x11\xE0\xA1\xB1\x1A\xE1", 8)) {
    fseek(fd, 0, SEEK_SET);
    return ole;
  }

  return unknown;
}

/* to position the file cursor at the beginning of text content */
long int seek_textstart(struct doc_descriptor *desc) {
  FILE *fd = desc->filedes;
  char buf[1024];
  long int property_pos, startblock, textstart;
  long int length;
  short int flags, charset;
  int i, j, fini, prev;
  char name[13];

  fseek(fd, 0, SEEK_SET);

  /* get properties location */
  fread(buf, 1, 1024, fd);
  memcpy(&property_pos, buf + 0x30, 4);

  /* looking for 'WordDocument' entry */
  fseek(fd, ( property_pos + 1) * 512, SEEK_SET);
  prev = ftell(fd);
  fread(buf, 1, 1024, fd);
  fini = 0;
  for (i=0; !fini; i++) {
    memset(name, 0x00, 13);
    for (j=0; j<26; j+=2) {
      memcpy(name + j/2, buf + i + j, 1);
    }
    memcpy(name + 13, "\0", 1);
    if (!strncmp(name, "WordDocument", 12)) {
      fini = 1;
    }
    if (!fini && i > 1024) {
      fseek(fd, -26, SEEK_CUR);
      prev = ftell(fd);
      if (fread(buf, 1, 1024, fd) > 26) {
	i=0;
      } else {
	fini = 1;
      }
    }
  }

  /* get start block for stream */
  if (!strncmp(name, "WordDocument", 12)) {
    fseek(fd, prev + i - 1, SEEK_SET);
    fread(buf, 1, 1024, fd);
    memcpy(&startblock, buf + 0x74, 4);

    /* position at begining of stream */
    fseek(fd, (startblock + 1) * 512, SEEK_SET);

    /* get text start offset, text length, charset and flags*/
    fread(buf, 1, 32, fd);
    memcpy(&textstart, buf + 24, 4);
    memcpy(&length, buf + 28, 4);
    length -= textstart;
    memcpy(&charset, buf + 20, 2);
    if (charset) {
      fprintf(stderr, "not standard charset, output might be incorrect, charset code : %d\n", charset);
    }
    memcpy(&flags, buf + 10, 2);
    if (flags & 0x100) {
      fprintf(stderr, "File encrypted, unable to read\n");
      return -2;
    }

    /* position on text start */
    ((struct ParserState *)(desc->myState))->begin_byte = (startblock + 1) * 512 + textstart;
    fseek(fd, textstart - 32, SEEK_CUR);

  } else {
    fprintf(stderr, "unable to find WordDocument entry\n");
    return -2;
  }

  return length;
}

/* to read the next paragraph */
int read_next(struct doc_descriptor *desc, char *out, int size) {
  char buf[BUFSIZE];
  int len, fini, i, l;
  int len_available;
  
  l=0;
  fini = 0;

  /* ensure that we do not read to much */
  len_available = (ftell(desc->filedes) >=
		   ((struct ParserState *)(desc->myState))->begin_byte + desc->size - BUFSIZE ?
		   ((struct ParserState *)(desc->myState))->begin_byte + desc->size - ftell(desc->filedes):
		   BUFSIZE - 1);
		   
  len = fread(buf, 1, len_available, desc->filedes);

  while (len > 0 && !fini) {
    for(i=0; !fini && i<len; i++) {

      /* footnote */
      if (!memcmp(buf + i, "\x02", 1)) {
	memcpy(out + l, "*", 1);
	l++;

	/* tabs */
      } else if (!memcmp(buf + i, "\x07", 1) || !memcmp(buf + i, "\x09", 1)) {
	memcpy(out + l, " ", 1);
	l++;
	
	/* end of paragraph */
      } else if (!memcmp(buf + i, "\x0D", 1) || !memcmp(buf + i, "\x0B", 1)) {
	fini = 1;
	fseek(desc->filedes, i - len + 1, SEEK_CUR);

	/* hyphens */
      } else if (!memcmp(buf + i, "\x1E", 1) || !memcmp(buf + i, "\x1F", 1)) {
	memcpy(out + l, "-", 1);
	l++;

	/* zero-width unbreakable space*/
      } else if (!memcmp(buf + i, "\xfe\xff", 2)) {
	i++;

	/* every other character */
      } else if(buf[i] >= 32 || buf[i] < 0) {
	memcpy(out + l, buf + i, 1);
	l++;
      }

      /* stop when output buffer is full */
      if (!fini && l >= INTERNAL_BUFSIZE-1) {
	fini = 1;
	fseek(desc->filedes, i - len + 1, SEEK_CUR);
      }
    }

    /* fill a new buffer */
    if (!fini) {
      len_available = (ftell(desc->filedes) >=
		       ((struct ParserState *)(desc->myState))->begin_byte + desc->size - BUFSIZE + 1?
		       ((struct ParserState *)(desc->myState))->begin_byte + desc->size - ftell(desc->filedes):
		       BUFSIZE - 1);
      len = fread(buf, 1, len_available, desc->filedes);
    }
  }
  /* is end of document reached ? */
  if (l == 0 && len == 0) {
    return NO_MORE_DATA;
  }
  
  /* end output buffer propely */
  memcpy(out + l, "\0", 1);

  return l;
}
