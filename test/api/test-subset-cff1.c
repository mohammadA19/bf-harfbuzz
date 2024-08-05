/*
 * Copyright © 2018 Adobe Inc.
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
 * Adobe Author(s): Michiharu Ariza
 */

#include "hb-test.h"
#include "hb-subset-test.h"

/* Unit tests for CFF subsetting */

static void
test_subset_cff1_noop (void)
{
  face_t *face_abc = test_open_font_file("fonts/SourceSansPro-Regular.abc.otf");

  set_t *codepoints = set_create ();
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'b');
  set_add (codepoints, 'c');
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_abc, face_abc_subset, HB_TAG ('C','F','F',' '));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
}

static void
test_subset_cff1 (void)
{
  face_t *face_abc = test_open_font_file ("fonts/SourceSansPro-Regular.abc.otf");
  face_t *face_ac = test_open_font_file ("fonts/SourceSansPro-Regular.ac.otf");

  set_t *codepoints = set_create ();
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('C','F','F',' '));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_cff1_strip_hints (void)
{
  face_t *face_abc = test_open_font_file ("fonts/SourceSansPro-Regular.abc.otf");
  face_t *face_ac = test_open_font_file ("fonts/SourceSansPro-Regular.ac.nohints.otf");

  set_t *codepoints = set_create ();
  subset_input_t *input;
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);
  face_abc_subset = subset_test_create_subset (face_abc, input);
  set_destroy (codepoints);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('C', 'F', 'F', ' '));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_cff1_desubr (void)
{
  face_t *face_abc = test_open_font_file ("fonts/SourceSansPro-Regular.abc.otf");
  face_t *face_ac = test_open_font_file ("fonts/SourceSansPro-Regular.ac.nosubrs.otf");

  set_t *codepoints = set_create ();
  subset_input_t *input;
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_DESUBROUTINIZE);
  face_abc_subset = subset_test_create_subset (face_abc, input);
  set_destroy (codepoints);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('C','F','F',' '));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_cff1_desubr_strip_hints (void)
{
  face_t *face_abc = test_open_font_file ("fonts/SourceSansPro-Regular.abc.otf");
  face_t *face_ac = test_open_font_file ("fonts/SourceSansPro-Regular.ac.nosubrs.nohints.otf");

  set_t *codepoints = set_create ();
  subset_input_t *input;
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  input = subset_test_create_input (codepoints);
  subset_input_set_flags (input,
                             HB_SUBSET_FLAGS_NO_HINTING | HB_SUBSET_FLAGS_DESUBROUTINIZE);
  face_abc_subset = subset_test_create_subset (face_abc, input);
  set_destroy (codepoints);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('C', 'F', 'F', ' '));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_cff1_j (void)
{
  face_t *face_41_3041_4c2e = test_open_font_file ("fonts/SourceHanSans-Regular.41,3041,4C2E.otf");
  face_t *face_41_4c2e = test_open_font_file ("fonts/SourceHanSans-Regular.41,4C2E.otf");

  set_t *codepoints = set_create ();
  face_t *face_41_3041_4c2e_subset;
  set_add (codepoints, 0x41);
  set_add (codepoints, 0x4C2E);
  face_41_3041_4c2e_subset = subset_test_create_subset (face_41_3041_4c2e, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_41_4c2e, face_41_3041_4c2e_subset, HB_TAG ('C','F','F',' '));

  face_destroy (face_41_3041_4c2e_subset);
  face_destroy (face_41_3041_4c2e);
  face_destroy (face_41_4c2e);
}

