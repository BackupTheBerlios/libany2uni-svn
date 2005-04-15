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
