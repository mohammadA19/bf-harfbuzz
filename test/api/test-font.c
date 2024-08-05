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

#include "hb-test.h"

/* Unit tests for hb-font.h */


static const char test_data[] = "test\0data";


static void
test_face_empty (void)
{
  face_t *created_from_empty;
  face_t *created_from_null;

  g_assert (face_get_empty ());

  created_from_empty = face_create (blob_get_empty (), 0);
  g_assert (face_get_empty () != created_from_empty);

  created_from_null = face_create (NULL, 0);
  g_assert (face_get_empty () != created_from_null);

  g_assert (face_reference_table (face_get_empty (), HB_TAG ('h','e','a','d')) == blob_get_empty ());

  g_assert_cmpint (face_get_upem (face_get_empty ()), ==, 1000);

  face_destroy (created_from_null);
  face_destroy (created_from_empty);
}

static void
test_face_create (void)
{
  face_t *face;
  blob_t *blob;

  blob = blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  face = face_create (blob, 0);
  blob_destroy (blob);

  g_assert (face_reference_table (face, HB_TAG ('h','e','a','d')) == blob_get_empty ());

  g_assert_cmpint (face_get_upem (face), ==, 1000);

  face_destroy (face);
}


static void
free_up (void *user_data)
{
  int *freed = (int *) user_data;

  g_assert (!*freed);

  (*freed)++;
}

static blob_t *
get_table (face_t *face HB_UNUSED, tag_t tag, void *user_data HB_UNUSED)
{
  if (tag == HB_TAG ('a','b','c','d'))
    return blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);

  return blob_get_empty ();
}

static void
test_face_createfortables (void)
{
  face_t *face;
  blob_t *blob;
  const char *data;
  unsigned int len;
  int freed = 0;

  face = face_create_for_tables (get_table, &freed, free_up);
  g_assert (!freed);

  g_assert (face_reference_table (face, HB_TAG ('h','e','a','d')) == blob_get_empty ());

  blob = face_reference_table (face, HB_TAG ('a','b','c','d'));
  g_assert (blob != blob_get_empty ());

  data = blob_get_data (blob, &len);
  g_assert_cmpint (len, ==, sizeof (test_data));
  g_assert (0 == memcmp (data, test_data, sizeof (test_data)));
  blob_destroy (blob);

  g_assert_cmpint (face_get_upem (face), ==, 1000);

  face_destroy (face);
  g_assert (freed);
}

static void
_test_font_nil_funcs (font_t *font)
{
  codepoint_t glyph;
  position_t x, y;
  glyph_extents_t extents;
  unsigned int upem = face_get_upem (font_get_face (font));

  x = y = 13;
  g_assert (!font_get_glyph_contour_point (font, 17, 2, &x, &y));
  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, 0);

  x = font_get_glyph_h_advance (font, 17);
  g_assert_cmpint (x, ==, upem);

  extents.x_bearing = extents.y_bearing = 13;
  extents.width = extents.height = 15;
  font_get_glyph_extents (font, 17, &extents);
  g_assert_cmpint (extents.x_bearing, ==, 0);
  g_assert_cmpint (extents.y_bearing, ==, 0);
  g_assert_cmpint (extents.width, ==, 0);
  g_assert_cmpint (extents.height, ==, 0);

  glyph = 3;
  g_assert (!font_get_glyph (font, 17, 2, &glyph));
  g_assert_cmpint (glyph, ==, 0);
}

static void
_test_fontfuncs_nil (font_funcs_t *ffuncs)
{
  blob_t *blob;
  face_t *face;
  font_t *font;
  font_t *subfont;
  int freed = 0;

  blob = blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  face = face_create (blob, 0);
  blob_destroy (blob);
  g_assert (!face_is_immutable (face));
  font = font_create (face);
  g_assert (font);
  g_assert (face_is_immutable (face));
  face_destroy (face);


  font_set_funcs (font, ffuncs, &freed, free_up);
  g_assert_cmpint (freed, ==, 0);

  _test_font_nil_funcs (font);

  subfont = font_create_sub_font (font);
  g_assert (subfont);

  g_assert_cmpint (freed, ==, 0);
  font_destroy (font);
  g_assert_cmpint (freed, ==, 0);

  _test_font_nil_funcs (subfont);

  font_destroy (subfont);
  g_assert_cmpint (freed, ==, 1);
}

