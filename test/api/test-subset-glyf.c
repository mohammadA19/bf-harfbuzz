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

static void check_maxp_field (uint8_t *raw_maxp, unsigned int offset, uint16_t expected_value)
{
  uint16_t actual_value = (raw_maxp[offset] << 8) + raw_maxp[offset + 1];
  g_assert_cmpuint(expected_value, ==, actual_value);
}

static void check_maxp_num_glyphs (face_t *face, uint16_t expected_num_glyphs, bool hints)
{
  blob_t *maxp_blob = face_reference_table (face, HB_TAG ('m','a','x', 'p'));

  unsigned int maxp_len;
  uint8_t *raw_maxp = (uint8_t *) blob_get_data(maxp_blob, &maxp_len);

  check_maxp_field (raw_maxp, 4, expected_num_glyphs); // numGlyphs
  if (!hints)
  {
    check_maxp_field (raw_maxp, 14, 1); // maxZones
    check_maxp_field (raw_maxp, 16, 0); // maxTwilightPoints
    check_maxp_field (raw_maxp, 18, 0); // maxStorage
    check_maxp_field (raw_maxp, 20, 0); // maxFunctionDefs
    check_maxp_field (raw_maxp, 22, 0); // maxInstructionDefs
    check_maxp_field (raw_maxp, 24, 0); // maxStackElements
    check_maxp_field (raw_maxp, 26, 0); // maxSizeOfInstructions
  }

  blob_destroy (maxp_blob);
}

static void
test_subset_glyf (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  face_t *face_ac = test_open_font_file ("fonts/Roboto-Regular.ac.ttf");

  set_t *codepoints = set_create();
  face_t *face_abc_subset;
  set_add (codepoints, 97);
  set_add (codepoints, 99);
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  check_maxp_num_glyphs(face_abc_subset, 3, true);
  subset_test_check (face_ac, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','l','y','f'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_glyf_set_overlaps_flag (void)
{
  face_t *face_abcAE = test_open_font_file ("fonts/Roboto-Regular.abcAE.ttf");
  face_t *face_bAE = test_open_font_file ("fonts/Roboto-Regular.bAE.ttf");

  set_t *codepoints = set_create();
  face_t *face_abcAE_subset;
  set_add (codepoints, 32);
  set_add (codepoints, 98);
  set_add (codepoints, 508);

  subset_input_t* input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_SET_OVERLAPS_FLAG);
  face_abcAE_subset = subset_test_create_subset (face_abcAE, input);
  set_destroy (codepoints);

  subset_test_check (face_bAE, face_abcAE_subset, HB_TAG ('l','o','c', 'a'));
  subset_test_check (face_bAE, face_abcAE_subset, HB_TAG ('g','l','y','f'));

  face_destroy (face_abcAE_subset);
  face_destroy (face_abcAE);
  face_destroy (face_bAE);
}

static void
test_subset_glyf_with_input_glyphs (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  face_t *face_ac = test_open_font_file ("fonts/Roboto-Regular.ac.ttf");

  set_t *glyphs = set_create();
  face_t *face_abc_subset;
  set_add (glyphs, 1);
  set_add (glyphs, 3);
  face_abc_subset =
      subset_test_create_subset (face_abc, subset_test_create_input_from_glyphs (glyphs));
  set_destroy (glyphs);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','l','y','f'));
  subset_test_check (face_ac, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  check_maxp_num_glyphs(face_abc_subset, 3, true);

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_glyf_with_components (void)
{
  face_t *face_components = test_open_font_file ("fonts/Roboto-Regular.components.ttf");
  face_t *face_subset = test_open_font_file ("fonts/Roboto-Regular.components.subset.ttf");

  set_t *codepoints = set_create();
  face_t *face_generated_subset;
  set_add (codepoints, 0x1fc);
  face_generated_subset = subset_test_create_subset (face_components, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_subset, face_generated_subset, HB_TAG ('g','l','y','f'));
  subset_test_check (face_subset, face_generated_subset, HB_TAG ('l','o','c', 'a'));
  check_maxp_num_glyphs(face_generated_subset, 4, true);

  face_destroy (face_generated_subset);
  face_destroy (face_subset);
  face_destroy (face_components);
}

static void
test_subset_glyf_with_gsub (void)
{
  face_t *face_fil = test_open_font_file ("fonts/Roboto-Regular.gsub.fil.ttf");
  face_t *face_fi = test_open_font_file ("fonts/Roboto-Regular.gsub.fi.ttf");
  subset_input_t *input;
  face_t *face_subset;

  set_t *codepoints = set_create();
  set_add (codepoints, 102); // f
  set_add (codepoints, 105); // i

  input = subset_test_create_input (codepoints);
  set_destroy (codepoints);
  set_del (subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG), HB_TAG('G', 'S', 'U', 'B'));
  set_del (subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG), HB_TAG('G', 'P', 'O', 'S'));
  set_del (subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG), HB_TAG('G', 'D', 'E', 'F'));

  face_subset = subset_test_create_subset (face_fil, input);

  subset_test_check (face_fi, face_subset, HB_TAG ('g','l','y','f'));
  subset_test_check (face_fi, face_subset, HB_TAG ('l','o','c', 'a'));
  check_maxp_num_glyphs(face_subset, 5, true);

  face_destroy (face_subset);
  face_destroy (face_fi);
  face_destroy (face_fil);
}

