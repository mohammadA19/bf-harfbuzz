/*
 * Copyright © 2009  Red Hat, Inc.
 * Copyright © 2012  Google, Inc.
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
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#include "hb.hh"

#include "hb-shaper.hh"
#include "hb-shape-plan.hh"
#include "hb-buffer.hh"
#include "hb-font.hh"
#include "hb-machinery.hh"


#ifndef HB_NO_SHAPER

/**
 * SECTION:hb-shape
 * @title: hb-shape
 * @short_description: Conversion of text strings into positioned glyphs
 * @include: hb.h
 *
 * Shaping is the central operation of HarfBuzz. Shaping operates on buffers,
 * which are sequences of Unicode characters that use the same font and have
 * the same text direction, script, and language. After shaping the buffer
 * contains the output glyphs and their positions.
 **/


static inline void free_static_shaper_list ();

static const char * const nil_shaper_list[] = {nullptr};

static struct shaper_list_lazy_loader_t : lazy_loader_t<const char *,
							      shaper_list_lazy_loader_t>
{
  static const char ** create ()
  {
    const char **shaper_list = (const char **) calloc (1 + HB_SHAPERS_COUNT, sizeof (const char *));
    if (unlikely (!shaper_list))
      return nullptr;

    const shaper_entry_t *shapers = _shapers_get ();
    unsigned int i;
    for (i = 0; i < HB_SHAPERS_COUNT; i++)
      shaper_list[i] = shapers[i].name;
    shaper_list[i] = nullptr;

    atexit (free_static_shaper_list);

    return shaper_list;
  }
  static void destroy (const char **l)
  { free (l); }
  static const char * const * get_null ()
  { return nil_shaper_list; }
} static_shaper_list;

static inline
void free_static_shaper_list ()
{
  static_shaper_list.free_instance ();
}


/**
 * shape_list_shapers:
 *
 * Retrieves the list of shapers supported by HarfBuzz.
 *
 * Return value: (transfer none) (array zero-terminated=1): an array of
 *    constant strings
 *
 * Since: 0.9.2
 **/
const char **
shape_list_shapers ()
{
  return static_shaper_list.get_unconst ();
}


/**
 * shape_full:
 * @font: an #font_t to use for shaping
 * @buffer: an #buffer_t to shape
 * @features: (array length=num_features) (nullable): an array of user
 *    specified #feature_t or `NULL`
 * @num_features: the length of @features array
 * @shaper_list: (array zero-terminated=1) (nullable): a `NULL`-terminated
 *    array of shapers to use or `NULL`
 *
 * See shape() for details. If @shaper_list is not `NULL`, the specified
 * shapers will be used in the given order, otherwise the default shapers list
 * will be used.
 *
 * Return value: false if all shapers failed, true otherwise
 *
 * Since: 0.9.2
 **/
bool_t
shape_full (font_t          *font,
	       buffer_t        *buffer,
	       const feature_t *features,
	       unsigned int        num_features,
	       const char * const *shaper_list)
{
  if (unlikely (!buffer->len))
    return true;

  buffer->enter ();

  buffer_t *text_buffer = nullptr;
  if (buffer->flags & HB_BUFFER_FLAG_VERIFY)
  {
    text_buffer = buffer_create ();
    buffer_append (text_buffer, buffer, 0, -1);
  }

  shape_plan_t *shape_plan = shape_plan_create_cached2 (font->face, &buffer->props,
							      features, num_features,
							      font->coords, font->num_coords,
							      shaper_list);

  bool_t res = shape_plan_execute (shape_plan, font, buffer, features, num_features);

  if (buffer->max_ops <= 0)
    buffer->shaping_failed = true;

  shape_plan_destroy (shape_plan);

  if (text_buffer)
  {
    if (res && buffer->successful && !buffer->shaping_failed
	    && text_buffer->successful
	    && !buffer->verify (text_buffer,
				font,
				features,
				num_features,
				shaper_list))
      res = false;
    buffer_destroy (text_buffer);
  }

  buffer->leave ();

  return res;
}

