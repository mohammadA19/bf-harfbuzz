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

/* Unit tests for sbix subsetting */

static void
test_subset_sbix_noop (void)
{
  face_t *face_XY = test_open_font_file ("fonts/sbix.ttf");

  set_t *codepoints = set_create ();
  face_t *face_XY_subset;
  set_add (codepoints, 88);
  set_add (codepoints, 89);
  face_XY_subset = subset_test_create_subset (face_XY, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_XY, face_XY_subset, HB_TAG ('s','b','i','x'));

  face_destroy (face_XY_subset);
  face_destroy (face_XY);
}

static void
test_subset_sbix_keep_one (void)
{
  face_t *face_XY = test_open_font_file ("fonts/sbix.ttf");
  face_t *face_X = test_open_font_file ("fonts/sbix_X.ttf");

  set_t *codepoints = set_create ();
  face_t *face_X_subset;
  set_add (codepoints, 88);
  face_X_subset = subset_test_create_subset (face_XY, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_X, face_X_subset, HB_TAG ('s','b','i','x'));

  face_destroy (face_X_subset);
  face_destroy (face_XY);
  face_destroy (face_X);
}

// TODO: add a test that doesn't use contiguous codepoints.
// TODO: add a test that keeps no codepoints.

int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_subset_sbix_noop);
  test_add (test_subset_sbix_keep_one);

  return test_run();
}
