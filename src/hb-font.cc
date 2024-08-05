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

#include "hb-font.hh"
#include "hb-draw.hh"
#include "hb-paint.hh"
#include "hb-machinery.hh"

#include "hb-ot.h"

#include "hb-ot-var-avar-table.hh"
#include "hb-ot-var-fvar-table.hh"


/**
 * SECTION:hb-font
 * @title: hb-font
 * @short_description: Font objects
 * @include: hb.h
 *
 * Functions for working with font objects.
 *
 * A font object represents a font face at a specific size and with
 * certain other parameters (pixels-per-em, points-per-em, variation
 * settings) specified. Font objects are created from font face
 * objects, and are used as input to shape(), among other things.
 *
 * Client programs can optionally pass in their own functions that
 * implement the basic, lower-level queries of font objects. This set
 * of font functions is defined by the virtual methods in
 * #font_funcs_t.
 *
 * HarfBuzz provides a built-in set of lightweight default
 * functions for each method in #font_funcs_t.
 *
 * The default font functions are implemented in terms of the
 * #font_funcs_t methods of the parent font object.  This allows
 * client programs to override only the methods they need to, and
 * otherwise inherit the parent font's implementation, if any.
 **/


/*
 * font_funcs_t
 */

static bool_t
font_get_font_h_extents_nil (font_t         *font HB_UNUSED,
				void              *font_data HB_UNUSED,
				font_extents_t *extents,
				void              *user_data HB_UNUSED)
{
  memset (extents, 0, sizeof (*extents));
  return false;
}

static bool_t
font_get_font_h_extents_default (font_t         *font,
				    void              *font_data HB_UNUSED,
				    font_extents_t *extents,
				    void              *user_data HB_UNUSED)
{
  bool_t ret = font->parent->get_font_h_extents (extents);
  if (ret) {
    extents->ascender = font->parent_scale_y_distance (extents->ascender);
    extents->descender = font->parent_scale_y_distance (extents->descender);
    extents->line_gap = font->parent_scale_y_distance (extents->line_gap);
  }
  return ret;
}

static bool_t
font_get_font_v_extents_nil (font_t         *font HB_UNUSED,
				void              *font_data HB_UNUSED,
				font_extents_t *extents,
				void              *user_data HB_UNUSED)
{
  memset (extents, 0, sizeof (*extents));
  return false;
}

static bool_t
font_get_font_v_extents_default (font_t         *font,
				    void              *font_data HB_UNUSED,
				    font_extents_t *extents,
				    void              *user_data HB_UNUSED)
{
  bool_t ret = font->parent->get_font_v_extents (extents);
  if (ret) {
    extents->ascender = font->parent_scale_x_distance (extents->ascender);
    extents->descender = font->parent_scale_x_distance (extents->descender);
    extents->line_gap = font->parent_scale_x_distance (extents->line_gap);
  }
  return ret;
}

static bool_t
font_get_nominal_glyph_nil (font_t      *font HB_UNUSED,
			       void           *font_data HB_UNUSED,
			       codepoint_t  unicode HB_UNUSED,
			       codepoint_t *glyph,
			       void           *user_data HB_UNUSED)
{
  *glyph = 0;
  return false;
}

static bool_t
font_get_nominal_glyph_default (font_t      *font,
				   void           *font_data HB_UNUSED,
				   codepoint_t  unicode,
				   codepoint_t *glyph,
				   void           *user_data HB_UNUSED)
{
  if (font->has_nominal_glyphs_func_set ())
  {
    return font->get_nominal_glyphs (1, &unicode, 0, glyph, 0);
  }
  return font->parent->get_nominal_glyph (unicode, glyph);
}

#define font_get_nominal_glyphs_nil font_get_nominal_glyphs_default

static unsigned int
font_get_nominal_glyphs_default (font_t            *font,
				    void                 *font_data HB_UNUSED,
				    unsigned int          count,
				    const codepoint_t *first_unicode,
				    unsigned int          unicode_stride,
				    codepoint_t       *first_glyph,
				    unsigned int          glyph_stride,
				    void                 *user_data HB_UNUSED)
{
  if (font->has_nominal_glyph_func_set ())
  {
    for (unsigned int i = 0; i < count; i++)
    {
      if (!font->get_nominal_glyph (*first_unicode, first_glyph))
	return i;

      first_unicode = &StructAtOffsetUnaligned<codepoint_t> (first_unicode, unicode_stride);
      first_glyph = &StructAtOffsetUnaligned<codepoint_t> (first_glyph, glyph_stride);
    }
    return count;
  }

  return font->parent->get_nominal_glyphs (count,
					   first_unicode, unicode_stride,
					   first_glyph, glyph_stride);
}

static bool_t
font_get_variation_glyph_nil (font_t      *font HB_UNUSED,
				 void           *font_data HB_UNUSED,
				 codepoint_t  unicode HB_UNUSED,
				 codepoint_t  variation_selector HB_UNUSED,
				 codepoint_t *glyph,
				 void           *user_data HB_UNUSED)
{
  *glyph = 0;
  return false;
}

static bool_t
font_get_variation_glyph_default (font_t      *font,
				     void           *font_data HB_UNUSED,
				     codepoint_t  unicode,
				     codepoint_t  variation_selector,
				     codepoint_t *glyph,
				     void           *user_data HB_UNUSED)
{
  return font->parent->get_variation_glyph (unicode, variation_selector, glyph);
}


static position_t
font_get_glyph_h_advance_nil (font_t      *font,
				 void           *font_data HB_UNUSED,
				 codepoint_t  glyph HB_UNUSED,
				 void           *user_data HB_UNUSED)
{
  return font->x_scale;
}

static position_t
font_get_glyph_h_advance_default (font_t      *font,
				     void           *font_data HB_UNUSED,
				     codepoint_t  glyph,
				     void           *user_data HB_UNUSED)
{
  if (font->has_glyph_h_advances_func_set ())
  {
    position_t ret;
    font->get_glyph_h_advances (1, &glyph, 0, &ret, 0);
    return ret;
  }
  return font->parent_scale_x_distance (font->parent->get_glyph_h_advance (glyph));
}

static position_t
font_get_glyph_v_advance_nil (font_t      *font,
				 void           *font_data HB_UNUSED,
				 codepoint_t  glyph HB_UNUSED,
				 void           *user_data HB_UNUSED)
{
  /* TODO use font_extents.ascender+descender */
  return font->y_scale;
}

static position_t
font_get_glyph_v_advance_default (font_t      *font,
				     void           *font_data HB_UNUSED,
				     codepoint_t  glyph,
				     void           *user_data HB_UNUSED)
{
  if (font->has_glyph_v_advances_func_set ())
  {
    position_t ret;
    font->get_glyph_v_advances (1, &glyph, 0, &ret, 0);
    return ret;
  }
  return font->parent_scale_y_distance (font->parent->get_glyph_v_advance (glyph));
}

#define font_get_glyph_h_advances_nil font_get_glyph_h_advances_default

static void
font_get_glyph_h_advances_default (font_t*            font,
				      void*                 font_data HB_UNUSED,
				      unsigned int          count,
				      const codepoint_t *first_glyph,
				      unsigned int          glyph_stride,
				      position_t        *first_advance,
				      unsigned int          advance_stride,
				      void                 *user_data HB_UNUSED)
{
  if (font->has_glyph_h_advance_func_set ())
  {
    for (unsigned int i = 0; i < count; i++)
    {
      *first_advance = font->get_glyph_h_advance (*first_glyph);
      first_glyph = &StructAtOffsetUnaligned<codepoint_t> (first_glyph, glyph_stride);
      first_advance = &StructAtOffsetUnaligned<position_t> (first_advance, advance_stride);
    }
    return;
  }

  font->parent->get_glyph_h_advances (count,
				      first_glyph, glyph_stride,
				      first_advance, advance_stride);
  for (unsigned int i = 0; i < count; i++)
  {
    *first_advance = font->parent_scale_x_distance (*first_advance);
    first_advance = &StructAtOffsetUnaligned<position_t> (first_advance, advance_stride);
  }
}

#define font_get_glyph_v_advances_nil font_get_glyph_v_advances_default
static void
font_get_glyph_v_advances_default (font_t*            font,
				      void*                 font_data HB_UNUSED,
				      unsigned int          count,
				      const codepoint_t *first_glyph,
				      unsigned int          glyph_stride,
				      position_t        *first_advance,
				      unsigned int          advance_stride,
				      void                 *user_data HB_UNUSED)
{
  if (font->has_glyph_v_advance_func_set ())
  {
    for (unsigned int i = 0; i < count; i++)
    {
      *first_advance = font->get_glyph_v_advance (*first_glyph);
      first_glyph = &StructAtOffsetUnaligned<codepoint_t> (first_glyph, glyph_stride);
      first_advance = &StructAtOffsetUnaligned<position_t> (first_advance, advance_stride);
    }
    return;
  }

  font->parent->get_glyph_v_advances (count,
				      first_glyph, glyph_stride,
				      first_advance, advance_stride);
  for (unsigned int i = 0; i < count; i++)
  {
    *first_advance = font->parent_scale_y_distance (*first_advance);
    first_advance = &StructAtOffsetUnaligned<position_t> (first_advance, advance_stride);
  }
}

static bool_t
font_get_glyph_h_origin_nil (font_t      *font HB_UNUSED,
				void           *font_data HB_UNUSED,
				codepoint_t  glyph HB_UNUSED,
				position_t  *x,
				position_t  *y,
				void           *user_data HB_UNUSED)
{
  *x = *y = 0;
  return true;
}

static bool_t
font_get_glyph_h_origin_default (font_t      *font,
				    void           *font_data HB_UNUSED,
				    codepoint_t  glyph,
				    position_t  *x,
				    position_t  *y,
				    void           *user_data HB_UNUSED)
{
  bool_t ret = font->parent->get_glyph_h_origin (glyph, x, y);
  if (ret)
    font->parent_scale_position (x, y);
  return ret;
}

static bool_t
font_get_glyph_v_origin_nil (font_t      *font HB_UNUSED,
				void           *font_data HB_UNUSED,
				codepoint_t  glyph HB_UNUSED,
				position_t  *x,
				position_t  *y,
				void           *user_data HB_UNUSED)
{
  *x = *y = 0;
  return false;
}

static bool_t
font_get_glyph_v_origin_default (font_t      *font,
				    void           *font_data HB_UNUSED,
				    codepoint_t  glyph,
				    position_t  *x,
				    position_t  *y,
				    void           *user_data HB_UNUSED)
{
  bool_t ret = font->parent->get_glyph_v_origin (glyph, x, y);
  if (ret)
    font->parent_scale_position (x, y);
  return ret;
}

