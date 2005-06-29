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
 * charset mapping functions for PDF plugin
 */

#include "p_pdf.h"
#include "unicode/uchar.h"

#define MAX_NB_FONTS 20

int getEncodings(struct doc_descriptor *desc) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  int len, i, j;
  char buf[BUFSIZE];
  char name[10];
  struct encodingTable *encoding = state->encodings;
  int nbopened, found;
  int fontref[MAX_NB_FONTS], encodingref[MAX_NB_FONTS];
  int nbfonts, nbencodrefs;
  int curfonts, curencodrefs;

  nbfonts = nbencodrefs = 0;

  /* search fonts in current object */
  len = readObject(desc, buf, BUFSIZE);

  /* search dictionary */
  for (i = 0; i < len - 1 && strncmp(buf + i, "<<", 2); i++) {
    if(i >= len - 2) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
  }
  i += 2;
  if(i >= len - 10) {
    strncpy(buf, buf + i, len - i);
    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
    i = 0;
  }

  nbopened = 0;
  for ( ; i < len - 9 && (nbopened || strncmp(buf + i, ">>", 2)) &&
	  strncmp(buf + i, "/Resources", 10); i++) {
    if(i >= len - 10) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }

    if(!strncmp(buf + i, "<<", 2)) {
      nbopened++;
    } else if(!strncmp(buf + i, ">>", 2)) {
      nbopened--;
    }
  }

  /* if no ressources available */
  if (strncmp(buf + i, "/Resources", 10)) {
    return 0;
  }


  i += 10;
  while (!strncmp(buf + i, " ", 1) ||
	 !strncmp(buf + i, "\x0A", 1) ||
	 !strncmp(buf + i, "\x0D", 1)) {
    i++;
    if(i >= len - 10) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
  }
  
  /* enter ressources */
  if(strncmp(buf + i, "<<", 2)) {
    gotoRef(desc, getNumber(buf + i));
    len = readObject(desc, buf, BUFSIZE);
    i = 0;
  }

  /* search dictionary */
  for ( ; i < len - 1 && strncmp(buf + i, "<<", 2); i++) {
    if(i >= len - 5) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
  }
  i += 2;

  nbopened = 0;
  for ( ; i < len - 4 && (nbopened || strncmp(buf + i, ">>", 2)) &&
	  strncmp(buf + i, "/Font", 5); i++) {
    if(i >= len - 5) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }

    if(!strncmp(buf + i, "<<", 2)) {
      nbopened++;
    } else if(!strncmp(buf + i, ">>", 2)) {
      nbopened--;
    }
  }

  /* if no font available */
  if(strncmp(buf + i, "/Font", 5)) {
    return 0;
  }
  i += 5;
  if(i >= len - 2) {
    strncpy(buf, buf + i, len - i);
    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
    i = 0;
  }
  while (!strncmp(buf + i, " ", 1) ||
	 !strncmp(buf + i, "\x0A", 1) ||
	 !strncmp(buf + i, "\x0D", 1)) {
    i++;
    if(i >= len - 2) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
  }
  
  /* enter fonts */
  if(strncmp(buf + i, "<<", 2)) {
    gotoRef(desc, getNumber(buf + i));
    len = readObject(desc, buf, BUFSIZE);
    i = 0;
  }

  /* search dictionary */
  for ( ; i < len - 1 && strncmp(buf + i, "<<", 2); i++) {
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

  nbopened = 0;  
  for ( ; i < len - 1 && (nbopened || strncmp(buf + i, ">>", 2)); i++) {
    if(i >= len - 10) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }

    if(!strncmp(buf + i, "<<", 2)) {
      nbopened++;
    } else if(!strncmp(buf + i, ">>", 2)) {
      nbopened--;
    }

    /* a font has been found */
    if(!strncmp(buf + i, "/", 1)) {
      i++;
      if(i >= len - 10) {
	strncpy(buf, buf + i, len - i);
	len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	i = 0;
      }

      /* get font name */
      memset(name, '\0', 10);
      for(j = 0; strncmp(buf + i, " ", 1) &&
	    strncmp(buf + i, "\x0A", 1) &&
	    strncmp(buf + i, "\x0D", 1) &&
	    strncmp(buf + i, "/", 1) &&
	    strncmp(buf + i, "<", 1); j++) {
	strncpy(name + j, buf + i, 1);
	i++;
	if(i >= len) {
	  len = readObject(desc, buf, BUFSIZE);
	  i = 0;
	}
      }
      strncpy(name + j, " ", 1);

      /* search font in encoding table */
      found = 0;
      encoding = state->encodings;
      while(!found && encoding != NULL) {
	if(!strncmp(encoding->fontName, name, strlen(name))) {
	  found = 1;
	} else {
	  encoding = encoding->next;
	}
      }

      /* procede new font */
      if(!found) {

	/* create new encoding structure in linked list */
	if(state->encodings == NULL) {
	  encoding = (struct encodingTable *) malloc(sizeof(struct encodingTable));
	  state->encodings = encoding;
	} else {
	  encoding = state->encodings;
	  while(encoding->next != NULL) {
	    encoding = encoding->next;
	  }
	  encoding->next = (struct encodingTable *) malloc(sizeof(struct encodingTable));
	  encoding = encoding->next;
	}
	encoding->next = NULL;
	encoding->fontName = (char *) malloc(strlen(name) + 1);
	strncpy(encoding->fontName, name, strlen(name));
	strncpy(encoding->fontName + strlen(name), "\0", 1);
	encoding->encoding = NULL;
	encoding->diff = NULL;
	encoding->ToUnicode = -1;

	/* get font infos */
	if(i >= len - 10) {
	  strncpy(buf, buf + i, len - i);
	  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	  i = 0;
	}

	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if(i >= len - 10) {
	    strncpy(buf, buf + i, len - i);
	    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	    i = 0;
	  }
	}

	if(!strncmp(buf + i, "<<", 2)) {

	  /* analyze font dictionary */
	  i += 2;
	  if(i >= len - 10) {
	    strncpy(buf, buf + i, len - i);
	    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	    i = 0;
	  }
	  readFontDictionary(desc, buf + i, len - i, encoding, encodingref, &nbencodrefs);

	} else {
	  /* store font reference */
	  if(nbfonts >= MAX_NB_FONTS) {
	    fprintf(stderr, "to many fonts\n");
	    return ERR_TOMANYFONTS;
	  }
	  fontref[nbfonts] = getNumber(buf + i);
	  encoding->ToUnicode = -2;
	  nbfonts++;
	}
      }
    }
  }

  /* read indirect encodings */
  encoding = state->encodings;
  for(j = 0; j < nbencodrefs; j++) {
    while(encoding->encoding != NULL || encoding->ToUnicode == -2) {
      encoding = encoding->next;
    }
    gotoRef(desc, encodingref[j]);
    len = readObject(desc, buf, BUFSIZE);
    i = getNextLine(buf, len);
    readEncodingObject(desc, buf + i, len - i, encoding);
  }
  nbencodrefs = 0;

  /* procede indirect objects */
  curfonts = curencodrefs = 0;
  for(encoding = state->encodings; encoding != NULL; encoding = encoding->next) {

    /* procede indirect font dictionaries */
    if(encoding->ToUnicode == -2) {
      encoding->ToUnicode = -1;
      gotoRef(desc, fontref[curfonts]);
      len = readObject(desc, buf, BUFSIZE);
      for(i = 0; strncmp(buf + i, "<<", 2); i++) {
	if(i >= len - 10) {
	  strncpy(buf, buf + i, len - i);
	  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	  i = 0;
	}
      }
      if(i >= len - 10) {
	strncpy(buf, buf + i, len - i);
	len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	i = 0;
      }
      i += 2;

      readFontDictionary(desc, buf + i, len - i, encoding, encodingref, &nbencodrefs);
      curfonts++;
    }
  }

  /* read indirect encodings */
  encoding = state->encodings;
  for(j = 0; j < nbencodrefs; j++) {
    while(encoding->encoding != NULL || encoding->ToUnicode == -2) {
      encoding = encoding->next;
    }
    gotoRef(desc, encodingref[j]);
    len = readObject(desc, buf, BUFSIZE);
    i = getNextLine(buf, len);
    readEncodingObject(desc, buf + i, len - i, encoding);
  }

  /* read all ToUnicode CMaps */
  encoding = state->encodings;
  while(encoding != NULL) {
    if(encoding->ToUnicode != -1) {
      readToUnicodeCMap(desc, encoding->ToUnicode);
    }
    encoding = encoding->next;
  }

  return OK;
}


