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
 * internal funtions header
 */

#ifndef __INTERNALS_H__
#define __INTERNALS_H__

#include "misc.h"

/**
 * Determines the document format
 * according to its extension
 *
 * \param filename name of the document file
 * \return a code corresponding to the format
 */
int format_detection(char *filename);

/**
 * helper fonction used by format_detection
 *
 * \param filename name of the document file
 * \return the file extension as a string
 */
char *getextension(char *filename);

#endif /* __INTERNALS_H__ */
