
#include "userInterface.h"
#include <dlfcn.h>
#include <unistd.h>
#include <fcntl.h>
#include <unicode/ustring.h>
#include <sys/stat.h>
#include <sys/time.h>

/*
 * params : filename : name of the document to open
 *          desc     : the doc_descriptor where will be stored
 *                     informations about the document
 * return : OK (0) if success, an error code else
 *
 * the structure MUST be initialized before calling this fonction.
 */
int openDocument(char *filename, struct doc_descriptor *desc) {
  int format;
  void *handle = NULL;
  int (*initPlugin)(struct doc_descriptor *);
  struct stat st;

  /* filling some desc fields */
  desc->filename = filename;
  desc->nb_par_read = 0;
  desc->meta = NULL;

  desc->fd = open(filename, O_RDONLY);
  fstat(desc->fd, &st);
  desc->size = st.st_size;
  close(desc->fd);

  /* detecting and storing the file format */
  format = format_detection(filename);
  if (format == UNKNOWN) {
    return ERR_UNKNOWN_FORMAT;
  }
  desc->format = format;

  /* Load the right plugin */
  switch(desc->format) {

  case ABIWORD :
    handle = dlopen("/home/gwendal/libany2uni/trunk/src/plugins/abiword/p_abi.so", RTLD_LAZY);
    
    break;
  
  case SCRIBUS : 
    handle = dlopen("/home/gwendal/libany2uni/trunk/src/plugins/scribus/p_scribus.so", RTLD_LAZY);

    break;
  case XMLDOC :
    handle = dlopen("/home/gwendal/libany2uni/trunk/src/plugins/XML/p_xml.so", RTLD_LAZY);

    break;

  case KWORD :
  case KSPREAD :
  case KPRESENTER:
    handle = dlopen("/home/gwendal/libany2uni/trunk/src/plugins/koffice/p_koffice.so", RTLD_LAZY);

    break;

  case HTMLDOC :
    handle = dlopen("/home/gwendal/libany2uni/trunk/src/plugins/HTML/p_html.so", RTLD_LAZY);

    break;

  case OOWRITE:
  case OOCALC:
  case OOIMPRESS:
    handle = dlopen("/home/gwendal/libany2uni/trunk/src/plugins/openoffice/p_oo.so", RTLD_LAZY);

    break;

  case LATEX :
    handle = dlopen("/home/gwendal/libany2uni/trunk/src/plugins/latex/p_latex.so", RTLD_LAZY);

    break;

  default :
    break;
  }
  if(handle == NULL) {
    return ERR_DLOPEN;
  }
  /* initiaization */
  *(void **)(&initPlugin) = dlsym(handle, "initPlugin");
  if (*(void **)(&initPlugin) == NULL ) {
    return ERR_DLSYM;
  }
  (*initPlugin)(desc);
  
  desc->plugin_handle = handle;

  return OK;
}

/* params : desc : the doc_descriptor of the document to close
 * return : an error code
 */
int closeDocument(struct doc_descriptor *desc) {
  int (*closePlugin)(struct doc_descriptor *);

  /* closing the plugin */
  *(void **)(&closePlugin) = dlsym(desc->plugin_handle, "closePlugin");

  if (*(void **)(&closePlugin) == NULL ) {
    return ERR_DLSYM;
  }
  (*closePlugin)(desc);

  /* unloading the plugin */
  if (dlclose(desc->plugin_handle)) {
    return ERR_DLCLOSE;
  }

  /* free any useless things */

  return OK;
}

/* params : desc : the doc_descriptor of the document to read
 *          buf  : a buffer to receive the UTF-16 data
 * return : the length of text read
 */
int read_content(struct doc_descriptor *desc, UChar *buf) {
  int (*p_read_content)(struct doc_descriptor *, UChar *);

  /* using the plugin to read the text */
  *(void **)(&p_read_content) = dlsym(desc->plugin_handle, "p_read_content");

  if (*(void **)(&p_read_content) == NULL ) {
    return ERR_DLSYM;
  }
  return (*p_read_content)(desc, buf);
}

/* params : desc : the doc_descriptor of the document
 *          meta : the target structure
 * return : an error code
 */
int read_meta(struct doc_descriptor *desc, struct meta *meta) {
  int (*p_read_meta)(struct doc_descriptor *, struct meta *);

  /* using the plugin to read the metadata */
  *(void **)(&p_read_meta) = dlsym(desc->plugin_handle, "p_read_meta");

  if (*(void **)(&p_read_meta) == NULL ) {
    return ERR_DLSYM;
  }
  return (*p_read_meta)(desc, meta);
}

/* params : desc : the doc_descriptor of the document
 * return : an integer between 0 and 100
 *          (0 -> beginning of document,
 *          100 -> end of document).
 */
int getProgression(struct doc_descriptor *desc) {
  int (*p_getProgression)(struct doc_descriptor *);

  /* using the plugin */
  *(void **)(&p_getProgression) = dlsym(desc->plugin_handle, "p_getProgression");

  if (*(void **)(&p_getProgression) == NULL ) {
    return ERR_DLSYM;
  }
  return (*p_getProgression)(desc);
}

/* used for testing
 * during the initial developpement
 */
int main(int argc, char *argv[]) {
  struct doc_descriptor d, d2;
  struct meta meta;
  UChar buf[10000];
  FILE *fd;
  int e, f, i;
  struct timeval t1, t2;


  if (argc != 2) {
    printf("usage : ./test <doc_file>\n");
    exit(0);
  }

/*  gettimeofday(&t1, NULL);


 for (i = 0; i<100; i++) {*/

  if (openDocument(argv[1], &d)) {
    printf("error openDocument\n");
    exit(0);
  }

  if (openDocument("../../format_abiword/test2.abw", &d2)) {
    printf("error openDocument2\n");
    exit(0);
  }

  fd = fopen("output", "w");

  e = read_content(&d, buf);
  while(e >= 0) {
    fwrite(buf, e, 1, fd);
    fwrite("  ", 2, 1, fd);
    f = read_content(&d2, buf);
    e = read_content(&d, buf);
  }

  while (read_meta(&d, &meta) >=0) {
    fwrite(meta.name, 2*meta.name_length, 1, fd);
    fwrite(meta.value, 2*meta.value_length, 1, fd);
  }


  fclose(fd);

  if (closeDocument(&d2)) {
    printf("error closeDocument2\n");
  }

  if (closeDocument(&d)) {
    printf("error closeDocument\n");
  }

/*  }
  gettimeofday(&t2, NULL);

  printf("%d:%d\n", t1.tv_sec, t1.tv_usec);
  printf("%d:%d\n", t2.tv_sec, t2.tv_usec);
  printf("%d\n", (1000000 * (t2.tv_sec - t1.tv_sec) + t2.tv_usec - t1.tv_usec)/100);
*/
  return 0;
}