int setEncoding(struct doc_descriptor *desc, char *fontName) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  struct encodingTable *encoding = state->encodings;
  UErrorCode err;

  /* search font in table */
  while(encoding != NULL && strncmp(encoding->fontName, fontName,
				    strlen(encoding->fontName))) {
    encoding = encoding->next;
  }

  /* update ICU converter encoding */
  if(encoding != NULL && (encoding->encoding != NULL ||
			  encoding->ToUnicode != -1)) {
    state->currentEncoding = encoding;
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

int readFontDictionary(struct doc_descriptor *desc, char *buf, int buflen,
		       struct encodingTable *encoding, int *encodingref, int *nbencodrefs) {
  int nbopened;
  int i, len;

  nbopened = 0;
  len = buflen;
  for(i = 0; nbopened || strncmp(buf + i, ">>", 2); i++) {
    if(i >= len - 10) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }

    if(!strncmp(buf + i, "<<", 2)) {
      nbopened++;
    } else if(!strncmp(buf + i, ">>", 2)) {
      nbopened--;
    }

    /* get ToUnicode stream ref */
    if(!strncmp(buf + i, "/ToUnicode", 10)) {
      i += 10;
      while(!strncmp(buf + i, " ", 1) ||
	    !strncmp(buf + i, "\x0A", 1) ||
	    !strncmp(buf + i, "\x0D", 1)) {
	i++;
	if(i >= len - 10) {
	  strncpy(buf, buf + i, len - i);
	  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	  i = 0;
	}
      }
      encoding->ToUnicode = getNumber(buf + i);
	     
    }

    /* get encoding */
    if(!strncmp(buf + i, "/Encoding", 9)) {
     i += 9;
      while(!strncmp(buf + i, " ", 1) ||
	    !strncmp(buf + i, "\x0A", 1) ||
	    !strncmp(buf + i, "\x0D", 1)) {
	i++;
	if(i >= len - 10) {
	  strncpy(buf, buf + i, len - i);
	  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	  i = 0;
	}
      }
      if(strncmp(buf + i, "/", 1) && strncmp(buf + i, "<<", 2)) {
	/* this is a reference to an encoding dictionary */
	if(*nbencodrefs >= MAX_NB_FONTS) {
	  fprintf(stderr, "to many references to encoding dictionary\n");
	  return ERR_TOMANYFONTS;
	}
	encodingref[*nbencodrefs] = getNumber(buf + i);
	(*nbencodrefs)++;
      } else {
	readEncodingObject(desc, buf + i, len - i, encoding);
      }
    }
  }
  return OK;
}


