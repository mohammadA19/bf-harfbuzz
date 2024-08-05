#include <assert.h>
#include <stdlib.h>

#include <hb-ot.h>

#include "hb-fuzzer.hh"

struct _draw_data_t
{
  unsigned path_len;
  float path_start_x;
  float path_start_y;
  float path_last_x;
  float path_last_y;
};

#include <cstdio>
static void
_move_to (draw_funcs_t *dfuncs, void *draw_data_,
	  draw_state_t *st,
	  float to_x, float to_y,
	  void *user_data)
{
  _draw_data_t *draw_data = (_draw_data_t *) draw_data_;
  assert (!st->path_open);
  draw_data->path_start_x = draw_data->path_last_x = to_x;
  draw_data->path_start_y = draw_data->path_last_y = to_y;
}

static void
_line_to (draw_funcs_t *dfuncs, void *draw_data_,
	  draw_state_t *st,
	  float to_x, float to_y,
	  void *user_data)
{
  _draw_data_t *draw_data = (_draw_data_t *) draw_data_;
  assert (st->path_open);
  ++draw_data->path_len;
  draw_data->path_last_x = to_x;
  draw_data->path_last_y = to_y;
}

static void
_quadratic_to (draw_funcs_t *dfuncs, void *draw_data_,
	       draw_state_t *st,
	       float control_x, float control_y,
	       float to_x, float to_y,
	       void *user_data)
{
  _draw_data_t *draw_data = (_draw_data_t *) draw_data_;
  assert (st->path_open);
  ++draw_data->path_len;
  draw_data->path_last_x = to_x;
  draw_data->path_last_y = to_y;
}

static void
_cubic_to (draw_funcs_t *dfuncs, void *draw_data_,
	   draw_state_t *st,
	   float control1_x, float control1_y,
	   float control2_x, float control2_y,
	   float to_x, float to_y,
	   void *user_data)
{
  _draw_data_t *draw_data = (_draw_data_t *) draw_data_;
  assert (st->path_open);
  ++draw_data->path_len;
  draw_data->path_last_x = to_x;
  draw_data->path_last_y = to_y;
}

static void
_close_path (draw_funcs_t *dfuncs, void *draw_data_,
	     draw_state_t *st,
	     void *user_data)
{
  _draw_data_t *draw_data = (_draw_data_t *) draw_data_;
  assert (st->path_open && draw_data->path_len != 0);
  draw_data->path_len = 0;
  assert (draw_data->path_start_x == draw_data->path_last_x &&
	  draw_data->path_start_y == draw_data->path_last_y);
}

/* Similar to test-ot-face.c's #test_font() */
static void misc_calls_for_gid (face_t *face, font_t *font, set_t *set, codepoint_t cp)
{
  /* Other gid specific misc calls */
  face_collect_variation_unicodes (face, cp, set);

  codepoint_t g;
  font_get_nominal_glyph (font, cp, &g);
  font_get_variation_glyph (font, cp, cp, &g);
  font_get_glyph_h_advance (font, cp);
  font_get_glyph_v_advance (font, cp);
  position_t x, y;
  font_get_glyph_h_origin (font, cp, &x, &y);
  font_get_glyph_v_origin (font, cp, &x, &y);
  font_get_glyph_contour_point (font, cp, 0, &x, &y);
  char buf[64];
  font_get_glyph_name (font, cp, buf, sizeof (buf));

  ot_color_palette_get_name_id (face, cp);
  ot_color_palette_color_get_name_id (face, cp);
  ot_color_palette_get_flags (face, cp);
  ot_color_palette_get_colors (face, cp, 0, nullptr, nullptr);
  ot_color_glyph_get_layers (face, cp, 0, nullptr, nullptr);
  blob_destroy (ot_color_glyph_reference_svg (face, cp));
  blob_destroy (ot_color_glyph_reference_png (font, cp));

  ot_layout_get_ligature_carets (font, HB_DIRECTION_LTR, cp, 0, nullptr, nullptr);

  ot_math_get_glyph_italics_correction (font, cp);
  ot_math_get_glyph_top_accent_attachment (font, cp);
  ot_math_is_glyph_extended_shape (face, cp);
  ot_math_get_glyph_kerning (font, cp, HB_OT_MATH_KERN_BOTTOM_RIGHT, 0);
  ot_math_get_glyph_variants (font, cp, HB_DIRECTION_TTB, 0, nullptr, nullptr);
  ot_math_get_glyph_assembly (font, cp, HB_DIRECTION_BTT, 0, nullptr, nullptr, nullptr);
}

extern "C" int LLVMFuzzerTestOneInput (const uint8_t *data, size_t size)
{
  alloc_state = _fuzzing_alloc_state (data, size);

  blob_t *blob = blob_create ((const char *) data, size,
				    HB_MEMORY_MODE_READONLY, nullptr, nullptr);
  face_t *face = face_create (blob, 0);
  font_t *font = font_create (face);

  unsigned num_coords = 0;
  if (size) num_coords = data[size - 1];
  num_coords = ot_var_get_axis_count (face) > num_coords ? num_coords : ot_var_get_axis_count (face);
  int *coords = (int *) calloc (num_coords, sizeof (int));
  if (size > num_coords + 1)
    for (unsigned i = 0; i < num_coords; ++i)
      coords[i] = ((int) data[size - num_coords + i - 1] - 128) * 10;
  font_set_var_coords_normalized (font, coords, num_coords);
  free (coords);

  unsigned glyph_count = face_get_glyph_count (face);
  glyph_count = glyph_count > 16 ? 16 : glyph_count;

  _draw_data_t draw_data = {0, 0, 0, 0, 0};

  draw_funcs_t *funcs = draw_funcs_create ();
  draw_funcs_set_move_to_func (funcs, (draw_move_to_func_t) _move_to, nullptr, nullptr);
  draw_funcs_set_line_to_func (funcs, (draw_line_to_func_t) _line_to, nullptr, nullptr);
  draw_funcs_set_quadratic_to_func (funcs, (draw_quadratic_to_func_t) _quadratic_to, nullptr, nullptr);
  draw_funcs_set_cubic_to_func (funcs, (draw_cubic_to_func_t) _cubic_to, nullptr, nullptr);
  draw_funcs_set_close_path_func (funcs, (draw_close_path_func_t) _close_path, nullptr, nullptr);
  volatile unsigned counter = !glyph_count;
  set_t *set = set_create ();
  for (unsigned gid = 0; gid < glyph_count; ++gid)
  {
    font_draw_glyph (font, gid, funcs, &draw_data);

    /* Glyph extents also may practices the similar path, call it now that is related */
    glyph_extents_t extents;
    if (font_get_glyph_extents (font, gid, &extents))
      counter += !!extents.width + !!extents.height + !!extents.x_bearing + !!extents.y_bearing;

    if (!counter) counter += 1;

    /* other misc calls */
    misc_calls_for_gid (face, font, set, gid);
  }
  set_destroy (set);
  assert (counter);
  draw_funcs_destroy (funcs);

  font_destroy (font);
  face_destroy (face);
  blob_destroy (blob);
  return 0;
}
