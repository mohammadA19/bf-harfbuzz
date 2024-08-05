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

/* Unit tests for hdmx subsetting */


static void
test_subset_hdmx_simple_subset (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");
  face_t *face_ac = test_open_font_file ("fonts/Roboto-Regular.ac.ttf");

  set_t *codepoints = set_create ();
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'c');
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_ac, face_abc_subset, HB_TAG ('h','d','m','x'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_ac);
}

static void
test_subset_hdmx_multiple_device_records (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Regular.multihdmx.abc.ttf");
  face_t *face_a = test_open_font_file ("fonts/Roboto-Regular.multihdmx.a.ttf");

  set_t *codepoints = set_create ();
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_a, face_abc_subset, HB_TAG ('h','d','m','x'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
  face_destroy (face_a);
}

static void
test_subset_hdmx_invalid (void)
{
  face_t *face = test_open_font_file ("../fuzzing/fonts/crash-ccc61c92d589f895174cdef6ff2e3b20e9999a1a");

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
test_subset_hdmx_noop (void)
{
  face_t *face_abc = test_open_font_file ("fonts/Roboto-Regular.abc.ttf");

  set_t *codepoints = set_create();
  face_t *face_abc_subset;
  set_add (codepoints, 'a');
  set_add (codepoints, 'b');
  set_add (codepoints, 'c');
  face_abc_subset = subset_test_create_subset (face_abc, subset_test_create_input (codepoints));
  set_destroy (codepoints);

  subset_test_check (face_abc, face_abc_subset, HB_TAG ('h','d','m','x'));

  face_destroy (face_abc_subset);
  face_destroy (face_abc);
}

int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_subset_hdmx_simple_subset);
  test_add (test_subset_hdmx_multiple_device_records);
  test_add (test_subset_hdmx_invalid);
  test_add (test_subset_hdmx_noop);

  return test_run();
}
