/*
 * Copyright © 2018  Ebrahim Byagowi
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
 */

#include "hb-test.h"

/* Unit tests for hb-map.h */


static void
test_map_basic (void)
{
  map_t *empty = map_get_empty ();
  map_t *m;
  g_assert (map_is_empty (empty));
  g_assert (!map_allocation_successful (empty));
  map_destroy (empty);

  m = map_create ();
  g_assert (map_allocation_successful (m));
  g_assert (map_is_empty (m));

  map_set (m, 213, 223);
  map_set (m, 643, 675);
  g_assert_cmpint (map_get_population (m), ==, 2);

  g_assert_cmpint (map_get (m, 213), ==, 223);
  g_assert (!map_has (m, 123));
  g_assert (map_has (m, 213));

  map_del (m, 213);
  g_assert (!map_has (m, 213));

  g_assert_cmpint (map_get (m, 643), ==, 675);
  map_set (m, 237, 673);
  g_assert (map_has (m, 237));
  map_clear (m);
  g_assert (!map_has (m, 237));
  g_assert (!map_has (m, 643));
  g_assert_cmpint (map_get_population (m), ==, 0);

  map_destroy (m);
}

static void
test_map_userdata (void)
{
  map_t *m = map_create ();

  user_data_key_t key[2];
  int *data = (int *) malloc (sizeof (int));
  int *data2;
  *data = 3123;
  map_set_user_data (m, &key[0], data, free, TRUE);
  g_assert_cmpint (*((int *) map_get_user_data (m, &key[0])), ==, 3123);

  data2 = (int *) malloc (sizeof (int));
  *data2 = 6343;
  map_set_user_data (m, &key[0], data2, free, FALSE);
  g_assert_cmpint (*((int *) map_get_user_data (m, &key[0])), ==, 3123);
  map_set_user_data (m, &key[0], data2, free, TRUE);
  g_assert_cmpint (*((int *) map_get_user_data (m, &key[0])), ==, 6343);

  map_destroy (m);
}

static void
test_map_refcount (void)
{
  map_t *m = map_create ();
  map_t *m2;
  map_set (m, 213, 223);
  g_assert_cmpint (map_get (m, 213), ==, 223);

  m2 = map_reference (m);
  map_destroy (m);

  /* We copied its reference so it is still usable after one destroy */
  g_assert (map_has (m, 213));
  g_assert (map_has (m2, 213));

  map_destroy (m2);

  /* Now you can't access them anymore */
}

static void
test_map_get_population (void)
{
  map_t *m = map_create ();

  map_set (m, 12, 21);
  g_assert_cmpint (map_get_population (m), ==, 1);
  map_set (m, 78, 87);
  g_assert_cmpint (map_get_population (m), ==, 2);

  map_set (m, 78, 87);
  g_assert_cmpint (map_get_population (m), ==, 2);
  map_set (m, 78, 13);
  g_assert_cmpint (map_get_population (m), ==, 2);

  map_set (m, 95, 56);
  g_assert_cmpint (map_get_population (m), ==, 3);

  map_del (m, 78);
  g_assert_cmpint (map_get_population (m), ==, 2);

  map_del (m, 103);
  g_assert_cmpint (map_get_population (m), ==, 2);

  map_destroy (m);
}

int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_map_basic);
  test_add (test_map_userdata);
  test_add (test_map_refcount);
  test_add (test_map_get_population);

  return test_run();
}
