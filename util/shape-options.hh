/*
 * Copyright © 2011  Google, Inc.
 *
 *  This is part of HarfBuzz, a text shaping library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Google Author(s): Behdad Esfahbod
 */

#ifndef SHAPE_OPTIONS_HH
#define SHAPE_OPTIONS_HH

#include "options.hh"

struct shape_options_t
{
  ~shape_options_t ()
  {
    g_free (direction);
    g_free (language);
    g_free (script);
    free (features);
    g_strfreev (shapers);
  }

  void add_options (option_parser_t *parser);

  void setup_buffer (buffer_t *buffer)
  {
    buffer_set_direction (buffer, direction_from_string (direction, -1));
    buffer_set_script (buffer, script_from_string (script, -1));
    buffer_set_language (buffer, language_from_string (language, -1));
    buffer_set_flags (buffer, (buffer_flags_t)
				 (HB_BUFFER_FLAG_DEFAULT |
				  (bot ? HB_BUFFER_FLAG_BOT : 0) |
				  (eot ? HB_BUFFER_FLAG_EOT : 0) |
				  (verify ? HB_BUFFER_FLAG_VERIFY : 0) |
				  (unsafe_to_concat ? HB_BUFFER_FLAG_PRODUCE_UNSAFE_TO_CONCAT : 0) |
				  (safe_to_insert_tatweel ? HB_BUFFER_FLAG_PRODUCE_SAFE_TO_INSERT_TATWEEL : 0) |
				  (preserve_default_ignorables ? HB_BUFFER_FLAG_PRESERVE_DEFAULT_IGNORABLES : 0) |
				  (remove_default_ignorables ? HB_BUFFER_FLAG_REMOVE_DEFAULT_IGNORABLES : 0) |
				  0));
    buffer_set_invisible_glyph (buffer, invisible_glyph);
    buffer_set_not_found_glyph (buffer, not_found_glyph);
    buffer_set_cluster_level (buffer, cluster_level);
    buffer_guess_segment_properties (buffer);
  }

  void populate_buffer (buffer_t *buffer, const char *text, int text_len,
			const char *text_before, const char *text_after,
			font_t *font)
  {
    buffer_clear_contents (buffer);

    if (glyphs)
    {
      /* Call the setup_buffer first while the buffer is empty,
       * as guess_segment_properties doesn't like glyphs in the buffer. */

      setup_buffer (buffer);
      char *glyphs_text = (char *) text;
      int glyphs_len = text_len;
      if (glyphs_len < 0)
	glyphs_len = strlen (glyphs_text);

      if (glyphs_len && glyphs_text[glyphs_len - 1] != ']')
      {
	glyphs_text = g_strdup_printf ("%*s]", glyphs_len, glyphs_text);
	glyphs_len = -1;
      }

      buffer_deserialize_glyphs (buffer,
				    glyphs_text, glyphs_len,
				    nullptr,
				    font,
				    HB_BUFFER_SERIALIZE_FORMAT_TEXT);

      if (!strchr (glyphs_text, '+'))
      {
        scale_advances = false;
        unsigned count;
	direction_t direction = buffer_get_direction (buffer);
	glyph_info_t *infos = buffer_get_glyph_infos (buffer, &count);
	glyph_position_t *positions = buffer_get_glyph_positions (buffer, &count);
	for (unsigned i = 0; i < count; i++)
	  font_get_glyph_advance_for_direction (font,
						   infos[i].codepoint,
						   direction,
						   &positions[i].x_advance,
						   &positions[i].y_advance);
      }

      if (glyphs_text != text)
        g_free (glyphs_text);

      return;
    }

    if (text_before) {
      unsigned int len = strlen (text_before);
      buffer_add_utf8 (buffer, text_before, len, len, 0);
    }
    buffer_add_utf8 (buffer, text, text_len, 0, text_len);
    if (text_after) {
      buffer_add_utf8 (buffer, text_after, -1, 0, 0);
    }

    if (!utf8_clusters) {
      /* Reset cluster values to refer to Unicode character index
       * instead of UTF-8 index. */
      unsigned int num_glyphs = buffer_get_length (buffer);
      glyph_info_t *info = buffer_get_glyph_infos (buffer, nullptr);
      for (unsigned int i = 0; i < num_glyphs; i++)
      {
	info->cluster = i;
	info++;
      }
    }

    setup_buffer (buffer);
  }

