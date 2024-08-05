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

/* Unit tests for CFF2 subsetting */

static void
test_subset_cff2_noop (void)
{
  face_t *face_abc = test_open_font_file("fonts/AdobeVFPrototype.abc.otf");

  set_t *codepoints = set_create ();
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'b');
  set_add (codepoints, 'c');
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_abc, face_abc_subset, HB_TAG ('C','F','F','2'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
}

static void
test_subset_cff2 (void)
{
  face_t *face_abc = test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  face_t *face_ac = test_open_font_file ("fonts/AdobeVFPrototype.ac.otf");

  set_t *codepoints = set_create ();
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('C','F','F','2'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_cff2_strip_hints (void)
{
  face_t *face_abc = test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  face_t *face_ac = test_open_font_file ("fonts/AdobeVFPrototype.ac.nohints.otf");

  set_t *codepoints = set_create ();
  subset_input_t *input;
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_NO_HINTING);
  face_abc_subset = subset_test_create_subset (face_abc, input);
  set_destroy (codepoints);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('C', 'F', 'F', '2'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_cff2_desubr (void)
{
  face_t *face_abc = test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  face_t *face_ac = test_open_font_file ("fonts/AdobeVFPrototype.ac.nosubrs.otf");

  set_t *codepoints = set_create ();
  subset_input_t *input;
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_DESUBROUTINIZE);
  face_abc_subset = subset_test_create_subset (face_abc, input);
  set_destroy (codepoints);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('C', 'F', 'F', '2'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_cff2_desubr_strip_hints (void)
{
  face_t *face_abc = test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  face_t *face_ac = test_open_font_file ("fonts/AdobeVFPrototype.ac.nosubrs.nohints.otf");

  set_t *codepoints = set_create ();
  subset_input_t *input;
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  input = subset_test_create_input (codepoints);
  subset_input_set_flags (input,
                             HB_SUBSET_FLAGS_DESUBROUTINIZE | HB_SUBSET_FLAGS_NO_HINTING);
  face_abc_subset = subset_test_create_subset (face_abc, input);
  set_destroy (codepoints);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('C', 'F', 'F', '2'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_cff2_retaingids (void)
{
  face_t *face_abc = test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  face_t *face_ac = test_open_font_file ("fonts/AdobeVFPrototype.ac.retaingids.otf");

  set_t *codepoints = set_create ();
  subset_input_t *input;
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  face_abc_subset = subset_test_create_subset (face_abc, input);
  set_destroy (codepoints);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('C', 'F', 'F', '2'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

#ifdef HB_EXPERIMENTAL_API
static void
test_subset_cff2_iftb_requirements (void)
{
  face_t *face_abc = test_open_font_file ("fonts/AdobeVFPrototype.abc.otf");
  face_t *face_long_off = test_open_font_file ("fonts/AdobeVFPrototype.abc.long_off.otf");

  set_t *codepoints = set_create();
  face_t *face_abc_subset;
  set_add (codepoints, 97);
  set_add (codepoints, 98);
  set_add (codepoints, 99);

  subset_input_t *input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_IFTB_REQUIREMENTS);
  face_abc_subset = subset_test_create_subset (face_abc, input);
  set_destroy (codepoints);

  subset_test_check (face_long_off, face_abc_subset, HB_TAG ('C','F','F', '2'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_long_off);

}
#endif

int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_subset_cff2_noop);
  test_add (test_subset_cff2);
  test_add (test_subset_cff2_strip_hints);
  test_add (test_subset_cff2_desubr);
  test_add (test_subset_cff2_desubr_strip_hints);
  test_add (test_subset_cff2_retaingids);

#ifdef HB_EXPERIMENTAL_API
  test_add (test_subset_cff2_iftb_requirements);
#endif

  return test_run ();
}