static void
test_fontfuncs_empty (void)
{
  g_assert (font_funcs_get_empty ());
  g_assert (font_funcs_is_immutable (font_funcs_get_empty ()));
  _test_fontfuncs_nil (font_funcs_get_empty ());
}

static void
test_fontfuncs_nil (void)
{
  font_funcs_t *ffuncs;

  ffuncs = font_funcs_create ();

  g_assert (!font_funcs_is_immutable (ffuncs));
  _test_fontfuncs_nil (font_funcs_get_empty ());

  font_funcs_destroy (ffuncs);
}

static bool_t
contour_point_func1 (font_t *font HB_UNUSED, void *font_data HB_UNUSED,
		     codepoint_t glyph, unsigned int point_index HB_UNUSED,
		     position_t *x, position_t *y,
		     void *user_data HB_UNUSED)
{
  if (glyph == 1) {
    *x = 2;
    *y = 3;
    return TRUE;
  }
  if (glyph == 2) {
    *x = 4;
    *y = 5;
    return TRUE;
  }

  return FALSE;
}

static bool_t
contour_point_func2 (font_t *font, void *font_data HB_UNUSED,
		     codepoint_t glyph, unsigned int point_index,
		     position_t *x, position_t *y,
		     void *user_data HB_UNUSED)
{
  if (glyph == 1) {
    *x = 6;
    *y = 7;
    return TRUE;
  }

  return font_get_glyph_contour_point (font_get_parent (font),
					  glyph, point_index, x, y);
}

static position_t
glyph_h_advance_func1 (font_t *font HB_UNUSED, void *font_data HB_UNUSED,
		       codepoint_t glyph,
		       void *user_data HB_UNUSED)
{
  if (glyph == 1)
    return 8;

  return 0;
}

