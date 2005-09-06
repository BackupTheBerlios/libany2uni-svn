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

#include "p_mht.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>

#define BUFSIZE 2048

int
initReader (struct doc_descriptor *desc)
{
    struct mhtState *state = (struct mhtState *) (desc->myState);
    UErrorCode err;
    char encoding[40];
    int i;

    /* search Content-Type */
    state->len = read (desc->fd, state->buf, BUFSIZE);
    state->cursor = 0;
    while (strncasecmp (state->buf + state->cursor, "Content-Type", 12))
      {

          if (getNextLine (desc))
            {
                fprintf (stderr, "Unexpected end of file\n");
                return INIT_ERROR;
            }

          if (state->cursor >= state->len - 12)
            {
                strncpy (state->buf, state->buf + state->cursor,
                         state->len - state->cursor);
                state->len =
                    read (desc->fd, state->buf + state->len - state->cursor,
                          BUFSIZE - state->len + state->cursor) + state->len -
                    state->cursor;
                if (state->len == 0)
                  {
                      fprintf (stderr, "Unexpected end of file\n");
                      return INIT_ERROR;
                  }
                state->cursor = 0;
            }
      }
    state->cursor += 12;
    if (state->cursor >= state->len - 12)
      {
          strncpy (state->buf, state->buf + state->cursor,
                   state->len - state->cursor);
          state->len =
              read (desc->fd, state->buf + state->len - state->cursor,
                    BUFSIZE - state->len + state->cursor) + state->len -
              state->cursor;
          if (state->len == 0)
            {
                fprintf (stderr, "Unexpected end of file\n");
                return INIT_ERROR;
            }
          state->cursor = 0;
      }
    state->cursor += 2;
    state->monopart = 0;
    if (!strncasecmp (state->buf + state->cursor, "text/html", 9))
      {

          /* unique html file */
          state->monopart = 1;

          /* get encoding */
          for (; strncasecmp (state->buf + state->cursor, "charset=\"", 9) &&
               strncasecmp (state->buf + state->cursor, "Content-", 8);
               state->cursor++)
            {
                if (state->cursor >= state->len - 9)
                  {
                      strncpy (state->buf, state->buf + state->cursor,
                               state->len - state->cursor);
                      state->len =
                          read (desc->fd,
                                state->buf + state->len - state->cursor,
                                BUFSIZE - state->len + state->cursor) +
                          state->len - state->cursor;
                      if (state->len == 0)
                        {
                            fprintf (stderr, "Unexpected end of file\n");
                            return INIT_ERROR;
                        }
                      state->cursor = 0;
                  }
            }
          if (strncasecmp (state->buf + state->cursor, "charset=\"", 9))
            {
                fprintf (stderr, "no charset found\n");
                return ERR_KEYWORD;
            }
          state->cursor += 9;

          memset (encoding, '\x00', 40);
          for (i = 0; strncmp (state->buf + state->cursor, "\"", 1);
               i++, state->cursor++)
            {
                if (state->cursor >= state->len)
                  {
                      state->len = read (desc->fd, state->buf, BUFSIZE);
                      if (state->len == 0)
                        {
                            fprintf (stderr, "Unexpected end of file\n");
                            return INIT_ERROR;
                        }
                      state->cursor = 0;
                  }
                strncpy (encoding + i, state->buf + state->cursor, 1);
            }

          /* initialize converter */
          err = U_ZERO_ERROR;
          desc->conv = ucnv_open (encoding, &err);
          if (U_FAILURE (err))
            {
                fprintf (stderr, "Unable to open ICU converter\n");
                return ERR_ICU;
            }

          /* go to html beginning */
          if (state->cursor >= state->len - 14)
            {
                strncpy (state->buf, state->buf + state->cursor,
                         state->len - state->cursor);
                state->len =
                    read (desc->fd, state->buf + state->len - state->cursor,
                          BUFSIZE - state->len + state->cursor) + state->len -
                    state->cursor;
                if (state->len == 0)
                  {
                      fprintf (stderr, "Unexpected end of file\n");
                      return INIT_ERROR;
                  }
                state->cursor = 0;
            }
          for (;
               strncasecmp (state->buf + state->cursor, "<!DOCTYPE HTML", 14);
               state->cursor++)
            {
                if (state->cursor >= state->len - 14)
                  {
                      strncpy (state->buf, state->buf + state->cursor,
                               state->len - state->cursor);
                      state->len =
                          read (desc->fd,
                                state->buf + state->len - state->cursor,
                                BUFSIZE - state->len + state->cursor) +
                          state->len - state->cursor;
                      if (state->len == 0)
                        {
                            fprintf (stderr, "Unexpected end of file\n");
                            return INIT_ERROR;
                        }
                      state->cursor = 0;
                  }
            }

          /* search 'test/html' part */
      }
    else if (getNextHTMLpart (desc))
      {
          return INIT_ERROR;
      }

    return OK;
}


