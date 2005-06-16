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
 * Functions that all plugins must implement
 */

#ifndef __P_INTERFACE__
#define __P_INTERFACE__

#include <unicode/utypes.h>
#include <unicode/ucnv.h>
#include "../misc.h"


/**
 * plugin initialisation
 *
 * \param desc the document descriptor
 * \return OK or an error code
 */
int initPlugin(struct doc_descriptor *desc);


/**
 * close the plugin
 *
 * \param desc the document descriptor
 * \return OK or an error code
 */
int closePlugin(struct doc_descriptor *desc);


/**
 * reads the next paragraph
 *
 * \param desc the document descriptor
 * \param buf target buffer (MUST be initialized)
 * \return the length of text read, NO_MORE_DATA if
 *         the end of document is reached, or an error code
 */
int p_read_content(struct doc_descriptor *desc, UChar *buf);


/**
 * reads the next metadata
 *
 * \note metadata aren't generally read all at once.
 *        Some new metadata may appear during the processing
 *        if the document. A call to this function returns
 *        NO_MORE_META if no more metadata has been met until the
 *        current time. It doesn't means that there is no more
 *        metadata in the document.
 *
 * \param desc the file descriptor
 * \param meta the target structure (MUST be initialized)
 * \return OK, NO_MORE_META or an error code
 */
int p_read_meta(struct doc_descriptor *desc, struct meta *meta);


/**
 * gets an indicator of the progression in the processing
 *
 * \param desc the file descriptor
 * \return an integer between 0 and 100
 *          (0 -> beginning of document,
 *          100 -> end of document).
 */
int p_getProgression(struct doc_descriptor *desc);


#endif /* __P_INTERFACE__ */