static void
test_subset_cff1_j_strip_hints (void)
{
  face_t *face_41_3041_4c2e = test_open_font_file ("fonts/SourceHanSans-Regular.41,3041,4C2E.otf");
  face_t *face_41_4c2e = test_open_font_file ("fonts/SourceHanSans-Regular.41,4C2E.nohints.otf");

  set_t *codepoints = set_create ();
  face_t *face_41_3041_4c2e_subset;
  subset_input_t *input;
  set_add (codepoints, 0x41);
  set_add (codepoints, 0x4C2E);
  input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);
  face_41_3041_4c2e_subset = subset_test_create_subset (face_41_3041_4c2e, input);
  set_destroy (codepoints);

  subset_test_check (face_41_4c2e, face_41_3041_4c2e_subset, HB_TAG ('C','F','F',' '));

  face_destroy (face_41_3041_4c2e_subset);
  face_destroy (face_41_3041_4c2e);
  face_destroy (face_41_4c2e);
}

static void
test_subset_cff1_j_desubr (void)
{
  face_t *face_41_3041_4c2e = test_open_font_file ("fonts/SourceHanSans-Regular.41,3041,4C2E.otf");
  face_t *face_41_4c2e = test_open_font_file ("fonts/SourceHanSans-Regular.41,4C2E.nosubrs.otf");

  set_t *codepoints = set_create ();
  face_t *face_41_3041_4c2e_subset;
  subset_input_t *input;
  set_add (codepoints, 0x41);
  set_add (codepoints, 0x4C2E);
  input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_DESUBROUTINIZE);
  face_41_3041_4c2e_subset = subset_test_create_subset (face_41_3041_4c2e, input);
  set_destroy (codepoints);

  subset_test_check (face_41_4c2e, face_41_3041_4c2e_subset, HB_TAG ('C','F','F',' '));

  face_destroy (face_41_3041_4c2e_subset);
  face_destroy (face_41_3041_4c2e);
  face_destroy (face_41_4c2e);
}

static void
test_subset_cff1_j_desubr_strip_hints (void)
{
  face_t *face_41_3041_4c2e = test_open_font_file ("fonts/SourceHanSans-Regular.41,3041,4C2E.otf");
  face_t *face_41_4c2e = test_open_font_file ("fonts/SourceHanSans-Regular.41,4C2E.nosubrs.nohints.otf");

  set_t *codepoints = set_create ();
  face_t *face_41_3041_4c2e_subset;
  subset_input_t *input;
  set_add (codepoints, 0x41);
  set_add (codepoints, 0x4C2E);
  input = subset_test_create_input (codepoints);
  subset_input_set_flags (input,
                             HB_SUBSET_FLAGS_NO_HINTING | HB_SUBSET_FLAGS_DESUBROUTINIZE);
  face_41_3041_4c2e_subset = subset_test_create_subset (face_41_3041_4c2e, input);
  set_destroy (codepoints);

  subset_test_check (face_41_4c2e, face_41_3041_4c2e_subset, HB_TAG ('C','F','F',' '));

  face_destroy (face_41_3041_4c2e_subset);
  face_destroy (face_41_3041_4c2e);
  face_destroy (face_41_4c2e);
}

static void
test_subset_cff1_expert (void)
{
  face_t *face = test_open_font_file ("fonts/cff1_expert.otf");
  face_t *face_subset = test_open_font_file ("fonts/cff1_expert.2D,F6E9,FB00.otf");

  set_t *codepoints = set_create ();
  face_t *face_test;
  set_add (codepoints, 0x2D);
  set_add (codepoints, 0xF6E9);
  set_add (codepoints, 0xFB00);
  face_test = subset_test_create_subset (face, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_subset, face_test, HB_TAG ('C','F','F',' '));

  face_destroy (face_test);
  face_destroy (face_subset);
  face_destroy (face);
}

static void
test_subset_cff1_seac (void)
{
  face_t *face = test_open_font_file ("fonts/cff1_seac.otf");
  face_t *face_subset = test_open_font_file ("fonts/cff1_seac.C0.otf");
  face_t *face_test;

  set_t *codepoints = set_create ();
  set_add (codepoints, 0xC0);  /* Agrave */
  face_test = subset_test_create_subset (face, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_subset, face_test, HB_TAG ('C','F','F',' '));

  face_destroy (face_test);
  face_destroy (face_subset);
  face_destroy (face);
}