static void
test_fontfuncs_subclassing (void)
{
  blob_t *blob;
  face_t *face;

  font_funcs_t *ffuncs1;
  font_funcs_t *ffuncs2;

  font_t *font1;
  font_t *font2;
  font_t *font3;

  position_t x;
  position_t y;

  blob = blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  face = face_create (blob, 0);
  blob_destroy (blob);
  font1 = font_create (face);
  face_destroy (face);
  font_set_scale (font1, 10, 10);

  /* setup font1 */
  ffuncs1 = font_funcs_create ();
  font_funcs_set_glyph_contour_point_func (ffuncs1, contour_point_func1, NULL, NULL);
  font_funcs_set_glyph_h_advance_func (ffuncs1, glyph_h_advance_func1, NULL, NULL);
  font_set_funcs (font1, ffuncs1, NULL, NULL);
  font_funcs_destroy (ffuncs1);

  x = y = 1;
  g_assert (font_get_glyph_contour_point_for_origin (font1, 1, 2, HB_DIRECTION_LTR, &x, &y));
  g_assert_cmpint (x, ==, 2);
  g_assert_cmpint (y, ==, 3);
  g_assert (font_get_glyph_contour_point_for_origin (font1, 2, 5, HB_DIRECTION_LTR, &x, &y));
  g_assert_cmpint (x, ==, 4);
  g_assert_cmpint (y, ==, 5);
  g_assert (!font_get_glyph_contour_point_for_origin (font1, 3, 7, HB_DIRECTION_RTL, &x, &y));
  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, 0);
  x = font_get_glyph_h_advance (font1, 1);
  g_assert_cmpint (x, ==, 8);
  x = font_get_glyph_h_advance (font1, 2);
  g_assert_cmpint (x, ==, 0);

  /* creating sub-font doesn't make the parent font immutable;
   * making a font immutable however makes it's lineage immutable.
   */
  font2 = font_create_sub_font (font1);
  font3 = font_create_sub_font (font2);
  g_assert (!font_is_immutable (font1));
  g_assert (!font_is_immutable (font2));
  g_assert (!font_is_immutable (font3));
  font_make_immutable (font3);
  g_assert (font_is_immutable (font1));
  g_assert (font_is_immutable (font2));
  g_assert (font_is_immutable (font3));
  font_destroy (font2);
  font_destroy (font3);

  font2 = font_create_sub_font (font1);
  font_destroy (font1);

  /* setup font2 to override some funcs */
  ffuncs2 = font_funcs_create ();
  font_funcs_set_glyph_contour_point_func (ffuncs2, contour_point_func2, NULL, NULL);
  font_set_funcs (font2, ffuncs2, NULL, NULL);
  font_funcs_destroy (ffuncs2);

  x = y = 1;
  g_assert (font_get_glyph_contour_point_for_origin (font2, 1, 2, HB_DIRECTION_LTR, &x, &y));
  g_assert_cmpint (x, ==, 6);
  g_assert_cmpint (y, ==, 7);
  g_assert (font_get_glyph_contour_point_for_origin (font2, 2, 5, HB_DIRECTION_RTL, &x, &y));
  g_assert_cmpint (x, ==, 4);
  g_assert_cmpint (y, ==, 5);
  g_assert (!font_get_glyph_contour_point_for_origin (font2, 3, 7, HB_DIRECTION_LTR, &x, &y));
  g_assert_cmpint (x, ==, 0);
  g_assert_cmpint (y, ==, 0);
  x = font_get_glyph_h_advance (font2, 1);
  g_assert_cmpint (x, ==, 8);
  x = font_get_glyph_h_advance (font2, 2);
  g_assert_cmpint (x, ==, 0);

  /* setup font3 to override scale */
  font3 = font_create_sub_font (font2);
  font_set_scale (font3, 20, 30);

  x = y = 1;
  g_assert (font_get_glyph_contour_point_for_origin (font3, 1, 2, HB_DIRECTION_RTL, &x, &y));
  g_assert_cmpint (x, ==, 6*2);
  g_assert_cmpint (y, ==, 7*3);
  g_assert (font_get_glyph_contour_point_for_origin (font3, 2, 5, HB_DIRECTION_LTR, &x, &y));
  g_assert_cmpint (x, ==, 4*2);
  g_assert_cmpint (y, ==, 5*3);
  g_assert (!font_get_glyph_contour_point_for_origin (font3, 3, 7, HB_DIRECTION_LTR, &x, &y));
  g_assert_cmpint (x, ==, 0*2);
  g_assert_cmpint (y, ==, 0*3);
  x = font_get_glyph_h_advance (font3, 1);
  g_assert_cmpint (x, ==, 8*2);
  x = font_get_glyph_h_advance (font3, 2);
  g_assert_cmpint (x, ==, 0*2);


  font_destroy (font3);
  font_destroy (font2);
}

static bool_t
nominal_glyph_func (font_t *font HB_UNUSED,
		    void *font_data HB_UNUSED,
		    codepoint_t unicode HB_UNUSED,
		    codepoint_t *glyph,
		    void *user_data HB_UNUSED)
{
  *glyph = 0;
  return FALSE;
}

static unsigned int
nominal_glyphs_func (font_t *font HB_UNUSED,
		     void *font_data HB_UNUSED,
		     unsigned int count HB_UNUSED,
		     const codepoint_t *first_unicode HB_UNUSED,
		     unsigned int unicode_stride HB_UNUSED,
		     codepoint_t *first_glyph HB_UNUSED,
		     unsigned int glyph_stride HB_UNUSED,
		     void *user_data HB_UNUSED)
{
  return 0;
}

