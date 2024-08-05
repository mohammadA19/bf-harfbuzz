/*
 * Copyright © 2020  Google, Inc.
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
 * Google Author(s): Calder Kitagawa
 */

#include "hb-test.h"
#include "hb-subset-test.h"

/* Unit tests for CBDT/CBLC subsetting */

static void
test_subset_cbdt_noop (void)
{
  face_t *face = test_open_font_file ("fonts/NotoColorEmoji.subset.ttf");

  set_t *codepoints = set_create ();
  face_t *face_subset;
  set_add (codepoints, 0x38);
  set_add (codepoints, 0x39);
  set_add (codepoints, 0xAE);
  set_add (codepoints, 0x2049);
  set_add (codepoints, 0x20E3);
  face_subset = subset_test_create_subset (face, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face, face_subset, HB_TAG ('C','B','L','C'));
  subset_test_check (face, face_subset, HB_TAG ('C','B','D','T'));

  face_destroy (face_subset);
  face_destroy (face);
}

static void
test_subset_cbdt_keep_one (void)
{
  face_t *face = test_open_font_file ("fonts/NotoColorEmoji.subset.ttf");
  face_t *face_expected = test_open_font_file ("fonts/NotoColorEmoji.subset.default.39.ttf");

  set_t *codepoints = set_create ();
  face_t *face_subset;
  set_add (codepoints, 0x39);
  face_subset = subset_test_create_subset (face, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_expected, face_subset, HB_TAG ('C','B','L','C'));
  subset_test_check (face_expected, face_subset, HB_TAG ('C','B','D','T'));

  face_destroy (face_subset);
  face_destroy (face_expected);
  face_destroy (face);
}

static void
test_subset_cbdt_keep_one_last_subtable (void)
{
  face_t *face = test_open_font_file ("fonts/NotoColorEmoji.subset.ttf");
  face_t *face_expected = test_open_font_file ("fonts/NotoColorEmoji.subset.default.2049.ttf");

  set_t *codepoints = set_create ();
  face_t *face_subset;
  set_add (codepoints, 0x2049);
  face_subset = subset_test_create_subset (face, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_expected, face_subset, HB_TAG ('C','B','L','C'));
  subset_test_check (face_expected, face_subset, HB_TAG ('C','B','D','T'));

  face_destroy (face_subset);
  face_destroy (face_expected);
  face_destroy (face);
}

static void
test_subset_cbdt_keep_multiple_subtables (void)
{
  face_t *face = test_open_font_file ("fonts/NotoColorEmoji.subset.multiple_size_tables.ttf");
  face_t *face_expected = test_open_font_file ("fonts/NotoColorEmoji.subset.multiple_size_tables.default.38,AE,2049.ttf");

  set_t *codepoints = set_create ();
  face_t *face_subset;
  set_add (codepoints, 0x38);
  set_add (codepoints, 0xAE);
  set_add (codepoints, 0x2049);
  face_subset = subset_test_create_subset (face, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_expected, face_subset, HB_TAG ('C','B','L','C'));
  subset_test_check (face_expected, face_subset, HB_TAG ('C','B','D','T'));

  face_destroy (face_subset);
  face_destroy (face_expected);
  face_destroy (face);
}

static void
test_subset_cbdt_index_format_3 (void)
{
  face_t *face = test_open_font_file ("fonts/NotoColorEmoji.subset.index_format3.ttf");
  face_t *face_expected = test_open_font_file ("fonts/NotoColorEmoji.subset.index_format3.default.38,AE,2049.ttf");

  set_t *codepoints = set_create ();
  face_t *face_subset;
  set_add (codepoints, 0x38);
  set_add (codepoints, 0xAE);
  set_add (codepoints, 0x2049);
  face_subset = subset_test_create_subset (face, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_expected, face_subset, HB_TAG ('C','B','L','C'));
  subset_test_check (face_expected, face_subset, HB_TAG ('C','B','D','T'));

  face_destroy (face_subset);
  face_destroy (face_expected);
  face_destroy (face);
}

// TODO: add support/tests for index formats 2,4,5 (image formats are treated as
// opaque blobs when subsetting so don't need to be tested separately).
// TODO: add a test that keeps no codepoints.

int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_subset_cbdt_noop);
  test_add (test_subset_cbdt_keep_one);
  test_add (test_subset_cbdt_keep_one_last_subtable);
  // The following use manually crafted expectation files as they are not
  // binary compatible with FontTools.
  test_add (test_subset_cbdt_keep_multiple_subtables);
  // Can use FontTools after https://github.com/fonttools/fonttools/issues/1817
  // is resolved.
  test_add (test_subset_cbdt_index_format_3);

  return test_run();
}
