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

/* Unit tests for hb-subset-glyf.h */

static void
test_subset_32_tables (void)
{
  face_t *face = test_open_font_file ("../fuzzing/fonts/oom-6ef8c96d3710262511bcc730dce9c00e722cb653");

  subset_input_t *input = subset_input_create_or_fail ();
  set_t *codepoints = subset_input_unicode_set (input);
  face_t *subset;

  set_add (codepoints, 'a');
  set_add (codepoints, 'b');
  set_add (codepoints, 'c');

  subset = subset_or_fail (face, input);
  g_assert (subset);
  g_assert (subset != face_get_empty ());

  subset_input_destroy (input);
  face_destroy (subset);
  face_destroy (face);
}

static void
test_subset_no_inf_loop (void)
{
  face_t *face = test_open_font_file ("../fuzzing/fonts/clusterfuzz-testcase-minimized-hb-subset-fuzzer-5521982557782016");

  subset_input_t *input = subset_input_create_or_fail ();
  set_t *codepoints = subset_input_unicode_set (input);
  face_t *subset;

  set_add (codepoints, 'a');
  set_add (codepoints, 'b');
  set_add (codepoints, 'c');

  subset = subset_or_fail (face, input);
  g_assert (!subset);

  subset_input_destroy (input);
  face_destroy (subset);
  face_destroy (face);
}

static void
test_subset_crash (void)
{
  face_t *face = test_open_font_file ("../fuzzing/fonts/crash-4b60576767ee4d9fe1cc10959d89baf73d4e8249");

  subset_input_t *input = subset_input_create_or_fail ();
  set_t *codepoints = subset_input_unicode_set (input);
  face_t *subset;

  set_add (codepoints, 'a');
  set_add (codepoints, 'b');
  set_add (codepoints, 'c');

  subset = subset_or_fail (face, input);
  g_assert (!subset);

  subset_input_destroy (input);
  face_destroy (subset);
  face_destroy (face);
}

static void
test_subset_set_flags (void)
{
  subset_input_t *input = subset_input_create_or_fail ();

  g_assert (subset_input_get_flags (input) == HB_SUBSET_FLAGS_DEFAULT);

  subset_input_set_flags (input,
                             HB_SUBSET_FLAGS_NAME_LEGACY |
                             HB_SUBSET_FLAGS_NOTDEF_OUTLINE |
                             HB_SUBSET_FLAGS_GLYPH_NAMES);

  g_assert (subset_input_get_flags (input) ==
            (subset_flags_t) (
            HB_SUBSET_FLAGS_NAME_LEGACY |
            HB_SUBSET_FLAGS_NOTDEF_OUTLINE |
            HB_SUBSET_FLAGS_GLYPH_NAMES));

  subset_input_set_flags (input,
                             HB_SUBSET_FLAGS_NAME_LEGACY |
                             HB_SUBSET_FLAGS_NOTDEF_OUTLINE |
                             HB_SUBSET_FLAGS_NO_PRUNE_UNICODE_RANGES);

  g_assert (subset_input_get_flags (input) ==
            (subset_flags_t) (
            HB_SUBSET_FLAGS_NAME_LEGACY |
            HB_SUBSET_FLAGS_NOTDEF_OUTLINE |
            HB_SUBSET_FLAGS_NO_PRUNE_UNICODE_RANGES));


  subset_input_destroy (input);
}


static void
test_subset_sets (void)
{
  subset_input_t *input = subset_input_create_or_fail ();
  set_t* set = set_create ();

  set_add (subset_input_set (input, HB_SUBSET_SETS_GLYPH_INDEX), 83);
  set_add (subset_input_set (input, HB_SUBSET_SETS_UNICODE), 85);

  set_clear (subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG));
  set_add (subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG), 87);

  set_add (set, 83);
  g_assert (set_is_equal (subset_input_glyph_set (input), set));
  set_clear (set);

  set_add (set, 85);
  g_assert (set_is_equal (subset_input_unicode_set (input), set));
  set_clear (set);

  set_add (set, 87);
  g_assert (set_is_equal (subset_input_set (input, HB_SUBSET_SETS_LAYOUT_FEATURE_TAG), set));
  set_clear (set);

  set_destroy (set);
  subset_input_destroy (input);
}

static void
test_subset_plan (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  face_t *face_ac = test_open_font_file ("fonts/Roboto-Regular.ac.ttf");

  set_t *codepoints = set_create();
  set_add (codepoints, 97);
  set_add (codepoints, 99);
  subset_input_t* input = subset_test_create_input (codepoints);
  set_destroy (codepoints);

  subset_plan_t* plan = subset_plan_create_or_fail (face_abc, input);
  g_assert (plan);

  const map_t* mapping = subset_plan_old_to_new_glyph_mapping (plan);
  g_assert (map_get (mapping, 1) == 1);
  g_assert (map_get (mapping, 3) == 2);

  mapping = subset_plan_new_to_old_glyph_mapping (plan);
  g_assert (map_get (mapping, 1) == 1);
  g_assert (map_get (mapping, 2) == 3);

  mapping = subset_plan_unicode_to_old_glyph_mapping (plan);
  g_assert (map_get (mapping, 0x63) == 3);

  face_t* face_abc_subset = subset_plan_execute_or_fail (plan);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','l','y','f'));

  subset_input_destroy (input);
  subset_plan_destroy (plan);
  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static blob_t*
_ref_table (face_t *face, tag_t tag, void *user_data)
{
  return face_reference_table ((face_t*) user_data, tag);
}

static void
test_subset_create_for_tables_face (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  face_t *face_ac = test_open_font_file ("fonts/Roboto-Regular.ac.ttf");
  face_t *face_create_for_tables = face_create_for_tables (
      _ref_table,
      face_abc,
      NULL);

  set_t *codepoints = set_create();
  set_add (codepoints, 97);
  set_add (codepoints, 99);

  subset_input_t* input = subset_test_create_input (codepoints);
  set_destroy (codepoints);

  face_t* face_abc_subset = subset_or_fail (face_create_for_tables, input);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','l','y','f'));
  subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','a','s','p'));

  subset_input_destroy (input);
  face_destroy (face_abc_subset);
  face_destroy (face_create_for_tables);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_subset_32_tables);
  test_add (test_subset_no_inf_loop);
  test_add (test_subset_crash);
  test_add (test_subset_set_flags);
  test_add (test_subset_sets);
  test_add (test_subset_plan);
  test_add (test_subset_create_for_tables_face);

  return test_run();
}
