/*
 * Copyright © 2019 Adobe Inc.
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

/* Unit tests for VVAR subsetting */

static void
test_subset_VVAR_noop (void)
{
  face_t *face_abc = test_open_font_file("fonts/SourceSerifVariable-Roman-VVAR.abc.ttf");

  set_t *codepoints = set_create ();
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'b');
  set_add (codepoints, 'c');
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_abc, face_abc_subset, HB_TAG ('V','V','A','R'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
}

static void
test_subset_VVAR (void)
{
  face_t *face_abc = test_open_font_file ("fonts/SourceSerifVariable-Roman-VVAR.abc.ttf");
  face_t *face_ac = test_open_font_file ("fonts/SourceSerifVariable-Roman-VVAR.ac.ttf");

  set_t *codepoints = set_create ();
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('V','V','A','R'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_VVAR_retaingids (void)
{
  face_t *face_abc = test_open_font_file ("fonts/SourceSerifVariable-Roman-VVAR.abc.ttf");
  face_t *face_ac = test_open_font_file ("fonts/SourceSerifVariable-Roman-VVAR.ac.retaingids.ttf");

  set_t *codepoints = set_create ();
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  subset_input_t *input = subset_test_create_input (codepoints);
  subset_input_set_flags (input, HB_SUBSET_FLAGS_RETAIN_GIDS);
  face_abc_subset = subset_test_create_subset (face_abc, input);
  set_destroy (codepoints);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('V','V','A','R'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_subset_VVAR_noop);
  test_add (test_subset_VVAR);
  test_add (test_subset_VVAR_retaingids);

  return test_run ();
}
