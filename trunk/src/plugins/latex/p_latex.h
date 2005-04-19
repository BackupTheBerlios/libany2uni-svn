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