int readEncodingObject(struct doc_descriptor *desc, char *buf, int buflen, struct encodingTable *encoding) {
  struct diffTable *diff = NULL;
  int i, j, len;
  int code = 0;
  char name[40];

  len = buflen;
  i = 0;
  if(!strncmp(buf + i, "/", 1)) {
    /* this is a standard encoding name */
    i++;
    if(i >= len - 16) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      i = 0;
    }
    if(!strncmp(buf + i, "WinAnsiEncoding", 15)) {
      encoding->encoding = (char *) malloc(7);
      strncpy(encoding->encoding, "cp1252\0", 7);

    } else if(!strncmp(buf + i, "MacRomanEncoding", 16)) {
      encoding->encoding = (char *) malloc(4);
      strncpy(encoding->encoding, "mac\0", 4);

    } else {
      encoding->encoding = (char *) malloc(7);
      strncpy(encoding->encoding, "latin1\0", 7);
    }

  } else if(!strncmp(buf + i, "<<", 2)) {
    /* this is an encoding dictionary */

    for( ; strncmp(buf + i, ">>", 2); i++) {
      if(i >= len - 14) {
	strncpy(buf, buf + i, len - i);
	len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	i = 0;
      }

      if(!strncmp(buf + i, "/BaseEncoding", 13)) {
	i++;
	if(i >= len - 16) {
	  strncpy(buf, buf + i, len - i);
	  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	  i = 0;
	}
	if(!strncmp(buf + i, "WinAnsiEncoding", 15)) {
	  encoding->encoding = (char *) malloc(7);
	  strncpy(encoding->encoding, "cp1252\0", 7);

	} else if(!strncmp(buf + i, "MacRomanEncoding", 16)) {
	  encoding->encoding = (char *) malloc(4);
	  strncpy(encoding->encoding, "mac\0", 4);
	}
      }

      if(!strncmp(buf + i, "/Differences", 12)) {

	/* fill diff table */
	for( ; strncmp(buf + i, "[", 1); i++) {
	  if(i >= len - 5) {
	    strncpy(buf, buf + i, len - i);
	    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	    i = 0;
	  }
	}
	i++;
	if(i >= len - 5) {
	  strncpy(buf, buf + i, len - i);
	  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	  i = 0;
	}
	for( ; strncmp(buf + i, "]", 1); ) {
	  if(i >= len - 5) {
	    strncpy(buf, buf + i, len - i);
	    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	    i = 0;
	  }
	  if(strncmp(buf + i, " ", 1) &&
	     strncmp(buf + i, "\x0A", 1) &&
	     strncmp(buf + i, "\x0D", 1)) {

	    if(!strncmp(buf + i, "/", 1)) {
	      /* character name */
	      if(encoding->diff == NULL) {
		encoding->diff = (struct diffTable *) malloc(sizeof(struct diffTable));
		(encoding->diff)->next = NULL;
		diff = encoding->diff;
	      } else {
		diff->next = (struct diffTable *) malloc(sizeof(struct diffTable));
		diff = diff->next;
		diff->next = NULL;
	      }
	      diff->code = code;
	      code++;

	      /* get name */
	      memset(name, '\0', 5);
	      i++;
	      if(i >= len - 5) {
		strncpy(buf, buf + i, len - i);
		len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
		i = 0;
	      }
	      for(j = 0; strncmp(buf + i, " ", 1) &&
		     strncmp(buf + i, "\x0A", 1) &&
		     strncmp(buf + i, "\x0A", 1) &&
		     strncmp(buf + i, "]", 1) &&
		     strncmp(buf + i, "/", 1); j++) {
		strncpy(name + j, buf + i, 1);
		i++;
		if(i >= len - 5) {
		  strncpy(buf, buf + i, len - i);
		  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
		  i = 0;
		}
	      }
	      diff->name = (char *) malloc(j + 1);
	      strncpy(diff->name, name, j);
	      strncpy(diff->name + j, "\0", 1);

	    } else {
	      /* code */
	      code = getNumber(buf + i);
	      for( ; strncmp(buf + i, " ", 1) &&
		     strncmp(buf + i, "\x0A", 1) &&
		     strncmp(buf + i, "/", 1) &&
		     strncmp(buf + i, "\x0D", 1); i++) {
		if(i >= len - 5) {
		  strncpy(buf, buf + i, len - i);
		  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
		  i = 0;
		}
	      }
	    }
	  } else {
	    i++;
	  }
	}
      }

    }
    if(encoding->encoding == NULL) {
      encoding->encoding = (char *) malloc(7);
      strncpy(encoding->encoding, "latin1\0", 7);
    }
  }
 
  return OK; 
}