static position_t
font_get_glyph_h_kerning_nil (font_t      *font HB_UNUSED,
				 void           *font_data HB_UNUSED,
				 codepoint_t  left_glyph HB_UNUSED,
				 codepoint_t  right_glyph HB_UNUSED,
				 void           *user_data HB_UNUSED)
{
  return 0;
}

static position_t
font_get_glyph_h_kerning_default (font_t      *font,
				     void           *font_data HB_UNUSED,
				     codepoint_t  left_glyph,
				     codepoint_t  right_glyph,
				     void           *user_data HB_UNUSED)
{
  return font->parent_scale_x_distance (font->parent->get_glyph_h_kerning (left_glyph, right_glyph));
}

#ifndef HB_DISABLE_DEPRECATED
static position_t
font_get_glyph_v_kerning_nil (font_t      *font HB_UNUSED,
				 void           *font_data HB_UNUSED,
				 codepoint_t  top_glyph HB_UNUSED,
				 codepoint_t  bottom_glyph HB_UNUSED,
				 void           *user_data HB_UNUSED)
{
  return 0;
}

static position_t
font_get_glyph_v_kerning_default (font_t      *font,
				     void           *font_data HB_UNUSED,
				     codepoint_t  top_glyph,
				     codepoint_t  bottom_glyph,
				     void           *user_data HB_UNUSED)
{
  return font->parent_scale_y_distance (font->parent->get_glyph_v_kerning (top_glyph, bottom_glyph));
}
#endif

static bool_t
font_get_glyph_extents_nil (font_t          *font HB_UNUSED,
			       void               *font_data HB_UNUSED,
			       codepoint_t      glyph HB_UNUSED,
			       glyph_extents_t *extents,
			       void               *user_data HB_UNUSED)
{
  memset (extents, 0, sizeof (*extents));
  return false;
}

static bool_t
font_get_glyph_extents_default (font_t          *font,
				   void               *font_data HB_UNUSED,
				   codepoint_t      glyph,
				   glyph_extents_t *extents,
				   void               *user_data HB_UNUSED)
{
  bool_t ret = font->parent->get_glyph_extents (glyph, extents);
  if (ret) {
    font->parent_scale_position (&extents->x_bearing, &extents->y_bearing);
    font->parent_scale_distance (&extents->width, &extents->height);
  }
  return ret;
}

static bool_t
font_get_glyph_contour_point_nil (font_t      *font HB_UNUSED,
				     void           *font_data HB_UNUSED,
				     codepoint_t  glyph HB_UNUSED,
				     unsigned int    point_index HB_UNUSED,
				     position_t  *x,
				     position_t  *y,
				     void           *user_data HB_UNUSED)
{
  *x = *y = 0;
  return false;
}

static bool_t
font_get_glyph_contour_point_default (font_t      *font,
					 void           *font_data HB_UNUSED,
					 codepoint_t  glyph,
					 unsigned int    point_index,
					 position_t  *x,
					 position_t  *y,
					 void           *user_data HB_UNUSED)
{
  bool_t ret = font->parent->get_glyph_contour_point (glyph, point_index, x, y);
  if (ret)
    font->parent_scale_position (x, y);
  return ret;
}

static bool_t
font_get_glyph_name_nil (font_t      *font HB_UNUSED,
			    void           *font_data HB_UNUSED,
			    codepoint_t  glyph HB_UNUSED,
			    char           *name,
			    unsigned int    size,
			    void           *user_data HB_UNUSED)
{
  if (size) *name = '\0';
  return false;
}

static bool_t
font_get_glyph_name_default (font_t      *font,
				void           *font_data HB_UNUSED,
				codepoint_t  glyph,
				char           *name,
				unsigned int    size,
				void           *user_data HB_UNUSED)
{
  return font->parent->get_glyph_name (glyph, name, size);
}

static bool_t
font_get_glyph_from_name_nil (font_t      *font HB_UNUSED,
				 void           *font_data HB_UNUSED,
				 const char     *name HB_UNUSED,
				 int             len HB_UNUSED, /* -1 means nul-terminated */
				 codepoint_t *glyph,
				 void           *user_data HB_UNUSED)
{
  *glyph = 0;
  return false;
}

static bool_t
font_get_glyph_from_name_default (font_t      *font,
				     void           *font_data HB_UNUSED,
				     const char     *name,
				     int             len, /* -1 means nul-terminated */
				     codepoint_t *glyph,
				     void           *user_data HB_UNUSED)
{
  return font->parent->get_glyph_from_name (name, len, glyph);
}

static void
font_draw_glyph_nil (font_t       *font HB_UNUSED,
			void            *font_data HB_UNUSED,
			codepoint_t   glyph,
			draw_funcs_t *draw_funcs,
			void            *draw_data,
			void            *user_data HB_UNUSED)
{
}

static void
font_paint_glyph_nil (font_t *font HB_UNUSED,
                         void *font_data HB_UNUSED,
                         codepoint_t glyph HB_UNUSED,
                         paint_funcs_t *paint_funcs HB_UNUSED,
                         void *paint_data HB_UNUSED,
                         unsigned int palette HB_UNUSED,
                         color_t foreground HB_UNUSED,
                         void *user_data HB_UNUSED)
{
}

typedef struct font_draw_glyph_default_adaptor_t {
  draw_funcs_t *draw_funcs;
  void		  *draw_data;
  float		   x_scale;
  float		   y_scale;
  float		   slant;
} font_draw_glyph_default_adaptor_t;

static void
draw_move_to_default (draw_funcs_t *dfuncs HB_UNUSED,
			 void *draw_data,
			 draw_state_t *st,
			 float to_x, float to_y,
			 void *user_data HB_UNUSED)
{
  font_draw_glyph_default_adaptor_t *adaptor = (font_draw_glyph_default_adaptor_t *) draw_data;
  float x_scale = adaptor->x_scale;
  float y_scale = adaptor->y_scale;
  float slant   = adaptor->slant;

  adaptor->draw_funcs->emit_move_to (adaptor->draw_data, *st,
				     x_scale * to_x + slant * to_y, y_scale * to_y);
}

static void
draw_line_to_default (draw_funcs_t *dfuncs HB_UNUSED, void *draw_data,
			 draw_state_t *st,
			 float to_x, float to_y,
			 void *user_data HB_UNUSED)
{
  font_draw_glyph_default_adaptor_t *adaptor = (font_draw_glyph_default_adaptor_t *) draw_data;
  float x_scale = adaptor->x_scale;
  float y_scale = adaptor->y_scale;
  float slant   = adaptor->slant;

  st->current_x = st->current_x * x_scale + st->current_y * slant;
  st->current_y = st->current_y * y_scale;

  adaptor->draw_funcs->emit_line_to (adaptor->draw_data, *st,
				     x_scale * to_x + slant * to_y, y_scale * to_y);
}

static void
draw_quadratic_to_default (draw_funcs_t *dfuncs HB_UNUSED, void *draw_data,
			      draw_state_t *st,
			      float control_x, float control_y,
			      float to_x, float to_y,
			      void *user_data HB_UNUSED)
{
  font_draw_glyph_default_adaptor_t *adaptor = (font_draw_glyph_default_adaptor_t *) draw_data;
  float x_scale = adaptor->x_scale;
  float y_scale = adaptor->y_scale;
  float slant   = adaptor->slant;

  st->current_x = st->current_x * x_scale + st->current_y * slant;
  st->current_y = st->current_y * y_scale;

  adaptor->draw_funcs->emit_quadratic_to (adaptor->draw_data, *st,
					  x_scale * control_x + slant * control_y, y_scale * control_y,
					  x_scale * to_x + slant * to_y, y_scale * to_y);
}

static void
draw_cubic_to_default (draw_funcs_t *dfuncs HB_UNUSED, void *draw_data,
			  draw_state_t *st,
			  float control1_x, float control1_y,
			  float control2_x, float control2_y,
			  float to_x, float to_y,
			  void *user_data HB_UNUSED)
{
  font_draw_glyph_default_adaptor_t *adaptor = (font_draw_glyph_default_adaptor_t *) draw_data;
  float x_scale = adaptor->x_scale;
  float y_scale = adaptor->y_scale;
  float slant   = adaptor->slant;

  st->current_x = st->current_x * x_scale + st->current_y * slant;
  st->current_y = st->current_y * y_scale;

  adaptor->draw_funcs->emit_cubic_to (adaptor->draw_data, *st,
				      x_scale * control1_x + slant * control1_y, y_scale * control1_y,
				      x_scale * control2_x + slant * control2_y, y_scale * control2_y,
				      x_scale * to_x + slant * to_y, y_scale * to_y);
}

static void
draw_close_path_default (draw_funcs_t *dfuncs HB_UNUSED, void *draw_data,
			    draw_state_t *st,
			    void *user_data HB_UNUSED)
{
  font_draw_glyph_default_adaptor_t *adaptor = (font_draw_glyph_default_adaptor_t *) draw_data;

  adaptor->draw_funcs->emit_close_path (adaptor->draw_data, *st);
}

static const draw_funcs_t _draw_funcs_default = {
  HB_OBJECT_HEADER_STATIC,

  {
#define HB_DRAW_FUNC_IMPLEMENT(name) draw_##name##_default,
    HB_DRAW_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_DRAW_FUNC_IMPLEMENT
  }
};

static void
font_draw_glyph_default (font_t       *font,
				 void            *font_data HB_UNUSED,
				 codepoint_t   glyph,
				 draw_funcs_t *draw_funcs,
				 void            *draw_data,
				 void            *user_data HB_UNUSED)
{
  font_draw_glyph_default_adaptor_t adaptor = {
    draw_funcs,
    draw_data,
    font->parent->x_scale ? (float) font->x_scale / (float) font->parent->x_scale : 0.f,
    font->parent->y_scale ? (float) font->y_scale / (float) font->parent->y_scale : 0.f,
    font->parent->y_scale ? (font->slant - font->parent->slant) *
			    (float) font->x_scale / (float) font->parent->y_scale : 0.f
  };

  font->parent->draw_glyph (glyph,
				 const_cast<draw_funcs_t *> (&_draw_funcs_default),
				 &adaptor);
}

static void
font_paint_glyph_default (font_t *font,
                             void *font_data,
                             codepoint_t glyph,
                             paint_funcs_t *paint_funcs,
                             void *paint_data,
                             unsigned int palette,
                             color_t foreground,
                             void *user_data)
{
  paint_funcs->push_transform (paint_data,
    font->parent->x_scale ? (float) font->x_scale / (float) font->parent->x_scale : 0.f,
    font->parent->y_scale ? (font->slant - font->parent->slant) *
			    (float) font->x_scale / (float) font->parent->y_scale : 0.f,
    0.f,
    font->parent->y_scale ? (float) font->y_scale / (float) font->parent->y_scale : 0.f,
    0.f, 0.f);

  font->parent->paint_glyph (glyph, paint_funcs, paint_data, palette, foreground);

  paint_funcs->pop_transform (paint_data);
}

