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
 * Google Author(s): Roderick Sheeter
 */

#include "hb-test.h"
#include "hb-subset-test.h"

/* Unit tests for cmap subsetting */

static void
test_subset_cmap (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  face_t *face_ac = test_open_font_file ("fonts/Roboto-Regular.ac.ttf");

  set_t *codepoints = set_create ();
  face_t *face_abc_subset;
  set_add (codepoints, 97);
  set_add (codepoints, 99);
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('c','m','a','p'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_cmap_non_consecutive_glyphs (void)
{
  face_t *face = test_open_font_file ("fonts/Roboto-Regular.D7,D8,D9,DA,DE.ttf");

  set_t *codepoints = set_create ();
  face_t *face_subset;
  set_add (codepoints, 0xD7);
  set_add (codepoints, 0xD8);
  set_add (codepoints, 0xD9);
  set_add (codepoints, 0xDA);
  set_add (codepoints, 0xDE);

  face_subset = subset_test_create_subset (face, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face, face_subset, HB_TAG ('c','m','a','p'));

  face_destroy (face_subset);
  face_destroy (face);
}

static void
test_subset_cmap_noop (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");

  set_t *codepoints = set_create();
  face_t *face_abc_subset;
  set_add (codepoints, 97);
  set_add (codepoints, 98);
  set_add (codepoints, 99);
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_abc, face_abc_subset, HB_TAG ('c','m','a','p'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
}

static void
test_subset_cmap4_no_exceeding_maximum_codepoint (void)
{
  face_t *face_origin = test_open_font_file ("fonts/Mplus1p-Regular.ttf");
  face_t *face_expected = test_open_font_file ("fonts/Mplus1p-Regular-cmap4-testing.ttf");

  set_t *codepoints = set_create ();
  face_t *face_subset;
  set_add (codepoints, 0x20);
  set_add (codepoints, 0x21);
  set_add (codepoints, 0x1d542);
  set_add (codepoints, 0x201a2);

  face_subset = subset_test_create_subset (face_origin, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_expected, face_subset, HB_TAG ('c','m','a','p'));

  face_destroy (face_subset);
  face_destroy (face_expected);
  face_destroy (face_origin);
}

static void
test_subset_cmap_empty_tables (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  face_t *face_empty = test_open_font_file ("fonts/Roboto-Regular.empty.ttf");

  set_t *codepoints = set_create ();
  face_t *face_abc_subset;
  set_add (codepoints, 100);
  set_add (codepoints, 101);
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_empty, face_abc_subset, HB_TAG ('c','m','a','p'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_empty);
}

static void
test_subset_cmap_noto_color_emoji_noop (void)
{
  face_t *face = test_open_font_file ("fonts/NotoColorEmoji.cmap.ttf");

  set_t *codepoints = set_create ();
  face_t *face_subset;
  set_add (codepoints, 0x38);
  set_add (codepoints, 0x39);
  set_add (codepoints, 0xAE);
  set_add (codepoints, 0x2049);
  set_add (codepoints, 0x20E3);
  face_subset = subset_test_create_subset (face, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face, face_subset, HB_TAG ('c','m','a','p'));

  face_destroy (face_subset);
  face_destroy (face);
}

static void
test_subset_cmap_noto_color_emoji_non_consecutive_glyphs (void)
{
  face_t *face = test_open_font_file ("fonts/NotoColorEmoji.cmap.ttf");
  face_t *face_expected = test_open_font_file ("fonts/NotoColorEmoji.cmap.38,AE,2049.ttf");

  set_t *codepoints = set_create ();
  face_t *face_subset;
  set_add (codepoints, 0x38);
  set_add (codepoints, 0xAE);
  set_add (codepoints, 0x2049);
  face_subset = subset_test_create_subset (face, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_expected, face_subset, HB_TAG ('c','m','a','p'));

  face_destroy (face_subset);
  face_destroy (face_expected);
  face_destroy (face);
}

// TODO(rsheeter) test cmap to no codepoints

int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_subset_cmap);
  test_add (test_subset_cmap_noop);
  test_add (test_subset_cmap_non_consecutive_glyphs);
  test_add (test_subset_cmap4_no_exceeding_maximum_codepoint);
  test_add (test_subset_cmap_empty_tables);
  test_add (test_subset_cmap_noto_color_emoji_noop);
  test_add (test_subset_cmap_noto_color_emoji_non_consecutive_glyphs);

  return test_run();
}
