/* callback functions for scribus sax parser */
/* see expat documentation for information */

#ifndef __SAXHANDLERS_SCRIBUS_H__
#define __SAXHANDLERS_SCRIBUS_H__


#include <expat.h>

#ifndef XMLCALL
#if defined(_MSC_EXTENSIONS) && !defined(__BEOS__) && !defined(__CYGWIN__)
#define XMLCALL __cdecl
#elif defined(__GNUC__)
#define XMLCALL __attribute__((cdecl))
#else
#define XMLCALL
#endif
#endif


void XMLCALL characters(void *user_data, const char *ch, int len);

void XMLCALL startElement(void *user_data, const char *name, const char **attrs);

void XMLCALL endElement(void *user_data, const char *name);

#endif /* __SAXHANDLERS__SCRIBUS_H__ */