DEFINE_NULL_INSTANCE (font_funcs_t) =
{
  HB_OBJECT_HEADER_STATIC,

  nullptr,
  nullptr,
  {
    {
#define HB_FONT_FUNC_IMPLEMENT(get_,name) font_##get_##name##_nil,
      HB_FONT_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_FONT_FUNC_IMPLEMENT
    }
  }
};

static const font_funcs_t _font_funcs_default = {
  HB_OBJECT_HEADER_STATIC,

  nullptr,
  nullptr,
  {
    {
#define HB_FONT_FUNC_IMPLEMENT(get_,name) font_##get_##name##_default,
      HB_FONT_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_FONT_FUNC_IMPLEMENT
    }
  }
};


/**
 * font_funcs_create:
 *
 * Creates a new #font_funcs_t structure of font functions.
 *
 * Return value: (transfer full): The font-functions structure
 *
 * Since: 0.9.2
 **/
font_funcs_t *
font_funcs_create ()
{
  font_funcs_t *ffuncs;

  if (!(ffuncs = object_create<font_funcs_t> ()))
    return font_funcs_get_empty ();

  ffuncs->get = _font_funcs_default.get;

  return ffuncs;
}

/**
 * font_funcs_get_empty:
 *
 * Fetches an empty font-functions structure.
 *
 * Return value: (transfer full): The font-functions structure
 *
 * Since: 0.9.2
 **/
font_funcs_t *
font_funcs_get_empty ()
{
  return const_cast<font_funcs_t *> (&_font_funcs_default);
}

/**
 * font_funcs_reference: (skip)
 * @ffuncs: The font-functions structure
 *
 * Increases the reference count on a font-functions structure.
 *
 * Return value: The font-functions structure
 *
 * Since: 0.9.2
 **/
font_funcs_t *
font_funcs_reference (font_funcs_t *ffuncs)
{
  return object_reference (ffuncs);
}

/**
 * font_funcs_destroy: (skip)
 * @ffuncs: The font-functions structure
 *
 * Decreases the reference count on a font-functions structure. When
 * the reference count reaches zero, the font-functions structure is
 * destroyed, freeing all memory.
 *
 * Since: 0.9.2
 **/
void
font_funcs_destroy (font_funcs_t *ffuncs)
{
  if (!object_destroy (ffuncs)) return;

  if (ffuncs->destroy)
  {
#define HB_FONT_FUNC_IMPLEMENT(get_,name) if (ffuncs->destroy->name) \
    ffuncs->destroy->name (!ffuncs->user_data ? nullptr : ffuncs->user_data->name);
    HB_FONT_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_FONT_FUNC_IMPLEMENT
  }

  free (ffuncs->destroy);
  free (ffuncs->user_data);

  free (ffuncs);
}

/**
 * font_funcs_set_user_data: (skip)
 * @ffuncs: The font-functions structure
 * @key: The user-data key to set
 * @data: A pointer to the user data set
 * @destroy: (nullable): A callback to call when @data is not needed anymore
 * @replace: Whether to replace an existing data with the same key
 *
 * Attaches a user-data key/data pair to the specified font-functions structure.
 *
 * Return value: `true` if success, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
font_funcs_set_user_data (font_funcs_t    *ffuncs,
			     user_data_key_t *key,
			     void *              data,
			     destroy_func_t   destroy /* May be NULL. */,
			     bool_t           replace)
{
  return object_set_user_data (ffuncs, key, data, destroy, replace);
}

/**
 * font_funcs_get_user_data: (skip)
 * @ffuncs: The font-functions structure
 * @key: The user-data key to query
 *
 * Fetches the user data associated with the specified key,
 * attached to the specified font-functions structure.
 *
 * Return value: (transfer none): A pointer to the user data
 *
 * Since: 0.9.2
 **/
void *
font_funcs_get_user_data (const font_funcs_t *ffuncs,
			     user_data_key_t    *key)
{
  return object_get_user_data (ffuncs, key);
}


/**
 * font_funcs_make_immutable:
 * @ffuncs: The font-functions structure
 *
 * Makes a font-functions structure immutable.
 *
 * Since: 0.9.2
 **/
void
font_funcs_make_immutable (font_funcs_t *ffuncs)
{
  if (object_is_immutable (ffuncs))
    return;

  object_make_immutable (ffuncs);
}

/**
 * font_funcs_is_immutable:
 * @ffuncs: The font-functions structure
 *
 * Tests whether a font-functions structure is immutable.
 *
 * Return value: `true` if @ffuncs is immutable, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
font_funcs_is_immutable (font_funcs_t *ffuncs)
{
  return object_is_immutable (ffuncs);
}


static bool
_font_funcs_set_preamble (font_funcs_t    *ffuncs,
			     bool                func_is_null,
			     void              **user_data,
			     destroy_func_t  *destroy)
{
  if (object_is_immutable (ffuncs))
  {
    if (*destroy)
      (*destroy) (*user_data);
    return false;
  }

  if (func_is_null)
  {
    if (*destroy)
      (*destroy) (*user_data);
    *destroy = nullptr;
    *user_data = nullptr;
  }

  return true;
}

static bool
_font_funcs_set_middle (font_funcs_t   *ffuncs,
			   void              *user_data,
			   destroy_func_t  destroy)
{
  if (user_data && !ffuncs->user_data)
  {
    ffuncs->user_data = (decltype (ffuncs->user_data)) calloc (1, sizeof (*ffuncs->user_data));
    if (unlikely (!ffuncs->user_data))
      goto fail;
  }
  if (destroy && !ffuncs->destroy)
  {
    ffuncs->destroy = (decltype (ffuncs->destroy)) calloc (1, sizeof (*ffuncs->destroy));
    if (unlikely (!ffuncs->destroy))
      goto fail;
  }

  return true;

fail:
  if (destroy)
    (destroy) (user_data);
  return false;
}

#define HB_FONT_FUNC_IMPLEMENT(get_,name) \
									 \
void                                                                     \
font_funcs_set_##name##_func (font_funcs_t             *ffuncs,    \
				 font_##get_##name##_func_t func,     \
				 void                        *user_data, \
				 destroy_func_t            destroy)   \
{                                                                        \
  if (!_font_funcs_set_preamble (ffuncs, !func, &user_data, &destroy))\
      return;                                                            \
									 \
  if (ffuncs->destroy && ffuncs->destroy->name)                          \
    ffuncs->destroy->name (!ffuncs->user_data ? nullptr : ffuncs->user_data->name); \
                                                                         \
  if (!_font_funcs_set_middle (ffuncs, user_data, destroy))           \
      return;                                                            \
									 \
  if (func)                                                              \
    ffuncs->get.f.name = func;                                           \
  else                                                                   \
    ffuncs->get.f.name = font_##get_##name##_default;                   \
									 \
  if (ffuncs->user_data)                                                 \
    ffuncs->user_data->name = user_data;                                 \
  if (ffuncs->destroy)                                                   \
    ffuncs->destroy->name = destroy;                                     \
}

HB_FONT_FUNCS_IMPLEMENT_CALLBACKS
#undef HB_FONT_FUNC_IMPLEMENT

bool
font_t::has_func_set (unsigned int i)
{
  return this->klass->get.array[i] != _font_funcs_default.get.array[i];
}

bool
font_t::has_func (unsigned int i)
{
  return has_func_set (i) ||
	 (parent && parent != &_Null_font_t && parent->has_func (i));
}

/* Public getters */

/**
 * font_get_h_extents:
 * @font: #font_t to work upon
 * @extents: (out): The font extents retrieved
 *
 * Fetches the extents for a specified font, for horizontal
 * text segments.
 *
 * Return value: `true` if data found, `false` otherwise
 *
 * Since: 1.1.3
 **/
bool_t
font_get_h_extents (font_t         *font,
		       font_extents_t *extents)
{
  return font->get_font_h_extents (extents);
}

/**
 * font_get_v_extents:
 * @font: #font_t to work upon
 * @extents: (out): The font extents retrieved
 *
 * Fetches the extents for a specified font, for vertical
 * text segments.
 *
 * Return value: `true` if data found, `false` otherwise
 *
 * Since: 1.1.3
 **/
bool_t
font_get_v_extents (font_t         *font,
		       font_extents_t *extents)
{
  return font->get_font_v_extents (extents);
}

/**
 * font_get_glyph:
 * @font: #font_t to work upon
 * @unicode: The Unicode code point to query
 * @variation_selector: A variation-selector code point
 * @glyph: (out): The glyph ID retrieved
 *
 * Fetches the glyph ID for a Unicode code point in the specified
 * font, with an optional variation selector.
 *
 * If @variation_selector is 0, calls font_get_nominal_glyph();
 * otherwise calls font_get_variation_glyph().
 *
 * Return value: `true` if data found, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
font_get_glyph (font_t      *font,
		   codepoint_t  unicode,
		   codepoint_t  variation_selector,
		   codepoint_t *glyph)
{
  if (unlikely (variation_selector))
    return font->get_variation_glyph (unicode, variation_selector, glyph);
  return font->get_nominal_glyph (unicode, glyph);
}

/**
 * font_get_nominal_glyph:
 * @font: #font_t to work upon
 * @unicode: The Unicode code point to query
 * @glyph: (out): The glyph ID retrieved
 *
 * Fetches the nominal glyph ID for a Unicode code point in the
 * specified font.
 *
 * This version of the function should not be used to fetch glyph IDs
 * for code points modified by variation selectors. For variation-selector
 * support, user font_get_variation_glyph() or use font_get_glyph().
 *
 * Return value: `true` if data found, `false` otherwise
 *
 * Since: 1.2.3
 **/
bool_t
font_get_nominal_glyph (font_t      *font,
			   codepoint_t  unicode,
			   codepoint_t *glyph)
{
  return font->get_nominal_glyph (unicode, glyph);
}

/**
 * font_get_nominal_glyphs:
 * @font: #font_t to work upon
 * @count: number of code points to query
 * @first_unicode: The first Unicode code point to query
 * @unicode_stride: The stride between successive code points
 * @first_glyph: (out): The first glyph ID retrieved
 * @glyph_stride: The stride between successive glyph IDs
 *
 * Fetches the nominal glyph IDs for a sequence of Unicode code points. Glyph
 * IDs must be returned in a #codepoint_t output parameter. Stops at the
 * first unsupported glyph ID.
 *
 * Return value: the number of code points processed
 *
 * Since: 2.6.3
 **/