int
getText (struct doc_descriptor *desc, UChar * buf, int size)
{
    struct mhtState *state = (struct mhtState *) (desc->myState);
    UErrorCode err;
    char *src;
    UChar *dest, esc[3];
    char tmp[2];
    int l, i, v, space_added;

    l = 0;
    space_added = 0;
    if (state->cursor >= state->len - 7)
      {
          strncpy (state->buf, state->buf + state->cursor,
                   state->len - state->cursor);
          state->len =
              read (desc->fd, state->buf + state->len - state->cursor,
                    BUFSIZE - state->len + state->cursor) + state->len -
              state->cursor;
          if (state->len == 0)
            {
                fprintf (stderr, "Unexpected end of file\n");
                return INIT_ERROR;
            }
          state->cursor = 0;
      }
    if (!strncasecmp (state->buf + state->cursor, "</HTML>", 7))
      {
          if (state->monopart || getNextHTMLpart)
            {
                return NO_MORE_DATA;
            }
      }

    while (strncasecmp (state->buf + state->cursor, "</HTML>", 7))
      {

          /* skip javascript */
          if (!strncasecmp (state->buf + state->cursor, "<SCRIPT", 7))
            {
                while (strncasecmp
                       (state->buf + state->cursor, "</SCRIPT>", 9))
                  {
                      state->cursor++;
                      if (state->cursor >= state->len - 9)
                        {
                            strncpy (state->buf, state->buf + state->cursor,
                                     state->len - state->cursor);
                            state->len =
                                read (desc->fd,
                                      state->buf + state->len - state->cursor,
                                      BUFSIZE - state->len + state->cursor) +
                                state->len - state->cursor;
                            if (state->len == 0)
                              {
                                  fprintf (stderr,
                                           "Unexpected end of file\n");
                                  return INIT_ERROR;
                              }
                            state->cursor = 0;
                        }
                  }
                state->cursor += 9;
                if (state->cursor >= state->len - 7)
                  {
                      strncpy (state->buf, state->buf + state->cursor,
                               state->len - state->cursor);
                      state->len =
                          read (desc->fd,
                                state->buf + state->len - state->cursor,
                                BUFSIZE - state->len + state->cursor) +
                          state->len - state->cursor;
                      if (state->len == 0)
                        {
                            fprintf (stderr, "Unexpected end of file\n");
                            return INIT_ERROR;
                        }
                      state->cursor = 0;
                  }

                /* end of paragraph */
            }
          else if (!strncasecmp (state->buf + state->cursor, "<p", 2)
                   || !strncasecmp (state->buf + state->cursor, "<br", 3)
                   || !strncasecmp (state->buf + state->cursor, "<div", 4))
            {
                while (strncmp (state->buf + state->cursor, ">", 1))
                  {
                      state->cursor++;
                      if (state->cursor >= state->len - 7)
                        {
                            strncpy (state->buf, state->buf + state->cursor,
                                     state->len - state->cursor);
                            state->len =
                                read (desc->fd,
                                      state->buf + state->len - state->cursor,
                                      BUFSIZE - state->len + state->cursor) +
                                state->len - state->cursor;
                            if (state->len == 0)
                              {
                                  fprintf (stderr,
                                           "Unexpected end of file\n");
                                  return INIT_ERROR;
                              }
                            state->cursor = 0;
                        }
                  }
                state->cursor++;
                if (state->cursor >= state->len - 7)
                  {
                      strncpy (state->buf, state->buf + state->cursor,
                               state->len - state->cursor);
                      state->len =
                          read (desc->fd,
                                state->buf + state->len - state->cursor,
                                BUFSIZE - state->len + state->cursor) +
                          state->len - state->cursor;
                      if (state->len == 0)
                        {
                            fprintf (stderr, "Unexpected end of file\n");
                            return INIT_ERROR;
                        }
                      state->cursor = 0;
                  }
                if (l > 0 && !space_added)
                  {
                      buf[l]=0x20;
                      l++;
                      space_added = 1;
                      return l * 2;
                  }

            }


          /* skip markups */
          else if (!strncmp (state->buf + state->cursor, "<", 1))
            {
                while (strncmp (state->buf + state->cursor, ">", 1))
                  {
                      state->cursor++;
                      if (state->cursor >= state->len - 7)
                        {
                            strncpy (state->buf, state->buf + state->cursor,
                                     state->len - state->cursor);
                            state->len =
                                read (desc->fd,
                                      state->buf + state->len - state->cursor,
                                      BUFSIZE - state->len + state->cursor) +
                                state->len - state->cursor;
                            if (state->len == 0)
                              {
                                  fprintf (stderr,
                                           "Unexpected end of file\n");
                                  return INIT_ERROR;
                              }
                            state->cursor = 0;
                        }
                  }
                state->cursor++;
                if (state->cursor >= state->len - 7)
                  {
                      strncpy (state->buf, state->buf + state->cursor,
                               state->len - state->cursor);
                      state->len =
                          read (desc->fd,
                                state->buf + state->len - state->cursor,
                                BUFSIZE - state->len + state->cursor) +
                          state->len - state->cursor;
                      if (state->len == 0)
                        {
                            fprintf (stderr, "Unexpected end of file\n");
                            return INIT_ERROR;
                        }
                      state->cursor = 0;
                  }

                if (!space_added)
                  {
                      buf[l]=0x20;
                      l++;
                      space_added = 1;
                  }

                /* unicode character */
            }
          else if (!strncmp (state->buf + state->cursor, "=", 1))
            {
                state->cursor++;
                if (isxdigit (state->buf[state->cursor]))
                  {
                      for (v = 0, i = 0; i < 2; i++)
                        {
                            v *= 16;
                            strncpy (tmp, state->buf + state->cursor, 1);
                            strncpy (tmp + 1, "\0", 1);
                            state->cursor++;
                            if (islower (tmp[0]))
                              {
                                  v += tmp[0] - 87;
                              }
                            else if (isupper (tmp[0]))
                              {
                                  v += tmp[0] - 55;
                              }
                            else
                              {
                                  v += atoi (tmp);
                              }
                        }
                      buf[l] = v;
                      l++;
                      if (v!=0x20) {space_added = 0;}
                      else {space_added = 1; }
                      if ((2 * l >= size - 2))
                        {
                            buf[l]=0x20;
                            l++;
                            space_added = 1;
                            return 2 * l;
                        }

                  }
                else
                  {
                      state->cursor += 2;
                  }

                /* escape character */
            }
          else if (!strncmp (state->buf + state->cursor, "&", 1))
            {
                if (state->cursor >= state->len - 8)
                  {
                      strncpy (state->buf, state->buf + state->cursor,
                               state->len - state->cursor);
                      state->len =
                          read (desc->fd,
                                state->buf + state->len - state->cursor,
                                BUFSIZE - state->len + state->cursor) +
                          state->len - state->cursor;
                      if (state->len == 0)
                        {
                            fprintf (stderr, "Unexpected end of file\n");
                            return INIT_ERROR;
                        }
                      state->cursor = 0;
                  }
                memset (esc, '\x00', 6);
                state->cursor += escapeChar (state->buf + state->cursor, esc);
                memcpy (buf + l, esc, 2 * u_strlen (esc));
                l += u_strlen (esc);
                if ((2 * l >= size - 2) && !space_added)
                  {
                      buf[l]=0x20;
                      l++;
                      space_added = 1;
                      return 2 * l;
                  }

                /* text content */
            }
          else
            {
                if (strncmp (state->buf + state->cursor, "\x0A", 1) &&
                    strncmp (state->buf + state->cursor, "\x0D", 1) &&
                    (!isblank(state->buf[state->cursor]) || !space_added) )
                  {
                      if (!isblank(state->buf[state->cursor])) {
                        space_added = 0;
                      }
                      else {space_added=1;}
                      dest = buf + l;
                      src = state->buf + state->cursor;
                      err = U_ZERO_ERROR;
                      ucnv_toUnicode (desc->conv, &dest, buf + size / 2,
                                      &src, state->buf + state->cursor + 1,
                                      NULL, FALSE, &err);
                      if (U_FAILURE (err))
                        {
                            fprintf (stderr, "Unable to convert buffer\n");
                            return ERR_ICU;
                        }
                      l += (dest - buf - l);

                      if (2 * l >= size - 2)
                        {
                            buf[l]=0x20;
                            l++;
                            space_added = 1;
                            return 2 * l;
                        }
                  }
                else
                  {             /* linefeeds are irrelevant to content in MIME */
                  }
                state->cursor++;
                if (state->cursor >= state->len - 7)
                  {
                      strncpy (state->buf, state->buf + state->cursor,
                               state->len - state->cursor);
                      state->len =
                          read (desc->fd,
                                state->buf + state->len - state->cursor,
                                BUFSIZE - state->len + state->cursor) +
                          state->len - state->cursor;
                      if (state->len == 0)
                        {
                            fprintf (stderr, "Unexpected end of file\n");
                            return INIT_ERROR;
                        }
                      state->cursor = 0;
                  }
            }
      }

    if (l > 0 && !space_added)
      {
          buf[l]=0x20;
          l++;
      }
    return 2 * l;
}