static void
test_fontfuncs_parallels (void)
{
  blob_t *blob;
  face_t *face;

  font_funcs_t *ffuncs1;
  font_funcs_t *ffuncs2;

  font_t *font0;
  font_t *font1;
  font_t *font2;
  codepoint_t glyph;

  blob = blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  face = face_create (blob, 0);
  blob_destroy (blob);
  font0 = font_create (face);
  face_destroy (face);

  /* setup sub-font1 */
  font1 = font_create_sub_font (font0);
  font_destroy (font0);
  ffuncs1 = font_funcs_create ();
  font_funcs_set_nominal_glyph_func (ffuncs1, nominal_glyph_func, NULL, NULL);
  font_set_funcs (font1, ffuncs1, NULL, NULL);
  font_funcs_destroy (ffuncs1);

  /* setup sub-font2 */
  font2 = font_create_sub_font (font1);
  font_destroy (font1);
  ffuncs2 = font_funcs_create ();
  font_funcs_set_nominal_glyphs_func (ffuncs1, nominal_glyphs_func, NULL, NULL);
  font_set_funcs (font2, ffuncs2, NULL, NULL);
  font_funcs_destroy (ffuncs2);

  /* Just test that calling get_nominal_glyph doesn't infinite-loop. */
  font_get_nominal_glyph (font2, 0x0020u, &glyph);

  font_destroy (font2);
}

static void
test_font_empty (void)
{
  font_t *created_from_empty;
  font_t *created_from_null;
  font_t *created_sub_from_null;

  g_assert (font_get_empty ());

  created_from_empty = font_create (face_get_empty ());
  g_assert (font_get_empty () != created_from_empty);

  created_from_null = font_create (NULL);
  g_assert (font_get_empty () != created_from_null);

  created_sub_from_null = font_create_sub_font (NULL);
  g_assert (font_get_empty () != created_sub_from_null);

  g_assert (font_is_immutable (font_get_empty ()));

  g_assert (font_get_face (font_get_empty ()) == face_get_empty ());
  g_assert (font_get_parent (font_get_empty ()) == NULL);

  font_destroy (created_sub_from_null);
  font_destroy (created_from_null);
  font_destroy (created_from_empty);
}

