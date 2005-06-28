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

#ifndef __P_HTML_H__
#define __P_HTML_H__

#include "../p_interface.h"

/**
 * handles special characters like '&amp;'
 *
 * \param desc the document descriptor
 * \param buf the input token
 * \param res the target string (MUST be initialized)
 * \return the length of the input token
 */
int escapeChar(struct doc_descriptor *desc, char *buf, char *res);

/**
 * get the next 'paragraph'
 *
 * \param desc the document descriptor
 * \param buf the target buffer (MUST be initialized)
 * \param size maximum size of buf
 * \return the length of text read
 */
int getText(struct doc_descriptor *desc, char *buf, int size);

/**
 * get character encoding of file
 *
 * \param fd file handler
 * \param encoding the target string (MUST be initialized)
 */
int getEncoding(int fd, char *encoding);


#endif /* __P_HTML_H__ */
