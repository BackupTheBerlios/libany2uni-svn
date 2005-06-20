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

/* test program for multithreading with any2uni */

/* compile :
gcc -c threadtest.c
gcc -o threadtest threadtest.o -lany2uni -lpthread
*/

#include <stdio.h>
#include <libany2uni/userInterface.h>
#include <sys/types.h>
#include <unistd.h>


int main(int argc, char *argv[]) {
  struct doc_descriptor d;
  pid_t pid1, pid2;
  int e;
  UChar buf[10000];
  FILE *fd;

  if(argc != 4) {
    printf("usage : ./threadtest doc1 doc2 doc3\n");
    return -1;
  }

  pid1 = pid2 = 0;
  pid1 = fork();
  if(pid1 == -1) {
    fprintf(stderr, "error fork 1\n");
    exit(0);

  } else if(pid1) {
    pid2 = fork();
    if(pid2 == -1) {
      fprintf(stderr, "error fork 2\n");
      exit(0);

    } else if(pid2) {
      /* read doc 1 */
      if(openDocument(argv[1], &d)) {
	printf("error open doc1\n");
	exit(0);
      }
      fd = fopen("out1", "w");

      e = read_content(&d, buf);
      while(e >= 0) {
	fwrite(buf, e, 1, fd);
	fwrite("  ", 2, 1, fd);
	e = read_content(&d, buf);
      }

      fclose(fd);
      if(closeDocument(&d)) {
	printf("error close doc1\n");
      }

    } else {
      /* read doc 2 */
      if(openDocument(argv[2], &d)) {
	printf("error open doc2\n");
	exit(0);
      }
      fd = fopen("out2", "w");

      e = read_content(&d, buf);
      while(e >= 0) {
	fwrite(buf, e, 1, fd);
	fwrite("  ", 2, 1, fd);
	e = read_content(&d, buf);
      }

      fclose(fd);
      if(closeDocument(&d)) {
	printf("error close doc2\n");
      }
    }

  } else {
    /* read doc 3 */
    if(openDocument(argv[3], &d)) {
      printf("error open doc3\n");
      exit(0);
    }
    fd = fopen("out3", "w");

    e = read_content(&d, buf);
    while(e >= 0) {
      fwrite(buf, e, 1, fd);
      fwrite("  ", 2, 1, fd);
      e = read_content(&d, buf);
    }

    fclose(fd);
    if(closeDocument(&d)) {
      printf("error close doc3\n");
    }

  }


  return 0;
}