int mapCharset(struct doc_descriptor *desc, int charcode, UChar *out, int *l) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  struct diffTable *diff;
  struct CMapList *cmaplist;
  struct ToUnicodeCMap *cmap;
  UChar dest[6];
  UErrorCode err;
  char src[2], name[40], buf[BUFSIZE], value[6];
  char glyphpath[512];
  int len, i, v;

  memset(dest, '\x00', 6);
  memset(value, '\x00', 6);
  memset(src, '\x00', 2);

  if(state->currentEncoding != NULL && state->currentEncoding->ToUnicode > 0) {

    /* use ToUnicode CMap */
    for(cmaplist = state->cmaplist; cmaplist->ref != state->currentEncoding->ToUnicode;
	cmaplist = cmaplist->next) {}
    v = charcode + 256*state->last;
    cmap = g_hash_table_lookup(cmaplist->cmap, &v);
    if(cmap != NULL) {
      if(u_isdefined(cmap->value[0])) {
	memcpy(out+(*l)/2, cmap->value, cmap->vallength);
	*l += cmap->vallength;
      }
      state->last = 0;
      state->last_available = 0;
      return OK;
    } else {
      state->last = charcode;
      state->last_available = 1;
      return OK;      
    }
    
  }    

  if(state->currentEncoding != NULL && state->currentEncoding->diff != NULL) {

    /* look for charcode in diffTable */
    for(diff = state->currentEncoding->diff;
	diff != NULL && diff->code != charcode;
	diff = diff->next) {}
    if(diff != NULL) {
      if(!strncmp(diff->name, ".notdef", 7)) {
	memcpy(out + (*l)/2, "\x20\x00", 2);
	l += 2;
	return OK;
      }
      if(state->glyphfile == -1) {
	memset(glyphpath, '\x00', 512);
	sprintf(glyphpath, "%s/libany2uni/glyphlist.txt", INSTALL_PATH);
	state->glyphfile = open(glyphpath, O_RDONLY);
      }
      lseek(state->glyphfile, 0, SEEK_SET);
      len = read(state->glyphfile, buf, BUFSIZE);
      i = 0;
      while(!strncmp(buf, "#", 1)){
	if(i >= len - 72) {
	  strncpy(buf, buf + i, len - i);
	  len = read(state->glyphfile, buf + len - i, BUFSIZE - len + i) + len - i;
	  i = 0;
	}
	i += getNextLine(buf + i, len - i);
      }

      /* find char name in glyphfile */
      strncpy(name, diff->name, strlen(diff->name));
      strncpy(name + strlen(diff->name), ";", 1);
      while(len > 0 && strncmp(buf + i, name, strlen(diff->name)+1)) {
	if(i >= len - 42) {
	  strncpy(buf, buf + i, len - i);
	  len = read(state->glyphfile, buf + len - i, BUFSIZE - len + i) + len - i;
	  i = 0;
	}
	i += getNextLine(buf + i, len - i);
      }
      if(len > 0) {
	if(i >= len - strlen(diff->name) - 1) {
	  strncpy(buf, buf + i, len - i);
	  len = read(state->glyphfile, buf + len - i, BUFSIZE - len + i) + len - i;
	  i = 0;
	}
	i += strlen(diff->name) + 1;
	if(i >= len - 4) {
	  strncpy(buf, buf + i, len - i);
	  len = read(state->glyphfile, buf + len - i, BUFSIZE - len + i) + len - i;
	  i = 0;
	}

	/* get unicode value (UTF-16BE) */
	strncpy(value, buf + i, 4);
	v = 0;
	for(i = 0; i < 2; i++) {
	  v *= 16;
	  if(value[2*i] == 'A') {
	    v += 10;
	  } else if(value[2*i] == 'B') {
	    v += 11;
	  } else if(value[2*i] == 'C') {
	    v += 12;
	  } else if(value[2*i] == 'D') {
	    v += 13;
	  } else if(value[2*i] == 'E') {
	    v += 14;
	  } else if(value[2*i] == 'F') {
	    v += 15;
	  } else {
	    v += value[2*i] - 48;
	  }
	  v *= 16;
	  if(value[2*i+1] == 'A') {
	    v += 10;
	  } else if(value[2*i+1] == 'B') {
	    v += 11;
	  } else if(value[2*i+1] == 'C') {
	    v += 12;
	  } else if(value[2*i+1] == 'D') {
	    v += 13;
	  } else if(value[2*i+1] == 'E') {
	    v += 14;
	  } else if(value[2*i+1] == 'F') {
	    v += 15;
	  } else {
	    v += value[2*i+1] - 48;
	  }
	}
	if(u_isdefined(v)) {
	  memcpy(out+(*l)/2, &v, 2);
	  *l += 2;
	}
	return OK;
      }
    }


  }

  /* convert using current encoding */
  sprintf(src, "%c", charcode);
  strncpy(src + 1, "\0", 1);
  err = U_ZERO_ERROR;
  ucnv_toUChars(desc->conv, dest, 4,
		src, 1, &err);
  if (U_FAILURE(err)) {
    fprintf(stderr, "Unable to convert buffer\n");
    return ERR_ICU;
  }
  memcpy(out + (*l)/2, dest, 2);
  *l += 2;

  return OK;
}


