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

/* Unit tests for hmtx subsetting */

static void check_num_hmetrics(face_t *face, uint16_t expected_num_hmetrics)
{
  blob_t *hhea_blob = face_reference_table (face, HB_TAG ('h','h','e','a'));
  blob_t *hmtx_blob = face_reference_table (face, HB_TAG ('h','m','t','x'));

  // TODO I sure wish I could just use the hmtx table struct!
  unsigned int hhea_len;
  uint8_t *raw_hhea = (uint8_t *) blob_get_data(hhea_blob, &hhea_len);
  uint16_t num_hmetrics = (raw_hhea[hhea_len - 2] << 8) + raw_hhea[hhea_len - 1];
  g_assert_cmpuint(expected_num_hmetrics, ==, num_hmetrics);

  blob_destroy (hhea_blob);
  blob_destroy (hmtx_blob);
}

static void
test_subset_hmtx_simple_subset (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  face_t *face_ac = test_open_font_file ("fonts/Roboto-Regular.ac.ttf");

  set_t *codepoints = set_create ();
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  check_num_hmetrics(face_abc_subset, 3); /* nothing has same width */
  subset_test_check (face_ac, face_abc_subset, HB_TAG ('h','m','t','x'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}


static void
test_subset_hmtx_monospace (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Inconsolata-Regular.abc.ttf");
  face_t *face_ac = test_open_font_file ("fonts/Inconsolata-Regular.ac.ttf");

  set_t *codepoints = set_create ();
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  check_num_hmetrics(face_abc_subset, 1); /* everything has same width */
  subset_test_check (face_ac, face_abc_subset, HB_TAG ('h','m','t','x'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}


static void
test_subset_hmtx_keep_num_metrics (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Inconsolata-Regular.abc.widerc.ttf");
  face_t *face_ac = test_open_font_file ("fonts/Inconsolata-Regular.ac.widerc.ttf");

  set_t *codepoints = set_create ();
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  check_num_hmetrics(face_abc_subset, 3); /* c is wider */
  subset_test_check (face_ac, face_abc_subset, HB_TAG ('h','m','t','x'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_hmtx_decrease_num_metrics (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Inconsolata-Regular.abc.widerc.ttf");
  face_t *face_ab = test_open_font_file ("fonts/Inconsolata-Regular.ab.ttf");

  set_t *codepoints = set_create ();
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'b');
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  check_num_hmetrics(face_abc_subset, 1); /* everything left has same width */
  subset_test_check (face_ab, face_abc_subset, HB_TAG ('h','m','t','x'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ab);
}

static void
test_subset_hmtx_noop (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");

  set_t *codepoints = set_create();
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'b');
  set_add (codepoints, 'c');
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  check_num_hmetrics(face_abc_subset, 4); /* nothing has same width */
  subset_test_check (face_abc, face_abc_subset, HB_TAG ('h','m','t','x'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
}

static void
test_subset_invalid_hmtx (void)
{
  face_t *face = test_open_font_file ("../fuzzing/fonts/crash-e4e0bb1458a91b692eba492c907ae1f94e635480");
  face_t *subset;

  subset_input_t *input = subset_input_create_or_fail ();
  set_t *codepoints = subset_input_unicode_set (input);
  set_add (codepoints, 'a');
  set_add (codepoints, 'b');
  set_add (codepoints, 'c');

  subset = subset_or_fail (face, input);
  g_assert (!subset);

  subset_input_destroy (input);
  face_destroy (subset);
  face_destroy (face);
}

int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_subset_hmtx_simple_subset);
  test_add (test_subset_hmtx_monospace);
  test_add (test_subset_hmtx_keep_num_metrics);
  test_add (test_subset_hmtx_decrease_num_metrics);
  test_add (test_subset_hmtx_noop);
  test_add (test_subset_invalid_hmtx);

  return test_run();
}
