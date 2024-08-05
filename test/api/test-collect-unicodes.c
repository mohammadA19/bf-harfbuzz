/*
 * Copyright © 2018  Google, Inc.
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
 * Google Author(s): Garret Rieger
 */

#include "hb-test.h"
#include "hb-subset-test.h"

static void
test_collect_unicodes_format4 (void)
{
  face_t *face = test_open_font_file ("fonts/Roboto-Regular.abc.format4.ttf");
  set_t *codepoints = set_create();
  codepoint_t cp;

  face_collect_unicodes (face, codepoints);

  cp = HB_SET_VALUE_INVALID;
  g_assert (set_next (codepoints, &cp));
  g_assert_cmpuint (0x61, ==, cp);
  g_assert (set_next (codepoints, &cp));
  g_assert_cmpuint (0x62, ==, cp);
  g_assert (set_next (codepoints, &cp));
  g_assert_cmpuint (0x63, ==, cp);
  g_assert (!set_next (codepoints, &cp));

  set_destroy (codepoints);
  face_destroy (face);
}

static void
test_collect_unicodes_format12_notdef (void)
{
  face_t *face = test_open_font_file ("fonts/cmunrm.otf");
  set_t *codepoints = set_create();
  codepoint_t cp;

  face_collect_unicodes (face, codepoints);

  cp = HB_SET_VALUE_INVALID;
  g_assert (set_next (codepoints, &cp));
  g_assert_cmpuint (0x20, ==, cp);
  g_assert (set_next (codepoints, &cp));
  g_assert_cmpuint (0x21, ==, cp);
  g_assert (set_next (codepoints, &cp));
  g_assert_cmpuint (0x22, ==, cp);

  set_destroy (codepoints);
  face_destroy (face);
}

static void
test_collect_unicodes_format12 (void)
{
  face_t *face = test_open_font_file ("fonts/Roboto-Regular.abc.format12.ttf");
  set_t *codepoints = set_create();
  codepoint_t cp;

  face_collect_unicodes (face, codepoints);

  cp = HB_SET_VALUE_INVALID;
  g_assert (set_next (codepoints, &cp));
  g_assert_cmpuint (0x61, ==, cp);
  g_assert (set_next (codepoints, &cp));
  g_assert_cmpuint (0x62, ==, cp);
  g_assert (set_next (codepoints, &cp));
  g_assert_cmpuint (0x63, ==, cp);
  g_assert (!set_next (codepoints, &cp));

  set_destroy (codepoints);
  face_destroy (face);
}

static void
test_collect_unicodes (void)
{
  face_t *face = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  set_t *codepoints = set_create();
  set_t *codepoints2 = set_create();
  map_t *mapping = map_create();
  codepoint_t cp;

  face_collect_unicodes (face, codepoints);
  face_collect_nominal_glyph_mapping (face, mapping, codepoints2);

  g_assert (set_is_equal (codepoints, codepoints2));
  g_assert_cmpuint (set_get_population (codepoints), ==, 3);
  g_assert_cmpuint (map_get_population (mapping), ==, 3);

  cp = HB_SET_VALUE_INVALID;
  g_assert (set_next (codepoints, &cp));
  g_assert (map_has (mapping, cp));
  g_assert_cmpuint (0x61, ==, cp);
  g_assert (set_next (codepoints, &cp));
  g_assert (map_has (mapping, cp));
  g_assert_cmpuint (0x62, ==, cp);
  g_assert (set_next (codepoints, &cp));
  g_assert (map_has (mapping, cp));
  g_assert_cmpuint (0x63, ==, cp);
  g_assert (!set_next (codepoints, &cp));

  set_destroy (codepoints);
  set_destroy (codepoints2);
  map_destroy (mapping);
  face_destroy (face);
}

int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_collect_unicodes);
  test_add (test_collect_unicodes_format4);
  test_add (test_collect_unicodes_format12);
  test_add (test_collect_unicodes_format12_notdef);

  return test_run();
}
