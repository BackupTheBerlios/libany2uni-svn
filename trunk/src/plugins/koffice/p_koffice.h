#ifndef __P_KOFFICE_H__
#define __P_KOFFICE_H__

#include "../p_interface.h"

/**
 * fills metadata structures using the appropriate file in the archive
 *
 * \param desc the document descriptor
 * \return OK or an error code
 */
int getMeta(struct doc_descriptor* desc);

/**
 * parses the next paragraph, handles metadata
 *
 * \param desc the document descriptor
 * \param out the target buffer (MUST be initialized)
 * \return the size of out
 */
int parse(struct doc_descriptor* desc, char *out);


#endif /* __P_KOFFICE_H__ */
