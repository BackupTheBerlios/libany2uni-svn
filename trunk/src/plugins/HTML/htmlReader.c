/*
  This file is part of the libany2uni project, an universal
  text extractor in unicode utf-16
  Copyright (C) 2005  Gwendal Dufresne, modified by Romuald Texier
  $Id$

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

#include "p_html.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>

#define BUFSIZE 2048


int
escapeChar (struct doc_descriptor *desc, char *buf, UChar * res)
{
  char token[9];
  int i;

  /* copying token into local buffer */
  i = 0;
  while (i <= 8 && strncmp (buf + i, ";", 1))
    {
      strncpy (token + i, buf + i, 1);
      i++;
    }
  if (strncmp (buf + i, ";\0", 2))
    {
      strncpy (token + i, buf + i, 1);
    }
  else
    {

      /* if it does not seem to be a token, result is '&' */
      *res = 0x26;
      return 1;
    }

  /* identifying token */
  if (!strncmp (token, "&amp;", 5))
    {
      *res = 0x26;
      return 5;
    }
  else if (!strncmp (token, "&lt;", 4))
    {
      *res = 0x3C;
      return 4;
    }
  else if (!strncmp (token, "&gt;", 4))
    {
      *res = 0x3E;
      return 4;
    }
  else if (!strncmp (token, "&quot;", 6))
    {
      *res = 0x22;
      return 6;
    }
  else if (!strncmp (token, "&eacute;", 8))
    {
      *res = 0xE9;
      return 8;
    }
  else if (!strncmp (token, "&Eacute;", 8))
    {
      *res = 0xC9;
      return 8;
    }
  else if (!strncmp (token, "&egrave;", 8))
    {
      *res = 0xE8;
      return 8;
    }
  else if (!strncmp (token, "&Egrave;", 8))
    {
      *res = 0xC8;
      return 8;
    }
  else if (!strncmp (token, "&ecirc;", 7))
    {
      *res = 0xEA;
      return 7;
    }
  else if (!strncmp (token, "&agrave;", 8))
    {
      *res = 0xE0;
      return 8;
    }
  else if (!strncmp (token, "&iuml;", 6))
    {
      *res = 0xEF;
      return 6;
    }
  else if (!strncmp (token, "&ccedil;", 8))
    {
      *res = 0xE7;
      return 8;
    }
  else if (!strncmp (token, "&ntilde;", 8))
    {
      *res = 0xF1;
      return 8;
    }
  else if (!strncmp (token, "&copy;", 6))
    {
      *res = 0xA9;
      return 6;
    }
  else if (!strncmp (token, "&reg;", 5))
    {
      *res = 0xAE;
      return 5;
    }
  else if (!strncmp (token, "&deg;", 5))
    {
      *res = 0xB0;
      return 5;
    }
  else if (!strncmp (token, "&ordm;", 6))
    {
      *res = 0xBA;
      return 6;
    }
  else if (!strncmp (token, "&laquo;", 7))
    {
      *res = 0xAB;
      return 7;
    }
  else if (!strncmp (token, "&raquo;", 7))
    {
      *res = 0xBB;
      return 7;
    }
  else if (!strncmp (token, "&micro;", 7))
    {
      *res = 0xB5;
      return 7;
    }
  else if (!strncmp (token, "&para;", 6))
    {
      *res = 0xB6;
      return 6;
    }
  else if (!strncmp (token, "&frac14;", 8))
    {
      *res = 0xBC;
      return 8;
    }
  else if (!strncmp (token, "&frac12;", 8))
    {
      *res = 0xBD;
      return 8;
    }
  else if (!strncmp (token, "&frac34;", 8))
    {
      *res = 0xBE;
      return 8;
    }
  else if (!strncmp (token, "&#", 2))
    {
      res[0] = atoi (token + 2);
      return i + 1;
    }
  else
    {
      *res = 0x20;
      return i + 1;
    }
}