  bool_t shape (font_t *font, buffer_t *buffer, const char **error=nullptr)
  {
    if (glyphs)
    {
      /* Scale positions. */
      int x_scale, y_scale;
      font_get_scale (font, &x_scale, &y_scale);
      unsigned upem = face_get_upem (font_get_face (font));
      unsigned count;
      auto *positions = buffer_get_glyph_positions (buffer, &count);
      for (unsigned i = 0; i < count; i++)
      {
	auto &pos = positions[i];
	pos.x_offset = pos.x_offset * x_scale / upem;
	pos.y_offset = pos.y_offset * y_scale / upem;
	if (scale_advances)
	{
	  pos.x_advance = pos.x_advance * x_scale / upem;
	  pos.y_advance = pos.y_advance * y_scale / upem;
	}
      }
    }
    else
    {
      if (advance <= 0)
      {
	if (!shape_full (font, buffer, features, num_features, shapers))
	{
	  if (error)
	    *error = "Shaping failed.";
	  goto fail;
	}

	if (advance < 0)
	{
	  float unit = (1 << SUBPIXEL_BITS);

	  /* Calculate buffer advance */
	  float w = 0;
	  unsigned count = 0;
	  glyph_position_t *pos = buffer_get_glyph_positions (buffer, &count);
	  if (HB_DIRECTION_IS_HORIZONTAL (buffer_get_direction (buffer)))
	    for (unsigned i = 0; i < count; i++)
	      w += pos[i].x_advance;
	  else
	    for (unsigned i = 0; i < count; i++)
	      w += pos[i].y_advance;

	  printf ("Default size: %u\n", (unsigned) roundf (w / unit));
	  exit (0);
	}
      }
#ifdef HB_EXPERIMENTAL_API
      else
      {
        float unit = (1 << SUBPIXEL_BITS);
        float target_advance = advance * unit;
	float w = 0;
	tag_t var_tag;
	float var_value;
	if (!shape_justify (font, buffer, features, num_features, shapers,
			       target_advance - unit * 0.5f, target_advance + unit * 0.5f,
			       &w, &var_tag, &var_value))
	{
	  if (error)
	    *error = "Shaping failed.";
	  goto fail;
	}
      }
#endif
    }

    if (normalize_glyphs)
      buffer_normalize_glyphs (buffer);

    return true;

  fail:
    return false;
  }

  void shape_closure (const char *text, int text_len,
		      font_t *font, buffer_t *buffer,
		      set_t *glyphs)
  {
    buffer_reset (buffer);
    buffer_add_utf8 (buffer, text, text_len, 0, text_len);
    setup_buffer (buffer);
    ot_shape_glyphs_closure (font, buffer, features, num_features, glyphs);
  }

  /* Buffer properties */
  char *direction = nullptr;
  char *language = nullptr;
  char *script = nullptr;

  /* Buffer flags */
  bool_t bot = false;
  bool_t eot = false;
  bool_t preserve_default_ignorables = false;
  bool_t remove_default_ignorables = false;

  feature_t *features = nullptr;
  unsigned int num_features = 0;
  char **shapers = nullptr;
  signed advance = 0;
  bool_t utf8_clusters = false;
  codepoint_t invisible_glyph = 0;
  codepoint_t not_found_glyph = 0;
  buffer_cluster_level_t cluster_level = HB_BUFFER_CLUSTER_LEVEL_DEFAULT;
  bool_t normalize_glyphs = false;
  bool_t glyphs = false;
  bool scale_advances = true;
  bool_t verify = false;
  bool_t unsafe_to_concat = false;
  bool_t safe_to_insert_tatweel = false;
  unsigned int num_iterations = 1;
};


static gboolean
parse_shapers (const char *name G_GNUC_UNUSED,
	       const char *arg,
	       gpointer    data,
	       GError    **error)
{
  shape_options_t *shape_opts = (shape_options_t *) data;
  char **shapers = g_strsplit (arg, ",", 0);

  for (char **shaper = shapers; *shaper; shaper++)
  {
    bool found = false;
    for (const char **shaper = shape_list_shapers (); *shaper; shaper++) {
      if (strcmp (*shaper, *shaper) == 0)
      {
	found = true;
	break;
      }
    }
    if (!found)
    {
      g_set_error (error, G_OPTION_ERROR, G_OPTION_ERROR_BAD_VALUE,
		   "Unknown or unsupported shaper: %s", *shaper);
      g_strfreev (shapers);
      return false;
    }
  }

  g_strfreev (shape_opts->shapers);
  shape_opts->shapers = shapers;
  return true;
}

