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

#ifndef __P_RTF_H__
#define __P_RTF_H__

#include "../p_interface.h"
#include <ctype.h>

#define BUFSIZE 2048


/**
 * metadata type : string, number or date
 */
enum metaType {
  string,
  number,
  date,
};


/**
 * parses the next paragraph
 *
 * \param desc the document descriptor
 * \param out the target buffer (MUST be initialized)
 * \param size the output buffer max size
 * \return the size of out
 */
int getText(struct doc_descriptor *desc, UChar *out, int size);


/**
 * to skip a bloc between braces
 *
 * \param desc the document descriptor
 * \return an error code
 */
int skipBloc(struct doc_descriptor *desc);


/**
 * to get a keyword and to apply it if needed
 *
 * \param desc the document descriptor
 * \param out output buffer
 * \param size output buffer size
 * \param pointer to end of buffer
 * \return an error code
 */
int doKeyword(struct doc_descriptor *desc, UChar *out, int size, int *l);


/**
 * to get current metadata
 *
 * \param desc the document descriptor
 * \param name name of the current metadata
 * \param type metadata type
 * \param param keyword parameter if any
 * \return an error code
 */
int doMeta(struct doc_descriptor *desc, char *name, enum metaType type, int param);


/**
 * the famous max function !
 *
 * \param a,b integers to compare
 * \return the maximum between a and b
 */
int max(int a, int b);


#endif /* __P_RTF_H__ */