unsigned int
font_get_nominal_glyphs (font_t *font,
			    unsigned int count,
			    const codepoint_t *first_unicode,
			    unsigned int unicode_stride,
			    codepoint_t *first_glyph,
			    unsigned int glyph_stride)
{
  return font->get_nominal_glyphs (count,
				   first_unicode, unicode_stride,
				   first_glyph, glyph_stride);
}

/**
 * font_get_variation_glyph:
 * @font: #font_t to work upon
 * @unicode: The Unicode code point to query
 * @variation_selector: The  variation-selector code point to query
 * @glyph: (out): The glyph ID retrieved
 *
 * Fetches the glyph ID for a Unicode code point when followed by
 * by the specified variation-selector code point, in the specified
 * font.
 *
 * Return value: `true` if data found, `false` otherwise
 *
 * Since: 1.2.3
 **/
bool_t
font_get_variation_glyph (font_t      *font,
			     codepoint_t  unicode,
			     codepoint_t  variation_selector,
			     codepoint_t *glyph)
{
  return font->get_variation_glyph (unicode, variation_selector, glyph);
}

/**
 * font_get_glyph_h_advance:
 * @font: #font_t to work upon
 * @glyph: The glyph ID to query
 *
 * Fetches the advance for a glyph ID in the specified font,
 * for horizontal text segments.
 *
 * Return value: The advance of @glyph within @font
 *
 * Since: 0.9.2
 **/
position_t
font_get_glyph_h_advance (font_t      *font,
			     codepoint_t  glyph)
{
  return font->get_glyph_h_advance (glyph);
}

/**
 * font_get_glyph_v_advance:
 * @font: #font_t to work upon
 * @glyph: The glyph ID to query
 *
 * Fetches the advance for a glyph ID in the specified font,
 * for vertical text segments.
 *
 * Return value: The advance of @glyph within @font
 *
 * Since: 0.9.2
 **/
position_t
font_get_glyph_v_advance (font_t      *font,
			     codepoint_t  glyph)
{
  return font->get_glyph_v_advance (glyph);
}

/**
 * font_get_glyph_h_advances:
 * @font: #font_t to work upon
 * @count: The number of glyph IDs in the sequence queried
 * @first_glyph: The first glyph ID to query
 * @glyph_stride: The stride between successive glyph IDs
 * @first_advance: (out): The first advance retrieved
 * @advance_stride: The stride between successive advances
 *
 * Fetches the advances for a sequence of glyph IDs in the specified
 * font, for horizontal text segments.
 *
 * Since: 1.8.6
 **/
void
font_get_glyph_h_advances (font_t*            font,
			      unsigned int          count,
			      const codepoint_t *first_glyph,
			      unsigned              glyph_stride,
			      position_t        *first_advance,
			      unsigned              advance_stride)
{
  font->get_glyph_h_advances (count, first_glyph, glyph_stride, first_advance, advance_stride);
}
/**
 * font_get_glyph_v_advances:
 * @font: #font_t to work upon
 * @count: The number of glyph IDs in the sequence queried
 * @first_glyph: The first glyph ID to query
 * @glyph_stride: The stride between successive glyph IDs
 * @first_advance: (out): The first advance retrieved
 * @advance_stride: (out): The stride between successive advances
 *
 * Fetches the advances for a sequence of glyph IDs in the specified
 * font, for vertical text segments.
 *
 * Since: 1.8.6
 **/
void
font_get_glyph_v_advances (font_t*            font,
			      unsigned int          count,
			      const codepoint_t *first_glyph,
			      unsigned              glyph_stride,
			      position_t        *first_advance,
			      unsigned              advance_stride)
{
  font->get_glyph_v_advances (count, first_glyph, glyph_stride, first_advance, advance_stride);
}

/**
 * font_get_glyph_h_origin:
 * @font: #font_t to work upon
 * @glyph: The glyph ID to query
 * @x: (out): The X coordinate of the origin
 * @y: (out): The Y coordinate of the origin
 *
 * Fetches the (X,Y) coordinates of the origin for a glyph ID
 * in the specified font, for horizontal text segments.
 *
 * Return value: `true` if data found, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
font_get_glyph_h_origin (font_t      *font,
			    codepoint_t  glyph,
			    position_t  *x,
			    position_t  *y)
{
  return font->get_glyph_h_origin (glyph, x, y);
}

/**
 * font_get_glyph_v_origin:
 * @font: #font_t to work upon
 * @glyph: The glyph ID to query
 * @x: (out): The X coordinate of the origin
 * @y: (out): The Y coordinate of the origin
 *
 * Fetches the (X,Y) coordinates of the origin for a glyph ID
 * in the specified font, for vertical text segments.
 *
 * Return value: `true` if data found, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
font_get_glyph_v_origin (font_t      *font,
			    codepoint_t  glyph,
			    position_t  *x,
			    position_t  *y)
{
  return font->get_glyph_v_origin (glyph, x, y);
}

/**
 * font_get_glyph_h_kerning:
 * @font: #font_t to work upon
 * @left_glyph: The glyph ID of the left glyph in the glyph pair
 * @right_glyph: The glyph ID of the right glyph in the glyph pair
 *
 * Fetches the kerning-adjustment value for a glyph-pair in
 * the specified font, for horizontal text segments.
 *
 * <note>It handles legacy kerning only (as returned by the corresponding
 * #font_funcs_t function).</note>
 *
 * Return value: The kerning adjustment value
 *
 * Since: 0.9.2
 **/
position_t
font_get_glyph_h_kerning (font_t      *font,
			     codepoint_t  left_glyph,
			     codepoint_t  right_glyph)
{
  return font->get_glyph_h_kerning (left_glyph, right_glyph);
}

#ifndef HB_DISABLE_DEPRECATED
/**
 * font_get_glyph_v_kerning:
 * @font: #font_t to work upon
 * @top_glyph: The glyph ID of the top glyph in the glyph pair
 * @bottom_glyph: The glyph ID of the bottom glyph in the glyph pair
 *
 * Fetches the kerning-adjustment value for a glyph-pair in
 * the specified font, for vertical text segments.
 *
 * <note>It handles legacy kerning only (as returned by the corresponding
 * #font_funcs_t function).</note>
 *
 * Return value: The kerning adjustment value
 *
 * Since: 0.9.2
 * Deprecated: 2.0.0
 **/
position_t
font_get_glyph_v_kerning (font_t      *font,
			     codepoint_t  top_glyph,
			     codepoint_t  bottom_glyph)
{
  return font->get_glyph_v_kerning (top_glyph, bottom_glyph);
}
#endif

/**
 * font_get_glyph_extents:
 * @font: #font_t to work upon
 * @glyph: The glyph ID to query
 * @extents: (out): The #glyph_extents_t retrieved
 *
 * Fetches the #glyph_extents_t data for a glyph ID
 * in the specified font.
 *
 * Return value: `true` if data found, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
font_get_glyph_extents (font_t          *font,
			   codepoint_t      glyph,
			   glyph_extents_t *extents)
{
  return font->get_glyph_extents (glyph, extents);
}

/**
 * font_get_glyph_contour_point:
 * @font: #font_t to work upon
 * @glyph: The glyph ID to query
 * @point_index: The contour-point index to query
 * @x: (out): The X value retrieved for the contour point
 * @y: (out): The Y value retrieved for the contour point
 *
 * Fetches the (x,y) coordinates of a specified contour-point index
 * in the specified glyph, within the specified font.
 *
 * Return value: `true` if data found, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
font_get_glyph_contour_point (font_t      *font,
				 codepoint_t  glyph,
				 unsigned int    point_index,
				 position_t  *x,
				 position_t  *y)
{
  return font->get_glyph_contour_point (glyph, point_index, x, y);
}

/**
 * font_get_glyph_name:
 * @font: #font_t to work upon
 * @glyph: The glyph ID to query
 * @name: (out) (array length=size): Name string retrieved for the glyph ID
 * @size: Length of the glyph-name string retrieved
 *
 * Fetches the glyph-name string for a glyph ID in the specified @font.
 *
 * According to the OpenType specification, glyph names are limited to 63
 * characters and can only contain (a subset of) ASCII.
 *
 * Return value: `true` if data found, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
font_get_glyph_name (font_t      *font,
			codepoint_t  glyph,
			char           *name,
			unsigned int    size)
{
  return font->get_glyph_name (glyph, name, size);
}

/**
 * font_get_glyph_from_name:
 * @font: #font_t to work upon
 * @name: (array length=len): The name string to query
 * @len: The length of the name queried
 * @glyph: (out): The glyph ID retrieved
 *
 * Fetches the glyph ID that corresponds to a name string in the specified @font.
 *
 * <note>Note: @len == -1 means the name string is null-terminated.</note>
 *
 * Return value: `true` if data found, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
font_get_glyph_from_name (font_t      *font,
			     const char     *name,
			     int             len, /* -1 means nul-terminated */
			     codepoint_t *glyph)
{
  return font->get_glyph_from_name (name, len, glyph);
}

#ifndef HB_DISABLE_DEPRECATED
/**
 * font_get_glyph_shape:
 * @font: #font_t to work upon
 * @glyph: The glyph ID
 * @dfuncs: #draw_funcs_t to draw to
 * @draw_data: User data to pass to draw callbacks
 *
 * Fetches the glyph shape that corresponds to a glyph in the specified @font.
 * The shape is returned by way of calls to the callbacks of the @dfuncs
 * objects, with @draw_data passed to them.
 *
 * Since: 4.0.0
 * Deprecated: 7.0.0: Use font_draw_glyph() instead
 */
void
font_get_glyph_shape (font_t *font,
		         codepoint_t glyph,
		         draw_funcs_t *dfuncs, void *draw_data)
{
  font_draw_glyph (font, glyph, dfuncs, draw_data);
}
#endif

/**
 * font_draw_glyph:
 * @font: #font_t to work upon
 * @glyph: The glyph ID
 * @dfuncs: #draw_funcs_t to draw to
 * @draw_data: User data to pass to draw callbacks
 *
 * Draws the outline that corresponds to a glyph in the specified @font.
 *
 * The outline is returned by way of calls to the callbacks of the @dfuncs
 * objects, with @draw_data passed to them.
 *
 * Since: 7.0.0
 **/
void
font_draw_glyph (font_t *font,
			 codepoint_t glyph,
			 draw_funcs_t *dfuncs, void *draw_data)
{
  font->draw_glyph (glyph, dfuncs, draw_data);
}

/**
 * font_paint_glyph:
 * @font: #font_t to work upon
 * @glyph: The glyph ID
 * @pfuncs: #paint_funcs_t to paint with
 * @paint_data: User data to pass to paint callbacks
 * @palette_index: The index of the font's color palette to use
 * @foreground: The foreground color, unpremultipled
 *
 * Paints the glyph.
 *
 * The painting instructions are returned by way of calls to
 * the callbacks of the @funcs object, with @paint_data passed
 * to them.
 *
 * If the font has color palettes (see ot_color_has_palettes()),
 * then @palette_index selects the palette to use. If the font only
 * has one palette, this will be 0.
 *
 * Since: 7.0.0
 */