static G_GNUC_NORETURN gboolean
list_shapers (const char *name G_GNUC_UNUSED,
	      const char *arg G_GNUC_UNUSED,
	      gpointer    data G_GNUC_UNUSED,
	      GError    **error G_GNUC_UNUSED)
{
  for (const char **shaper = shape_list_shapers (); *shaper; shaper++)
    g_printf ("%s\n", *shaper);

  exit(0);
}


static gboolean
parse_features (const char *name G_GNUC_UNUSED,
		const char *arg,
		gpointer    data,
		GError    **error G_GNUC_UNUSED)
{
  shape_options_t *shape_opts = (shape_options_t *) data;
  char *s = (char *) arg;
  size_t l = strlen (s);
  char *p;

  shape_opts->num_features = 0;
  g_free (shape_opts->features);
  shape_opts->features = nullptr;

  /* if the string is quoted, strip the quotes */
  if (s[0] == s[l - 1] && (s[0] == '\"' || s[0] == '\''))
  {
    s[l - 1] = '\0';
    s++;
  }

  if (!*s)
    return true;

  /* count the features first, so we can allocate memory */
  p = s;
  do {
    shape_opts->num_features++;
    p = strpbrk (p, ", ");
    if (p)
      p++;
  } while (p);

  shape_opts->features = (feature_t *) calloc (shape_opts->num_features, sizeof (*shape_opts->features));
  if (!shape_opts->features)
    return false;

  /* now do the actual parsing */
  p = s;
  shape_opts->num_features = 0;
  while (p && *p) {
    char *end = strpbrk (p, ", ");
    if (feature_from_string (p, end ? end - p : -1, &shape_opts->features[shape_opts->num_features]))
      shape_opts->num_features++;
    p = end ? end + 1 : nullptr;
  }

  return true;
}

