/*
 * Copyright © 2022  Red Hat, Inc.
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
 * Red Hat Author(s): Matthias Clasen
 */

#include "hb.hh"

#ifdef HAVE_CAIRO

#include "hb-cairo.h"

#include "hb-cairo-utils.hh"

#include "hb-machinery.hh"
#include "hb-utf.hh"


/**
 * SECTION:hb-cairo
 * @title: hb-cairo
 * @short_description: Cairo integration
 * @include: hb-cairo.h
 *
 * Functions for using HarfBuzz with the cairo library.
 *
 * HarfBuzz supports using cairo for rendering.
 **/

static void
cairo_move_to (draw_funcs_t *dfuncs HB_UNUSED,
		  void *draw_data,
		  draw_state_t *st HB_UNUSED,
		  float to_x, float to_y,
		  void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) draw_data;

  cairo_move_to (cr, (double) to_x, (double) to_y);
}

static void
cairo_line_to (draw_funcs_t *dfuncs HB_UNUSED,
		  void *draw_data,
		  draw_state_t *st HB_UNUSED,
		  float to_x, float to_y,
		  void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) draw_data;

  cairo_line_to (cr, (double) to_x, (double) to_y);
}

static void
cairo_cubic_to (draw_funcs_t *dfuncs HB_UNUSED,
		   void *draw_data,
		   draw_state_t *st HB_UNUSED,
		   float control1_x, float control1_y,
		   float control2_x, float control2_y,
		   float to_x, float to_y,
		   void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) draw_data;

  cairo_curve_to (cr,
                  (double) control1_x, (double) control1_y,
                  (double) control2_x, (double) control2_y,
                  (double) to_x, (double) to_y);
}

static void
cairo_close_path (draw_funcs_t *dfuncs HB_UNUSED,
		     void *draw_data,
		     draw_state_t *st HB_UNUSED,
		     void *user_data HB_UNUSED)
{
  cairo_t *cr = (cairo_t *) draw_data;

  cairo_close_path (cr);
}

static inline void free_static_cairo_draw_funcs ();

static struct cairo_draw_funcs_lazy_loader_t : draw_funcs_lazy_loader_t<cairo_draw_funcs_lazy_loader_t>
{
  static draw_funcs_t *create ()
  {
    draw_funcs_t *funcs = draw_funcs_create ();

    draw_funcs_set_move_to_func (funcs, cairo_move_to, nullptr, nullptr);
    draw_funcs_set_line_to_func (funcs, cairo_line_to, nullptr, nullptr);
    draw_funcs_set_cubic_to_func (funcs, cairo_cubic_to, nullptr, nullptr);
    draw_funcs_set_close_path_func (funcs, cairo_close_path, nullptr, nullptr);

    draw_funcs_make_immutable (funcs);

    atexit (free_static_cairo_draw_funcs);

    return funcs;
  }
} static_cairo_draw_funcs;

static inline
void free_static_cairo_draw_funcs ()
{
  static_cairo_draw_funcs.free_instance ();
}

static draw_funcs_t *
cairo_draw_get_funcs ()
{
  return static_cairo_draw_funcs.get_unconst ();
}


#ifdef HAVE_CAIRO_USER_FONT_FACE_SET_RENDER_COLOR_GLYPH_FUNC

static void
cairo_push_transform (paint_funcs_t *pfuncs HB_UNUSED,
			 void *paint_data,
			 float xx, float yx,
			 float xy, float yy,
			 float dx, float dy,
			 void *user_data HB_UNUSED)
{
  cairo_context_t *c = (cairo_context_t *) paint_data;
  cairo_t *cr = c->cr;

  cairo_matrix_t m;

  cairo_save (cr);
  cairo_matrix_init (&m, (double) xx, (double) yx,
                         (double) xy, (double) yy,
                         (double) dx, (double) dy);
  cairo_transform (cr, &m);
}

static void
cairo_pop_transform (paint_funcs_t *pfuncs HB_UNUSED,
		        void *paint_data,
		        void *user_data HB_UNUSED)
{
  cairo_context_t *c = (cairo_context_t *) paint_data;
  cairo_t *cr = c->cr;

  cairo_restore (cr);
}