static void
test_subset_cff1_dotsection (void)
{
  face_t *face = test_open_font_file ("fonts/cff1_dotsect.otf");
  face_t *face_subset = test_open_font_file ("fonts/cff1_dotsect.nohints.otf");

  set_t *codepoints = set_create ();
  subset_input_t *input;
  face_t *face_test;
  set_add (codepoints, 0x69);  /* i */
  input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);
  face_test = subset_test_create_subset (face, input);
  set_destroy (codepoints);

  subset_test_check (face_subset, face_test, HB_TAG ('C','F','F',' '));

  face_destroy (face_test);
  face_destroy (face_subset);
  face_destroy (face);
}

static void
test_subset_cff1_retaingids (void)
{
  face_t *face_abc = test_open_font_file ("fonts/SourceSansPro-Regular.abc.otf");
  face_t *face_ac = test_open_font_file ("fonts/SourceSansPro-Regular.ac.retaingids.otf");

  set_t *codepoints = set_create ();
  subset_input_t *input;
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  face_abc_subset = subset_test_create_subset (face_abc, input);
  set_destroy (codepoints);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('C','F','F',' '));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_cff1_j_retaingids (void)
{
  face_t *face_41_3041_4c2e = test_open_font_file ("fonts/SourceHanSans-Regular.41,3041,4C2E.otf");
  face_t *face_41_4c2e = test_open_font_file ("fonts/SourceHanSans-Regular.41,4C2E.retaingids.otf");

  set_t *codepoints = set_create ();
  subset_input_t *input;
  face_t *face_41_3041_4c2e_subset;
  set_add (codepoints, 0x41);
  set_add (codepoints, 0x4C2E);
  input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  face_41_3041_4c2e_subset = subset_test_create_subset (face_41_3041_4c2e, input);
  set_destroy (codepoints);

  subset_test_check (face_41_4c2e, face_41_3041_4c2e_subset, HB_TAG ('C','F','F',' '));

  face_destroy (face_41_3041_4c2e_subset);
  face_destroy (face_41_3041_4c2e);
  face_destroy (face_41_4c2e);
}

#ifdef HB_EXPERIMENTAL_API
static void
test_subset_cff1_iftb_requirements (void)
{
  face_t *face_abc = test_open_font_file ("fonts/SourceSansPro-Regular.abc.otf");
  face_t *face_long_off = test_open_font_file ("fonts/SourceSansPro-Regular.abc.long_off.otf");

  set_t *codepoints = set_create();
  face_t *face_abc_subset;
  set_add (codepoints, 97);
  set_add (codepoints, 98);
  set_add (codepoints, 99);

  subset_input_t *input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_IFTB_REQUIREMENTS);
  face_abc_subset = subset_test_create_subset (face_abc, input);
  set_destroy (codepoints);

  subset_test_check (face_long_off, face_abc_subset, HB_TAG ('C','F','F', ' '));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_long_off);

}
#endif


int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_subset_cff1_noop);
  test_add (test_subset_cff1);
  test_add (test_subset_cff1_strip_hints);
  test_add (test_subset_cff1_desubr);
  test_add (test_subset_cff1_desubr_strip_hints);
  test_add (test_subset_cff1_j);
  test_add (test_subset_cff1_j_strip_hints);
  test_add (test_subset_cff1_j_desubr);
  test_add (test_subset_cff1_j_desubr_strip_hints);
  test_add (test_subset_cff1_expert);
  test_add (test_subset_cff1_seac);
  test_add (test_subset_cff1_dotsection);
  test_add (test_subset_cff1_retaingids);
  test_add (test_subset_cff1_j_retaingids);

#ifdef HB_EXPERIMENTAL_API
  test_add (test_subset_cff1_iftb_requirements);
#endif

  return test_run ();
}
