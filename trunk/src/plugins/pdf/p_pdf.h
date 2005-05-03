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

#ifndef __P_PDF_H__
#define __P_PDF_H__

#include "../p_interface.h"
#include <zlib.h>
#include <unicode/ustring.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#define BUFSIZE 2048



/**
 * to read the next paragraph
 *
 * \param desc the document descriptor
 * \param out the target buffer (MUST be initialized)
 * \param size the output buffer max size
 * \return the size of out
 */
int getText(struct doc_descriptor *desc, char *out, int size);


/**
 * get next paragraph from current stream
 *
 * \param desc the document descriptor
 * \param out the target buffer (MUST be initialized)
 * \param size the output buffer max size
 * \return the size of out
 */
int procedeStream(struct doc_descriptor *desc, char *out, int size);


/**
 * to initialize the reader by finding cross reference
 * table, catalog, and page tree.
 *
 * \param desc the document descriptor
 * \return an error code
 */
int initReader(struct doc_descriptor *desc);


/**
 * to position file cursor on the object
 * pointed by ref. (recursive)
 *
 * \param desc the document descriptor
 * \param xref a corss reference table offset (usually root xref)
 * \param ref the object's reference 
 * \return an error code
 */
int gotoRef(struct doc_descriptor *desc, size_t xref, int ref);


/**
 * to get the current token in input (like strtok)
 *
 * \param input the input buffer, begining with the token
 * \param output output string
 * \return length of output
 */
int getKeyword(char *input, char *output);


/**
 * to obtain reference to next stream
 *
 * \param desc the document descriptor
 * \return the next stream reference, -1 if end of document
 */
int getNextStream(struct doc_descriptor *desc);


/**
 * to obtain reference to next page
 *
 * \param desc the document descriptor
 * \return the next page reference, -1 if no more page
 */
int getNextPage(struct doc_descriptor *desc);


/**
 * to identify a filter string
 *
 * \param buf the filter string
 * \return the filter code
 */
enum filter identifyFilter(char *buf);


/**
 * to get the length of text in buf before
 * the next line.
 * 
 * \param buf the current buffer
 * \param size size of buffer
 * \return offset for next line
 */
int getNextLine(char *buf, int size);


/**
 * to get the integer value of the next string in buf
 *
 * \param buf buffer begining with a string number
 * \return the string number value
 */
int getNumber(char *buf);


/**
 * to get the value of a dictionary field
 * 
 * \param desc the document descriptor
 * \param buf buffer containing the dictionary
 * \param size input buffer size
 * \param name name of the field
 * \value output string for value
 * \return 0 for success, -1 if not found
 */
int getValue(struct doc_descriptor *desc, char *buf, int size, char *name, char *value);


/**
 * to update encoding in ICU converter, according to the current font
 *
 * \param desc the document descriptor
 * \param fontName name of the font as it appears in the stream
 * \return an error code
 */
int setEncoding(struct doc_descriptor *desc, char *fontName);


/**
 * to get encoding for new fonts (in current page
 * or current node in tree)
 *
 * \param desc the document descriptor
 * \return an error code
 */
int getEncodings(struct doc_descriptor *desc);


/**
 * to apply a filter to a stream and copy the result in desc->myState->stream.
 *
 * \param desc the document descriptor
 * \param filter the filter to apply
 * \param buf source buffer
 * \param buflen size of source buffer
 * \return an error code
 */
int applyFilter(struct doc_descriptor *desc, enum filter filter, char *buf, int buflen);


/**
 * to get the pdf version of file (1.0 to 1.6)
 *
 * \param fd file descriptor
 * \return the version number
 */
int version(int fd);


#endif /* __P_LATEX_H__ */