static bool_t
cairo_paint_color_glyph (paint_funcs_t *pfuncs HB_UNUSED,
			    void *paint_data,
			    codepoint_t glyph,
			    font_t *font,
			    void *user_data HB_UNUSED)
{
  cairo_context_t *c = (cairo_context_t *) paint_data;
  cairo_t *cr = c->cr;

  cairo_save (cr);

  position_t x_scale, y_scale;
  font_get_scale (font, &x_scale, &y_scale);
  cairo_scale (cr, x_scale, y_scale);

  cairo_glyph_t cairo_glyph = { glyph, 0, 0 };
  cairo_set_scaled_font (cr, c->scaled_font);
  cairo_set_font_size (cr, 1);
  cairo_show_glyphs (cr, &cairo_glyph, 1);

  cairo_restore (cr);

  return true;
}

static void
cairo_push_clip_glyph (paint_funcs_t *pfuncs HB_UNUSED,
			  void *paint_data,
			  codepoint_t glyph,
			  font_t *font,
			  void *user_data HB_UNUSED)
{
  cairo_context_t *c = (cairo_context_t *) paint_data;
  cairo_t *cr = c->cr;

  cairo_save (cr);
  cairo_new_path (cr);
  font_draw_glyph (font, glyph, cairo_draw_get_funcs (), cr);
  cairo_close_path (cr);
  cairo_clip (cr);
}

static void
cairo_push_clip_rectangle (paint_funcs_t *pfuncs HB_UNUSED,
			      void *paint_data,
			      float xmin, float ymin, float xmax, float ymax,
			      void *user_data HB_UNUSED)
{
  cairo_context_t *c = (cairo_context_t *) paint_data;
  cairo_t *cr = c->cr;

  cairo_save (cr);
  cairo_rectangle (cr,
                   (double) xmin, (double) ymin,
                   (double) (xmax - xmin), (double) (ymax - ymin));
  cairo_clip (cr);
}

static void
cairo_pop_clip (paint_funcs_t *pfuncs HB_UNUSED,
		   void *paint_data,
		   void *user_data HB_UNUSED)
{
  cairo_context_t *c = (cairo_context_t *) paint_data;
  cairo_t *cr = c->cr;

  cairo_restore (cr);
}

static void
cairo_push_group (paint_funcs_t *pfuncs HB_UNUSED,
		     void *paint_data,
		     void *user_data HB_UNUSED)
{
  cairo_context_t *c = (cairo_context_t *) paint_data;
  cairo_t *cr = c->cr;

  cairo_save (cr);
  cairo_push_group (cr);
}

static void
cairo_pop_group (paint_funcs_t *pfuncs HB_UNUSED,
		    void *paint_data,
		    paint_composite_mode_t mode,
		    void *user_data HB_UNUSED)
{
  cairo_context_t *c = (cairo_context_t *) paint_data;
  cairo_t *cr = c->cr;

  cairo_pop_group_to_source (cr);
  cairo_set_operator (cr, _paint_composite_mode_to_cairo (mode));
  cairo_paint (cr);

  cairo_restore (cr);
}

static void
cairo_paint_color (paint_funcs_t *pfuncs HB_UNUSED,
		      void *paint_data,
		      bool_t use_foreground,
		      color_t color,
		      void *user_data HB_UNUSED)
{
  cairo_context_t *c = (cairo_context_t *) paint_data;
  cairo_t *cr = c->cr;

  if (use_foreground)
  {
#ifdef HAVE_CAIRO_USER_SCALED_FONT_GET_FOREGROUND_SOURCE
    double r, g, b, a;
    cairo_pattern_t *foreground = cairo_user_scaled_font_get_foreground_source (c->scaled_font);
    if (cairo_pattern_get_rgba (foreground, &r, &g, &b, &a) == CAIRO_STATUS_SUCCESS)
      cairo_set_source_rgba (cr, r, g, b, a * color_get_alpha (color) / 255.);
    else
#endif
      cairo_set_source_rgba (cr, 0, 0, 0, color_get_alpha (color) / 255.);
  }
  else
    cairo_set_source_rgba (cr,
			   color_get_red (color) / 255.,
			   color_get_green (color) / 255.,
			   color_get_blue (color) / 255.,
			   color_get_alpha (color) / 255.);
  cairo_paint (cr);
}

static bool_t
cairo_paint_image (paint_funcs_t *pfuncs HB_UNUSED,
		      void *paint_data,
		      blob_t *blob,
		      unsigned width,
		      unsigned height,
		      tag_t format,
		      float slant,
		      glyph_extents_t *extents,
		      void *user_data HB_UNUSED)
{
  cairo_context_t *c = (cairo_context_t *) paint_data;

  return _cairo_paint_glyph_image (c, blob, width, height, format, slant, extents);
}