int
getNextLine (struct doc_descriptor *desc)
{
    struct mhtState *state = (struct mhtState *) (desc->myState);

    /* skipping non EOL characters */
    for (; strncmp (state->buf + state->cursor, "\x0A", 1) &&
         strncmp (state->buf + state->cursor, "\x0D", 1); state->cursor++)
      {
          if (state->cursor >= state->len)
            {
                state->len = read (desc->fd, state->buf, BUFSIZE);
                if (state->len == 0)
                  {
                      return NO_MORE_DATA;
                  }
                state->cursor = 0;
            }
      }

    /* skipping EOL characters */
    for (; (!strncmp (state->buf + state->cursor, "\x0A", 1) ||
            !strncmp (state->buf + state->cursor, "\x0D", 1));
         state->cursor++)
      {
          if (state->cursor >= state->len)
            {
                state->len = read (desc->fd, state->buf, BUFSIZE);
                if (state->len == 0)
                  {
                      return NO_MORE_DATA;
                  }
                state->cursor = 0;
            }
      }

    return OK;

}


int
getNextHTMLpart (struct doc_descriptor *desc)
{
    struct mhtState *state = (struct mhtState *) (desc->myState);
    UErrorCode err;
    int i;
    char encoding[40];

    do
      {
          if (state->cursor >= state->len - 15)
            {
                strncpy (state->buf, state->buf + state->cursor,
                         state->len - state->cursor);
                state->len =
                    read (desc->fd, state->buf + state->len - state->cursor,
                          BUFSIZE - state->len + state->cursor) + state->len -
                    state->cursor;
                if (state->len == 0)
                  {
                      return NO_MORE_DATA;
                  }
                state->cursor = 0;
            }

          /* search next part */
          for (;
               strncasecmp (state->buf + state->cursor, "----=_NextPart_",
                            15); state->cursor++)
            {
                if (state->cursor >= state->len - 15)
                  {
                      strncpy (state->buf, state->buf + state->cursor,
                               state->len - state->cursor);
                      state->len =
                          read (desc->fd,
                                state->buf + state->len - state->cursor,
                                BUFSIZE - state->len + state->cursor) +
                          state->len - state->cursor;
                      if (state->len == 0)
                        {
                            return NO_MORE_DATA;
                        }
                      state->cursor = 0;
                  }
            }

          /* check content-type */
          if (getNextLine (desc))
            {
                return NO_MORE_DATA;
            }
          if (state->cursor >= state->len - 14)
            {
                strncpy (state->buf, state->buf + state->cursor,
                         state->len - state->cursor);
                state->len =
                    read (desc->fd, state->buf + state->len - state->cursor,
                          BUFSIZE - state->len + state->cursor) + state->len -
                    state->cursor;
                if (state->len == 0)
                  {
                      return NO_MORE_DATA;
                  }
                state->cursor = 0;
            }
          while (strncasecmp
                 (state->buf + state->cursor, "Content-Type: ", 14))
            {
                if (getNextLine (desc))
                  {
                      return NO_MORE_DATA;
                  }
                if (state->cursor >= state->len - 14)
                  {
                      strncpy (state->buf, state->buf + state->cursor,
                               state->len - state->cursor);
                      state->len =
                          read (desc->fd,
                                state->buf + state->len - state->cursor,
                                BUFSIZE - state->len + state->cursor) +
                          state->len - state->cursor;
                      if (state->len == 0)
                        {
                            return NO_MORE_DATA;
                        }
                      state->cursor = 0;
                  }
            }
          state->cursor += 14;

          if (state->cursor >= state->len - 9)
            {
                strncpy (state->buf, state->buf + state->cursor,
                         state->len - state->cursor);
                state->len =
                    read (desc->fd, state->buf + state->len - state->cursor,
                          BUFSIZE - state->len + state->cursor) + state->len -
                    state->cursor;
                if (state->len == 0)
                  {
                      return NO_MORE_DATA;
                  }
                state->cursor = 0;
            }
      }
    while (state->len
           && strncasecmp (state->buf + state->cursor, "text/html", 9));
    if (!state->len)
      {
          return NO_MORE_DATA;
      }

    /* get encoding */
    if (getNextLine (desc))
      {
          return NO_MORE_DATA;
      }
    for (; strncasecmp (state->buf + state->cursor, "charset=\"", 9) &&
         strncasecmp (state->buf + state->cursor, "Content-", 8);
         state->cursor++)
      {
          if (state->cursor >= state->len - 9)
            {
                strncpy (state->buf, state->buf + state->cursor,
                         state->len - state->cursor);
                state->len =
                    read (desc->fd, state->buf + state->len - state->cursor,
                          BUFSIZE - state->len + state->cursor) + state->len -
                    state->cursor;
                if (state->len == 0)
                  {
                      return NO_MORE_DATA;
                  }
                state->cursor = 0;
            }
      }
    if (strncasecmp (state->buf + state->cursor, "charset=\"", 9))
      {
          fprintf (stderr, "no charset found\n");
          return ERR_KEYWORD;
      }
    state->cursor += 9;

    memset (encoding, '\x00', 40);
    for (i = 0; strncmp (state->buf + state->cursor, "\"", 1);
         i++, state->cursor++)
      {
          if (state->cursor >= state->len)
            {
                state->len = read (desc->fd, state->buf, BUFSIZE);
                if (state->len == 0)
                  {
                      return NO_MORE_DATA;
                  }
                state->cursor = 0;
            }
          strncpy (encoding + i, state->buf + state->cursor, 1);
      }

    /* initialize converter */
    err = U_ZERO_ERROR;
    desc->conv = ucnv_open (encoding, &err);
    if (U_FAILURE (err))
      {
          fprintf (stderr, "Unable to open ICU converter\n");
          return ERR_ICU;
      }

    /* go to html beginning */
    if (state->cursor >= state->len - 14)
      {
          strncpy (state->buf, state->buf + state->cursor,
                   state->len - state->cursor);
          state->len =
              read (desc->fd, state->buf + state->len - state->cursor,
                    BUFSIZE - state->len + state->cursor) + state->len -
              state->cursor;
          if (state->len == 0)
            {
                return NO_MORE_DATA;
            }
          state->cursor = 0;
      }
    for (; strncasecmp (state->buf + state->cursor, "<!DOCTYPE HTML", 14);
         state->cursor++)
      {
          if (state->cursor >= state->len - 14)
            {
                strncpy (state->buf, state->buf + state->cursor,
                         state->len - state->cursor);
                state->len =
                    read (desc->fd, state->buf + state->len - state->cursor,
                          BUFSIZE - state->len + state->cursor) + state->len -
                    state->cursor;
                if (state->len == 0)
                  {
                      return NO_MORE_DATA;
                  }
                state->cursor = 0;
            }
      }

    return OK;
}


