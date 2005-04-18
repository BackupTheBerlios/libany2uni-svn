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
};

#endif /* __LATEXTAGS_H__ */