static void
cairo_paint_linear_gradient (paint_funcs_t *pfuncs HB_UNUSED,
				void *paint_data,
				color_line_t *color_line,
				float x0, float y0,
				float x1, float y1,
				float x2, float y2,
				void *user_data HB_UNUSED)
{
  cairo_context_t *c = (cairo_context_t *) paint_data;

  _cairo_paint_linear_gradient (c, color_line, x0, y0, x1, y1, x2, y2);
}

static void
cairo_paint_radial_gradient (paint_funcs_t *pfuncs HB_UNUSED,
				void *paint_data,
				color_line_t *color_line,
				float x0, float y0, float r0,
				float x1, float y1, float r1,
				void *user_data HB_UNUSED)
{
  cairo_context_t *c = (cairo_context_t *) paint_data;

  _cairo_paint_radial_gradient (c, color_line, x0, y0, r0, x1, y1, r1);
}

static void
cairo_paint_sweep_gradient (paint_funcs_t *pfuncs HB_UNUSED,
			       void *paint_data,
			       color_line_t *color_line,
			       float x0, float y0,
			       float start_angle, float end_angle,
			       void *user_data HB_UNUSED)
{
  cairo_context_t *c = (cairo_context_t *) paint_data;

  _cairo_paint_sweep_gradient (c, color_line, x0, y0, start_angle, end_angle);
}

static const cairo_user_data_key_t color_cache_key = {0};

static void
_cairo_destroy_map (void *p)
{
  map_destroy ((map_t *) p);
}

static bool_t
cairo_paint_custom_palette_color (paint_funcs_t *funcs,
                                     void *paint_data,
                                     unsigned int color_index,
                                     color_t *color,
                                     void *user_data HB_UNUSED)
{
#ifdef HAVE_CAIRO_FONT_OPTIONS_GET_CUSTOM_PALETTE_COLOR
  cairo_context_t *c = (cairo_context_t *) paint_data;
  cairo_t *cr = c->cr;

#define HB_DEADBEEF HB_TAG(0xDE,0xAD,0xBE,0xEF)

  map_t *color_cache = c->color_cache;
  codepoint_t *v;
  if (likely (color_cache && color_cache->has (color_index, &v)))
  {
    if (*v == HB_DEADBEEF)
      return false;
    *color = *v;
    return true;
  }

  cairo_font_options_t *options;
  double red, green, blue, alpha;

  options = cairo_font_options_create ();
  cairo_get_font_options (cr, options);
  if (CAIRO_STATUS_SUCCESS ==
      cairo_font_options_get_custom_palette_color (options, color_index,
                                                   &red, &green, &blue, &alpha))
  {
    cairo_font_options_destroy (options);
    *color = HB_COLOR (round (255 * blue),
		       round (255 * green),
		       round (255 * red),
		       round (255 * alpha));

    if (likely (color_cache && *color != HB_DEADBEEF))
      color_cache->set (color_index, *color);

    return true;
  }
  cairo_font_options_destroy (options);

  if (likely (color_cache))
    color_cache->set (color_index, HB_DEADBEEF);

#undef HB_DEADBEEF

#endif

  return false;
}

static inline void free_static_cairo_paint_funcs ();

static struct cairo_paint_funcs_lazy_loader_t : paint_funcs_lazy_loader_t<cairo_paint_funcs_lazy_loader_t>
{
  static paint_funcs_t *create ()
  {
    paint_funcs_t *funcs = paint_funcs_create ();

    paint_funcs_set_push_transform_func (funcs, cairo_push_transform, nullptr, nullptr);
    paint_funcs_set_pop_transform_func (funcs, cairo_pop_transform, nullptr, nullptr);
    paint_funcs_set_color_glyph_func (funcs, cairo_paint_color_glyph, nullptr, nullptr);
    paint_funcs_set_push_clip_glyph_func (funcs, cairo_push_clip_glyph, nullptr, nullptr);
    paint_funcs_set_push_clip_rectangle_func (funcs, cairo_push_clip_rectangle, nullptr, nullptr);
    paint_funcs_set_pop_clip_func (funcs, cairo_pop_clip, nullptr, nullptr);
    paint_funcs_set_push_group_func (funcs, cairo_push_group, nullptr, nullptr);
    paint_funcs_set_pop_group_func (funcs, cairo_pop_group, nullptr, nullptr);
    paint_funcs_set_color_func (funcs, cairo_paint_color, nullptr, nullptr);
    paint_funcs_set_image_func (funcs, cairo_paint_image, nullptr, nullptr);
    paint_funcs_set_linear_gradient_func (funcs, cairo_paint_linear_gradient, nullptr, nullptr);
    paint_funcs_set_radial_gradient_func (funcs, cairo_paint_radial_gradient, nullptr, nullptr);
    paint_funcs_set_sweep_gradient_func (funcs, cairo_paint_sweep_gradient, nullptr, nullptr);
    paint_funcs_set_custom_palette_color_func (funcs, cairo_paint_custom_palette_color, nullptr, nullptr);

    paint_funcs_make_immutable (funcs);

    atexit (free_static_cairo_paint_funcs);

    return funcs;
  }
} static_cairo_paint_funcs;

