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
