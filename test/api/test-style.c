/*
 * Copyright © 2019  Ebrahim Byagowi
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

#include "hb-test.h"

#include <hb.h>

/* Unit tests for hb-style.h */

#define assert_cmpfloat(n1, n2) g_assert_cmpint ((int) (n1 * 100.f), ==, (int) (n2 * 100.f))

static void
test_empty_face (void)
{
  font_t *empty = font_get_empty ();

  assert_cmpfloat (style_get_value (empty, HB_STYLE_TAG_ITALIC), 0);
  assert_cmpfloat (style_get_value (empty, HB_STYLE_TAG_OPTICAL_SIZE), 12);
  assert_cmpfloat (style_get_value (empty, HB_STYLE_TAG_SLANT_ANGLE), 0);
  assert_cmpfloat (style_get_value (empty, HB_STYLE_TAG_WIDTH), 100);
  assert_cmpfloat (style_get_value (empty, HB_STYLE_TAG_WEIGHT), 400);
}

static void
test_regular_face (void)
{
  face_t *face = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  font_t *font = font_create (face);

  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_ITALIC), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_OPTICAL_SIZE), 12);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_SLANT_ANGLE), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WIDTH), 100);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WEIGHT), 400);

  font_destroy (font);
  face_destroy (face);
}

static void
test_bold_face (void)
{
  face_t *face = test_open_font_file ("fonts/NotoSans-Bold.ttf");
  font_t *font = font_create (face);

  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_ITALIC), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_OPTICAL_SIZE), 12);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_SLANT_ANGLE), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WIDTH), 100);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WEIGHT), 700);

  font_destroy (font);
  face_destroy (face);
}


static void
test_face_user_setting (void)
{
  face_t *face = test_open_font_file ("fonts/AdobeVFPrototype_vsindex.otf");
  font_t *font = font_create (face);

  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_ITALIC), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_OPTICAL_SIZE), 12);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_SLANT_ANGLE), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WIDTH), 100);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WEIGHT), 389.34f); /* its default weight */
  assert_cmpfloat (style_get_value (font, (style_tag_t) HB_TAG ('C','N','T','R')), 0);

  font_set_var_named_instance (font, 0);

  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_ITALIC), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_OPTICAL_SIZE), 12);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_SLANT_ANGLE), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WIDTH), 100);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WEIGHT), 200);
  assert_cmpfloat (style_get_value (font, (style_tag_t) HB_TAG ('C','N','T','R')), 0);

  font_set_var_named_instance (font, 1);

  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_ITALIC), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_OPTICAL_SIZE), 12);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_SLANT_ANGLE), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WIDTH), 100);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WEIGHT), 300);
  assert_cmpfloat (style_get_value (font, (style_tag_t) HB_TAG ('C','N','T','R')), 0);

  font_set_var_named_instance (font, 2);

  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_ITALIC), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_OPTICAL_SIZE), 12);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_SLANT_ANGLE), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WIDTH), 100);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WEIGHT), 400);
  assert_cmpfloat (style_get_value (font, (style_tag_t) HB_TAG ('C','N','T','R')), 0);

  font_set_var_named_instance (font, 3);

  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_ITALIC), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_OPTICAL_SIZE), 12);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_SLANT_ANGLE), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WIDTH), 100);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WEIGHT),600);
  assert_cmpfloat (style_get_value (font, (style_tag_t) HB_TAG ('C','N','T','R')), 0);

  font_set_var_named_instance (font, 4);

  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_ITALIC), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_OPTICAL_SIZE), 12);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_SLANT_ANGLE), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WIDTH), 100);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WEIGHT), 700);
  assert_cmpfloat (style_get_value (font, (style_tag_t) HB_TAG ('C','N','T','R')), 0);

  font_set_var_named_instance (font, 5);

  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_ITALIC), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_OPTICAL_SIZE), 12);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_SLANT_ANGLE), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WIDTH), 100);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WEIGHT), 900);
  assert_cmpfloat (style_get_value (font, (style_tag_t) HB_TAG ('C','N','T','R')), 0);

  font_set_var_named_instance (font, 6);

  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_ITALIC), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_OPTICAL_SIZE), 12);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_SLANT_ANGLE), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WIDTH), 100);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WEIGHT), 900);
  assert_cmpfloat (style_get_value (font, (style_tag_t) HB_TAG ('C','N','T','R')), 50);

  font_set_var_named_instance (font, 7);

  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_ITALIC), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_OPTICAL_SIZE), 12);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_SLANT_ANGLE), 0);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WIDTH), 100);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_WEIGHT), 900);
  assert_cmpfloat (style_get_value (font, (style_tag_t) HB_TAG ('C','N','T','R')), 100);

  font_destroy (font);
  face_destroy (face);
}

static void
test_synthetic_slant (void)
{
  face_t *face = test_open_font_file ("fonts/AdobeVFPrototype_vsindex.otf");
  font_t *font = font_create (face);

  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_SLANT_RATIO), 0);

  font_set_synthetic_slant (font, 0.2);

  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_SLANT_RATIO), 0.2);

  font_destroy (font);
  face_destroy (face);

  face = test_open_font_file ("fonts/notosansitalic.ttf");
  font = font_create (face);

  /* We expect a negative angle for a typical italic font,
   * which should give us a positive ratio
   */
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_SLANT_ANGLE), -12);
  assert_cmpfloat (style_get_value (font, HB_STYLE_TAG_SLANT_RATIO), 0.21);

  font_destroy (font);
  face_destroy (face);
}

int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_empty_face);
  test_add (test_regular_face);
  test_add (test_bold_face);
  test_add (test_face_user_setting);
  test_add (test_synthetic_slant);

  return test_run ();
}