static inline
void free_static_cairo_paint_funcs ()
{
  static_cairo_paint_funcs.free_instance ();
}

static paint_funcs_t *
cairo_paint_get_funcs ()
{
  return static_cairo_paint_funcs.get_unconst ();
}
#endif

static const cairo_user_data_key_t cairo_face_user_data_key = {0};
static const cairo_user_data_key_t cairo_font_user_data_key = {0};
static const cairo_user_data_key_t cairo_font_init_func_user_data_key = {0};
static const cairo_user_data_key_t cairo_font_init_user_data_user_data_key = {0};
static const cairo_user_data_key_t cairo_scale_factor_user_data_key = {0};

static void cairo_face_destroy (void *p) { face_destroy ((face_t *) p); }
static void cairo_font_destroy (void *p) { font_destroy ((font_t *) p); }

static cairo_status_t
cairo_init_scaled_font (cairo_scaled_font_t  *scaled_font,
			   cairo_t              *cr HB_UNUSED,
			   cairo_font_extents_t *extents)
{
  cairo_font_face_t *font_face = cairo_scaled_font_get_font_face (scaled_font);

  font_t *font = (font_t *) cairo_font_face_get_user_data (font_face,
								 &cairo_font_user_data_key);

  if (!font)
  {
    face_t *face = (face_t *) cairo_font_face_get_user_data (font_face,
								   &cairo_face_user_data_key);
    font = font_create (face);

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1,16,0)
    cairo_font_options_t *font_options = cairo_font_options_create ();

    // Set variations
    cairo_scaled_font_get_font_options (scaled_font, font_options);
    const char *variations = cairo_font_options_get_variations (font_options);
    vector_t<variation_t> vars;
    const char *p = variations;
    while (p && *p)
    {
      const char *end = strpbrk ((char *) p, ", ");
      variation_t var;
      if (variation_from_string (p, end ? end - p : -1, &var))
	vars.push (var);
      p = end ? end + 1 : nullptr;
    }
    font_set_variations (font, &vars[0], vars.length);

    cairo_font_options_destroy (font_options);
#endif

    // Set scale; Note: should NOT set slant, or we'll double-slant.
    unsigned scale_factor = cairo_font_face_get_scale_factor (font_face);
    if (scale_factor)
    {
      cairo_matrix_t font_matrix;
      cairo_scaled_font_get_scale_matrix (scaled_font, &font_matrix);
      font_set_scale (font,
			 round (font_matrix.xx * scale_factor),
			 round (font_matrix.yy * scale_factor));
    }

    auto *init_func = (cairo_font_init_func_t)
		      cairo_font_face_get_user_data (font_face,
						     &cairo_font_init_func_user_data_key);
    if (init_func)
    {
      void *user_data = cairo_font_face_get_user_data (font_face,
						       &cairo_font_init_user_data_user_data_key);
      font = init_func (font, scaled_font, user_data);
    }

    font_make_immutable (font);
  }

  cairo_scaled_font_set_user_data (scaled_font,
				   &cairo_font_user_data_key,
				   (void *) font_reference (font),
				   cairo_font_destroy);

  position_t x_scale, y_scale;
  font_get_scale (font, &x_scale, &y_scale);

  font_extents_t extents;
  font_get_h_extents (font, &extents);

  extents->ascent  = (double)  extents.ascender  / y_scale;
  extents->descent = (double) -extents.descender / y_scale;
  extents->height  = extents->ascent + extents->descent;

#ifdef HAVE_CAIRO_USER_FONT_FACE_SET_RENDER_COLOR_GLYPH_FUNC
  map_t *color_cache = map_create ();
  if (unlikely (CAIRO_STATUS_SUCCESS != cairo_scaled_font_set_user_data (scaled_font,
									 &color_cache_key,
									 color_cache,
									 _cairo_destroy_map)))
    map_destroy (color_cache);
#endif

  return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