void
shape_options_t::add_options (option_parser_t *parser)
{
  GOptionEntry entries[] =
  {
    {"list-shapers",	0, G_OPTION_FLAG_NO_ARG,
			      G_OPTION_ARG_CALLBACK,	(gpointer) &list_shapers,	"List available shapers and quit",	nullptr},
    {"shaper",		0, G_OPTION_FLAG_HIDDEN,
			      G_OPTION_ARG_CALLBACK,	(gpointer) &parse_shapers,	"Hidden duplicate of --shapers",	nullptr},
    {"shapers",		0, 0, G_OPTION_ARG_CALLBACK,	(gpointer) &parse_shapers,	"Set comma-separated list of shapers to try","list"},
    {"direction",	0, 0, G_OPTION_ARG_STRING,	&this->direction,		"Set text direction (default: auto)",	"ltr/rtl/ttb/btt"},
    {"language",	0, 0, G_OPTION_ARG_STRING,	&this->language,		"Set text language (default: $LANG)",	"BCP 47 tag"},
    {"script",		0, 0, G_OPTION_ARG_STRING,	&this->script,			"Set text script (default: auto)",	"ISO-15924 tag"},
    {"bot",		0, 0, G_OPTION_ARG_NONE,	&this->bot,			"Treat text as beginning-of-paragraph",	nullptr},
    {"eot",		0, 0, G_OPTION_ARG_NONE,	&this->eot,			"Treat text as end-of-paragraph",	nullptr},
#ifdef HB_EXPERIMENTAL_API
    {"justify-to",	0, 0,
			      G_OPTION_ARG_INT,		&this->advance,			"Target size to justify to",		"SIZE, or -1"},
#endif
    {"preserve-default-ignorables",0, 0, G_OPTION_ARG_NONE,	&this->preserve_default_ignorables,	"Preserve Default-Ignorable characters",	nullptr},
    {"remove-default-ignorables",0, 0, G_OPTION_ARG_NONE,	&this->remove_default_ignorables,	"Remove Default-Ignorable characters",	nullptr},
    {"invisible-glyph",	0, 0, G_OPTION_ARG_INT,		&this->invisible_glyph,		"Glyph value to replace Default-Ignorables with",	nullptr},
    {"not-found-glyph",	0, 0, G_OPTION_ARG_INT,		&this->not_found_glyph,		"Glyph value to replace not-found characters with",	nullptr},
    {"utf8-clusters",	0, 0, G_OPTION_ARG_NONE,	&this->utf8_clusters,		"Use UTF8 byte indices, not char indices",	nullptr},
    {"cluster-level",	0, 0, G_OPTION_ARG_INT,		&this->cluster_level,		"Cluster merging level (default: 0)",	"0/1/2"},
    {"normalize-glyphs",0, 0, G_OPTION_ARG_NONE,	&this->normalize_glyphs,	"Rearrange glyph clusters in nominal order",	nullptr},
    {"unsafe-to-concat",0, 0, G_OPTION_ARG_NONE,	&this->unsafe_to_concat,	"Produce unsafe-to-concat glyph flag",	nullptr},
    {"safe-to-insert-tatweel",0, 0, G_OPTION_ARG_NONE,	&this->safe_to_insert_tatweel,	"Produce safe-to-insert-tatweel glyph flag",	nullptr},
    {"glyphs",		0, 0, G_OPTION_ARG_NONE,	&this->glyphs,			"Interpret input as glyph string",	nullptr},
    {"verify",		0, 0, G_OPTION_ARG_NONE,	&this->verify,			"Perform sanity checks on shaping results",	nullptr},
    {"num-iterations",	'n',G_OPTION_FLAG_IN_MAIN,
			      G_OPTION_ARG_INT,		&this->num_iterations,		"Run shaper N times (default: 1)",	"N"},
    {nullptr}
  };
  parser->add_group (entries,
		     "shape",
		     "Shape options:",
		     "Options for the shaping process",
		     this);

  const gchar *features_help = "Comma-separated list of font features\n"
    "\n"
    "    Features can be enabled or disabled, either globally or limited to\n"
    "    specific character ranges.  The format for specifying feature settings\n"
    "    follows.  All valid CSS font-feature-settings values other than 'normal'\n"
    "    and the global values are also accepted, though not documented below.\n"
    "    CSS string escapes are not supported."
    "\n"
    "    The range indices refer to the positions between Unicode characters,\n"
    "    unless the --utf8-clusters is provided, in which case range indices\n"
    "    refer to UTF-8 byte indices. The position before the first character\n"
    "    is always 0.\n"
    "\n"
    "    The format is Python-esque.  Here is how it all works:\n"
    "\n"
    "      Syntax:       Value:    Start:    End:\n"
    "\n"
    "    Setting value:\n"
    "      \"kern\"        1         0         ∞         # Turn feature on\n"
    "      \"+kern\"       1         0         ∞         # Turn feature on\n"
    "      \"-kern\"       0         0         ∞         # Turn feature off\n"
    "      \"kern=0\"      0         0         ∞         # Turn feature off\n"
    "      \"kern=1\"      1         0         ∞         # Turn feature on\n"
    "      \"aalt=2\"      2         0         ∞         # Choose 2nd alternate\n"
    "\n"
    "    Setting index:\n"
    "      \"kern[]\"      1         0         ∞         # Turn feature on\n"
    "      \"kern[:]\"     1         0         ∞         # Turn feature on\n"
    "      \"kern[5:]\"    1         5         ∞         # Turn feature on, partial\n"
    "      \"kern[:5]\"    1         0         5         # Turn feature on, partial\n"
    "      \"kern[3:5]\"   1         3         5         # Turn feature on, range\n"
    "      \"kern[3]\"     1         3         3+1       # Turn feature on, single char\n"
    "\n"
    "    Mixing it all:\n"
    "\n"
    "      \"aalt[3:5]=2\" 2         3         5         # Turn 2nd alternate on for range";

  GOptionEntry entries2[] =
  {
    {"features",	0, 0, G_OPTION_ARG_CALLBACK,	(gpointer) &parse_features,	features_help,	"list"},
    {nullptr}
  };
  parser->add_group (entries2,
		     "features",
		     "Features options:",
		     "Options for font features used",
		     this);
}

#endif
