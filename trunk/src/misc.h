/*
 * This file contains the structures and defines for the library
 */

#ifndef __MISC_H__
#define __MISC_H__

#include <stdio.h>
#include "plugins/koffice/unzip.h"
#include <sys/types.h>
#include <unicode/utypes.h>
#include <unicode/ucnv.h>
#include <expat.h>

#define INTERNAL_BUFSIZE 5000


/*
 * codes for file formats
 */
#define UNKNOWN    0
#define ABIWORD    1
#define SCRIBUS    2
#define KWORD      3
#define KSPREAD    4
#define KPRESENTER 5
#define OOWRITE    6
#define OOCALC     7
#define OOIMPRESS  8
#define XMLDOC     9
#define HTMLDOC   10
#define LATEX     11
#define RTFDOC    12
#define MSWORD    13
#define MSEXCEL   14
#define MSPPT     15
#define PDFDOC    16
#define WPDOC     17
#define QXPRESS   18

/**
 * sax parser state structure for expat
 */
struct ParserState {
  int isTextContent;       /* indicate if next characters are to be 
			      treated as textual content */
  int isNote;              /* indicate if next characters are to be 
			      treated as note */
  int isMeta;              /* indicate if next characyers are to be
			      treated as metadata */
  char *ch;                /* contains the actual paragraph */
  int chlen;               /* length of the paragraph */
  XML_Parser *pparser;     /* expat sax parser */
  int suspended;           /* indicate if the parser is suspended because
			      a paragraph has been totally parsed */
  int meta_suspended;      /* indicate if the parser is suspended because
			      metadata have been parsed */
  int buflen;              /* size of the input buffer of the parser */
  struct meta *meta;       /* metadata structure created when new 
			      metadata are being parsed */
  UConverter *cnv;         /* the ICU converter from the doc_descriptor */
  long begin_byte;         /* byte where the text begins */
  int size_adjusted;       /* to know if size fitting is needed */
};

/**
 * metadata structure (linked list)
 */
struct meta {
  UChar *name;        /* name of the data */
  size_t name_length; /* length of the name (number of characters) */
  UChar *value;       /* value of the data */
  size_t value_length;/* length of the value (number of characters) */
  struct meta *next;  /* next metadata, NULL if none */
};

/**
 * document files description structure
 */
struct doc_descriptor {
  char    *filename;           /* name of the file */
  int     fd;                  /* file descriptor */
  unzFile unzFile;             /* file descriptor for zipped formats */
  off_t   size;                /* fils size in bytes */
  int     format;              /* the file format
				  (see section File Formats of this file) */
  void    *plugin_handle;      /* the plugin used by the document */
  int     nb_par_read;         /* number of paragraphs already read */
  XML_Parser parser;           /* expat sax parser */
  struct ParserState myState;  /* parser state */
  struct meta *meta;           /* linked list of meatdata structures */
  UConverter *conv;            /* ICU converter */
};

/*
 * return codes
 */
#define NO_MORE_DATA -1
#define NO_MORE_META -1


/*
 * errors
 */
#define OK                  0
#define ERR_FOPEN          -1
#define ERR_FCLOSE         -2
#define ERR_UNKNOWN_FORMAT -3
#define ERR_DLOPEN         -4
#define ERR_DLCLOSE        -5
#define ERR_DLSYM          -6
#define ERR_STREAMFILE     -7
#define ERR_ICU            -8
#define ERR_READ_META      -9


#endif /* __MSC_H__ */