cairo_text_to_glyphs (cairo_scaled_font_t        *scaled_font,
			 const char	            *utf8,
			 int		             utf8_len,
			 cairo_glyph_t	           **glyphs,
			 int		            *num_glyphs,
			 cairo_text_cluster_t      **clusters,
			 int		            *num_clusters,
			 cairo_text_cluster_flags_t *cluster_flags)
{
  font_t *font = (font_t *) cairo_scaled_font_get_user_data (scaled_font,
								   &cairo_font_user_data_key);

  buffer_t *buffer = buffer_create ();
  buffer_add_utf8 (buffer, utf8, utf8_len, 0, utf8_len);
  buffer_guess_segment_properties (buffer);
  shape (font, buffer, nullptr, 0);

  cairo_glyphs_from_buffer (buffer,
			       true,
			       font->x_scale, font->y_scale,
			       0., 0.,
			       utf8, utf8_len,
			       glyphs, (unsigned *) num_glyphs,
			       clusters, (unsigned *) num_clusters,
			       cluster_flags);

  buffer_destroy (buffer);

  return CAIRO_STATUS_SUCCESS;
}

static cairo_status_t
cairo_render_glyph (cairo_scaled_font_t  *scaled_font,
		       unsigned long         glyph,
		       cairo_t              *cr,
		       cairo_text_extents_t *extents)
{
  font_t *font = (font_t *) cairo_scaled_font_get_user_data (scaled_font,
								   &cairo_font_user_data_key);

  position_t x_scale, y_scale;
  font_get_scale (font, &x_scale, &y_scale);
  cairo_scale (cr, +1./x_scale, -1./y_scale);

  font_draw_glyph (font, glyph, cairo_draw_get_funcs (), cr);

  cairo_fill (cr);

  return CAIRO_STATUS_SUCCESS;
}

#ifdef HAVE_CAIRO_USER_FONT_FACE_SET_RENDER_COLOR_GLYPH_FUNC

static cairo_status_t
cairo_render_color_glyph (cairo_scaled_font_t  *scaled_font,
			     unsigned long         glyph,
			     cairo_t              *cr,
			     cairo_text_extents_t *extents)
{
  font_t *font = (font_t *) cairo_scaled_font_get_user_data (scaled_font,
								   &cairo_font_user_data_key);

  unsigned int palette = 0;
#ifdef CAIRO_COLOR_PALETTE_DEFAULT
  cairo_font_options_t *options = cairo_font_options_create ();
  cairo_scaled_font_get_font_options (scaled_font, options);
  palette = cairo_font_options_get_color_palette (options);
  cairo_font_options_destroy (options);
#endif

  color_t color = HB_COLOR (0, 0, 0, 255);
  position_t x_scale, y_scale;
  font_get_scale (font, &x_scale, &y_scale);
  cairo_scale (cr, +1./x_scale, -1./y_scale);

  cairo_context_t c;
  c.scaled_font = scaled_font;
  c.cr = cr;
  c.color_cache = (map_t *) cairo_scaled_font_get_user_data (scaled_font, &color_cache_key);

  font_paint_glyph (font, glyph, cairo_paint_get_funcs (), &c, palette, color);


  return CAIRO_STATUS_SUCCESS;
}

#endif

static cairo_font_face_t *
user_font_face_create (face_t *face)
{
  cairo_font_face_t *cairo_face;

  cairo_face = cairo_user_font_face_create ();
  cairo_user_font_face_set_init_func (cairo_face, cairo_init_scaled_font);
  cairo_user_font_face_set_text_to_glyphs_func (cairo_face, cairo_text_to_glyphs);
  cairo_user_font_face_set_render_glyph_func (cairo_face, cairo_render_glyph);
#ifdef HAVE_CAIRO_USER_FONT_FACE_SET_RENDER_COLOR_GLYPH_FUNC
  if (ot_color_has_png (face) || ot_color_has_layers (face) || ot_color_has_paint (face))
    cairo_user_font_face_set_render_color_glyph_func (cairo_face, cairo_render_color_glyph);
#endif

  if (unlikely (CAIRO_STATUS_SUCCESS != cairo_font_face_set_user_data (cairo_face,
								       &cairo_face_user_data_key,
								       (void *) face_reference (face),
								       cairo_face_destroy)))
    face_destroy (face);

  return cairo_face;
}

/**
 * cairo_font_face_create_for_font:
 * @font: a #font_t
 *
 * Creates a #cairo_font_face_t for rendering text according
 * to @font.
 *
 * Note that the scale of @font does not affect the rendering,
 * but the variations and slant that are set on @font do.
 *
 * Returns: (transfer full): a newly created #cairo_font_face_t
 *
 * Since: 7.0.0
 */
