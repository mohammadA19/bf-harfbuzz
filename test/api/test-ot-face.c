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

#ifndef TEST_OT_FACE_NO_MAIN
#include "hb-test.h"
#endif
#include <hb-aat.h>
#include <hb-ot.h>

/* Unit tests for hb-ot-*.h */

/* Return some dummy result so that compiler won't just optimize things */
static long long
test_font (font_t *font, codepoint_t cp)
{
  long long result = 0;

  face_t *face = font_get_face (font);
  set_t *set;
  codepoint_t g = 0;
  position_t x = 0, y = 0;
  char buf[5] = {0};
  unsigned int len = 0;
  glyph_extents_t extents = {0};
  ot_font_set_funcs (font);

  set = set_create ();
  face_collect_unicodes (face, set);
  face_collect_variation_selectors (face, set);
  face_collect_variation_unicodes (face, cp, set);

  font_get_nominal_glyph (font, cp, &g);
  font_get_variation_glyph (font, cp, cp, &g);
  font_get_glyph_h_advance (font, cp);
  font_get_glyph_v_advance (font, cp);
  font_get_glyph_h_origin (font, cp, &x, &y);
  font_get_glyph_v_origin (font, cp, &x, &y);
  font_get_glyph_extents (font, cp, &extents);
  font_get_glyph_contour_point (font, cp, 0, &x, &y);
  font_get_glyph_name (font, cp, buf, sizeof (buf));
  font_get_glyph_from_name (font, buf, strlen (buf), &g);

  ot_color_has_palettes (face);
  ot_color_palette_get_count (face);
  ot_color_palette_get_name_id (face, cp);
  ot_color_palette_color_get_name_id (face, cp);
  ot_color_palette_get_flags (face, cp);
  ot_color_palette_get_colors (face, cp, 0, NULL, NULL);
  ot_color_has_layers (face);
  ot_color_glyph_get_layers (face, cp, 0, NULL, NULL);
  ot_color_has_svg (face);
  blob_destroy (ot_color_glyph_reference_svg (face, cp));
  ot_color_has_png (face);
  blob_destroy (ot_color_glyph_reference_png (font, cp));

#ifndef HB_NO_AAT
  {
    aat_layout_feature_type_t feature = HB_AAT_LAYOUT_FEATURE_TYPE_ALL_TYPOGRAPHIC;
    unsigned count = 1;
    aat_layout_get_feature_types (face, 0, &count, &feature);
    aat_layout_feature_type_get_name_id (face, HB_AAT_LAYOUT_FEATURE_TYPE_CHARACTER_SHAPE);
    aat_layout_feature_selector_info_t setting = {0};
    unsigned default_index;
    count = 1;
    aat_layout_feature_type_get_selector_infos (face, HB_AAT_LAYOUT_FEATURE_TYPE_DESIGN_COMPLEXITY_TYPE, 0, &count, &setting, &default_index);
    result += count + feature + setting.disable + setting.disable + setting.name_id + setting.reserved + default_index;
  }
#endif

  set_t *lookup_indexes = set_create ();
  set_add (lookup_indexes, 0);

  map_t *lookup_mapping = map_create ();
  map_set (lookup_mapping, 0, 0);
  set_t *feature_indices = set_create ();
  set_destroy (lookup_indexes);
  set_destroy (feature_indices);
  map_destroy (lookup_mapping);

  ot_layout_get_baseline (font, HB_OT_LAYOUT_BASELINE_TAG_HANGING, HB_DIRECTION_RTL, HB_SCRIPT_HANGUL, HB_TAG_NONE, NULL);

  ot_layout_has_glyph_classes (face);
  ot_layout_has_substitution (face);
  ot_layout_has_positioning (face);

  ot_layout_get_ligature_carets (font, HB_DIRECTION_LTR, cp, 0, NULL, NULL);

  {
    unsigned temp = 0, temp2 = 0;
    ot_name_id_t name = HB_OT_NAME_ID_FULL_NAME;
    ot_layout_get_size_params (face, &temp, &temp, &name, &temp, &temp);
    tag_t cv01 = HB_TAG ('c','v','0','1');
    unsigned feature_index = 0;
    ot_layout_language_find_feature (face, HB_OT_TAG_GSUB, 0,
					HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX,
					cv01, &feature_index);
    ot_layout_feature_get_name_ids (face, HB_OT_TAG_GSUB, feature_index,
				       &name, &name, &name, &temp, &name);
    temp = 1;
    ot_layout_feature_get_characters (face, HB_OT_TAG_GSUB, feature_index, 0, &temp, &g);
    temp = 1;
    ot_layout_language_get_feature_indexes (face, HB_OT_TAG_GSUB, 0, 0, 0, &temp, &temp2);

    result += temp + name + feature_index + temp2;
  }

  ot_math_has_data (face);
  for (unsigned constant = HB_OT_MATH_CONSTANT_SCRIPT_PERCENT_SCALE_DOWN; constant <= HB_OT_MATH_CONSTANT_RADICAL_DEGREE_BOTTOM_RAISE_PERCENT; constant++) {
    ot_math_get_constant (font, (ot_math_constant_t)constant);
  }

  ot_math_get_glyph_italics_correction (font, cp);
  ot_math_get_glyph_top_accent_attachment (font, cp);
  ot_math_is_glyph_extended_shape (face, cp);

  {
    ot_math_get_glyph_kerning (font, cp, HB_OT_MATH_KERN_BOTTOM_RIGHT, 0);
    ot_math_get_glyph_kernings (font, cp, HB_OT_MATH_KERN_BOTTOM_RIGHT, 0, NULL, NULL);
    ot_math_kern_entry_t entries[5];
    unsigned count = sizeof (entries) / sizeof (entries[0]);
    ot_math_get_glyph_kernings (font, cp, HB_OT_MATH_KERN_BOTTOM_RIGHT, 0, &count, entries);
  }

  {
    ot_math_get_glyph_variants (font, cp, HB_DIRECTION_LTR, 0, NULL, NULL);
    ot_math_get_glyph_variants (font, cp, HB_DIRECTION_TTB, 0, NULL, NULL);
    ot_math_glyph_variant_t entries[5];
    unsigned count = sizeof (entries) / sizeof (entries[0]);
    ot_math_get_glyph_variants (font, cp, HB_DIRECTION_LTR, 0, &count, entries);
  }

  {
    ot_math_get_min_connector_overlap (font, HB_DIRECTION_LTR);
    ot_math_get_glyph_assembly (font, cp, HB_DIRECTION_LTR, 0, NULL, NULL, NULL);
    ot_math_get_glyph_assembly (font, cp, HB_DIRECTION_TTB, 0, NULL, NULL, NULL);
    ot_math_glyph_part_t entries[5];
    unsigned count = sizeof (entries) / sizeof (entries[0]);
    position_t corr;
    ot_math_get_glyph_assembly (font, cp, HB_DIRECTION_LTR, 0, &count, entries, &corr);
  }

  ot_meta_get_entry_tags (face, 0, NULL, NULL);
  blob_destroy (ot_meta_reference_entry (face, HB_OT_META_TAG_DESIGN_LANGUAGES));

  ot_metrics_get_position (font, HB_OT_METRICS_TAG_HORIZONTAL_ASCENDER, NULL);
  ot_metrics_get_variation (font, HB_OT_METRICS_TAG_UNDERLINE_OFFSET);
  ot_metrics_get_x_variation (font, HB_OT_METRICS_TAG_STRIKEOUT_OFFSET);
  ot_metrics_get_y_variation (font, HB_OT_METRICS_TAG_SUPERSCRIPT_EM_X_OFFSET);

  len = sizeof (buf);
  ot_name_list_names (face, NULL);
  ot_name_get_utf8 (face, cp, NULL, &len, buf);
  ot_name_get_utf16 (face, cp, NULL, NULL, NULL);
  ot_name_get_utf32 (face, cp, NULL, NULL, NULL);

#if 0
  style_get_value (font, HB_STYLE_TAG_ITALIC);
  style_get_value (font, HB_STYLE_TAG_OPTICAL_SIZE);
  style_get_value (font, HB_STYLE_TAG_SLANT);
  style_get_value (font, HB_STYLE_TAG_WIDTH);
  style_get_value (font, HB_STYLE_TAG_WEIGHT);
#endif

  ot_var_get_axis_count (face);
  ot_var_get_axis_infos (face, 0, NULL, NULL);
  ot_var_normalize_variations (face, NULL, 0, NULL, 0);
  ot_var_normalize_coords (face, 0, NULL, NULL);

  draw_funcs_t *funcs = draw_funcs_create ();
  font_draw_glyph (font, cp, funcs, NULL);
  draw_funcs_destroy (funcs);

  set_destroy (set);

  return result + g + x + y + buf[0] + buf[1] + buf[2] + buf[3] + buf[4] + len +
	 extents.height + extents.width + extents.x_bearing + extents.y_bearing;
}

#ifndef TEST_OT_FACE_NO_MAIN
static void
test_ot_face_empty (void)
{
  test_font (font_get_empty (), 0);
}

static void
test_ot_var_axis_on_zero_named_instance (void)
{
  face_t *face = test_open_font_file ("fonts/Zycon.ttf");
  g_assert (ot_var_get_axis_count (face));
  face_destroy (face);
}

int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_ot_face_empty);
  test_add (test_ot_var_axis_on_zero_named_instance);

  return test_run();
}
#endif