void
font_paint_glyph (font_t *font,
                     codepoint_t glyph,
                     paint_funcs_t *pfuncs, void *paint_data,
                     unsigned int palette_index,
                     color_t foreground)
{
  font->paint_glyph (glyph, pfuncs, paint_data, palette_index, foreground);
}

/* A bit higher-level, and with fallback */

/**
 * font_get_extents_for_direction:
 * @font: #font_t to work upon
 * @direction: The direction of the text segment
 * @extents: (out): The #font_extents_t retrieved
 *
 * Fetches the extents for a font in a text segment of the
 * specified direction.
 *
 * Calls the appropriate direction-specific variant (horizontal
 * or vertical) depending on the value of @direction.
 *
 * Since: 1.1.3
 **/
void
font_get_extents_for_direction (font_t         *font,
				   direction_t     direction,
				   font_extents_t *extents)
{
  font->get_extents_for_direction (direction, extents);
}
/**
 * font_get_glyph_advance_for_direction:
 * @font: #font_t to work upon
 * @glyph: The glyph ID to query
 * @direction: The direction of the text segment
 * @x: (out): The horizontal advance retrieved
 * @y: (out):  The vertical advance retrieved
 *
 * Fetches the advance for a glyph ID from the specified font,
 * in a text segment of the specified direction.
 *
 * Calls the appropriate direction-specific variant (horizontal
 * or vertical) depending on the value of @direction.
 *
 * Since: 0.9.2
 **/
void
font_get_glyph_advance_for_direction (font_t      *font,
					 codepoint_t  glyph,
					 direction_t  direction,
					 position_t  *x,
					 position_t  *y)
{
  font->get_glyph_advance_for_direction (glyph, direction, x, y);
}
/**
 * font_get_glyph_advances_for_direction:
 * @font: #font_t to work upon
 * @direction: The direction of the text segment
 * @count: The number of glyph IDs in the sequence queried
 * @first_glyph: The first glyph ID to query
 * @glyph_stride: The stride between successive glyph IDs
 * @first_advance: (out): The first advance retrieved
 * @advance_stride: (out): The stride between successive advances
 *
 * Fetches the advances for a sequence of glyph IDs in the specified
 * font, in a text segment of the specified direction.
 *
 * Calls the appropriate direction-specific variant (horizontal
 * or vertical) depending on the value of @direction.
 *
 * Since: 1.8.6
 **/
HB_EXTERN void
font_get_glyph_advances_for_direction (font_t*            font,
					  direction_t        direction,
					  unsigned int          count,
					  const codepoint_t *first_glyph,
					  unsigned              glyph_stride,
					  position_t        *first_advance,
					  unsigned              advance_stride)
{
  font->get_glyph_advances_for_direction (direction, count, first_glyph, glyph_stride, first_advance, advance_stride);
}

/**
 * font_get_glyph_origin_for_direction:
 * @font: #font_t to work upon
 * @glyph: The glyph ID to query
 * @direction: The direction of the text segment
 * @x: (out): The X coordinate retrieved for the origin
 * @y: (out): The Y coordinate retrieved for the origin
 *
 * Fetches the (X,Y) coordinates of the origin for a glyph in
 * the specified font.
 *
 * Calls the appropriate direction-specific variant (horizontal
 * or vertical) depending on the value of @direction.
 *
 * Since: 0.9.2
 **/
void
font_get_glyph_origin_for_direction (font_t      *font,
					codepoint_t  glyph,
					direction_t  direction,
					position_t  *x,
					position_t  *y)
{
  return font->get_glyph_origin_for_direction (glyph, direction, x, y);
}

/**
 * font_add_glyph_origin_for_direction:
 * @font: #font_t to work upon
 * @glyph: The glyph ID to query
 * @direction: The direction of the text segment
 * @x: (inout): Input = The original X coordinate
 *     Output = The X coordinate plus the X-coordinate of the origin
 * @y: (inout): Input = The original Y coordinate
 *     Output = The Y coordinate plus the Y-coordinate of the origin
 *
 * Adds the origin coordinates to an (X,Y) point coordinate, in
 * the specified glyph ID in the specified font.
 *
 * Calls the appropriate direction-specific variant (horizontal
 * or vertical) depending on the value of @direction.
 *
 * Since: 0.9.2
 **/
void
font_add_glyph_origin_for_direction (font_t      *font,
					codepoint_t  glyph,
					direction_t  direction,
					position_t  *x,
					position_t  *y)
{
  return font->add_glyph_origin_for_direction (glyph, direction, x, y);
}

/**
 * font_subtract_glyph_origin_for_direction:
 * @font: #font_t to work upon
 * @glyph: The glyph ID to query
 * @direction: The direction of the text segment
 * @x: (inout): Input = The original X coordinate
 *     Output = The X coordinate minus the X-coordinate of the origin
 * @y: (inout): Input = The original Y coordinate
 *     Output = The Y coordinate minus the Y-coordinate of the origin
 *
 * Subtracts the origin coordinates from an (X,Y) point coordinate,
 * in the specified glyph ID in the specified font.
 *
 * Calls the appropriate direction-specific variant (horizontal
 * or vertical) depending on the value of @direction.
 *
 * Since: 0.9.2
 **/
void
font_subtract_glyph_origin_for_direction (font_t      *font,
					     codepoint_t  glyph,
					     direction_t  direction,
					     position_t  *x,
					     position_t  *y)
{
  return font->subtract_glyph_origin_for_direction (glyph, direction, x, y);
}

/**
 * font_get_glyph_kerning_for_direction:
 * @font: #font_t to work upon
 * @first_glyph: The glyph ID of the first glyph in the glyph pair to query
 * @second_glyph: The glyph ID of the second glyph in the glyph pair to query
 * @direction: The direction of the text segment
 * @x: (out): The horizontal kerning-adjustment value retrieved
 * @y: (out): The vertical kerning-adjustment value retrieved
 *
 * Fetches the kerning-adjustment value for a glyph-pair in the specified font.
 *
 * Calls the appropriate direction-specific variant (horizontal
 * or vertical) depending on the value of @direction.
 *
 * Since: 0.9.2
 **/
void
font_get_glyph_kerning_for_direction (font_t      *font,
					 codepoint_t  first_glyph,
					 codepoint_t  second_glyph,
					 direction_t  direction,
					 position_t  *x,
					 position_t  *y)
{
  return font->get_glyph_kerning_for_direction (first_glyph, second_glyph, direction, x, y);
}

/**
 * font_get_glyph_extents_for_origin:
 * @font: #font_t to work upon
 * @glyph: The glyph ID to query
 * @direction: The direction of the text segment
 * @extents: (out): The #glyph_extents_t retrieved
 *
 * Fetches the #glyph_extents_t data for a glyph ID
 * in the specified font, with respect to the origin in
 * a text segment in the specified direction.
 *
 * Calls the appropriate direction-specific variant (horizontal
 * or vertical) depending on the value of @direction.
 *
 * Return value: `true` if data found, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
font_get_glyph_extents_for_origin (font_t          *font,
				      codepoint_t      glyph,
				      direction_t      direction,
				      glyph_extents_t *extents)
{
  return font->get_glyph_extents_for_origin (glyph, direction, extents);
}

/**
 * font_get_glyph_contour_point_for_origin:
 * @font: #font_t to work upon
 * @glyph: The glyph ID to query
 * @point_index: The contour-point index to query
 * @direction: The direction of the text segment
 * @x: (out): The X value retrieved for the contour point
 * @y: (out): The Y value retrieved for the contour point
 *
 * Fetches the (X,Y) coordinates of a specified contour-point index
 * in the specified glyph ID in the specified font, with respect
 * to the origin in a text segment in the specified direction.
 *
 * Calls the appropriate direction-specific variant (horizontal
 * or vertical) depending on the value of @direction.
 *
 * Return value: `true` if data found, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
font_get_glyph_contour_point_for_origin (font_t      *font,
					    codepoint_t  glyph,
					    unsigned int    point_index,
					    direction_t  direction,
					    position_t  *x,
					    position_t  *y)
{
  return font->get_glyph_contour_point_for_origin (glyph, point_index, direction, x, y);
}

/**
 * font_glyph_to_string:
 * @font: #font_t to work upon
 * @glyph: The glyph ID to query
 * @s: (out) (array length=size): The string containing the glyph name
 * @size: Length of string @s
 *
 * Fetches the name of the specified glyph ID in @font and returns
 * it in string @s.
 *
 * If the glyph ID has no name in @font, a string of the form `gidDDD` is
 * generated, with `DDD` being the glyph ID.
 *
 * According to the OpenType specification, glyph names are limited to 63
 * characters and can only contain (a subset of) ASCII.
 *
 * Since: 0.9.2
 **/
void
font_glyph_to_string (font_t      *font,
			 codepoint_t  glyph,
			 char           *s,
			 unsigned int    size)
{
  font->glyph_to_string (glyph, s, size);
}

/**
 * font_glyph_from_string:
 * @font: #font_t to work upon
 * @s: (array length=len) (element-type uint8_t): string to query
 * @len: The length of the string @s
 * @glyph: (out): The glyph ID corresponding to the string requested
 *
 * Fetches the glyph ID from @font that matches the specified string.
 * Strings of the format `gidDDD` or `uniUUUU` are parsed automatically.
 *
 * <note>Note: @len == -1 means the string is null-terminated.</note>
 *
 * Return value: `true` if data found, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
font_glyph_from_string (font_t      *font,
			   const char     *s,
			   int             len,
			   codepoint_t *glyph)
{
  return font->glyph_from_string (s, len, glyph);
}


/*
 * font_t
 */

DEFINE_NULL_INSTANCE (font_t) =
{
  HB_OBJECT_HEADER_STATIC,

  0, /* serial */
  0, /* serial_coords */

  nullptr, /* parent */
  const_cast<face_t *> (&_Null_face_t),

  1000, /* x_scale */
  1000, /* y_scale */
  0.f, /* x_embolden */
  0.f, /* y_embolden */
  true, /* embolden_in_place */
  0, /* x_strength */
  0, /* y_strength */
  0.f, /* slant */
  0.f, /* slant_xy; */
  1.f, /* x_multf */
  1.f, /* y_multf */
  1<<16, /* x_mult */
  1<<16, /* y_mult */

  0, /* x_ppem */
  0, /* y_ppem */
  0, /* ptem */

  HB_FONT_NO_VAR_NAMED_INSTANCE, /* instance_index */
  0, /* num_coords */
  nullptr, /* coords */
  nullptr, /* design_coords */

  const_cast<font_funcs_t *> (&_Null_font_funcs_t),

  /* Zero for the rest is fine. */
};