cairo_font_face_t *
cairo_font_face_create_for_font (font_t *font)
{
  font_make_immutable (font);

  auto *cairo_face =  user_font_face_create (font->face);

  if (unlikely (CAIRO_STATUS_SUCCESS != cairo_font_face_set_user_data (cairo_face,
								       &cairo_font_user_data_key,
								       (void *) font_reference (font),
								       cairo_font_destroy)))
    font_destroy (font);

  return cairo_face;
}

/**
 * cairo_font_face_get_font:
 * @font_face: a #cairo_font_face_t
 *
 * Gets the #font_t that @font_face was created from.
 *
 * Returns: (nullable) (transfer none): the #font_t that @font_face was created from
 *
 * Since: 7.0.0
 */
font_t *
cairo_font_face_get_font (cairo_font_face_t *font_face)
{
  return (font_t *) cairo_font_face_get_user_data (font_face,
						      &cairo_font_user_data_key);
}

/**
 * cairo_font_face_create_for_face:
 * @face: a #face_t
 *
 * Creates a #cairo_font_face_t for rendering text according
 * to @face.
 *
 * Returns: (transfer full): a newly created #cairo_font_face_t
 *
 * Since: 7.0.0
 */
cairo_font_face_t *
cairo_font_face_create_for_face (face_t *face)
{
  face_make_immutable (face);

  return user_font_face_create (face);
}

/**
 * cairo_font_face_get_face:
 * @font_face: a #cairo_font_face_t
 *
 * Gets the #face_t associated with @font_face.
 *
 * Returns: (nullable) (transfer none): the #face_t associated with @font_face
 *
 * Since: 7.0.0
 */
face_t *
cairo_font_face_get_face (cairo_font_face_t *font_face)
{
  return (face_t *) cairo_font_face_get_user_data (font_face,
						      &cairo_face_user_data_key);
}

/**
 * cairo_font_face_set_font_init_func:
 * @font_face: a #cairo_font_face_t
 * @func: The virtual method to use
 * @user_data: user data accompanying the method
 * @destroy: function to call when @user_data is not needed anymore
 *
 * Set the virtual method to be called when a cairo
 * face created using cairo_font_face_create_for_face()
 * creates an #font_t for a #cairo_scaled_font_t.
 *
 * Since: 7.0.0
 */
void
cairo_font_face_set_font_init_func (cairo_font_face_t *font_face,
				       cairo_font_init_func_t func,
				       void *user_data,
				       destroy_func_t destroy)
{
  cairo_font_face_set_user_data (font_face,
				 &cairo_font_init_func_user_data_key,
				 (void *) func,
				 nullptr);
  if (unlikely (CAIRO_STATUS_SUCCESS != cairo_font_face_set_user_data (font_face,
								       &cairo_font_init_user_data_user_data_key,
								       (void *) user_data,
								       destroy)) && destroy)
  {
    destroy (user_data);
    cairo_font_face_set_user_data (font_face,
				   &cairo_font_init_func_user_data_key,
				   nullptr,
				   nullptr);
  }
}

/**
 * cairo_scaled_font_get_font:
 * @scaled_font: a #cairo_scaled_font_t
 *
 * Gets the #font_t associated with @scaled_font.
 *
 * Returns: (nullable) (transfer none): the #font_t associated with @scaled_font
 *
 * Since: 7.0.0
 */
font_t *
cairo_scaled_font_get_font (cairo_scaled_font_t *scaled_font)
{
  return (font_t *) cairo_scaled_font_get_user_data (scaled_font, &cairo_font_user_data_key);
}


/**
 * cairo_font_face_set_scale_factor:
 * @scale_factor: The scale factor to use. See below
 * @font_face: a #cairo_font_face_t
 *
 * Sets the scale factor of the @font_face. Default scale
 * factor is zero.
 *
 * When a #cairo_font_face_t is created from a #face_t using
 * cairo_font_face_create_for_face(), such face will create
 * #font_t objects during scaled-font creation.  The scale
 * factor defines how the scale set on such #font_t objects
 * relates to the font-matrix (as such font size) of the cairo
 * scaled-font.
 *
 * If the scale-factor is zero (default), then the scale of the
 * #font_t object will be left at default, which is the UPEM
 * value of the respective #face_t.
 *
 * If the scale-factor is set to non-zero, then the X and Y scale
 * of the #font_t object will be respectively set to the
 * @scale_factor times the xx and yy elements of the scale-matrix
 * of the cairo scaled-font being created.
 *
 * When using the cairo_glyphs_from_buffer() API to convert the
 * HarfBuzz glyph buffer that resulted from shaping with such a #font_t,
 * if the scale-factor was non-zero, you can pass it directly to
 * that API as both X and Y scale factors.
 *
 * If the scale-factor was zero however, or the cairo face was
 * created using the alternative constructor
 * cairo_font_face_create_for_font(), you need to calculate the
 * correct X/Y scale-factors to pass to cairo_glyphs_from_buffer()
 * by dividing the #font_t X/Y scale-factors by the
 * cairo scaled-font's scale-matrix XX/YY components respectively
 * and use those values.  Or if you know that relationship offhand
 * (because you set the scale of the #font_t yourself), use
 * the conversion rate involved.
 *
 * Since: 7.0.0
 */