/**
 * shape:
 * @font: an #font_t to use for shaping
 * @buffer: an #buffer_t to shape
 * @features: (array length=num_features) (nullable): an array of user
 *    specified #feature_t or `NULL`
 * @num_features: the length of @features array
 *
 * Shapes @buffer using @font turning its Unicode characters content to
 * positioned glyphs. If @features is not `NULL`, it will be used to control the
 * features applied during shaping. If two @features have the same tag but
 * overlapping ranges the value of the feature with the higher index takes
 * precedence.
 *
 * Since: 0.9.2
 **/
void
shape (font_t           *font,
	  buffer_t         *buffer,
	  const feature_t  *features,
	  unsigned int         num_features)
{
  shape_full (font, buffer, features, num_features, nullptr);
}


#ifdef HB_EXPERIMENTAL_API

static float
buffer_advance (buffer_t *buffer)
{
  float a = 0;
  auto *pos = buffer->pos;
  unsigned count = buffer->len;
  if (HB_DIRECTION_IS_HORIZONTAL (buffer->props.direction))
    for (unsigned i = 0; i < count; i++)
      a += pos[i].x_advance;
  else
    for (unsigned i = 0; i < count; i++)
      a += pos[i].y_advance;
  return a;
}

static void
reset_buffer (buffer_t *buffer,
	      array_t<const glyph_info_t> text)
{
  assert (buffer->ensure (text.length));
  buffer->have_positions = false;
  buffer->len = text.length;
  memcpy (buffer->info, text.arrayZ, text.length * sizeof (buffer->info[0]));
  buffer_set_content_type (buffer, HB_BUFFER_CONTENT_TYPE_UNICODE);
}

/**
 * shape_justify:
 * @font: a mutable #font_t to use for shaping
 * @buffer: an #buffer_t to shape
 * @features: (array length=num_features) (nullable): an array of user
 *    specified #feature_t or `NULL`
 * @num_features: the length of @features array
 * @shaper_list: (array zero-terminated=1) (nullable): a `NULL`-terminated
 *    array of shapers to use or `NULL`
 * @min_target_advance: Minimum advance width/height to aim for.
 * @max_target_advance: Maximum advance width/height to aim for.
 * @advance: (inout): Input/output advance width/height of the buffer.
 * @var_tag: (out): Variation-axis tag used for justification.
 * @var_value: (out): Variation-axis value used to reach target justification.
 *
 * See shape_full() for basic details. If @shaper_list is not `NULL`, the specified
 * shapers will be used in the given order, otherwise the default shapers list
 * will be used.
 *
 * In addition, justify the shaping results such that the shaping results reach
 * the target advance width/height, depending on the buffer direction.
 *
 * If the advance of the buffer shaped with shape_full() is already known,
 * put that in *advance. Otherwise set *advance to zero.
 *
 * This API is currently experimental and will probably change in the future.
 *
 * Return value: false if all shapers failed, true otherwise
 *
 * XSince: EXPERIMENTAL
 **/
