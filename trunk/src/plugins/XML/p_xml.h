#ifndef __P_XML_H__
#define __P_XML_H__

#include "../p_interface.h"

/**
 * parses the next paragraph, handles metadata
 *
 * \param desc the document descriptor
 * \param out the target buffer (MUST be initialized)
 * \return the size of out
 */
int parse(struct doc_descriptor* desc, char *out);


#endif /* __P_XML_H__ */