void
cairo_font_face_set_scale_factor (cairo_font_face_t *font_face,
				     unsigned int scale_factor)
{
  cairo_font_face_set_user_data (font_face,
				 &cairo_scale_factor_user_data_key,
				 (void *) (uintptr_t) scale_factor,
				 nullptr);
}

/**
 * cairo_font_face_get_scale_factor:
 * @font_face: a #cairo_font_face_t
 *
 * Gets the scale factor set on the @font_face. Defaults to zero.
 * See cairo_font_face_set_scale_factor() for details.
 *
 * Returns: the scale factor of @font_face
 *
 * Since: 7.0.0
 */
unsigned int
cairo_font_face_get_scale_factor (cairo_font_face_t *font_face)
{
  return (unsigned int) (uintptr_t)
	 cairo_font_face_get_user_data (font_face,
					&cairo_scale_factor_user_data_key);
}


/**
 * cairo_glyphs_from_buffer:
 * @buffer: a #buffer_t containing glyphs
 * @utf8_clusters: `true` if @buffer clusters are in bytes, instead of characters
 * @x_scale_factor: scale factor to divide #position_t Y values by
 * @y_scale_factor: scale factor to divide #position_t X values by
 * @x: X position to place first glyph
 * @y: Y position to place first glyph
 * @utf8: (nullable): the text that was shaped in @buffer
 * @utf8_len: the length of @utf8 in bytes
 * @glyphs: (out): return location for an array of #cairo_glyph_t
 * @num_glyphs: (inout): return location for the length of @glyphs
 * @clusters: (out) (nullable): return location for an array of cluster positions
 * @num_clusters: (inout) (nullable): return location for the length of @clusters
 * @cluster_flags: (out) (nullable): return location for cluster flags
 *
 * Extracts information from @buffer in a form that can be
 * passed to cairo_show_text_glyphs() or cairo_show_glyphs().
 * This API is modeled after cairo_scaled_font_text_to_glyphs() and
 * cairo_user_scaled_font_text_to_glyphs_func_t.
 *
 * The @num_glyphs argument should be preset to the number of glyph entries available
 * in the @glyphs buffer. If the @glyphs buffer is `NULL`, the value of
 * @num_glyphs must be zero.  If the provided glyph array is too short for
 * the conversion (or for convenience), a new glyph array may be allocated
 * using cairo_glyph_allocate() and placed in @glyphs.  Upon return,
 * @num_glyphs should contain the number of generated glyphs.  If the value
 * @glyphs points at has changed after the call, the caller will free the
 * allocated glyph array using cairo_glyph_free().  The caller will also free
 * the original value of @glyphs, so this function shouldn't do so.
 *
 * If @clusters is not `NULL`, then @num_clusters and @cluster_flags
 * should not be either, and @utf8 must be provided, and cluster
 * mapping will be computed. The semantics of how
 * cluster array allocation works is similar to the glyph array.  That is,
 * if @clusters initially points to a non-`NULL` value, that array may be used
 * as a cluster buffer, and @num_clusters points to the number of cluster
 * entries available there.  If the provided cluster array is too short for
 * the conversion (or for convenience), a new cluster array may be allocated
 * using cairo_text_cluster_allocate() and placed in @clusters.  In this case,
 * the original value of @clusters will still be freed by the caller.  Upon
 * return, @num_clusters will contain the number of generated clusters.
 * If the value @clusters points at has changed after the call, the caller
 * will free the allocated cluster array using cairo_text_cluster_free().
 *
 * See cairo_font_face_set_scale_factor() for the details of
 * the @scale_factor argument.
 *
 * The returned @glyphs vector actually has `@num_glyphs + 1` entries in
 * it and the x,y values of the extra entry at the end add up the advance
 * x,y of all the glyphs in the @buffer.
 *
 * Since: 7.0.0
 */