int
escapeChar (char *buf, UChar * res)
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
          *res=0x26;
          return 1;
      }

    /* identifying token */
    if (!strncasecmp (token, "&amp;", 5))
      {
          *res=0x26;
          return 5;
      }
    else if (!strncasecmp (token, "&lt;", 4))
      {
          *res=0x3C;
          return 4;
      }
    else if (!strncasecmp (token, "&gt;", 4))
      {
          *res=0x3E;
          return 4;
      }
    else if (!strncasecmp (token, "&quot;", 6))
      {
          *res=0x22;
          return 6;
      }
    else if (!strncmp (token, "&eacute;", 8))
      {
          *res=0xE9;
          return 8;
      }
    else if (!strncmp (token, "&Eacute;", 8))
      {
          *res=0xC9;
          return 8;
      }
    else if (!strncmp (token, "&egrave;", 8))
      {
          *res=0xE8;
          return 8;
      }
    else if (!strncmp (token, "&Egrave;", 8))
      {
          *res=0xC8;
          return 8;
      }
    else if (!strncmp (token, "&ecirc;", 7))
      {
          *res=0xEA;
          return 7;
      }
    else if (!strncmp (token, "&agrave;", 8))
      {
          *res=0xE0;
          return 8;
      }
    else if (!strncmp (token, "&iuml;", 6))
      {
          *res=0xEF;
          return 6;
      }
    else if (!strncmp (token, "&ccedil;", 8))
      {
          *res=0xE7;
          return 8;
      }
    else if (!strncmp (token, "&ntilde;", 8))
      {
          *res=0xF1;
          return 8;
      }
    else if (!strncmp (token, "&copy;", 6))
      {
          *res=0xA9;
          return 6;
      }
    else if (!strncmp (token, "&reg;", 5))
      {
          *res=0xAE;
          return 5;
      }
    else if (!strncmp (token, "&deg;", 5))
      {
          *res=0xB0;
          return 5;
      }
    else if (!strncmp (token, "&ordm;", 6))
      {
          *res=0xBA;
          return 6;
      }
    else if (!strncmp (token, "&laquo;", 7))
      {
          *res=0xAB;
          return 7;
      }
    else if (!strncmp (token, "&raquo;", 7))
      {
          *res=0xBB;
          return 7;
      }
    else if (!strncmp (token, "&micro;", 7))
      {
          *res=0xB5;
          return 7;
      }
    else if (!strncmp (token, "&para;", 6))
      {
          *res=0xB6;
          return 6;
      }
    else if (!strncmp (token, "&frac14;", 8))
      {
          *res=0xBC;
          return 8;
      }
    else if (!strncmp (token, "&frac12;", 8))
      {
          *res=0xBD;
          return 8;
      }
    else if (!strncmp (token, "&frac34;", 8))
      {
          *res=0xBE;
          return 8;
      }
    else if (!strncmp (token, "&#", 2))
      {
          res[0] = atoi (token + 2);
          return i+1;
      }
    else
      {
          *res=0x20;
          return i + 1;
      }
}