int readToUnicodeCMap(struct doc_descriptor *desc, int ToUnicode) {
  struct pdfState *state = ((struct pdfState *)(desc->myState));
  struct CMapList *cmaplist;
  struct ToUnicodeCMap *cmap = NULL;
  UErrorCode err;
  char buf[BUFSIZE], tmp[20];
  int len, i, len2, j;
  unsigned int v;
  int nbopened, code, lastcode, codelength, vallength;

  /* create new CMap in list */
  if(state->cmaplist == NULL) {
    state->cmaplist = (struct CMapList *) malloc(sizeof(struct CMapList));
    cmaplist = state->cmaplist;
  } else {
    for(cmaplist = state->cmaplist; cmaplist->ref != ToUnicode && cmaplist->next != NULL;
	cmaplist = cmaplist->next) {}
    if(cmaplist->ref == ToUnicode) {
      /* This CMap already exists */
      return OK;
    }
    cmaplist->next = (struct CMapList *) malloc(sizeof(struct CMapList));
    cmaplist = cmaplist->next;
  }
  cmaplist->next = NULL;
  cmaplist->cmap = g_hash_table_new((GHashFunc)g_int_hash, (GCompareFunc)g_int_equal);
  cmaplist->ref = ToUnicode;

  /* read CMap object */
  gotoRef(desc, ToUnicode);
  len = readObject(desc, buf, BUFSIZE);
  for(i = 0; strncmp(buf + i, "/Length", 7); i++) {}
  i += 7;
  for( ; !strncmp(buf + i, " ", 1) ||
	 !strncmp(buf + i, "\x0A", 1) ||
	 !strncmp(buf + i, "\x0D", 1) ||
	 !strncmp(buf + i, "/", 1); i++) {}
  for(j = 0; strncmp(buf + i + j, " ", 1) &&
	strncmp(buf + i + j, "/", 1) &&
	strncmp(buf + i + j, "\x0A", 1) &&
	strncmp(buf + i + j, "\x0D", 1); j++) {}
  if(strncmp(buf + i + j, "/", 1) && strncmp(buf + i + j, ">>", 2)) {
    j++;
  }
  for( ; !strncmp(buf + i + j, " ", 1) ||
	 !strncmp(buf + i + j, "\x0A", 1) ||
	 !strncmp(buf + i + j, "\x0D", 1); j++) {}
  if(!strncmp(buf + i + j, "/", 1) || !strncmp(buf + i + j, ">>", 2)) {
    
    /* Length is an integer */
    state->streamlength = state->length = getNumber(buf + i);

    /* Length is a reference */
  } else {
    gotoRef(desc, getNumber(buf + i));
    len = read(desc->fd, buf, BUFSIZE);
    i = getNextLine(buf, len);
    while(!strncmp(buf + i, " ", 1) ||
	  !strncmp(buf + i, "\x0A", 1) ||
	  !strncmp(buf + i, "\x0D", 1)) {
      i++;
    }
    state->streamlength = state->length = getNumber(buf + i);
  }

  state->objectStream = ToUnicode;
  state->first = 0;
  state->offsetInStream = 0;
  len = readObject(desc, buf, BUFSIZE);
  state->objectStream = ToUnicode;

  /* read CMap dictionary */
  for(i = 0; strncmp(buf + i, "<<", 2); i++) {
    if(i >= len - 2) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      state->objectStream = ToUnicode;
      i = 0;
    }
  }
  i+=2;
  if(i >= len - 2) {
    strncpy(buf, buf + i, len - i);
    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
    state->objectStream = ToUnicode;
    i = 0;
  }
  nbopened = 0;
  for( ; nbopened || strncmp(buf + i, ">>", 2); i++) {
    if(i >= len - 6) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      state->objectStream = ToUnicode;
      i = 0;
    }

    if(!strncmp(buf + i, "<<", 2)) {
      nbopened++;
    } else if(!strncmp(buf + i, ">>", 2)) {
      nbopened--;
    }

    if(!strncmp(buf + i, "/UseCMap", 8)) {
      /* ignored */
    }
  }

  ucnv_close(desc->conv);
  err = U_ZERO_ERROR;
  desc->conv = ucnv_open("utf16LE", &err);
  if (U_FAILURE(err)) {
    fprintf(stderr, "unable to open ICU converter\n");
    return ERR_ICU;
  }

  /* get CMap */
  for( ; strncmp(buf + i, "endcmap", 7); i++) {
    if(i >= len - 20) {
      strncpy(buf, buf + i, len - i);
      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
      state->objectStream = ToUnicode;
      i = 0;
    }

    /* unique codes */
    if(!strncmp(buf + i, "beginbfchar", 11)) {
      i += 11;
      if(i >= len - 9) {
	strncpy(buf, buf + i, len - i);
	len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	state->objectStream = ToUnicode;
	i = 0;
      }
      while(strncmp(buf + i, "endbfchar", 9)) {

	for( ; strncmp(buf + i, "<", 1); i++) {
	  if(i >= len - 9) {
	    strncpy(buf, buf + i, len - i);
	    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	    state->objectStream = ToUnicode;
	    i = 0;
	  }
	}
	i++;

	/* create new entry in ToUnicodeCMap list */
	cmap = (struct ToUnicodeCMap *) malloc(sizeof(struct ToUnicodeCMap));
	cmap->codelength = 0;

	/* get hex code and code length */
	cmap->code = 0;
	while(strncmp(buf + i, ">", 1)) {
	  cmap->codelength++;
	  memset(tmp, '\x00', 20);
	  for(j = 0; j < 2 && strncmp(buf + i, ">", 1); j++) {
	    strncpy(tmp, buf + i, 1);
	    cmap->code *= 16;
	    if(!strncmp(tmp, "a", 1) || !strncmp(tmp, "A", 1)) {
	      cmap->code += 10;
	    } else if(!strncmp(tmp, "b", 1) || !strncmp(tmp, "B", 1)) {
	      cmap->code += 11;
	    } else if(!strncmp(tmp, "c", 1) || !strncmp(tmp, "C", 1)) {
	      cmap->code += 12;
	    } else if(!strncmp(tmp, "d", 1) || !strncmp(tmp, "D", 1)) {
	      cmap->code += 13;
	    } else if(!strncmp(tmp, "e", 1) || !strncmp(tmp, "E", 1)) {
	      cmap->code += 14;
	    } else if(!strncmp(tmp, "f", 1) || !strncmp(tmp, "F", 1)) {
	      cmap->code += 15;
	    } else if(!strncmp(tmp, ">", 1)) {
	      cmap->code += 0;
	    } else {
	      cmap->code += atoi(tmp);
	    }
	    if(strncmp(tmp, ">", 1)) {
	      i++;
	      if(i >= len - 9) {
		strncpy(buf, buf + i, len - i);
		len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
		state->objectStream = ToUnicode;
		i = 0;
	      }
	    }
	  }
	}

	/* get hex value */
	for( ; strncmp(buf + i, "<", 1); i++) {
	  if(i >= len - 9) {
	    strncpy(buf, buf + i, len - i);
	    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	    state->objectStream = ToUnicode;
	    i = 0;
	  }
	}
	i++;

	vallength = 0;
	v = 0;
	while(strncmp(buf + i, ">", 1)) {
	  memset(tmp, '\x00', 20);
	  vallength++;
	  for(j = 0; j < 2 && strncmp(buf + i, ">", 1); j++) {
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
	      if(i >= len - 9) {
		strncpy(buf, buf + i, len - i);
		len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
		state->objectStream = ToUnicode;
		i = 0;
	      }
	    }
	  }
	}

	cmap->value = (UChar *) malloc(10);
	cmap->vallength = vallength;
	err = U_ZERO_ERROR;
	len2 = ucnv_toUChars(desc->conv, cmap->value, 10, (char *)&v, vallength, &err);
	if (U_FAILURE(err)) {
	  fprintf(stderr, "Unable to convert buffer\n");
	  return ERR_ICU;
	}
	i++;
	if(i >= len - 9) {
	  strncpy(buf, buf + i, len - i);
	  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	  state->objectStream = ToUnicode;
	  i = 0;
	}
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if(i >= len - 9) {
	    strncpy(buf, buf + i, len - i);
	    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	    state->objectStream = ToUnicode;
	    i = 0;
	  }	  
	}
	g_hash_table_insert(cmaplist->cmap, &(cmap->code), cmap);
      }
    }

    /* ranges of codes */
    if(!strncmp(buf + i, "beginbfrange", 12)) {
      i += 12;
      if(i >= len - 10) {
	strncpy(buf, buf + i, len - i);
	len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	state->objectStream = ToUnicode;
	i = 0;
      }
      while(strncmp(buf + i, "endbfrange", 10)) {
	if(i >= len - 10) {
	  strncpy(buf, buf + i, len - i);
	  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	  state->objectStream = ToUnicode;
	  i = 0;
	}

	for( ; strncmp(buf + i, "<", 1); i++) {
	  if(i >= len - 10) {
	    strncpy(buf, buf + i, len - i);
	    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	    state->objectStream = ToUnicode;
	    i = 0;
	  }
	}
	i++;
	if(i >= len - 10) {
	  strncpy(buf, buf + i, len - i);
	  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	  state->objectStream = ToUnicode;
	  i = 0;
	}

	/* get first hex code and code length */
	code = 0;
	codelength = 0;
	while(strncmp(buf + i, ">", 1)) {
	  codelength++;
	  memset(tmp, '\x00', 20);
	  for(j = 0; j < 2 && strncmp(buf + i, ">", 1); j++) {
	    strncpy(tmp, buf + i, 1);
	    code *= 16;
	    if(!strncmp(tmp, "a", 1) || !strncmp(tmp, "A", 1)) {
	      code += 10;
	    } else if(!strncmp(tmp, "b", 1) || !strncmp(tmp, "B", 1)) {
	      code += 11;
	    } else if(!strncmp(tmp, "c", 1) || !strncmp(tmp, "C", 1)) {
	      code += 12;
	    } else if(!strncmp(tmp, "d", 1) || !strncmp(tmp, "D", 1)) {
	      code += 13;
	    } else if(!strncmp(tmp, "e", 1) || !strncmp(tmp, "E", 1)) {
	      code += 14;
	    } else if(!strncmp(tmp, "f", 1) || !strncmp(tmp, "F", 1)) {
	      code += 15;
	    } else if(!strncmp(tmp, ">", 1)) {
	      code += 0;
	    } else {
	      code += atoi(tmp);
	    }
	    if(strncmp(tmp, ">", 1)) {
	      i++;
	      if(i >= len - 10) {
		strncpy(buf, buf + i, len - i);
		len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
		state->objectStream = ToUnicode;
		i = 0;
	      }
	    }
	  }
	}

	for( ; strncmp(buf + i, "<", 1); i++) {
	  if(i >= len - 10) {
	    strncpy(buf, buf + i, len - i);
	    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	    state->objectStream = ToUnicode;
	    i = 0;
	  }
	}
	i++;

	/* get last code */
	lastcode = 0;
	while(strncmp(buf + i, ">", 1)) {
	  memset(tmp, '\x00', 20);
	  for(j = 0; j < 2; j++) {
	    strncpy(tmp, buf + i, 1);
	    lastcode *= 16;
	    if(!strncmp(tmp, "a", 1) || !strncmp(tmp, "A", 1)) {
	      lastcode += 10;
	    } else if(!strncmp(tmp, "b", 1) || !strncmp(tmp, "B", 1)) {
	      lastcode += 11;
	    } else if(!strncmp(tmp, "c", 1) || !strncmp(tmp, "C", 1)) {
	      lastcode += 12;
	    } else if(!strncmp(tmp, "d", 1) || !strncmp(tmp, "D", 1)) {
	      lastcode += 13;
	    } else if(!strncmp(tmp, "e", 1) || !strncmp(tmp, "E", 1)) {
	      lastcode += 14;
	    } else if(!strncmp(tmp, "f", 1) || !strncmp(tmp, "F", 1)) {
	      lastcode += 15;
	    } else if(!strncmp(tmp, ">", 1)) {
	      lastcode += 0;
	    } else {
	      lastcode += atoi(tmp);
	    }
	    if(strncmp(tmp, ">", 1)) {
	      i++;
	      if(i >= len - 10) {
		strncpy(buf, buf + i, len - i);
		len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
		state->objectStream = ToUnicode;
		i = 0;
	      }
	    }
	  }
	}

	/* get first value or array of values */
	for( ; strncmp(buf + i, "<", 1) && strncmp(buf + i, "[", 1); i++) {
	  if(i >= len - 10) {
	    strncpy(buf, buf + i, len - i);
	    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	    state->objectStream = ToUnicode;
	    i = 0;
	  }
	}
	if(!strncmp(buf + i, "<", 1)) {
	  /* it is the first value */
	  i++;
	  if(i >= len - 10) {
	    strncpy(buf, buf + i, len - i);
	    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	    state->objectStream = ToUnicode;
	    i = 0;
	  }

	  vallength = 0;
	  v = 0;
	  while(strncmp(buf + i, ">", 1)) {
	    memset(tmp, '\x00', 20);
	    vallength++;
	    for(j = 0; j < 2 && strncmp(buf + i, ">", 1); j++) {
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
		if(i >= len - 9) {
		  strncpy(buf, buf + i, len - i);
		  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
		  state->objectStream = ToUnicode;
		  i = 0;
		}
	      }
	    }
	  }

	  for( ; code <= lastcode; code++) {

	    /* create new entry in ToUnicodeCMap list */
	    cmap = (struct ToUnicodeCMap *) malloc(sizeof(struct ToUnicodeCMap));
	    cmap->codelength = codelength;
	    cmap->code = code;
	    cmap->vallength = vallength;

	    cmap->value = (UChar *) malloc(10);
	    err = U_ZERO_ERROR;
	    len2 = ucnv_toUChars(desc->conv, cmap->value, 10, (char *)&v, vallength, &err);
	    if (U_FAILURE(err)) {
	      fprintf(stderr, "Unable to convert buffer\n");
	      return ERR_ICU;
	    }
	    g_hash_table_insert(cmaplist->cmap, &(cmap->code), cmap);
	    v++;
	  }
	} else {

	  /* it is an array of values */
	  while(strncmp(buf + i, "]", 1)) {
	    i++;
	    if(i >= len - 10) {
	      strncpy(buf, buf + i, len - i);
	      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	      state->objectStream = ToUnicode;
	      i = 0;
	    }
	    while(strncmp(buf + i, "<", 1)) {
	      i++;
	      if(i >= len - 10) {
		strncpy(buf, buf + i, len - i);
		len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
		state->objectStream = ToUnicode;
		i = 0;
	      }
	    }
	    i++;
	    if(i >= len - 10) {
	      strncpy(buf, buf + i, len - i);
	      len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	      state->objectStream = ToUnicode;
	      i = 0;
	    }

	    vallength = 0;
	    v = 0;
	    while(strncmp(buf + i, ">", 1)) {
	      memset(tmp, '\x00', 20);
	      vallength++;
	      for(j = 0; j < 2 && strncmp(buf + i, ">", 1); j++) {
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
		  if(i >= len - 9) {
		    strncpy(buf, buf + i, len - i);
		    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
		    state->objectStream = ToUnicode;
		    i = 0;
		  }
		}
	      }
	    }
	    i++;
	    while(!strncmp(buf + i, " ", 1) ||
		  !strncmp(buf + i, "\x0A", 1) ||
		  !strncmp(buf + i, "\x0D", 1)) {
	      i++;
	      if(i >= len - 10) {
		strncpy(buf, buf + i, len - i);
		len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
		state->objectStream = ToUnicode;
		i = 0;
	      }	  
	    }
	    
	    /* create new entry in ToUnicodeCMap list */
	    cmap = (struct ToUnicodeCMap *) malloc(sizeof(struct ToUnicodeCMap));
	    cmap->codelength = codelength;
	    cmap->code = code;
	    code++;
	    cmap->vallength = vallength;
	    
	    cmap->value = (UChar *) malloc(10);
	    err = U_ZERO_ERROR;
	    len2 = ucnv_toUChars(desc->conv, cmap->value, 10, (char *)&v, vallength, &err);
	    if (U_FAILURE(err)) {
	      fprintf(stderr, "Unable to convert buffer\n");
	      return ERR_ICU;
	    }
	    g_hash_table_insert(cmaplist->cmap, &(cmap->code), cmap);
	  }
	}
	i++;
	if(i >= len - 10) {
	  strncpy(buf, buf + i, len - i);
	  len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	  state->objectStream = ToUnicode;
	  i = 0;
	}
	while(!strncmp(buf + i, " ", 1) ||
	      !strncmp(buf + i, "\x0A", 1) ||
	      !strncmp(buf + i, "\x0D", 1)) {
	  i++;
	  if(i >= len - 10) {
	    strncpy(buf, buf + i, len - i);
	    len = readObject(desc, buf + len - i, BUFSIZE - len + i) + len - i;
	    state->objectStream = ToUnicode;
	    i = 0;
	  }	  
	}
      }
    }
  }
  ucnv_close(desc->conv);
  err = U_ZERO_ERROR;
  desc->conv = ucnv_open("latin1", &err);
  if (U_FAILURE(err)) {
    fprintf(stderr, "unable to open ICU converter\n");
    return ERR_ICU;
  }

  return OK;
}