static void
test_font_properties (void)
{
  blob_t *blob;
  face_t *face;
  font_t *font;
  font_t *subfont;
  int x_scale, y_scale;
  unsigned int x_ppem, y_ppem;
  unsigned int upem;

  blob = blob_create (test_data, sizeof (test_data), HB_MEMORY_MODE_READONLY, NULL, NULL);
  face = face_create (blob, 0);
  blob_destroy (blob);
  font = font_create (face);
  face_destroy (face);


  g_assert (font_get_face (font) == face);
  g_assert (font_get_parent (font) == font_get_empty ());
  subfont = font_create_sub_font (font);
  g_assert (font_get_parent (subfont) == font);
  font_set_parent(subfont, NULL);
  g_assert (font_get_parent (subfont) == font_get_empty());
  font_set_parent(subfont, font);
  g_assert (font_get_parent (subfont) == font);
  font_set_parent(subfont, NULL);
  font_make_immutable (subfont);
  g_assert (font_get_parent (subfont) == font_get_empty());
  font_set_parent(subfont, font);
  g_assert (font_get_parent (subfont) == font_get_empty());
  font_destroy (subfont);


  /* Check scale */

  upem = face_get_upem (font_get_face (font));
  font_get_scale (font, NULL, NULL);
  x_scale = y_scale = 13;
  font_get_scale (font, &x_scale, NULL);
  g_assert_cmpint (x_scale, ==, upem);
  x_scale = y_scale = 13;
  font_get_scale (font, NULL, &y_scale);
  g_assert_cmpint (y_scale, ==, upem);
  x_scale = y_scale = 13;
  font_get_scale (font, &x_scale, &y_scale);
  g_assert_cmpint (x_scale, ==, upem);
  g_assert_cmpint (y_scale, ==, upem);

  font_set_scale (font, 17, 19);

  x_scale = y_scale = 13;
  font_get_scale (font, &x_scale, &y_scale);
  g_assert_cmpint (x_scale, ==, 17);
  g_assert_cmpint (y_scale, ==, 19);


  /* Check ppem */

  font_get_ppem (font, NULL, NULL);
  x_ppem = y_ppem = 13;
  font_get_ppem (font, &x_ppem, NULL);
  g_assert_cmpint (x_ppem, ==, 0);
  x_ppem = y_ppem = 13;
  font_get_ppem (font, NULL, &y_ppem);
  g_assert_cmpint (y_ppem, ==, 0);
  x_ppem = y_ppem = 13;
  font_get_ppem (font, &x_ppem, &y_ppem);
  g_assert_cmpint (x_ppem, ==, 0);
  g_assert_cmpint (y_ppem, ==, 0);

  font_set_ppem (font, 17, 19);

  x_ppem = y_ppem = 13;
  font_get_ppem (font, &x_ppem, &y_ppem);
  g_assert_cmpint (x_ppem, ==, 17);
  g_assert_cmpint (y_ppem, ==, 19);

  /* Check ptem */
  g_assert_cmpint (font_get_ptem (font), ==, 0);
  font_set_ptem (font, 42);
  g_assert_cmpint (font_get_ptem (font), ==, 42);


  /* Check immutable */

  g_assert (!font_is_immutable (font));
  font_make_immutable (font);
  g_assert (font_is_immutable (font));

  font_set_scale (font, 10, 12);
  x_scale = y_scale = 13;
  font_get_scale (font, &x_scale, &y_scale);
  g_assert_cmpint (x_scale, ==, 17);
  g_assert_cmpint (y_scale, ==, 19);

  font_set_ppem (font, 10, 12);
  x_ppem = y_ppem = 13;
  font_get_ppem (font, &x_ppem, &y_ppem);
  g_assert_cmpint (x_ppem, ==, 17);
  g_assert_cmpint (y_ppem, ==, 19);


  /* sub_font now */
  subfont = font_create_sub_font (font);
  font_destroy (font);

  g_assert (font_get_parent (subfont) == font);
  g_assert (font_get_face (subfont) == face);

  /* scale */
  x_scale = y_scale = 13;
  font_get_scale (subfont, &x_scale, &y_scale);
  g_assert_cmpint (x_scale, ==, 17);
  g_assert_cmpint (y_scale, ==, 19);
  font_set_scale (subfont, 10, 12);
  x_scale = y_scale = 13;
  font_get_scale (subfont, &x_scale, &y_scale);
  g_assert_cmpint (x_scale, ==, 10);
  g_assert_cmpint (y_scale, ==, 12);
  x_scale = y_scale = 13;
  font_get_scale (font, &x_scale, &y_scale);
  g_assert_cmpint (x_scale, ==, 17);
  g_assert_cmpint (y_scale, ==, 19);

  /* ppem */
  x_ppem = y_ppem = 13;
  font_get_ppem (subfont, &x_ppem, &y_ppem);
  g_assert_cmpint (x_ppem, ==, 17);
  g_assert_cmpint (y_ppem, ==, 19);
  font_set_ppem (subfont, 10, 12);
  x_ppem = y_ppem = 13;
  font_get_ppem (subfont, &x_ppem, &y_ppem);
  g_assert_cmpint (x_ppem, ==, 10);
  g_assert_cmpint (y_ppem, ==, 12);
  x_ppem = y_ppem = 13;
  font_get_ppem (font, &x_ppem, &y_ppem);
  g_assert_cmpint (x_ppem, ==, 17);
  g_assert_cmpint (y_ppem, ==, 19);

  font_destroy (subfont);
}

int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_face_empty);
  test_add (test_face_create);
  test_add (test_face_createfortables);

  test_add (test_fontfuncs_empty);
  test_add (test_fontfuncs_nil);
  test_add (test_fontfuncs_subclassing);
  test_add (test_fontfuncs_parallels);

  test_add (test_font_empty);
  test_add (test_font_properties);

  return test_run();
}
