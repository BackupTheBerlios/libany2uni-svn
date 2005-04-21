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

#ifndef __P_WORD_H__
#define __P_WORD_H__

#include "../p_interface.h"

#define BUFSIZE 2048

/* word version type */
enum wType {
  unknown,
  ole,
  oldword,
};

/**
 * parses the next paragraph
 *
 * \param desc the document descriptor
 * \param out the target buffer (MUST be initialized)
 * \param size the output buffer max size
 * \return the size of out
 */
int read_next(struct doc_descriptor *desc, char *out, int size);

/**
 * identifies the document version
 * 
 * \param fd the file descriptor
 *
 * \retrun the corresponding version code
 */
enum wType identify_version(FILE *fd);

/**
 * positions the file pointer to 
 * the begining of text content
 *
 * \param fd the file descriptor
 *
 * \return the text length, negative if error
 */
long int seek_textstart(struct doc_descriptor *desc);

#endif /* __P_WORD_H__ */
