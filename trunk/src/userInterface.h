#ifndef __USERINTERFACE_H__
#define __USERINTERFACE_H__

#include "misc.h"
#include "internals.h"
#include "unicode/utypes.h"

/**
 * Opens th document and fills the structure
 *
 * \param filename name of the document to open
 * \param desc     the doc_descriptor where will be stored
 *                 informations about the document
 * \return OK (0) if success, an error code else
 */
int openDocument(char *filename, struct doc_descriptor *desc);

/**
 * Close a document and
 * unloads the associated plugin
 *
 * \param desc  the doc_descriptor of the document to close
 * \return OK or an error code
 */
int closeDocument(struct doc_descriptor *desc);


/**
 * Read the next paragraph of the specified document
 *
 * \param desc  the doc_descriptor of the document to read
 * \param buf   a buffer to receive the UTF-16 data
 * \return the length of text read, NO_MORE_DATA if
 * the end of document is reached, or an error code
 */
int read_content(struct doc_descriptor *desc, UChar *buf);

/**
 * Reads the next metadata available and puts it in meta,
 *
 * \param desc the doc_descriptor of the document
 * \param meta the target structure
 * \return OK or an error code
 */
int read_meta(struct doc_descriptor *desc, struct meta *meta);

/**
 * get an indicator of the progression in the document
 *
 * \param desc the doc_descriptor of the document
 * \return an integer between 0 and 100
 *          (0 -> beginning of document,
 *          100 -> end of document).
 */
int getProgression(struct doc_descriptor *desc);






#endif /* __USERINTERFACE_H__ */
