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
test_subset_nameids (void)
{
  face_t *face_origin = test_open_font_file ("fonts/nameID.origin.ttf");
  face_t *face_expected = test_open_font_file ("fonts/nameID.expected.ttf");

  set_t *name_ids = set_create();
  face_t *face_subset;
  set_add (name_ids, 0);
  set_add (name_ids, 9);
  face_subset = subset_test_create_subset (face_origin, subset_test_create_input_from_nameids (name_ids));
  set_destroy (name_ids);

  subset_test_check (face_expected, face_subset, HB_TAG ('n','a','m','e'));

  face_destroy (face_subset);
  face_destroy (face_origin);
  face_destroy (face_expected);
}

static void
test_subset_nameids_with_dup_strs (void)
{
  face_t *face_origin = test_open_font_file ("fonts/nameID.dup.origin.ttf");
  face_t *face_expected = test_open_font_file ("fonts/nameID.dup.expected.ttf");

  set_t *name_ids = set_create();
  face_t *face_subset;
  set_add (name_ids, 1);
  set_add (name_ids, 3);
  face_subset = subset_test_create_subset (face_origin, subset_test_create_input_from_nameids (name_ids));
  set_destroy (name_ids);

  subset_test_check (face_expected, face_subset, HB_TAG ('n','a','m','e'));

  face_destroy (face_subset);
  face_destroy (face_origin);
  face_destroy (face_expected);
}

#ifdef HB_EXPERIMENTAL_API
static void
test_subset_name_overrides (void)
{
  face_t *face_origin = test_open_font_file ("fonts/nameID.origin.ttf");
  face_t *face_expected = test_open_font_file ("fonts/nameID.override.expected.ttf");

  char str1[] = "Roboto Test";
  char str1_3[] = "Roboto Test unicode platform";
  char str2[] = "Bold";
  char str6[] = "Roboto-Bold";
  char str12[] = "Non ascii test Ü";
  char str16[] = "Roboto-test-inserting";
 
  set_t *name_ids = set_create();
  face_t *face_subset;
  set_add_range (name_ids, 0, 13);

  subset_input_t *subset_input = subset_test_create_input_from_nameids (name_ids);
  subset_input_override_name_table (subset_input, 1, 1, 0, 0, str1, -1);
  subset_input_override_name_table (subset_input, 1, 3, 1, 0x409, str1_3, -1);
  subset_input_override_name_table (subset_input, 2, 1, 0, 0, str2, 4);
  subset_input_override_name_table (subset_input, 6, 1, 0, 0, str6, -1);
  subset_input_override_name_table (subset_input, 12, 1, 0, 0, str12, -1);
  subset_input_override_name_table (subset_input, 14, 1, 0, 0, NULL, -1);
  subset_input_override_name_table (subset_input, 16, 1, 0, 0, str16, -1);

  face_subset = subset_test_create_subset (face_origin, subset_input);
  set_destroy (name_ids);

  subset_test_check (face_expected, face_subset, HB_TAG ('n','a','m','e'));

  face_destroy (face_subset);
  face_destroy (face_origin);
  face_destroy (face_expected);
}
#endif

int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_subset_nameids);
  test_add (test_subset_nameids_with_dup_strs);
#ifdef HB_EXPERIMENTAL_API
  test_add (test_subset_name_overrides);
#endif

  return test_run();
}
