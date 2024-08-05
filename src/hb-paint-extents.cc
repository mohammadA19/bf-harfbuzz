/*
 * Copyright © 2022 Behdad Esfahbod
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
 */

#include "hb.hh"

#ifndef HB_NO_PAINT

#include "hb-paint-extents.hh"

#include "hb-draw.h"

#include "hb-machinery.hh"


/*
 * This file implements bounds-extraction as well as boundedness
 * computation of COLRv1 fonts as described in:
 *
 * https://learn.microsoft.com/en-us/typography/opentype/spec/colr#glyph-metrics-and-boundedness
 */

static void
paint_extents_push_transform (paint_funcs_t *funcs HB_UNUSED,
				 void *paint_data,
				 float xx, float yx,
				 float xy, float yy,
				 float dx, float dy,
				 void *user_data HB_UNUSED)
{
  paint_extents_context_t *c = (paint_extents_context_t *) paint_data;

  c->push_transform (transform_t {xx, yx, xy, yy, dx, dy});
}

static void
paint_extents_pop_transform (paint_funcs_t *funcs HB_UNUSED,
			        void *paint_data,
				void *user_data HB_UNUSED)
{
  paint_extents_context_t *c = (paint_extents_context_t *) paint_data;

  c->pop_transform ();
}

static void
draw_extents_move_to (draw_funcs_t *dfuncs HB_UNUSED,
			 void *data,
			 draw_state_t *st,
			 float to_x, float to_y,
			 void *user_data HB_UNUSED)
{
  extents_t *extents = (extents_t *) data;

  extents->add_point (to_x, to_y);
}

static void
draw_extents_line_to (draw_funcs_t *dfuncs HB_UNUSED,
			 void *data,
			 draw_state_t *st,
			 float to_x, float to_y,
			 void *user_data HB_UNUSED)
{
  extents_t *extents = (extents_t *) data;

  extents->add_point (to_x, to_y);
}

static void
draw_extents_quadratic_to (draw_funcs_t *dfuncs HB_UNUSED,
			      void *data,
			      draw_state_t *st,
			      float control_x, float control_y,
			      float to_x, float to_y,
			      void *user_data HB_UNUSED)
{
  extents_t *extents = (extents_t *) data;

  extents->add_point (control_x, control_y);
  extents->add_point (to_x, to_y);
}

static void
draw_extents_cubic_to (draw_funcs_t *dfuncs HB_UNUSED,
			  void *data,
			  draw_state_t *st,
			  float control1_x, float control1_y,
			  float control2_x, float control2_y,
			  float to_x, float to_y,
			  void *user_data HB_UNUSED)
{
  extents_t *extents = (extents_t *) data;

  extents->add_point (control1_x, control1_y);
  extents->add_point (control2_x, control2_y);
  extents->add_point (to_x, to_y);
}

static inline void free_static_draw_extents_funcs ();

static struct draw_extents_funcs_lazy_loader_t : draw_funcs_lazy_loader_t<draw_extents_funcs_lazy_loader_t>
{
  static draw_funcs_t *create ()
  {
    draw_funcs_t *funcs = draw_funcs_create ();

    draw_funcs_set_move_to_func (funcs, draw_extents_move_to, nullptr, nullptr);
    draw_funcs_set_line_to_func (funcs, draw_extents_line_to, nullptr, nullptr);
    draw_funcs_set_quadratic_to_func (funcs, draw_extents_quadratic_to, nullptr, nullptr);
    draw_funcs_set_cubic_to_func (funcs, draw_extents_cubic_to, nullptr, nullptr);

    draw_funcs_make_immutable (funcs);

    atexit (free_static_draw_extents_funcs);

    return funcs;
  }
} static_draw_extents_funcs;

static inline
void free_static_draw_extents_funcs ()
{
  static_draw_extents_funcs.free_instance ();
}

static draw_funcs_t *
draw_extents_get_funcs ()
{
  return static_draw_extents_funcs.get_unconst ();
}

static void
paint_extents_push_clip_glyph (paint_funcs_t *funcs HB_UNUSED,
				  void *paint_data,
				  codepoint_t glyph,
				  font_t *font,
				  void *user_data HB_UNUSED)
{
  paint_extents_context_t *c = (paint_extents_context_t *) paint_data;

  extents_t extents;
  draw_funcs_t *draw_extent_funcs = draw_extents_get_funcs ();
  font_draw_glyph (font, glyph, draw_extent_funcs, &extents);
  c->push_clip (extents);
}

static void
paint_extents_push_clip_rectangle (paint_funcs_t *funcs HB_UNUSED,
				      void *paint_data,
				      float xmin, float ymin, float xmax, float ymax,
				      void *user_data)
{
  paint_extents_context_t *c = (paint_extents_context_t *) paint_data;

  extents_t extents = {xmin, ymin, xmax, ymax};
  c->push_clip (extents);
}

static void
paint_extents_pop_clip (paint_funcs_t *funcs HB_UNUSED,
			   void *paint_data,
			   void *user_data HB_UNUSED)
{
  paint_extents_context_t *c = (paint_extents_context_t *) paint_data;

  c->pop_clip ();
}

static void
paint_extents_push_group (paint_funcs_t *funcs HB_UNUSED,
			     void *paint_data,
			     void *user_data HB_UNUSED)
{
  paint_extents_context_t *c = (paint_extents_context_t *) paint_data;

  c->push_group ();
}

