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

#ifndef __P_ABI_H__
#define __P_ABI_H__

#include "../p_interface.h"

/**
 * parses the next paragraph, handles metadata
 *
 * \param desc the document descriptor
 * \param out the target buffer (MUST be initialized)
 * \return the size of out
 */
int parse(struct doc_descriptor* desc, char *out);


#endif /* __P_ABI_H__ */
