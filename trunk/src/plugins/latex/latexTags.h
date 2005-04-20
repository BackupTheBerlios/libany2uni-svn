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

/* list of LaTeX tags */

#ifndef __LATEXTAGS_H__
#define __LATEXTAGS_H__

enum latex_tag {
  ignore,
  ignore_all,
  replace_by_space,
  end,
  text_container,
  
  /* detect encoding */
  usepackage,

  /* metadata */
  title,
  author,
  date,
  name,
  address,
  signature,
  
  /* footnotes and captions */
  footnote,
  footnotemark,
  footnotetext,
  caption,

  /* end of paragraph */
  end_par,

  /* special characters */
  ccedil,
  Ccedil,
  backslash,
  tilde,
  leftbrace,
  rightbrace,
  percent,
  dollar,
  ampersand,
  sharp,
  pow,
  underscore,
  oe,
  OE,
  cute,
  grave,
  circonflex,
  umlaut,
  tilde_on,
  aring,
  Aring,
  ae,
  AE,
  oslash,
  Oslash,
  szlig,

  /* LaTeX logos */
  latex,
  latexe,
  tex,
};

#endif /* __LATEXTAGS_H__ */