static void
paint_extents_pop_group (paint_funcs_t *funcs HB_UNUSED,
			    void *paint_data,
			    paint_composite_mode_t mode,
			    void *user_data HB_UNUSED)
{
  paint_extents_context_t *c = (paint_extents_context_t *) paint_data;

  c->pop_group (mode);
}

static bool_t
paint_extents_paint_image (paint_funcs_t *funcs HB_UNUSED,
			      void *paint_data,
			      blob_t *blob HB_UNUSED,
			      unsigned int width HB_UNUSED,
			      unsigned int height HB_UNUSED,
			      tag_t format HB_UNUSED,
			      float slant HB_UNUSED,
			      glyph_extents_t *glyph_extents,
			      void *user_data HB_UNUSED)
{
  paint_extents_context_t *c = (paint_extents_context_t *) paint_data;

  extents_t extents = {(float) glyph_extents->x_bearing,
			  (float) glyph_extents->y_bearing + glyph_extents->height,
			  (float) glyph_extents->x_bearing + glyph_extents->width,
			  (float) glyph_extents->y_bearing};
  c->push_clip (extents);
  c->paint ();
  c->pop_clip ();

  return true;
}

static void
paint_extents_paint_color (paint_funcs_t *funcs HB_UNUSED,
			      void *paint_data,
			      bool_t use_foreground HB_UNUSED,
			      color_t color HB_UNUSED,
			      void *user_data HB_UNUSED)
{
  paint_extents_context_t *c = (paint_extents_context_t *) paint_data;

  c->paint ();
}

static void
paint_extents_paint_linear_gradient (paint_funcs_t *funcs HB_UNUSED,
				        void *paint_data,
				        color_line_t *color_line HB_UNUSED,
				        float x0 HB_UNUSED, float y0 HB_UNUSED,
				        float x1 HB_UNUSED, float y1 HB_UNUSED,
				        float x2 HB_UNUSED, float y2 HB_UNUSED,
				        void *user_data HB_UNUSED)
{
  paint_extents_context_t *c = (paint_extents_context_t *) paint_data;

  c->paint ();
}

static void
paint_extents_paint_radial_gradient (paint_funcs_t *funcs HB_UNUSED,
				        void *paint_data,
				        color_line_t *color_line HB_UNUSED,
				        float x0 HB_UNUSED, float y0 HB_UNUSED, float r0 HB_UNUSED,
				        float x1 HB_UNUSED, float y1 HB_UNUSED, float r1 HB_UNUSED,
				        void *user_data HB_UNUSED)
{
  paint_extents_context_t *c = (paint_extents_context_t *) paint_data;

  c->paint ();
}

static void
paint_extents_paint_sweep_gradient (paint_funcs_t *funcs HB_UNUSED,
				       void *paint_data,
				       color_line_t *color_line HB_UNUSED,
				       float cx HB_UNUSED, float cy HB_UNUSED,
				       float start_angle HB_UNUSED,
				       float end_angle HB_UNUSED,
				       void *user_data HB_UNUSED)
{
  paint_extents_context_t *c = (paint_extents_context_t *) paint_data;

  c->paint ();
}

static inline void free_static_paint_extents_funcs ();

static struct paint_extents_funcs_lazy_loader_t : paint_funcs_lazy_loader_t<paint_extents_funcs_lazy_loader_t>
{
  static paint_funcs_t *create ()
  {
    paint_funcs_t *funcs = paint_funcs_create ();

    paint_funcs_set_push_transform_func (funcs, paint_extents_push_transform, nullptr, nullptr);
    paint_funcs_set_pop_transform_func (funcs, paint_extents_pop_transform, nullptr, nullptr);
    paint_funcs_set_push_clip_glyph_func (funcs, paint_extents_push_clip_glyph, nullptr, nullptr);
    paint_funcs_set_push_clip_rectangle_func (funcs, paint_extents_push_clip_rectangle, nullptr, nullptr);
    paint_funcs_set_pop_clip_func (funcs, paint_extents_pop_clip, nullptr, nullptr);
    paint_funcs_set_push_group_func (funcs, paint_extents_push_group, nullptr, nullptr);
    paint_funcs_set_pop_group_func (funcs, paint_extents_pop_group, nullptr, nullptr);
    paint_funcs_set_color_func (funcs, paint_extents_paint_color, nullptr, nullptr);
    paint_funcs_set_image_func (funcs, paint_extents_paint_image, nullptr, nullptr);
    paint_funcs_set_linear_gradient_func (funcs, paint_extents_paint_linear_gradient, nullptr, nullptr);
    paint_funcs_set_radial_gradient_func (funcs, paint_extents_paint_radial_gradient, nullptr, nullptr);
    paint_funcs_set_sweep_gradient_func (funcs, paint_extents_paint_sweep_gradient, nullptr, nullptr);

    paint_funcs_make_immutable (funcs);

    atexit (free_static_paint_extents_funcs);

    return funcs;
  }
} static_paint_extents_funcs;

static inline
void free_static_paint_extents_funcs ()
{
  static_paint_extents_funcs.free_instance ();
}

paint_funcs_t *
paint_extents_get_funcs ()
{
  return static_paint_extents_funcs.get_unconst ();
}


#endif
