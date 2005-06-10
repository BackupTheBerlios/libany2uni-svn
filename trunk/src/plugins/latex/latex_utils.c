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
 * internal functions for LaTeX plugin
 */

#include "p_latex.h"
#include "latexTags.h"
#include <string.h>
#include <unicode/ustring.h>

/*
 * function used in getText to identify each encountered tag
 * (unknown tags are considered as text containers)
 */
enum latex_tag identifyTag(char *buf) {
  
  if (!strncmp(buf, "\x5c", 1)) {
    return end_par;
  } else if(!strncmp(buf, "verb+\x5c+", 7)) {
    return backslash;
  } else if(!strncmp(buf, "verb+~+", 7)) {
    return tilde;
  } else if(!strncmp(buf, "verb+^+", 7)) {
    return pow;
  } else if(!strncmp(buf, "{", 1)) {
    return leftbrace;
  } else if(!strncmp(buf, "}", 1)) {
    return rightbrace;
  } else if(!strncmp(buf, "%", 1)) {
    return percent;
  } else if(!strncmp(buf, "$", 1)) {
    return dollar;
  } else if(!strncmp(buf, "&", 1)) {
    return ampersand;
  } else if(!strncmp(buf, "#", 1)) {
    return sharp;
  } else if(!strncmp(buf, "_", 1)) {
    return underscore;
  } else if(!strncmp(buf, "oe ", 3)) {
    return oe;
  } else if(!strncmp(buf, "OE ", 3)) {
    return OE;
  } else if(!strncmp(buf, "'", 1)) {
    return cute;
  } else if(!strncmp(buf, "`", 1)) {
    return grave;
  } else if(!strncmp(buf, "\x22", 1)) {
    return umlaut;
  } else if(!strncmp(buf, "~", 1)) {
    return tilde_on;
  } else if(!strncmp(buf, "^", 1)) {
    return circonflex;
  } else if(!strncmp(buf, "aa ", 3)) {
    return aring;
  } else if(!strncmp(buf, "AA ", 3)) {
    return Aring;
  } else if(!strncmp(buf, "ae ", 3)) {
    return ae;
  } else if(!strncmp(buf, "AE ", 3)) {
    return AE;
  } else if(!strncmp(buf, "o ", 2)) {
    return oslash;
  } else if(!strncmp(buf, "O ", 2)) {
    return Oslash;
  } else if(!strncmp(buf, "c c", 3)) {
    return ccedil;
  } else if(!strncmp(buf, "c C", 3)) {
    return Ccedil;
  } else if(!strncmp(buf, "ss ", 3)) {
    return szlig;

    /* footnotes and captions */
  } else if(!strncmp(buf, "footnotemark", 12)) {
    return footnotemark;
  } else if(!strncmp(buf, "footnotetext", 12)) {
    return footnotetext;
  } else if(!strncmp(buf, "footnote", 8)) {
    return footnote;
  } else if(!strncmp(buf, "caption", 7)) {
    return caption;

    /* metadata */
  } else if(!strncmp(buf, "title", 5)) {
    return title;
  } else if(!strncmp(buf, "author", 6)) {
    return author;
  } else if(!strncmp(buf, "date", 4)) {
    return date;
  } else if(!strncmp(buf, "name", 4)) {
    return name;
  } else if(!strncmp(buf, "address", 7)) {
    return address;
  } else if(!strncmp(buf, "signature", 9)) {
    return signature;

    /* usepackage (useful for detecting encoding) */
  } else if(!strncmp(buf, "usepackage", 10)) {
    return usepackage;

    /* text containers */
  } else if(!strncmp(buf, "textbf", 6) ||
	    !strncmp(buf, "part", 4) ||
	    !strncmp(buf, "section", 7) ||
	    !strncmp(buf, "subsection", 10) ||
	    !strncmp(buf, "subsubsection", 13) ||
	    !strncmp(buf, "paragraph", 9) ||
	    !strncmp(buf, "subparagraph", 12) ||
	    !strncmp(buf, "chapter", 7) ||
	    !strncmp(buf, "flushleft", 9) ||
	    !strncmp(buf, "flushright", 10) ||
	    !strncmp(buf, "center", 6) ||
	    !strncmp(buf, "lettrine", 8) ||
	    !strncmp(buf, "mbox", 4) ||
	    !strncmp(buf, "quotation", 9) ||
	    !strncmp(buf, "verse", 5) ||
	    !strncmp(buf, "emph", 4) ||
	    !strncmp(buf, "texttt", 6) ||
	    !strncmp(buf, "textsl", 6) ||
	    !strncmp(buf, "textsc", 6) ||
	    !strncmp(buf, "textit", 6)) {
    return text_container;

    /* tags to replace by space */
  } else if(!strncmp(buf, ",", 1)) {
    return replace_by_space;

    /* end of paragraph */
  }else if(!strncmp(buf, "par", 3) && strncmp(buf, "parindent", 9)) {
    return end_par;
  } else if(!strncmp(buf, "end", 3)) {
    return end;

    /* tags to ignore with all their params */
  } else if(!strncmp(buf, "begin", 5) ||
	    !strncmp(buf, "selectlanguage", 14) ||
	    !strncmp(buf, "documentclass", 13) ||
	    !strncmp(buf, "newcommand", 10) ||
	    !strncmp(buf, "input", 5) ||
	    !strncmp(buf, "def", 3) ||
	    !strncmp(buf, "pagestyle", 9) ||
	    !strncmp(buf, "setmarginsrb",12) ||
	    !strncmp(buf, "thispagestyle", 13) ||
	    !strncmp(buf, "markright", 9) ||
	    !strncmp(buf, "renewcommand", 12) ||
	    !strncmp(buf, "setcounter", 10) ||
	    !strncmp(buf, "centerline", 10) ||
	    !strncmp(buf, "label", 5) ||
	    !strncmp(buf, "ref", 3) ||
	    !strncmp(buf, "arabic", 6) ||
	    !strncmp(buf, "roman", 5) ||
	    !strncmp(buf, "Roman", 5) ||
	    !strncmp(buf, "rule", 4) ||
	    !strncmp(buf, "alph", 4) ||
	    !strncmp(buf, "Alph", 4) ||
	    !strncmp(buf, "fnsymbol", 8) ||
	    !strncmp(buf, "cline", 5) ||
	    !strncmp(buf, "multicolumn", 11) ||
	    !strncmp(buf, "pageref", 7) ||
	    !strncmp(buf, "foreignlanguage", 15) ||
	    !strncmp(buf, "DeclareFontSubstitution", 23)) {
    return ignore_all;

    /* tags to ignore */
  } else if(!strncmp(buf, "item", 4) ||
	    !strncmp(buf, "tiny", 4) ||
	    !strncmp(buf, "scriptsize", 10) ||
	    !strncmp(buf, "footnotesize", 12) ||
	    !strncmp(buf, "small", 5) ||
	    !strncmp(buf, "normalize", 9) ||
	    !strncmp(buf, "large", 5) ||
	    !strncmp(buf, "Large", 5) ||
	    !strncmp(buf, "LARGE", 5) ||
	    !strncmp(buf, "huge", 4) ||
	    !strncmp(buf, "Huge", 4) ||
	    !strncmp(buf, "noindent", 8) ||
	    !strncmp(buf, "-", 1) ||
	    !strncmp(buf, "maketitle", 9) ||
	    !strncmp(buf, "tableofcontents", 15) ||
	    !strncmp(buf, "hline", 5) ||
	    !strncmp(buf, "newpage", 7) ||
	    !strncmp(buf, "protect", 7) ||
	    !strncmp(buf, "samepage", 8) ||
	    !strncmp(buf, "listoffigures", 13) ||
	    !strncmp(buf, "listoftables", 12) ||
	    !strncmp(buf, "(", 1) ||
	    !strncmp(buf, ")", 1) ||
	    !strncmp(buf, "linebreak", 9)) {
    return ignore;

    /* LaTeX logos */
  } else if(!strncmp(buf, "LaTeXe", 6)) {
    return latexe;
  } else if(!strncmp(buf, "LaTeX", 5)) {
    return latex;
  } else if(!strncmp(buf, "TeX", 3)) {
    return tex;

    /* default */
  } else {
    return text_container;
  }
}


