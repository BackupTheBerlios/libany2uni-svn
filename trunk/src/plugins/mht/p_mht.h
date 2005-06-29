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

#ifndef __P_MHT_H__
#define __P_MHT_H__

#include "../p_interface.h"

/**
 * get the next 'paragraph'
 *
 * \param desc the document descriptor
 * \param buf the target buffer (MUST be initialized)
 * \param size size of target buffer
 *\ return the length of text read
 */
int getText(struct doc_descriptor *desc, UChar *buf, int size);


/**
 * to initialize the reader (read header)
 *
 * \param desc the document descriptor
 * \return an error code
 */
int initReader(struct doc_descriptor *desc);


/**
 * to place the cursor to
 * the next line.
 * 
 * \param desc the document descriptor
 * \return an error code
 */
int getNextLine(struct doc_descriptor *desc);


/**
 * to place the cursor at the beginning
 * of the next HTML part
 * 
 * \param desc the document descriptor
 * \return an error code
 */
int getNextHTMLpart(struct doc_descriptor *desc);


/**
 * handles special characters like '&amp;'
 *
 * \param buf the input token
 * \param res the target string (MUST be initialized)
 * \return the length of the input token
 */
int escapeChar(char *buf, UChar *res);


#endif /* __P_MHT_H__ */
