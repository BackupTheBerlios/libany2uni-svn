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

/* test program for any2uni : read all files of a repertory */

/* compile :
gcc -c -g -Wall largetest.c
gcc -o largetest -g -Wall largetest.o -lany2uni
*/

#include <stdio.h>
#include <libany2uni/userInterface.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>


int main(int argc, char *argv[]) {
  struct doc_descriptor d;
  DIR *rep;
  struct dirent *entry;
  char completeName[512];
  UChar buf[10000];
  int e;

  if(argc != 2) {
    printf("Usage : ./largetest <directory>\n");
    return 0;
  }

  /* open directory */
  rep = opendir(argv[1]);
  if(rep == NULL) {
    fprintf(stderr, "Unable to open %s\n", argv[1]);
    return 0;
  }

  for(entry = readdir(rep); entry != NULL; entry = readdir(rep)) {
    if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
      continue;
    }
    memset(completeName, '\x00', 512);
    sprintf(completeName, "%s%s", argv[1], entry->d_name);
    printf("%s\n", completeName);

    if(openDocument(completeName, &d) == 0) {
      e = read_content(&d, buf);
      while(e >= 0) {
	e = read_content(&d, buf);
      }

      if(closeDocument(&d)) {
	printf("error close : %s\n", completeName);
      }
    }
  }
  closedir(rep);


  return 0;
}