static font_t *
_font_create (face_t *face)
{
  font_t *font;

  if (unlikely (!face))
    face = face_get_empty ();

  if (!(font = object_create<font_t> ()))
    return font_get_empty ();

  face_make_immutable (face);
  font->parent = font_get_empty ();
  font->face = face_reference (face);
  font->klass = font_funcs_get_empty ();
  font->data.init0 (font);
  font->x_scale = font->y_scale = face->get_upem ();
  font->embolden_in_place = true;
  font->x_multf = font->y_multf = 1.f;
  font->x_mult = font->y_mult = 1 << 16;
  font->instance_index = HB_FONT_NO_VAR_NAMED_INSTANCE;

  return font;
}

/**
 * font_create:
 * @face: a face.
 *
 * Constructs a new font object from the specified face.
 *
 * <note>Note: If @face's index value (as passed to face_create()
 * has non-zero top 16-bits, those bits minus one are passed to
 * font_set_var_named_instance(), effectively loading a named-instance
 * of a variable font, instead of the default-instance.  This allows
 * specifying which named-instance to load by default when creating the
 * face.</note>
 *
 * Return value: (transfer full): The new font object
 *
 * Since: 0.9.2
 **/
font_t *
font_create (face_t *face)
{
  font_t *font = _font_create (face);

#ifndef HB_NO_OT_FONT
  /* Install our in-house, very lightweight, funcs. */
  ot_font_set_funcs (font);
#endif

#ifndef HB_NO_VAR
  if (face && face->index >> 16)
    font_set_var_named_instance (font, (face->index >> 16) - 1);
#endif

  return font;
}

static void
_font_adopt_var_coords (font_t *font,
			   int *coords, /* 2.14 normalized */
			   float *design_coords,
			   unsigned int coords_length)
{
  free (font->coords);
  free (font->design_coords);

  font->coords = coords;
  font->design_coords = design_coords;
  font->num_coords = coords_length;

  font->mults_changed (); // Easiest to call this to drop cached data
}

/**
 * font_create_sub_font:
 * @parent: The parent font object
 *
 * Constructs a sub-font font object from the specified @parent font,
 * replicating the parent's properties.
 *
 * Return value: (transfer full): The new sub-font font object
 *
 * Since: 0.9.2
 **/
font_t *
font_create_sub_font (font_t *parent)
{
  if (unlikely (!parent))
    parent = font_get_empty ();

  font_t *font = _font_create (parent->face);

  if (unlikely (object_is_immutable (font)))
    return font;

  font->parent = font_reference (parent);

  font->x_scale = parent->x_scale;
  font->y_scale = parent->y_scale;
  font->x_embolden = parent->x_embolden;
  font->y_embolden = parent->y_embolden;
  font->embolden_in_place = parent->embolden_in_place;
  font->slant = parent->slant;
  font->x_ppem = parent->x_ppem;
  font->y_ppem = parent->y_ppem;
  font->ptem = parent->ptem;

  unsigned int num_coords = parent->num_coords;
  if (num_coords)
  {
    int *coords = (int *) calloc (num_coords, sizeof (parent->coords[0]));
    float *design_coords = (float *) calloc (num_coords, sizeof (parent->design_coords[0]));
    if (likely (coords && design_coords))
    {
      memcpy (coords, parent->coords, num_coords * sizeof (parent->coords[0]));
      memcpy (design_coords, parent->design_coords, num_coords * sizeof (parent->design_coords[0]));
      _font_adopt_var_coords (font, coords, design_coords, num_coords);
    }
    else
    {
      free (coords);
      free (design_coords);
    }
  }

  font->mults_changed ();

  return font;
}

/**
 * font_get_empty:
 *
 * Fetches the empty font object.
 *
 * Return value: (transfer full): The empty font object
 *
 * Since: 0.9.2
 **/
font_t *
font_get_empty ()
{
  return const_cast<font_t *> (&Null (font_t));
}

/**
 * font_reference: (skip)
 * @font: #font_t to work upon
 *
 * Increases the reference count on the given font object.
 *
 * Return value: (transfer full): The @font object
 *
 * Since: 0.9.2
 **/
font_t *
font_reference (font_t *font)
{
  return object_reference (font);
}

/**
 * font_destroy: (skip)
 * @font: #font_t to work upon
 *
 * Decreases the reference count on the given font object. When the
 * reference count reaches zero, the font is destroyed,
 * freeing all memory.
 *
 * Since: 0.9.2
 **/
void
font_destroy (font_t *font)
{
  if (!object_destroy (font)) return;

  font->data.fini ();

  if (font->destroy)
    font->destroy (font->user_data);

  font_destroy (font->parent);
  face_destroy (font->face);
  font_funcs_destroy (font->klass);

  free (font->coords);
  free (font->design_coords);

  free (font);
}

/**
 * font_set_user_data: (skip)
 * @font: #font_t to work upon
 * @key: The user-data key
 * @data: A pointer to the user data
 * @destroy: (nullable): A callback to call when @data is not needed anymore
 * @replace: Whether to replace an existing data with the same key
 *
 * Attaches a user-data key/data pair to the specified font object.
 *
 * Return value: `true` if success, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
font_set_user_data (font_t          *font,
		       user_data_key_t *key,
		       void *              data,
		       destroy_func_t   destroy /* May be NULL. */,
		       bool_t           replace)
{
  if (!object_is_immutable (font))
    font->serial++;

  return object_set_user_data (font, key, data, destroy, replace);
}

/**
 * font_get_user_data: (skip)
 * @font: #font_t to work upon
 * @key: The user-data key to query
 *
 * Fetches the user-data object associated with the specified key,
 * attached to the specified font object.
 *
 * Return value: (transfer none): Pointer to the user data
 *
 * Since: 0.9.2
 **/
void *
font_get_user_data (const font_t    *font,
		       user_data_key_t *key)
{
  return object_get_user_data (font, key);
}

/**
 * font_make_immutable:
 * @font: #font_t to work upon
 *
 * Makes @font immutable.
 *
 * Since: 0.9.2
 **/
void
font_make_immutable (font_t *font)
{
  if (object_is_immutable (font))
    return;

  if (font->parent)
    font_make_immutable (font->parent);

  object_make_immutable (font);
}

/**
 * font_is_immutable:
 * @font: #font_t to work upon
 *
 * Tests whether a font object is immutable.
 *
 * Return value: `true` if @font is immutable, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
font_is_immutable (font_t *font)
{
  return object_is_immutable (font);
}

/**
 * font_get_serial:
 * @font: #font_t to work upon
 *
 * Returns the internal serial number of the font. The serial
 * number is increased every time a setting on the font is
 * changed, using a setter function.
 *
 * Return value: serial number
 *
 * Since: 4.4.0
 **/
unsigned int
font_get_serial (font_t *font)
{
  return font->serial;
}

/**
 * font_changed:
 * @font: #font_t to work upon
 *
 * Notifies the @font that underlying font data has changed.
 * This has the effect of increasing the serial as returned
 * by font_get_serial(), which invalidates internal caches.
 *
 * Since: 4.4.0
 **/
void
font_changed (font_t *font)
{
  if (object_is_immutable (font))
    return;

  font->serial++;

  font->mults_changed ();
}

/**
 * font_set_parent:
 * @font: #font_t to work upon
 * @parent: The parent font object to assign
 *
 * Sets the parent font of @font.
 *
 * Since: 1.0.5
 **/
void
font_set_parent (font_t *font,
		    font_t *parent)
{
  if (object_is_immutable (font))
    return;

  if (parent == font->parent)
    return;

  font->serial++;

  if (!parent)
    parent = font_get_empty ();

  font_t *old = font->parent;

  font->parent = font_reference (parent);

  font_destroy (old);
}

/**
 * font_get_parent:
 * @font: #font_t to work upon
 *
 * Fetches the parent font of @font.
 *
 * Return value: (transfer none): The parent font object
 *
 * Since: 0.9.2
 **/
font_t *
font_get_parent (font_t *font)
{
  return font->parent;
}

/**
 * font_set_face:
 * @font: #font_t to work upon
 * @face: The #face_t to assign
 *
 * Sets @face as the font-face value of @font.
 *
 * Since: 1.4.3
 **/
void
font_set_face (font_t *font,
		  face_t *face)
{
  if (object_is_immutable (font))
    return;

  if (face == font->face)
    return;

  font->serial++;

  if (unlikely (!face))
    face = face_get_empty ();

  face_t *old = font->face;

  face_make_immutable (face);
  font->face = face_reference (face);
  font->mults_changed ();

  face_destroy (old);
}

/**
 * font_get_face:
 * @font: #font_t to work upon
 *
 * Fetches the face associated with the specified font object.
 *
 * Return value: (transfer none): The #face_t value
 *
 * Since: 0.9.2
 **/
face_t *
font_get_face (font_t *font)
{
  return font->face;
}


/**
 * font_set_funcs:
 * @font: #font_t to work upon
 * @klass: (closure font_data) (destroy destroy) (scope notified): The font-functions structure.
 * @font_data: Data to attach to @font
 * @destroy: (nullable): The function to call when @font_data is not needed anymore
 *
 * Replaces the font-functions structure attached to a font, updating
 * the font's user-data with @font-data and the @destroy callback.
 *
 * Since: 0.9.2
 **/
void
font_set_funcs (font_t         *font,
		   font_funcs_t   *klass,
		   void              *font_data,
		   destroy_func_t  destroy /* May be NULL. */)
{
  if (object_is_immutable (font))
  {
    if (destroy)
      destroy (font_data);
    return;
  }

  font->serial++;

  if (font->destroy)
    font->destroy (font->user_data);

  if (!klass)
    klass = font_funcs_get_empty ();

  font_funcs_reference (klass);
  font_funcs_destroy (font->klass);
  font->klass = klass;
  font->user_data = font_data;
  font->destroy = destroy;
}

/**
 * font_set_funcs_data:
 * @font: #font_t to work upon
 * @font_data: (destroy destroy) (scope notified): Data to attach to @font
 * @destroy: (nullable): The function to call when @font_data is not needed anymore
 *
 * Replaces the user data attached to a font, updating the font's
 * @destroy callback.
 *
 * Since: 0.9.2
 **/
void
font_set_funcs_data (font_t         *font,
		        void              *font_data,
		        destroy_func_t  destroy /* May be NULL. */)
{
  /* Destroy user_data? */
  if (object_is_immutable (font))
  {
    if (destroy)
      destroy (font_data);
    return;
  }

  font->serial++;

  if (font->destroy)
    font->destroy (font->user_data);

  font->user_data = font_data;
  font->destroy = destroy;
}


