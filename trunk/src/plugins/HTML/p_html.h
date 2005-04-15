#ifndef __P_HTML_H__
#define __P_HTML_H__

#include "../p_interface.h"

/**
 * handles special characters like '&amp;'
 *
 * \param buf the input token
 * \param res the target string (MUST be initialized)
 * \return the length of the input token
 */
int escapeChar(char *buf, char *res);

/**
 * get the next 'paragraph'
 *
 * \param desc the document descriptor
 * \param buf the target buffer (MUST be initialized)
 */
int getText(struct doc_descriptor *desc, char *buf);

/**
 * get character encoding of file
 *
 * \param fd file handler
 * \param encoding the target string (MUST be initialized)
 */
int getEncoding(int fd, char *encoding);


#endif /* __P_HTML_H__ */
