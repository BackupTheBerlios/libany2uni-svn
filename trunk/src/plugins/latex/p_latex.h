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

#ifndef __P_LATEX_H__
#define __P_LATEX_H__

#include "../p_interface.h"

#define BUFSIZE 2048

/**
 * parses the next paragraph, handles metadata
 *
 * \param desc the document descriptor
 * \param out the target buffer (MUST be initialized)
 * \param size the output buffer max size
 * \return the size of out
 */
int getText(struct doc_descriptor *desc, char *out, int size);

/**
 * identify the next tag ( \tag)
 * 
 * \param buf buffer containing the tag (after backslash)
 *
 * \retrun the corresponding tag code
 */
enum latex_tag identifyTag(char *buf);

#endif /* __P_LATEX_H__ */