/**
 * font_set_scale:
 * @font: #font_t to work upon
 * @x_scale: Horizontal scale value to assign
 * @y_scale: Vertical scale value to assign
 *
 * Sets the horizontal and vertical scale of a font.
 *
 * The font scale is a number related to, but not the same as,
 * font size. Typically the client establishes a scale factor
 * to be used between the two. For example, 64, or 256, which
 * would be the fractional-precision part of the font scale.
 * This is necessary because #position_t values are integer
 * types and you need to leave room for fractional values
 * in there.
 *
 * For example, to set the font size to 20, with 64
 * levels of fractional precision you would call
 * `font_set_scale(font, 20 * 64, 20 * 64)`.
 *
 * In the example above, even what font size 20 means is up to
 * you. It might be 20 pixels, or 20 points, or 20 millimeters.
 * HarfBuzz does not care about that.  You can set the point
 * size of the font using font_set_ptem(), and the pixel
 * size using font_set_ppem().
 *
 * The choice of scale is yours but needs to be consistent between
 * what you set here, and what you expect out of #position_t
 * as well has draw / paint API output values.
 *
 * Fonts default to a scale equal to the UPEM value of their face.
 * A font with this setting is sometimes called an "unscaled" font.
 *
 * Since: 0.9.2
 **/
void
font_set_scale (font_t *font,
		   int        x_scale,
		   int        y_scale)
{
  if (object_is_immutable (font))
    return;

  if (font->x_scale == x_scale && font->y_scale == y_scale)
    return;

  font->serial++;

  font->x_scale = x_scale;
  font->y_scale = y_scale;
  font->mults_changed ();
}

/**
 * font_get_scale:
 * @font: #font_t to work upon
 * @x_scale: (out): Horizontal scale value
 * @y_scale: (out): Vertical scale value
 *
 * Fetches the horizontal and vertical scale of a font.
 *
 * Since: 0.9.2
 **/
void
font_get_scale (font_t *font,
		   int       *x_scale,
		   int       *y_scale)
{
  if (x_scale) *x_scale = font->x_scale;
  if (y_scale) *y_scale = font->y_scale;
}

/**
 * font_set_ppem:
 * @font: #font_t to work upon
 * @x_ppem: Horizontal ppem value to assign
 * @y_ppem: Vertical ppem value to assign
 *
 * Sets the horizontal and vertical pixels-per-em (PPEM) of a font.
 *
 * These values are used for pixel-size-specific adjustment to
 * shaping and draw results, though for the most part they are
 * unused and can be left unset.
 *
 * Since: 0.9.2
 **/
void
font_set_ppem (font_t    *font,
		  unsigned int  x_ppem,
		  unsigned int  y_ppem)
{
  if (object_is_immutable (font))
    return;

  if (font->x_ppem == x_ppem && font->y_ppem == y_ppem)
    return;

  font->serial++;

  font->x_ppem = x_ppem;
  font->y_ppem = y_ppem;
}

/**
 * font_get_ppem:
 * @font: #font_t to work upon
 * @x_ppem: (out): Horizontal ppem value
 * @y_ppem: (out): Vertical ppem value
 *
 * Fetches the horizontal and vertical points-per-em (ppem) of a font.
 *
 * Since: 0.9.2
 **/
void
font_get_ppem (font_t    *font,
		  unsigned int *x_ppem,
		  unsigned int *y_ppem)
{
  if (x_ppem) *x_ppem = font->x_ppem;
  if (y_ppem) *y_ppem = font->y_ppem;
}

/**
 * font_set_ptem:
 * @font: #font_t to work upon
 * @ptem: font size in points.
 *
 * Sets the "point size" of a font. Set to zero to unset.
 * Used in CoreText to implement optical sizing.
 *
 * <note>Note: There are 72 points in an inch.</note>
 *
 * Since: 1.6.0
 **/
void
font_set_ptem (font_t *font,
		  float      ptem)
{
  if (object_is_immutable (font))
    return;

  if (font->ptem == ptem)
    return;

  font->serial++;

  font->ptem = ptem;
}

/**
 * font_get_ptem:
 * @font: #font_t to work upon
 *
 * Fetches the "point size" of a font. Used in CoreText to
 * implement optical sizing.
 *
 * Return value: Point size.  A value of zero means "not set."
 *
 * Since: 1.6.0
 **/
float
font_get_ptem (font_t *font)
{
  return font->ptem;
}

/**
 * font_set_synthetic_bold:
 * @font: #font_t to work upon
 * @x_embolden: the amount to embolden horizontally
 * @y_embolden: the amount to embolden vertically
 * @in_place: whether to embolden glyphs in-place
 *
 * Sets the "synthetic boldness" of a font.
 *
 * Positive values for @x_embolden / @y_embolden make a font
 * bolder, negative values thinner. Typical values are in the
 * 0.01 to 0.05 range. The default value is zero.
 *
 * Synthetic boldness is applied by offsetting the contour
 * points of the glyph shape.
 *
 * Synthetic boldness is applied when rendering a glyph via
 * font_draw_glyph().
 *
 * If @in_place is `false`, then glyph advance-widths are also
 * adjusted, otherwise they are not.  The in-place mode is
 * useful for simulating [font grading](https://fonts.google.com/knowledge/glossary/grade).
 *
 *
 * Since: 7.0.0
 **/
void
font_set_synthetic_bold (font_t *font,
			    float x_embolden,
			    float y_embolden,
			    bool_t in_place)
{
  if (object_is_immutable (font))
    return;

  if (font->x_embolden == x_embolden &&
      font->y_embolden == y_embolden &&
      font->embolden_in_place == (bool) in_place)
    return;

  font->serial++;

  font->x_embolden = x_embolden;
  font->y_embolden = y_embolden;
  font->embolden_in_place = in_place;
  font->mults_changed ();
}

/**
 * font_get_synthetic_bold:
 * @font: #font_t to work upon
 * @x_embolden: (out): return location for horizontal value
 * @y_embolden: (out): return location for vertical value
 * @in_place: (out): return location for in-place value
 *
 * Fetches the "synthetic boldness" parameters of a font.
 *
 * Since: 7.0.0
 **/
void
font_get_synthetic_bold (font_t *font,
			    float *x_embolden,
			    float *y_embolden,
			    bool_t *in_place)
{
  if (x_embolden) *x_embolden = font->x_embolden;
  if (y_embolden) *y_embolden = font->y_embolden;
  if (in_place) *in_place = font->embolden_in_place;
}

/**
 * font_set_synthetic_slant:
 * @font: #font_t to work upon
 * @slant: synthetic slant value.
 *
 * Sets the "synthetic slant" of a font.  By default is zero.
 * Synthetic slant is the graphical skew applied to the font
 * at rendering time.
 *
 * HarfBuzz needs to know this value to adjust shaping results,
 * metrics, and style values to match the slanted rendering.
 *
 * <note>Note: The glyph shape fetched via the font_draw_glyph()
 * function is slanted to reflect this value as well.</note>
 *
 * <note>Note: The slant value is a ratio.  For example, a
 * 20% slant would be represented as a 0.2 value.</note>
 *
 * Since: 3.3.0
 **/
HB_EXTERN void
font_set_synthetic_slant (font_t *font, float slant)
{
  if (object_is_immutable (font))
    return;

  if (font->slant == slant)
    return;

  font->serial++;

  font->slant = slant;
  font->mults_changed ();
}

/**
 * font_get_synthetic_slant:
 * @font: #font_t to work upon
 *
 * Fetches the "synthetic slant" of a font.
 *
 * Return value: Synthetic slant.  By default is zero.
 *
 * Since: 3.3.0
 **/
HB_EXTERN float
font_get_synthetic_slant (font_t *font)
{
  return font->slant;
}

#ifndef HB_NO_VAR
/*
 * Variations
 */

/**
 * font_set_variations:
 * @font: #font_t to work upon
 * @variations: (array length=variations_length): Array of variation settings to apply
 * @variations_length: Number of variations to apply
 *
 * Applies a list of font-variation settings to a font.
 *
 * Note that this overrides all existing variations set on @font.
 * Axes not included in @variations will be effectively set to their
 * default values.
 *
 * Since: 1.4.2
 */
void
font_set_variations (font_t            *font,
			const variation_t *variations,
			unsigned int          variations_length)
{
  if (object_is_immutable (font))
    return;

  font->serial_coords = ++font->serial;

  if (!variations_length && font->instance_index == HB_FONT_NO_VAR_NAMED_INSTANCE)
  {
    font_set_var_coords_normalized (font, nullptr, 0);
    return;
  }

  const OT::fvar &fvar = *font->face->table.fvar;
  auto axes = fvar.get_axes ();
  const unsigned coords_length = axes.length;

  int *normalized = coords_length ? (int *) calloc (coords_length, sizeof (int)) : nullptr;
  float *design_coords = coords_length ? (float *) calloc (coords_length, sizeof (float)) : nullptr;

  if (unlikely (coords_length && !(normalized && design_coords)))
  {
    free (normalized);
    free (design_coords);
    return;
  }

  /* Initialize design coords. */
  for (unsigned int i = 0; i < coords_length; i++)
    design_coords[i] = axes[i].get_default ();
  if (font->instance_index != HB_FONT_NO_VAR_NAMED_INSTANCE)
  {
    unsigned count = coords_length;
    /* This may fail if index is out-of-range;
     * That's why we initialize design_coords from fvar above
     * unconditionally. */
    ot_var_named_instance_get_design_coords (font->face, font->instance_index,
						&count, design_coords);
  }

  for (unsigned int i = 0; i < variations_length; i++)
  {
    const auto tag = variations[i].tag;
    const auto v = variations[i].value;
    for (unsigned axis_index = 0; axis_index < coords_length; axis_index++)
      if (axes[axis_index].axisTag == tag)
	design_coords[axis_index] = v;
  }

  ot_var_normalize_coords (font->face, coords_length, design_coords, normalized);
  _font_adopt_var_coords (font, normalized, design_coords, coords_length);
}

/**
 * font_set_variation:
 * @font: #font_t to work upon
 * @tag: The #tag_t tag of the variation-axis name
 * @value: The value of the variation axis
 *
 * Change the value of one variation axis on the font.
 *
 * Note: This function is expensive to be called repeatedly.
 *   If you want to set multiple variation axes at the same time,
 *   use font_set_variations() instead.
 *
 * Since: 7.1.0
 */