/*
 * extracts text and metadata
 */
int getText(struct doc_descriptor *desc, char *buf, int size) {
  int len, i, j, l, fini, nodepth, depth, isNote, vlen;
  char inbuf[BUFSIZE];
  char val[50];
  enum latex_tag tag;
  struct meta *meta;
  UErrorCode err;

  len = read(desc->fd, inbuf, BUFSIZE);

  l = 0;
  isNote = 0;
  fini = 0;
  while ( !fini && len > 0) {
    for (i = 0; !fini && i < len ; i++) {
      
      if (!strncmp(inbuf + i, "\x25", 1)) {
	
	/* skipping comments */
	while (i < len && strncmp(inbuf + i, "\n", 1)) {
	  i++;
	  if (i == len) {
	    len = read(desc->fd, inbuf, BUFSIZE);
	    i = 0;
	  }
	}

	/* replacing some characters by spaces */
      } else if(!strncmp(inbuf + i, "\t", 1) ||
		!strncmp(inbuf + i, "\n", 1) ||
		!strncmp(inbuf + i, "&", 1) ||
		!strncmp(inbuf + i, "{", 1) ||
		!strncmp(inbuf + i, "~", 1)) {
	strncpy(buf + l, " ", 1);
	l++;
	if (l == size - 1) {
	  lseek(desc->fd, i - len + 1, SEEK_CUR);
	  fini = 1;
	}

      } else if(!strncmp(inbuf + i, "\x5c", 1)) {
	/* handling new tag */
	i++;
	if (i > len - 26) {
	  strncpy(inbuf, inbuf + i, len - i);
	  len = read(desc->fd, inbuf + len - i, BUFSIZE - len + i) + len -i;
	  i = 0;
	}
	tag = identifyTag(inbuf + i);

	switch (tag) {

	case ignore:
	case ignore_all:
	case usepackage:
	  depth = 0;
	  nodepth = 0;

	  /* finding end of parameters (if any) */
	  for(; i < len && (!nodepth || depth); i++) {
	    if(!depth && (!strncmp(inbuf + i, " ", 1) ||
			   !strncmp(inbuf + i, "\x5c", 1) ||
			   !strncmp(inbuf + i, "\n", 1) ||
			   !strncmp(inbuf + i, "\x25", 1) ||
			   !strncmp(inbuf + i, "\t", 1))) {
	      nodepth = 1;
	    }
	    if (!strncmp(inbuf + i + 1, "{", 1) || !strncmp(inbuf + i + 1, "[", 1)) {
	      depth++;
	      i++;
	      nodepth = 0;
	    } else if (!strncmp(inbuf + i + 1, "}", 1) || !strncmp(inbuf + i + 1, "]", 1)) {
	      depth--;
	    } 
	    if (!depth && !nodepth) {
	      if (!strncmp(inbuf + i + 1, "{", 1) || !strncmp(inbuf + i + 1, "[", 1)) {
		depth++;
		i++;
	      }
	    }
	  }
	  if (i == len) {
	    len = read(desc->fd, inbuf, BUFSIZE);
	    i = 0;
	  }
	  i--;
	  break;
	  
	case replace_by_space:
	  strncpy(buf + l, " ", 1);
	  l++;
	  if (l == size - 1) {
	    lseek(desc->fd, i - len + 1, SEEK_CUR);
	    fini = 1;
	  }

	  break;	  

	  /* finding begining of text zone */
	case text_container :
	  /* finding end of tag */
	  i++;
	  if (i == len) {
	    len = read(desc->fd, inbuf, BUFSIZE);
	    i = 0;
	  }
	  while(strncmp(inbuf + i, " ", 1) &&
		strncmp(inbuf + i, "\n", 1) &&
		strncmp(inbuf + i, "\t", 1) &&
		strncmp(inbuf + i, "\x25", 1) &&
		strncmp(inbuf + i, "|", 1) &&
		strncmp(inbuf + i, "[", 1) &&
		strncmp(inbuf + i, "{", 1)) {
	    i++;
	    if (i == len) {
	      len = read(desc->fd, inbuf, BUFSIZE);
	      i = 0;
	    }
	  }
	    
	  /* skipping parameters */
	  while (!strncmp(inbuf + i, "[", 1)){
	    while(strncmp(inbuf + i, "]", 1)) {
	      i++;
	      if (i == len) {
		len = read(desc->fd, inbuf, BUFSIZE);
		i = 0;
	      }
	    }
	  }
	  i--;
	  break;
	  
	case end:
	  depth = 0;
	  nodepth = 1;

	  /* finding end of parameters (if any) */
	  while(i < len && (nodepth || depth)) {
	    if(nodepth && (!strncmp(inbuf + i, " ", 1) ||
			   !strncmp(inbuf + i, "\x5c", 1) ||
			   !strncmp(inbuf + i, "\x25", 1) ||
			   !strncmp(inbuf + i, "\n", 1) ||
			   !strncmp(inbuf + i, "\t", 1))) {
	      nodepth = 0; 
	      i--;
	    }
	    if (!strncmp(inbuf + i, "{", 1) || !strncmp(inbuf + i, "[", 1)) {
	      depth++;
	      nodepth = 0;
	    } else if (!strncmp(inbuf + i, "}", 1) || !strncmp(inbuf + i, "]", 1)) {
	      depth--;
	    } 
	    if (!depth && !nodepth) {
	      if (!strncmp(inbuf + i + 1, "{", 1) || !strncmp(inbuf + i + 1, "[", 1)) {
		depth++;
	      }
	    }
	    i++;
	  }
	  i--;
	  if (i == len) {
	    len = read(desc->fd, inbuf, BUFSIZE);
	    i = 0;
	  }

	case end_par:
	  if (l > 0) {
	    lseek(desc->fd, i - len + 1, SEEK_CUR);
	    fini = 1;
	  }
	  break;

	  /* notes */
	case footnote:
	case footnotemark:
	case footnotetext:
	case caption:
	  if (l < size - 1) {
	    strncpy(buf + l, " (", 2);
	    l += 2;
	    while(strncmp(inbuf + i, "{", 1)) {
	      i++;
	      if (i == len) {
		len = read(desc->fd, inbuf, BUFSIZE);
		i = 0;
	      }
	      isNote = 1;
	    }
	  }
	  
	  break;

	  /* special characters */
	case ccedil:
	  strncpy(buf + l, "\xe7", 1);
	  l++;
	  break;

	case Ccedil:
	  strncpy(buf + l, "\xc7", 1);
	  l++;
	  break;

	case backslash:
	  strncpy(buf + l, "\x5c", 1);
	  l++;
	  break;

	case tilde:
	  strncpy(buf + l, "~", 1);
	  l++;
	  break;

	case leftbrace:
	  strncpy(buf + l, "{", 1);
	  l++;
	  break;

	case rightbrace:
	  strncpy(buf + l, "}", 1);
	  l++;
	  break;

	case percent:
	  strncpy(buf + l, "%", 1);
	  l++;
	  break;

	case dollar:
	  strncpy(buf + l, "$", 1);
	  l++;
	  break;

	case ampersand:
	  strncpy(buf + l, "&", 1);
	  l++;
	  break;

	case sharp:
	  strncpy(buf + l, "#", 1);
	  l++;
	  break;

	case pow:
	  strncpy(buf + l, "^", 1);
	  l++;
	  break;

	case underscore:
	  strncpy(buf + l, "_", 1);
	  l++;
	  break;

	case oe:
	  strncpy(buf + l, "\xbd", 1);
	  l++;
	  break;

	case OE:
	  strncpy(buf + l, "\xbc", 1);
	  l++;
	  break;

	case cute:
	  i++;
	  if (i == len) {
	    len = read(desc->fd, inbuf, BUFSIZE);
	    i = 0;
	  }
	  if (!strncmp(inbuf + i, "e", 1)) {
	    strncpy(buf + l, "é", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "E", 1)) {
	    strncpy(buf + l, "\xc9", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "a", 1)) {
	    strncpy(buf + l, "\xe1", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "A", 1)) {
	    strncpy(buf + l, "\xc1", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "i", 1)) {
	    strncpy(buf + l, "\xed", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "I", 1)) {
	    strncpy(buf + l, "\xcd", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "o", 1)) {
	    strncpy(buf + l, "\xf3", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "O", 1)) {
	    strncpy(buf + l, "\xd3", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "u", 1)) {
	    strncpy(buf + l, "\xfa", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "U", 1)) {
	    strncpy(buf + l, "\xda", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "y", 1)) {
	    strncpy(buf + l, "\xfd", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "Y", 1)) {
	    strncpy(buf + l, "\xdd", 1);
	    l++;
	  }
	  break;

	case grave:
	  i++;
	  if (i == len) {
	    len = read(desc->fd, inbuf, BUFSIZE);
	    i = 0;
	  }
	  if (!strncmp(inbuf + i, "e", 1)) {
	    strncpy(buf + l, "è", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "E", 1)) {
	    strncpy(buf + l, "\xc8", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "a", 1)) {
	    strncpy(buf + l, "à", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "A", 1)) {
	    strncpy(buf + l, "\xc0", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "i", 1)) {
	    strncpy(buf + l, "\xec", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "I", 1)) {
	    strncpy(buf + l, "\xcc", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "o", 1)) {
	    strncpy(buf + l, "\xf2", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "O", 1)) {
	    strncpy(buf + l, "\xd2", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "u", 1)) {
	    strncpy(buf + l, "ù", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "U", 1)) {
	    strncpy(buf + l, "\xd9", 1);
	    l++;
	  }
	  break;

	case circonflex:
	  i++;
	  if (i == len) {
	    len = read(desc->fd, inbuf, BUFSIZE);
	    i = 0;
	  }
	  if (!strncmp(inbuf + i, "e", 1)) {
	    strncpy(buf + l, "ê", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "E", 1)) {
	    strncpy(buf + l, "Ê", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "a", 1)) {
	    strncpy(buf + l, "â", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "A", 1)) {
	    strncpy(buf + l, "Â", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "i", 1)) {
	    strncpy(buf + l, "î", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "I", 1)) {
	    strncpy(buf + l, "Î", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "o", 1)) {
	    strncpy(buf + l, "ô", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "O", 1)) {
	    strncpy(buf + l, "Ô", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "u", 1)) {
	    strncpy(buf + l, "û", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "U", 1)) {
	    strncpy(buf + l, "Û", 1);
	    l++;
	  }
	  break;

	case umlaut:
	  i++;
	  if (i == len) {
	    len = read(desc->fd, inbuf, BUFSIZE);
	    i = 0;
	  }
	  if (!strncmp(inbuf + i, "e", 1)) {
	    strncpy(buf + l, "ë", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "E", 1)) {
	    strncpy(buf + l, "Ë", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "a", 1)) {
	    strncpy(buf + l, "ä", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "A", 1)) {
	    strncpy(buf + l, "Ä", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "i", 1)) {
	    strncpy(buf + l, "ï", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "I", 1)) {
	    strncpy(buf + l, "Ï", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "o", 1)) {
	    strncpy(buf + l, "ö", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "O", 1)) {
	    strncpy(buf + l, "Ö", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "u", 1)) {
	    strncpy(buf + l, "ü", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "U", 1)) {
	    strncpy(buf + l, "Ü", 1);
	    l++;
	  }
	  break;

	case tilde_on:
	  i++;
	  if (i == len) {
	    len = read(desc->fd, inbuf, BUFSIZE);
	    i = 0;
	  }
	  if (!strncmp(inbuf + i, "a", 1)) {
	    strncpy(buf + l, "\xe3", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "A", 1)) {
	    strncpy(buf + l, "\xc3", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "o", 1)) {
	    strncpy(buf + l, "\xf5", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "O", 1)) {
	    strncpy(buf + l, "\xd5", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "n", 1)) {
	    strncpy(buf + l, "\xf1", 1);
	    l++;
	  } else if (!strncmp(inbuf + i, "N", 1)) {
	    strncpy(buf + l, "\xd1", 1);
	    l++;
	  }
	  break;

	case aring:
	  strncpy(buf + l, "\xe5", 1);
	  l++;
	  break;

	case Aring:
	  strncpy(buf + l, "\xc5", 1);
	  l++;
	  break;

	case ae:
	  strncpy(buf + l, "\xe6", 1);
	  l++;
	  break;

	case AE:
	  strncpy(buf + l, "\xc6", 1);
	  l++;
	  break;

	case oslash:
	  strncpy(buf + l, "\xf8", 1);
	  l++;
	  break;

	case Oslash:
	  strncpy(buf + l, "\xd8", 1);
	  l++;
	  break;

	case szlig:
	  strncpy(buf + l, "\xdf", 1);
	  l++;
	  break;

	  /* metadata */
	case title:
	  err = U_ZERO_ERROR;
	  /* finding last metadata of desc */
	  if (desc->meta == NULL) {
	    meta = (struct meta *) malloc(sizeof(struct meta));
	    desc->meta = meta;
	  } else {
	    meta = desc->meta;
	    while (meta->next != NULL) {
	      meta = desc->meta;
	    }
	    meta->next = (struct meta *) malloc(sizeof(struct meta));
	    meta = meta->next;
	  }
	  meta->next = NULL;
	  meta->name = (UChar *) malloc(12);

	  /* filling name field */
	  meta->name_length = 2 * ucnv_toUChars(desc->conv, meta->name ,
					    12, "title", 5, &err);
	  meta->name_length = u_strlen(meta->name);
	  if (U_FAILURE(err)) {
	    printf("error icu\n");
	  }

	  /* finding value */
	  while(strncmp(inbuf + i, "{", 1)) {
	    i++;
	    if (i == len) {
	      len = read(desc->fd, inbuf, BUFSIZE);
	      i = 0;
	    }
	  }
	  j = ++i;
	  while(j < len && j - i < 50 && strncmp(inbuf + j, "}", 1)) {
	    j++;
	  }
	  vlen = 0;
	  while (i<j) {
	    strncpy(val + vlen, inbuf + i, 1);
	    i++;
	    vlen++;
	  }
	  strncpy(val + vlen, "\0", 1);

	  /* converting to utf-16 and filling value field */
	  err = U_ZERO_ERROR;
	  meta->value = (UChar *) malloc(2*strlen(val) + 2);
	  meta->value_length = 2 * ucnv_toUChars(desc->conv,
						 meta->value ,2*strlen(val) + 2,
						 val, strlen(val), &err);
	  meta->value_length = u_strlen(meta->value);

	  if (U_FAILURE(err)) {
	    printf("error icu\n");
	  }

	  break;

	case author:
	  err = U_ZERO_ERROR;
	  /* finding last metadata of desc */
	  if (desc->meta == NULL) {
	    meta = (struct meta *) malloc(sizeof(struct meta));
	    desc->meta = meta;
	  } else {
	    meta = desc->meta;
	    while (meta->next != NULL) {
	      meta = desc->meta;
	    }
	    meta = meta->next;
	  }
	  meta->next = NULL;
	  meta->name = (UChar *) malloc(14);

	  /* filling name field */
	  meta->name_length = 2 * ucnv_toUChars(desc->conv, meta->name ,
					    14, "author", 6, &err);
	  meta->name_length = u_strlen(meta->name);
	  if (U_FAILURE(err)) {
	    printf("error icu\n");
	  }

	  /* finding value */
	  while(strncmp(inbuf + i, "{", 1)) {
	    i++;
	    if (i == len) {
	      len = read(desc->fd, inbuf, BUFSIZE);
	      i = 0;
	    }
	  }
	  j = ++i;
	  while(j < len && j - i < 50 && strncmp(inbuf + j, "}", 1)) {
	    j++;
	  }
	  vlen = 0;
	  while (i<j) {
	    if(!strncmp(inbuf + i, "~", 1)) {
	      strncpy(val + vlen, " ", 1);
	    } else {
	      strncpy(val + vlen, inbuf + i, 1);
	    }
	    i++;
	    vlen++;
	  }
	  strncpy(val + vlen, "\0", 1);

	  /* converting to utf-16 and filling value field */
	  err = U_ZERO_ERROR;
	  meta->value = (UChar *) malloc(2*strlen(val) + 2);
	  meta->value_length = 2 * ucnv_toUChars(desc->conv,
						 meta->value ,2*strlen(val) + 2,
						 val, strlen(val), &err);
	  meta->value_length = u_strlen(meta->value);

	  if (U_FAILURE(err)) {
	    printf("error icu\n");
	  }

	  break;

	case date:
	  if (strncmp(inbuf + i + 4, "\x5c", 1)) {
	    err = U_ZERO_ERROR;
	    /* finding last metadata of desc */
	    if (desc->meta == NULL) {
	      meta = (struct meta *) malloc(sizeof(struct meta));
	      desc->meta = meta;
	    } else {
	      meta = desc->meta;
	      while (meta->next != NULL) {
		meta = desc->meta;
	      }
	      meta = meta->next;
	    }
	    meta->next = NULL;
	    meta->name = (UChar *) malloc(10);
	    
	    /* filling name field */
	    meta->name_length = 2 * ucnv_toUChars(desc->conv, meta->name ,
						  10, "date", 4, &err);
	    meta->name_length = u_strlen(meta->name);
	    if (U_FAILURE(err)) {
	      printf("error icu\n");
	    }
	    
	    /* finding value */
	    while(strncmp(inbuf + i, "{", 1)) {
	      i++;
	      if (i == len) {
		len = read(desc->fd, inbuf, BUFSIZE);
		i = 0;
	      }
	    }
	    j = ++i;
	    while(j < len && j - i < 50 && strncmp(inbuf + j, "}", 1)) {
	      j++;
	    }
	    vlen = 0;
	    while (i<j) {
	      strncpy(val + vlen, inbuf + i, 1);
	      i++;
	      vlen++;
	    }
	    strncpy(val + vlen, "\0", 1);
	    
	    /* converting to utf-16 and filling value field */
	    err = U_ZERO_ERROR;
	    meta->value = (UChar *) malloc(2*strlen(val) + 2);
	    meta->value_length = 2 * ucnv_toUChars(desc->conv,
						   meta->value ,2*strlen(val) + 2,
						   val, strlen(val), &err);
	    meta->value_length = u_strlen(meta->value);
	    if (U_FAILURE(err)) {
	      printf("error icu\n");
	    }
	    
	  } else while (strncmp(inbuf + i, " ", 1) &&
			strncmp(inbuf + i, "\t", 1) &&
			strncmp(inbuf + i, "\n", 1)) {
	    i++;
	  }
	  
	  break;

	case latex:
	  if (l<INTERNAL_BUFSIZE - 5) {
	    strncpy(buf + l, "LaTeX", 5);
	    l += 5;
	  } else {
	    strncpy(buf + l, "LaTeX", INTERNAL_BUFSIZE - l);
	    l += INTERNAL_BUFSIZE - l;
	  }
	  i += 5;
	  break;

	case latexe:
	  if (l<INTERNAL_BUFSIZE - 7) {
	    strncpy(buf + l, "LaTeX2e", 7);
	    l += 7;
	  } else {
	    strncpy(buf + l, "LaTeX2e", INTERNAL_BUFSIZE - l);
	    l += INTERNAL_BUFSIZE - l;
	  }
	  i += 6;
	  break;

	case tex:
	  if (l<INTERNAL_BUFSIZE - 3) {
	    strncpy(buf + l, "TeX", 3);
	    l += 3;
	  } else {
	    strncpy(buf + l, "TeX", INTERNAL_BUFSIZE - l);
	    l += INTERNAL_BUFSIZE - l;
	  }
	  i += 3;
	  break;

	default :
	  break;
	}
	
      } else if(!strncmp(inbuf + i, "}", 1)) {
     	/* skipping end of text zone character */
	if(isNote) {
	    isNote = 0;
	  if(l < size - 1) {
	    strncpy(buf + l, ") ", 2);
	    l += 2;
	  }
	} else {
	  strncpy(buf + l, " ", 1);
	}

      } else {
	/* filling output buffer */
	strncpy(buf + l, inbuf + i, 1);
	l++;
	if (l == size - 1) {
	  lseek(desc->fd, i - len + 1, SEEK_CUR);
	  fini = 1;
	}

      }
    }
    if (!fini) {
      len = read(desc->fd, inbuf, BUFSIZE);
      fini = (len == 0);
    }
  }
  if (l > 0) {
    strncpy(buf + l, "\0", 1);
    l++;
  } else {
    return NO_MORE_DATA;
  }
  return l;
}