static void
test_subset_glyf_without_gsub (void)
{
  face_t *face_fil = test_open_font_file ("fonts/Roboto-Regular.gsub.fil.ttf");
  face_t *face_fi = test_open_font_file ("fonts/Roboto-Regular.nogsub.fi.ttf");
  subset_input_t *input;
  face_t *face_subset;

  set_t *codepoints = set_create();
  set_add (codepoints, 102); // f
  set_add (codepoints, 105); // i

  input = subset_test_create_input (codepoints);
  set_destroy (codepoints);
  set_add (subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG), HB_TAG('G', 'S', 'U', 'B'));
  set_add (subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG), HB_TAG('G', 'P', 'O', 'S'));
  set_add (subset_input_set (input, HB_SUBSET_SETS_DROP_TABLE_TAG), HB_TAG('G', 'D', 'E', 'F'));

  face_subset = subset_test_create_subset (face_fil, input);

  subset_test_check (face_fi, face_subset, HB_TAG ('g','l','y','f'));
  subset_test_check (face_fi, face_subset, HB_TAG ('l','o','c', 'a'));
  check_maxp_num_glyphs(face_subset, 3, true);

  face_destroy (face_subset);
  face_destroy (face_fi);
  face_destroy (face_fil);
}

static void
test_subset_glyf_noop (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");

  set_t *codepoints = set_create();
  face_t *face_abc_subset;
  set_add (codepoints, 97);
  set_add (codepoints, 98);
  set_add (codepoints, 99);
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  check_maxp_num_glyphs(face_abc_subset, 4, true);
  subset_test_check (face_abc, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  subset_test_check (face_abc, face_abc_subset, HB_TAG ('g','l','y','f'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
}

static void
test_subset_glyf_strip_hints_simple (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  face_t *face_ac = test_open_font_file ("fonts/Roboto-Regular.ac.nohints.ttf");

  set_t *codepoints = set_create();
  subset_input_t *input;
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);
  face_abc_subset = subset_test_create_subset (face_abc, input);
  set_destroy (codepoints);

  check_maxp_num_glyphs(face_abc_subset, 3, false);
  subset_test_check (face_ac, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','l','y','f'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_glyf_strip_hints_composite (void)
{
  face_t *face_components = test_open_font_file ("fonts/Roboto-Regular.components.ttf");
  face_t *face_subset = test_open_font_file ("fonts/Roboto-Regular.components.1fc.nohints.ttf");

  set_t *codepoints = set_create();
  subset_input_t *input;
  face_t *face_generated_subset;
  set_add (codepoints, 0x1fc);
  input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);

  face_generated_subset = subset_test_create_subset (face_components, input);
  set_destroy (codepoints);

  check_maxp_num_glyphs(face_generated_subset, 4, false);
  subset_test_check (face_subset, face_generated_subset, HB_TAG ('l','o','c', 'a'));
  subset_test_check (face_subset, face_generated_subset, HB_TAG ('g','l','y','f'));

  face_destroy (face_generated_subset);
  face_destroy (face_subset);
  face_destroy (face_components);
}

static void
test_subset_glyf_strip_hints_invalid (void)
{
  face_t *face = test_open_font_file ("../fuzzing/fonts/oom-ccc61c92d589f895174cdef6ff2e3b20e9999a1a");

  set_t *codepoints = set_create();
  const codepoint_t text[] =
  {
    'A', 'B', 'C', 'D', 'E', 'X', 'Y', 'Z', '1', '2',
    '3', '@', '_', '%', '&', ')', '*', '$', '!'
  };
  unsigned int i;
  subset_input_t *input;
  face_t *face_subset;

  for (i = 0; i < sizeof (text) / sizeof (codepoint_t); i++)
  {
    set_add (codepoints, text[i]);
  }

  input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);
  set_destroy (codepoints);

  face_subset = subset_or_fail (face, input);
  g_assert (!face_subset);

  subset_input_destroy (input);
  face_destroy (face);
}

static void
test_subset_glyf_retain_gids (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  face_t *face_ac = test_open_font_file ("fonts/Roboto-Regular.ac.retaingids.ttf");

  set_t *codepoints = set_create();
  face_t *face_abc_subset;
  set_add (codepoints, 97);
  set_add (codepoints, 99);

  subset_input_t *input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  face_abc_subset = subset_test_create_subset (face_abc, input);
  set_destroy (codepoints);

  check_maxp_num_glyphs(face_abc_subset, 4, true);
  subset_test_check (face_ac, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  subset_test_check (face_ac, face_abc_subset, HB_TAG ('g','l','y','f'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_glyf_retain_gids_truncates (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  face_t *face_a = test_open_font_file ("fonts/Roboto-Regular.a.retaingids.ttf");

  set_t *codepoints = set_create();
  face_t *face_abc_subset;
  set_add (codepoints, 97);

  subset_input_t *input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  face_abc_subset = subset_test_create_subset (face_abc, input);
  set_destroy (codepoints);

  check_maxp_num_glyphs(face_abc_subset, 2, true);
  subset_test_check (face_a, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  subset_test_check (face_a, face_abc_subset, HB_TAG ('g','l','y','f'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_a);
}

#ifdef HB_EXPERIMENTAL_API
static void
test_subset_glyf_iftb_requirements (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Variable.abc.ttf");
  face_t *face_long_loca = test_open_font_file ("fonts/Roboto-Variable.abc.long_loca.ttf");

  set_t *codepoints = set_create();
  face_t *face_abc_subset;
  set_add (codepoints, 97);
  set_add (codepoints, 98);
  set_add (codepoints, 99);

  subset_input_t *input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_IFTB_REQUIREMENTS);
  face_abc_subset = subset_test_create_subset (face_abc, input);
  set_destroy (codepoints);

  subset_test_check (face_long_loca, face_abc_subset, HB_TAG ('l','o','c', 'a'));
  subset_test_check (face_long_loca, face_abc_subset, HB_TAG ('g','l','y','f'));
  subset_test_check (face_long_loca, face_abc_subset, HB_TAG ('g','v','a','r'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_long_loca);

}
#endif

// TODO(grieger): test for long loca generation.

int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_subset_glyf_noop);
  test_add (test_subset_glyf);
  test_add (test_subset_glyf_set_overlaps_flag);
  test_add (test_subset_glyf_with_input_glyphs);
  test_add (test_subset_glyf_strip_hints_simple);
  test_add (test_subset_glyf_strip_hints_composite);
  test_add (test_subset_glyf_strip_hints_invalid);
  test_add (test_subset_glyf_with_components);
  test_add (test_subset_glyf_with_gsub);
  test_add (test_subset_glyf_without_gsub);
  test_add (test_subset_glyf_retain_gids);
  test_add (test_subset_glyf_retain_gids_truncates);

#ifdef HB_EXPERIMENTAL_API
  test_add (test_subset_glyf_iftb_requirements);
#endif

  return test_run();
}