int
getText (struct doc_descriptor *desc, UChar * buf, int size)
{
  struct meta *meta = NULL;
  char buf2[BUFSIZE];
  UErrorCode err;
  char *src;
  UChar *dest, esc[3];
  UChar name[1024], value[1024];
  int len, i, isMarkup, isJavascript, isMeta, l, j;
  int dangerousCut, fini, r, offset, endOfFile, space_added;

  space_added = 0;
  l = 0;
  fini = 0;
  endOfFile = 0;
  isJavascript = 0;
  dangerousCut = 0;
  isMarkup = 0;
  isMeta = 0;
  len = read (desc->fd, buf2, BUFSIZE);
  while (!fini && len > 0 && 2*l < size - 2)
    {

      /* consuming buffer */
      for (i = 0; 2*l < size - 2 && i < len && !dangerousCut && !fini; i++)
        {

          /* end of buffer are possible points of failure
             if a markup or a token is cut, it will not be
             parsed. */
          if (!endOfFile && i > len - 9 && (!strncmp (buf2 + i, "\x3c", 1) ||
                                            !strncmp (buf2 + i, "\x26", 1)))
            {
              dangerousCut = 1;
              break;
            }

          /* detecting end of javascript */
          if (isJavascript
              && !strncasecmp (buf2 + i, "</script>", 9))
            {
              isJavascript = 0;
              i += 9;
            }

          /* detecting new paragraph */
          if (l > 0 && !isJavascript
              && (!strncasecmp (buf2 + i, "<p", 2)
                  || !strncasecmp (buf2 + i, "<br", 3)
                  || !strncasecmp (buf2 + i, "<div", 4)))
            {
              fini = 1;
              i += 2;
              while (strncmp (buf2 + i, ">", 1))
                {
                  i++;
                }
              lseek (desc->fd, i - len, SEEK_CUR);
              break;
            }

          /* detecting begining of markup */
          if (!isJavascript && !isMarkup && !strncmp (buf2 + i, "\x3c", 1))
            {

              /* detecting begining of javascript */
              if (!strncasecmp (buf2 + i, "<script", 7))
                {
                  isJavascript = 1;

                }
              else if (!strncasecmp (buf2 + i, "<title", 6))
                {
                  err = U_ZERO_ERROR;
                  /* finding last metadata of desc */
                  if (desc->meta == NULL)
                    {
                      meta = (struct meta *) malloc (sizeof (struct meta));
                      desc->meta = meta;
                    }
                  else
                    {
                      meta = desc->meta;
                      while (meta->next != NULL)
                        {
                          meta = meta->next;
                        }
                      meta->next =
                        (struct meta *) malloc (sizeof (struct meta));
                      meta = meta->next;
                    }
                  meta->next = NULL;
                  meta->name = (UChar *) malloc (12);

                  /* filling name field */
                  meta->name_length =
                    2 * ucnv_toUChars (desc->conv, meta->name, 12, "title", 5,
                                       &err);
                  meta->name_length = u_strlen (meta->name);
                  if (U_FAILURE (err))
                    {
                      printf ("error icu\n");
                      return ERR_ICU;
                    }
                  isMeta = 1;

                }
              else if (!strncasecmp (buf2 + i, "<meta", 5))
                {
                  i += 5;
                  if (i >= size - 9)
                    {
                      strncpy (buf2, buf2 + i, len - i);
                      len =
                        read (desc->fd, buf2 + i,
                              BUFSIZE - len + i) + len - i;
                      i = 0;
                    }
                  for (; strncasecmp (buf2 + i, "name=\"", 6)  &&
                       strncmp (buf2 + i, "\x3E", 1); i++)
                    {
                      if (i >= size - 9)
                        {
                          strncpy (buf2, buf2 + i, len - i);
                          len =
                            read (desc->fd, buf2 + i,
                                  BUFSIZE - len + i) + len - i;
                          i = 0;
                        }
                    }
                  if (!strncmp (buf2 + i, "\x3E", 1))
                    {
                      continue;

                    }
                  else
                    {
                      i += 6;
                      /* get metadata name */
                      memset (name, '\x00', 2048);
                      for (j = 0; len != 0 && strncmp (buf2 + i, "\"", 1);
                           i++)
                        {
                          if (i >= size - 9)
                            {
                              strncpy (buf2, buf2 + i, len - i);
                              len =
                                read (desc->fd, buf2 + i,
                                      BUFSIZE - len + i) + len - i;
                              i = 0;
                            }
                          if (!strncmp (buf2 + i, "\x26", 1))
                            {
                              memset (esc, '\x00', 6);
                              offset = escapeChar (desc, buf2 + i, esc);
                              memcpy (name + j, esc, 2 * u_strlen (esc));
                              j += u_strlen (esc);
                              i += (offset - 1);
                            }
                          else
                            {

                              /* filling name buffer */
                              dest = name + j;
                              src = buf2 + i;
                              err = U_ZERO_ERROR;
                              ucnv_toUnicode (desc->conv, &dest, name + 1024,
                                              &src, buf2 + i + 1, NULL, FALSE,
                                              &err);
                              if (U_FAILURE (err))
                                {
                                  fprintf (stderr,
                                           "Unable to convert buffer\n");
                                  return ERR_ICU;
                                }
                              j += (dest - name - j);
                            }
                        }

                      /* get metadata value */
                      for (; strncasecmp (buf2 + i, "content=\"", 9) && strncmp (buf2 + i, "\x3E", 1); i++)
                        {
                          if (i >= size - 9)
                            {
                              strncpy (buf2, buf2 + i, len - i);
                              len =
                                read (desc->fd, buf2 + i,
                                      BUFSIZE - len + i) + len - i;
                              i = 0;
                            }
                        }
                      i += 9;
                      if (i >= size - 9)
                        {
                          strncpy (buf2, buf2 + i, len - i);
                          len =
                            read (desc->fd, buf2 + i,
                                  BUFSIZE - len + i) + len - i;
                          i = 0;
                        }
                      memset (value, '\x00', 2048);
                      for (j = 0; len != 0 && strncmp (buf2 + i, "\"", 1);
                           i++)
                        {
                          if (i >= size - 9)
                            {
                              strncpy (buf2, buf2 + i, len - i);
                              len =
                                read (desc->fd, buf2 + i,
                                      BUFSIZE - len + i) + len - i;
                              i = 0;
                            }
                          if (!strncmp (buf2 + i, "\x26", 1))
                            {
                              memset (esc, '\x00', 6);
                              offset = escapeChar (desc, buf2 + i, esc);
                              memcpy (value + j, esc, 2 * u_strlen (esc));
                              j += u_strlen (esc);
                              i += (offset - 1);
                            }
                          else
                            {

                              /* filling value buffer */
                              dest = value + j;
                              src = buf2 + i;
                              err = U_ZERO_ERROR;
                              ucnv_toUnicode (desc->conv, &dest, value + 1024,
                                              &src, buf2 + i + 1, NULL, FALSE,
                                              &err);
                              if (U_FAILURE (err))
                                {
                                  fprintf (stderr,
                                           "Unable to convert buffer\n");
                                  return ERR_ICU;
                                }
                              j += (dest - value - j);
                            }
                        }

                      /* insert metadata in list */
                      if (desc->meta == NULL)
                        {
                          meta =
                            (struct meta *) malloc (sizeof (struct meta));
                          desc->meta = meta;
                        }
                      else
                        {
                          meta = desc->meta;
                          while (meta->next != NULL)
                            {
                              meta = meta->next;
                            }
                          meta->next =
                            (struct meta *) malloc (sizeof (struct meta));
                          meta = meta->next;
                        }
                      meta->next = NULL;
                      meta->name = (UChar *) malloc (2 * u_strlen (name) + 2);
                      meta->value =
                        (UChar *) malloc (2 * u_strlen (value) + 2);
                      memset (meta->name, '\x00', 2 * u_strlen (name) + 2);
                      memset (meta->value, '\x00', 2 * u_strlen (value) + 2);
                      memcpy (meta->name, name, 2 * u_strlen (name));
                      memcpy (meta->value, value, 2 * u_strlen (value));
                      meta->name_length = u_strlen (name);
                      meta->value_length = u_strlen (value);

                      for (; strncmp (buf2 + i, "\x3E", 1); i++)
                        {
                          if (i >= size - 9)
                            {
                              strncpy (buf2, buf2 + i, len - i);
                              len =
                                read (desc->fd, buf2 + i,
                                      BUFSIZE - len + i) + len - i;
                              i = 0;
                            }
                        }
                      continue;
                    }

                }
              else
                {

                  isMarkup = 1;
                }
            }

          /* get metadata value */
          if (!isJavascript && isMeta)
            {
              for (; len != 0 && strncmp (buf2 + i, "\x3E", 1); i++)
                {
                  if (i >= size - 9)
                    {
                      strncpy (buf2, buf2 + i, len - i);
                      len =
                        read (desc->fd, buf2 + i,
                              BUFSIZE - len + i) + len - i;
                      i = 0;
                    }
                }
              i++;
              memset (value, '\x00', 2048);
              for (j = 0; len != 0 && strncmp (buf2 + i, "\x3C", 1); i++)
                {
                  if (i >= size - 9)
                    {
                      strncpy (buf2, buf2 + i, len - i);
                      len =
                        read (desc->fd, buf2 + i,
                              BUFSIZE - len + i) + len - i;
                      i = 0;
                    }
                  if (!strncmp (buf2 + i, "\x26", 1))
                    {
                      memset (esc, '\x00', 6);
                      offset = escapeChar (desc, buf2 + i, esc);
                      memcpy (value + j, esc, 2 * u_strlen (esc));
                      j += u_strlen (esc);
                      i += (offset - 1);
                    }
                  else
                    {

                      /* filling value buffer */
                      dest = value + j;
                      src = buf2 + i;
                      err = U_ZERO_ERROR;
                      ucnv_toUnicode (desc->conv, &dest, value + 1024,
                                      &src, buf2 + i + 1, NULL, FALSE, &err);
                      if (U_FAILURE (err))
                        {
                          fprintf (stderr, "Unable to convert buffer\n");
                          return ERR_ICU;
                        }
                      j += (dest - value - j);
                    }
                }
              meta->value = (UChar *) malloc (2 * (j + 1));
              memcpy (meta->value, value, 2 * u_strlen (value));
              meta->value_length = u_strlen (value);
              isMeta = 0;
              i += 7;
              continue;
            }

          /* detecting end of markup */
          if (!isJavascript && isMarkup && !strncmp (buf2 + i, "\x3e", 1))
            {
              if (!space_added && l > 0)
                {
                  buf[l] = 0x20;
                  l ++;
                  space_added = 1;
                }
              isMarkup = 0;
            }

          /* handling text */
          if (!isJavascript && !isMarkup && strncmp (buf2 + i, "\x3e", 1))
            {

              if (strncmp (buf2 + i, "\n", 1) && strncmp (buf2 + i, "\t", 1) && strncmp (buf2 + i, "\r", 1))
                {

                  /* converting tokens */
                  if (!isJavascript && !isMarkup
                      && !strncmp (buf2 + i, "\x26", 1))
                    {
                      memset (esc, '\x00', 6);
                      offset = escapeChar (desc, buf2 + i, esc);
                      if (memcmp (esc, "\x20\x00", u_strlen (esc)))
                        {
                          memcpy (buf + l, esc, 2 * u_strlen (esc));
                          l += u_strlen (esc);
                          space_added = 0;
                        }
                      else {
                        if (!space_added){
                            buf[l] = 0x20;
                            space_added = 1;
                            l++;
                        }
                      }
                      i += (offset - 1);
                    }
                  else
                    {
                      if (buf2[i] != 0x20 || !space_added){
                        /* filling output buffer */
                        dest = buf + l;
                        src = buf2 + i;
                        err = U_ZERO_ERROR;
                        ucnv_toUnicode (desc->conv, &dest, buf + size / 2,
                                        &src, buf2 + i + 1, NULL, FALSE, &err);
                        if (U_FAILURE (err))
                            {
                            fprintf (stderr, "Unable to convert buffer\n");
                            return ERR_ICU;
                            }
                        l += (dest - buf - l);
                        if (buf2[i] == 0x20) {space_added = 1;} else {space_added=0;}
                      }
                    }
                }
              else
                {
                  /* replace tabs and eol by spaces */
                  if (!space_added){
                    buf[l] = 0x20;
                    space_added = 1;
                    l++;
                  }
                }
            }
        }

      /* filling new buffer correctly */
      if (!fini)
        {
          if (dangerousCut)
            {
              r = len - i;
              strncpy (buf2, buf2 + i, r);
              len = read (desc->fd, buf2 + r, BUFSIZE - r) + r;
              if (len < 9)
                {
                  endOfFile = 1;
                }
              dangerousCut = 0;
            }
          else
            {
              len = read (desc->fd, buf2, BUFSIZE);
            }
        }

    }

  /* ending buffer properly */
  if (l > 0)
    {
      buf[l] = 0x20;
      return 2*l;
    }

  if (len == 0)
    {
      return NO_MORE_DATA;
    }

  return 2*l;
}

int
getEncoding (int fd, char *encoding)
{
  int i, len, none, r;
  char buf[2048];

  none = 0;
  i = 0;

  /* search for 'charset' in header */
  len = read (fd, buf, BUFSIZE);
  while (len > 0 && !none && strncasecmp (buf + i, "charset=", 8)
         )
    {
      if (len - i < 8)
        {
          r = len - i;
          strncpy (buf, buf + i, r);
          len = read (fd, buf + r, BUFSIZE - r) + r;
          i = 0;
          break;
        }
      if (!strncasecmp (buf + i, "<body", 5))
        {
          none = 1;
        }
      i++;
    }
  if (!none && len > 0)
    {
      len = 0;
      i += 8;

      /* copy charset */
      while (strncmp (buf + i + len, "\x22", 1))
        {
          len++;
        }
      strncpy (encoding, buf + i, len);
      strncpy (encoding + len, "\0", 1);

      /* default charset is US-ASCII */
    }
  else
    {
      strncpy (encoding, "US-ASCII\0", 9);
    }

  lseek (fd, 0, SEEK_SET);

  return OK;
}