void
cairo_glyphs_from_buffer (buffer_t *buffer,
			     bool_t utf8_clusters,
			     double x_scale_factor,
			     double y_scale_factor,
			     double x,
			     double y,
			     const char *utf8,
			     int utf8_len,
			     cairo_glyph_t **glyphs,
			     unsigned int *num_glyphs,
			     cairo_text_cluster_t **clusters,
			     unsigned int *num_clusters,
			     cairo_text_cluster_flags_t *cluster_flags)
{
  if (utf8 && utf8_len < 0)
    utf8_len = strlen (utf8);

  unsigned orig_num_glyphs = *num_glyphs;
  *num_glyphs = buffer_get_length (buffer);
  glyph_info_t *glyph = buffer_get_glyph_infos (buffer, nullptr);
  glyph_position_t *position = buffer_get_glyph_positions (buffer, nullptr);
  if (orig_num_glyphs < *num_glyphs + 1)
    *glyphs = cairo_glyph_allocate (*num_glyphs + 1);

  if (clusters && utf8)
  {
    unsigned orig_num_clusters = *num_clusters;
    *num_clusters = *num_glyphs ? 1 : 0;
    for (unsigned int i = 1; i < *num_glyphs; i++)
      if (glyph[i].cluster != glyph[i-1].cluster)
	(*num_clusters)++;
    if (orig_num_clusters < *num_clusters)
      *clusters = cairo_text_cluster_allocate (*num_clusters);
  }

  double x_scale = x_scale_factor ? 1. / x_scale_factor : 0.;
  double y_scale = y_scale_factor ? 1. / y_scale_factor : 0.;
  position_t hx = 0, hy = 0;
  int i;
  for (i = 0; i < (int) *num_glyphs; i++)
  {
    (*glyphs)[i].index = glyph[i].codepoint;
    (*glyphs)[i].x = x + (+position->x_offset + hx) * x_scale;
    (*glyphs)[i].y = y + (-position->y_offset + hy) * y_scale;
    hx +=  position->x_advance;
    hy += -position->y_advance;

    position++;
  }
  (*glyphs)[i].index = -1;
  (*glyphs)[i].x = round (hx * x_scale);
  (*glyphs)[i].y = round (hy * y_scale);

  if (clusters && *num_clusters && utf8)
  {
    memset ((void *) *clusters, 0, *num_clusters * sizeof ((*clusters)[0]));
    bool_t backward = HB_DIRECTION_IS_BACKWARD (buffer_get_direction (buffer));
    *cluster_flags = backward ? CAIRO_TEXT_CLUSTER_FLAG_BACKWARD : (cairo_text_cluster_flags_t) 0;
    unsigned int cluster = 0;
    const char *start = utf8, *end;
    (*clusters)[cluster].num_glyphs++;
    if (backward)
    {
      for (i = *num_glyphs - 2; i >= 0; i--)
      {
	if (glyph[i].cluster != glyph[i+1].cluster)
	{
	  assert (glyph[i].cluster > glyph[i+1].cluster);
	  if (utf8_clusters)
	    end = start + glyph[i].cluster - glyph[i+1].cluster;
	  else
	    end = (const char *) utf_offset_to_pointer<utf8_t> ((const uint8_t *) start,
								      (signed) (glyph[i].cluster - glyph[i+1].cluster));
	  (*clusters)[cluster].num_bytes = end - start;
	  start = end;
	  cluster++;
	}
	(*clusters)[cluster].num_glyphs++;
      }
      (*clusters)[cluster].num_bytes = utf8 + utf8_len - start;
    }
    else
    {
      for (i = 1; i < (int) *num_glyphs; i++)
      {
	if (glyph[i].cluster != glyph[i-1].cluster)
	{
	  assert (glyph[i].cluster > glyph[i-1].cluster);
	  if (utf8_clusters)
	    end = start + glyph[i].cluster - glyph[i-1].cluster;
	  else
	    end = (const char *) utf_offset_to_pointer<utf8_t> ((const uint8_t *) start,
								      (signed) (glyph[i].cluster - glyph[i-1].cluster));
	  (*clusters)[cluster].num_bytes = end - start;
	  start = end;
	  cluster++;
	}
	(*clusters)[cluster].num_glyphs++;
      }
      (*clusters)[cluster].num_bytes = utf8 + utf8_len - start;
    }
  }
  else if (num_clusters)
    *num_clusters = 0;
}

#endif