bool_t
shape_justify (font_t          *font,
		  buffer_t        *buffer,
		  const feature_t *features,
		  unsigned int        num_features,
		  const char * const *shaper_list,
		  float               min_target_advance,
		  float               max_target_advance,
		  float              *advance, /* IN/OUT */
		  tag_t           *var_tag, /* OUT */
		  float              *var_value /* OUT */)
{
  // TODO Negative font scales?

  /* If default advance already matches target, nothing to do. Shape and return. */
  if (min_target_advance <= *advance && *advance <= max_target_advance)
  {
    *var_tag = HB_TAG_NONE;
    *var_value = 0.0f;
    return shape_full (font, buffer,
			  features, num_features,
			  shaper_list);
  }

  face_t *face = font->face;

  /* Choose variation tag to use for justification. */

  tag_t tag = HB_TAG_NONE;
  ot_var_axis_info_t axis_info;

  tag_t tags[] =
  {
    HB_TAG ('j','s','t','f'),
    HB_TAG ('w','d','t','h'),
  };
  for (unsigned i = 0; i < ARRAY_LENGTH (tags); i++)
    if (ot_var_find_axis_info (face, tags[i], &axis_info))
    {
      tag = *var_tag = tags[i];
      break;
    }

  /* If no suitable variation axis found, can't justify.  Just shape and return. */
  if (!tag)
  {
    *var_tag = HB_TAG_NONE;
    *var_value = 0.0f;
    if (shape_full (font, buffer,
		       features, num_features,
		       shaper_list))
    {
      *advance = buffer_advance (buffer);
      return true;
    }
    else
      return false;
  }

  /* Copy buffer text as we need it so we can shape multiple times. */
  unsigned text_len = buffer->len;
  auto *text_info = (glyph_info_t *) malloc (text_len * sizeof (buffer->info[0]));
  if (unlikely (text_len && !text_info))
    return false;
  memcpy (text_info, buffer->info, text_len * sizeof (buffer->info[0]));
  auto text = array<const glyph_info_t> (text_info, text_len);

  /* If default advance was not provided to us, calculate it. */
  if (!*advance)
  {
    font_set_variation (font, tag, axis_info.default_value);
    if (!shape_full (font, buffer,
			features, num_features,
			shaper_list))
      return false;
    *advance = buffer_advance (buffer);
  }

  /* If default advance already matches target, nothing to do. Shape and return.
   * Do this again, in case advance was just calculated.
   */
  if (min_target_advance <= *advance && *advance <= max_target_advance)
  {
    *var_tag = HB_TAG_NONE;
    *var_value = 0.0f;
    return true;
  }

  /* Prepare for running the solver. */
  double a, b, ya, yb;
  if (*advance < min_target_advance)
  {
    /* Need to expand. */
    ya = (double) *advance;
    a = (double) axis_info.default_value;
    b = (double) axis_info.max_value;

    /* Shape buffer for maximum expansion to use as other
     * starting point for the solver. */
    font_set_variation (font, tag, (float) b);
    reset_buffer (buffer, text);
    if (!shape_full (font, buffer,
			features, num_features,
			shaper_list))
      return false;
    yb = (double) buffer_advance (buffer);
    /* If the maximum expansion is less than max target,
     * there's nothing to solve for. Just return it. */
    if (yb <= (double) max_target_advance)
    {
      *var_value = (float) b;
      *advance = (float) yb;
      return true;
    }
  }
  else
  {
    /* Need to shrink. */
    yb = (double) *advance;
    a = (double) axis_info.min_value;
    b = (double) axis_info.default_value;

    /* Shape buffer for maximum shrinkate to use as other
     * starting point for the solver. */
    font_set_variation (font, tag, (float) a);
    reset_buffer (buffer, text);
    if (!shape_full (font, buffer,
			features, num_features,
			shaper_list))
      return false;
    ya = (double) buffer_advance (buffer);
    /* If the maximum shrinkate is more than min target,
     * there's nothing to solve for. Just return it. */
    if (ya >= (double) min_target_advance)
    {
      *var_value = (float) a;
      *advance = (float) ya;
      return true;
    }
  }

  /* Run the solver to find a var axis value that hits
   * the desired width. */

  double epsilon = (b - a) / (1<<14);
  bool failed = false;

  auto f = [&] (double x)
  {
    font_set_variation (font, tag, (float) x);
    reset_buffer (buffer, text);
    if (unlikely (!shape_full (font, buffer,
				  features, num_features,
				  shaper_list)))
    {
      failed = true;
      return (double) min_target_advance;
    }

    double w = (double) buffer_advance (buffer);
    DEBUG_MSG (JUSTIFY, nullptr, "Trying '%c%c%c%c' axis parameter %f. Advance %g. Target: min %g max %g",
	       HB_UNTAG (tag), x, w,
	       (double) min_target_advance, (double) max_target_advance);
    return w;
  };

  double y = 0;
  double itp = solve_itp (f,
			  a, b,
			  epsilon,
			  (double) min_target_advance, (double) max_target_advance,
			  ya, yb, y);

  free (text_info);

  if (failed)
    return false;

  *var_value = (float) itp;
  *advance = (float) y;

  return true;
}

#endif


#endif
