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

#ifndef __P_EXCEL_H__
#define __P_EXCEL_H__

#include "../p_interface.h"

#define BUFSIZE 2048
#define BBSIZE  512
#define SBSIZE  64


/**
 * parses the next paragraph
 *
 * \param desc the document descriptor
 * \param out the target buffer (MUST be initialized)
 * \param size the output buffer max size
 * \return the size of out
 */
int read_next(struct doc_descriptor *desc, UChar *out, int size);


/**
 * to initialize the reader ( prepares reading
 * of an OLE file)
 *
 * \param desc the document descriptor
 * \return an error code
 */
int initOLE(struct doc_descriptor *desc);


/**
 * to get the next (Small or Big) Block of the current list
 *
 * \param desc the document descriptor
 * \return an error code
 */
int getNextBlock(struct doc_descriptor *desc);


/**
 * to read the current block of an OLE file.
 *
 * \param desc the document descriptor
 * \param out target buffer
 * \return the number of bytes read
 */
int readOLE(struct doc_descriptor *desc, char *out);


/**
 * to get the string pointed to by i in buf
 *
 * \param desc the document descriptor
 * \param target target utf-16 string
 * \param lastsstpartbegin position of the beginning of the last record for the SST
 * \param recordlen length of current record
 * \return an error code
 */
int getUnicodeString(struct doc_descriptor *desc,
		     UChar **target,
		     unsigned long *lastsstpartbegin,
		     int *recordlen);


/**
 * to fill the list of BBD structure
 */
int getBBD(struct doc_descriptor *desc);


/**
 * to free a BBD linked list
 */
int freeBBD(struct BBD *BBD);


#endif /* __P_EXCEL_H__ */