void
font_set_variation (font_t *font,
		       tag_t tag,
		       float    value)
{
  if (object_is_immutable (font))
    return;

  font->serial_coords = ++font->serial;

  // TODO Share some of this code with set_variations()

  const OT::fvar &fvar = *font->face->table.fvar;
  auto axes = fvar.get_axes ();
  const unsigned coords_length = axes.length;

  int *normalized = coords_length ? (int *) calloc (coords_length, sizeof (int)) : nullptr;
  float *design_coords = coords_length ? (float *) calloc (coords_length, sizeof (float)) : nullptr;

  if (unlikely (coords_length && !(normalized && design_coords)))
  {
    free (normalized);
    free (design_coords);
    return;
  }

  /* Initialize design coords. */
  if (font->design_coords)
  {
    assert (coords_length == font->num_coords);
    for (unsigned int i = 0; i < coords_length; i++)
      design_coords[i] = font->design_coords[i];
  }
  else
  {
    for (unsigned int i = 0; i < coords_length; i++)
      design_coords[i] = axes[i].get_default ();
    if (font->instance_index != HB_FONT_NO_VAR_NAMED_INSTANCE)
    {
      unsigned count = coords_length;
      /* This may fail if index is out-of-range;
       * That's why we initialize design_coords from fvar above
       * unconditionally. */
      ot_var_named_instance_get_design_coords (font->face, font->instance_index,
						  &count, design_coords);
    }
  }

  for (unsigned axis_index = 0; axis_index < coords_length; axis_index++)
    if (axes[axis_index].axisTag == tag)
      design_coords[axis_index] = value;

  ot_var_normalize_coords (font->face, coords_length, design_coords, normalized);
  _font_adopt_var_coords (font, normalized, design_coords, coords_length);

}

/**
 * font_set_var_coords_design:
 * @font: #font_t to work upon
 * @coords: (array length=coords_length): Array of variation coordinates to apply
 * @coords_length: Number of coordinates to apply
 *
 * Applies a list of variation coordinates (in design-space units)
 * to a font.
 *
 * Note that this overrides all existing variations set on @font.
 * Axes not included in @coords will be effectively set to their
 * default values.
 *
 * Since: 1.4.2
 */
void
font_set_var_coords_design (font_t    *font,
			       const float  *coords,
			       unsigned int  coords_length)
{
  if (object_is_immutable (font))
    return;

  font->serial_coords = ++font->serial;

  int *normalized = coords_length ? (int *) calloc (coords_length, sizeof (int)) : nullptr;
  float *design_coords = coords_length ? (float *) calloc (coords_length, sizeof (float)) : nullptr;

  if (unlikely (coords_length && !(normalized && design_coords)))
  {
    free (normalized);
    free (design_coords);
    return;
  }

  if (coords_length)
    memcpy (design_coords, coords, coords_length * sizeof (font->design_coords[0]));

  ot_var_normalize_coords (font->face, coords_length, coords, normalized);
  _font_adopt_var_coords (font, normalized, design_coords, coords_length);
}

/**
 * font_set_var_named_instance:
 * @font: a font.
 * @instance_index: named instance index.
 *
 * Sets design coords of a font from a named-instance index.
 *
 * Since: 2.6.0
 */
void
font_set_var_named_instance (font_t *font,
				unsigned int instance_index)
{
  if (object_is_immutable (font))
    return;

  if (font->instance_index == instance_index)
    return;

  font->serial_coords = ++font->serial;

  font->instance_index = instance_index;
  font_set_variations (font, nullptr, 0);
}

/**
 * font_get_var_named_instance:
 * @font: a font.
 *
 * Returns the currently-set named-instance index of the font.
 *
 * Return value: Named-instance index or %HB_FONT_NO_VAR_NAMED_INSTANCE.
 *
 * Since: 7.0.0
 **/
unsigned int
font_get_var_named_instance (font_t *font)
{
  return font->instance_index;
}

/**
 * font_set_var_coords_normalized:
 * @font: #font_t to work upon
 * @coords: (array length=coords_length): Array of variation coordinates to apply
 * @coords_length: Number of coordinates to apply
 *
 * Applies a list of variation coordinates (in normalized units)
 * to a font.
 *
 * Note that this overrides all existing variations set on @font.
 * Axes not included in @coords will be effectively set to their
 * default values.
 *
 * <note>Note: Coordinates should be normalized to 2.14.</note>
 *
 * Since: 1.4.2
 */
void
font_set_var_coords_normalized (font_t    *font,
				   const int    *coords, /* 2.14 normalized */
				   unsigned int  coords_length)
{
  if (object_is_immutable (font))
    return;

  font->serial_coords = ++font->serial;

  int *copy = coords_length ? (int *) calloc (coords_length, sizeof (coords[0])) : nullptr;
  int *unmapped = coords_length ? (int *) calloc (coords_length, sizeof (coords[0])) : nullptr;
  float *design_coords = coords_length ? (float *) calloc (coords_length, sizeof (design_coords[0])) : nullptr;

  if (unlikely (coords_length && !(copy && unmapped && design_coords)))
  {
    free (copy);
    free (unmapped);
    free (design_coords);
    return;
  }

  if (coords_length)
  {
    memcpy (copy, coords, coords_length * sizeof (coords[0]));
    memcpy (unmapped, coords, coords_length * sizeof (coords[0]));
  }

  /* Best effort design coords simulation */
  font->face->table.avar->unmap_coords (unmapped, coords_length);
  for (unsigned int i = 0; i < coords_length; ++i)
    design_coords[i] = font->face->table.fvar->unnormalize_axis_value (i, unmapped[i]);
  free (unmapped);

  _font_adopt_var_coords (font, copy, design_coords, coords_length);
}

/**
 * font_get_var_coords_normalized:
 * @font: #font_t to work upon
 * @length: (out): Number of coordinates retrieved
 *
 * Fetches the list of normalized variation coordinates currently
 * set on a font.
 *
 * Note that this returned array may only contain values for some
 * (or none) of the axes; omitted axes effectively have zero values.
 *
 * Return value is valid as long as variation coordinates of the font
 * are not modified.
 *
 * Return value: coordinates array
 *
 * Since: 1.4.2
 */
const int *
font_get_var_coords_normalized (font_t    *font,
				   unsigned int *length)
{
  if (length)
    *length = font->num_coords;

  return font->coords;
}

/**
 * font_get_var_coords_design:
 * @font: #font_t to work upon
 * @length: (out): Number of coordinates retrieved
 *
 * Fetches the list of variation coordinates (in design-space units) currently
 * set on a font.
 *
 * Note that this returned array may only contain values for some
 * (or none) of the axes; omitted axes effectively have their default
 * values.
 *
 * Return value is valid as long as variation coordinates of the font
 * are not modified.
 *
 * Return value: coordinates array
 *
 * Since: 3.3.0
 */
const float *
font_get_var_coords_design (font_t *font,
			       unsigned int *length)
{
  if (length)
    *length = font->num_coords;

  return font->design_coords;
}
#endif

#ifndef HB_DISABLE_DEPRECATED
/*
 * Deprecated get_glyph_func():
 */

struct trampoline_closure_t
{
  void *user_data;
  destroy_func_t destroy;
  unsigned int ref_count;
};

template <typename FuncType>
struct trampoline_t
{
  trampoline_closure_t closure; /* Must be first. */
  FuncType func;
};

template <typename FuncType>
static trampoline_t<FuncType> *
trampoline_create (FuncType           func,
		   void              *user_data,
		   destroy_func_t  destroy)
{
  typedef trampoline_t<FuncType> trampoline_t;

  trampoline_t *trampoline = (trampoline_t *) calloc (1, sizeof (trampoline_t));

  if (unlikely (!trampoline))
    return nullptr;

  trampoline->closure.user_data = user_data;
  trampoline->closure.destroy = destroy;
  trampoline->closure.ref_count = 1;
  trampoline->func = func;

  return trampoline;
}

static void
trampoline_reference (trampoline_closure_t *closure)
{
  closure->ref_count++;
}

static void
trampoline_destroy (void *user_data)
{
  trampoline_closure_t *closure = (trampoline_closure_t *) user_data;

  if (--closure->ref_count)
    return;

  if (closure->destroy)
    closure->destroy (closure->user_data);
  free (closure);
}

typedef trampoline_t<font_get_glyph_func_t> font_get_glyph_trampoline_t;

static bool_t
font_get_nominal_glyph_trampoline (font_t      *font,
				      void           *font_data,
				      codepoint_t  unicode,
				      codepoint_t *glyph,
				      void           *user_data)
{
  font_get_glyph_trampoline_t *trampoline = (font_get_glyph_trampoline_t *) user_data;
  return trampoline->func (font, font_data, unicode, 0, glyph, trampoline->closure.user_data);
}

static bool_t
font_get_variation_glyph_trampoline (font_t      *font,
					void           *font_data,
					codepoint_t  unicode,
					codepoint_t  variation_selector,
					codepoint_t *glyph,
					void           *user_data)
{
  font_get_glyph_trampoline_t *trampoline = (font_get_glyph_trampoline_t *) user_data;
  return trampoline->func (font, font_data, unicode, variation_selector, glyph, trampoline->closure.user_data);
}

/**
 * font_funcs_set_glyph_func:
 * @ffuncs: The font-functions structure
 * @func: (closure user_data) (destroy destroy) (scope notified): callback function
 * @user_data: data to pass to @func
 * @destroy: (nullable): function to call when @user_data is not needed anymore
 *
 * Deprecated.  Use font_funcs_set_nominal_glyph_func() and
 * font_funcs_set_variation_glyph_func() instead.
 *
 * Since: 0.9.2
 * Deprecated: 1.2.3
 **/
void
font_funcs_set_glyph_func (font_funcs_t          *ffuncs,
			      font_get_glyph_func_t  func,
			      void                     *user_data,
			      destroy_func_t         destroy /* May be NULL. */)
{
  if (object_is_immutable (ffuncs))
  {
    if (destroy)
      destroy (user_data);
    return;
  }

  font_get_glyph_trampoline_t *trampoline;

  trampoline = trampoline_create (func, user_data, destroy);
  if (unlikely (!trampoline))
  {
    if (destroy)
      destroy (user_data);
    return;
  }

  /* Since we pass it to two destroying functions. */
  trampoline_reference (&trampoline->closure);

  font_funcs_set_nominal_glyph_func (ffuncs,
					font_get_nominal_glyph_trampoline,
					trampoline,
					trampoline_destroy);

  font_funcs_set_variation_glyph_func (ffuncs,
					  font_get_variation_glyph_trampoline,
					  trampoline,
					  trampoline_destroy);
}
#endif


#ifndef HB_DISABLE_DEPRECATED
void
font_funcs_set_glyph_shape_func (font_funcs_t               *ffuncs,
                                   font_get_glyph_shape_func_t  func,
                                   void                           *user_data,
                                   destroy_func_t               destroy /* May be NULL. */)
{
  font_funcs_set_draw_glyph_func (ffuncs, func, user_data, destroy);
}
#endif
