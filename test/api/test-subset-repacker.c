/*
 * Copyright © 2022  Google, Inc.
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
 */

#include "hb-test.h"
#include "hb-subset-test.h"

#ifdef HB_EXPERIMENTAL_API
#include "hb-subset-repacker.h"

char test_gsub_data[106] = "\x0\x1\x0\x0\x0\xa\x0\x1e\x0\x2c\x0\x1\x6c\x61\x74\x6e\x0\x8\x0\x4\x0\x0\x0\x0\xff\xff\x0\x1\x0\x0\x0\x1\x74\x65\x73\x74\x0\x8\x0\x0\x0\x1\x0\x1\x0\x2\x0\x2a\x0\x6\x0\x5\x0\x0\x0\x1\x0\x8\x0\x1\x0\x8\x0\x1\x0\xe\x0\x1\x0\x1\x0\x1\x0\x1\x0\x4\x0\x2\x0\x1\x0\x2\x0\x1\x0\x0\x0\x1\x0\x0\x0\x1\x0\x8\x0\x1\x0\x6\x0\x1\x0\x1\x0\x1\x0\x2";

static void
test_repack_with_cy_struct (void)
{
  object_t *objs = calloc (15, sizeof (object_t));

  objs[0].head = &(test_gsub_data[100]);
  objs[0].tail = &(test_gsub_data[105]) + 1;
  objs[0].num_real_links = 0;
  objs[0].num_virtual_links = 0;
  objs[0].real_links = NULL;
  objs[0].virtual_links = NULL;

  objs[1].head = &(test_gsub_data[94]);
  objs[1].tail = &(test_gsub_data[100]);
  objs[1].num_real_links = 1;
  objs[1].num_virtual_links = 0;
  objs[1].real_links = malloc (sizeof (link_t));
  objs[1].real_links[0].width = 2;
  objs[1].real_links[0].position = 2;
  objs[1].real_links[0].objidx = 1;
  objs[1].virtual_links = NULL;


  objs[2].head = &(test_gsub_data[86]);
  objs[2].tail = &(test_gsub_data[94]);
  objs[2].num_real_links = 1;
  objs[2].num_virtual_links = 0;
  objs[2].real_links = malloc (sizeof (link_t));
  objs[2].real_links[0].width = 2;
  objs[2].real_links[0].position = 6;
  objs[2].real_links[0].objidx = 2;
  objs[2].virtual_links = NULL;

  objs[3].head = &(test_gsub_data[76]);
  objs[3].tail = &(test_gsub_data[86]);
  objs[3].num_real_links = 0;
  objs[3].num_virtual_links = 0;
  objs[3].real_links = NULL;
  objs[3].virtual_links = NULL;

  objs[4].head = &(test_gsub_data[72]);
  objs[4].tail = &(test_gsub_data[76]);
  objs[4].num_real_links = 1;
  objs[4].num_virtual_links = 0;
  objs[4].real_links = malloc (sizeof (link_t));
  objs[4].real_links[0].width = 2;
  objs[4].real_links[0].position = 2;
  objs[4].real_links[0].objidx = 4;
  objs[4].virtual_links = NULL;

  objs[5].head = &(test_gsub_data[66]);
  objs[5].tail = &(test_gsub_data[72]);
  objs[5].num_real_links = 0;
  objs[5].num_virtual_links = 0;
  objs[5].real_links = NULL;
  objs[5].virtual_links = NULL;

  objs[6].head = &(test_gsub_data[58]);
  objs[6].tail = &(test_gsub_data[66]);
  objs[6].num_real_links = 2;
  objs[6].num_virtual_links = 0;
  objs[6].real_links = calloc (2, sizeof (link_t));
  objs[6].real_links[0].width = 2;
  objs[6].real_links[0].position = 6;
  objs[6].real_links[0].objidx = 5;
  objs[6].real_links[1].width = 2;
  objs[6].real_links[1].position = 2;
  objs[6].real_links[1].objidx = 6;
  objs[6].virtual_links = NULL;

  objs[7].head = &(test_gsub_data[50]);
  objs[7].tail = &(test_gsub_data[58]);
  objs[7].num_real_links = 1;
  objs[7].num_virtual_links = 0;
  objs[7].real_links = malloc (sizeof (link_t));
  objs[7].real_links[0].width = 2;
  objs[7].real_links[0].position = 6;
  objs[7].real_links[0].objidx = 7;
  objs[7].virtual_links = NULL;

  objs[8].head = &(test_gsub_data[44]);
  objs[8].tail = &(test_gsub_data[50]);
  objs[8].num_real_links = 2;
  objs[8].num_virtual_links = 0;
  objs[8].real_links = calloc (2, sizeof (link_t));
  objs[8].real_links[0].width = 2;
  objs[8].real_links[0].position = 2;
  objs[8].real_links[0].objidx = 3;
  objs[8].real_links[1].width = 2;
  objs[8].real_links[1].position = 4;
  objs[8].real_links[1].objidx = 8;
  objs[8].virtual_links = NULL;

  objs[9].head = &(test_gsub_data[38]);
  objs[9].tail = &(test_gsub_data[44]);
  objs[9].num_real_links = 0;
  objs[9].num_virtual_links = 0;
  objs[9].real_links = NULL;
  objs[9].virtual_links = NULL;

  objs[10].head = &(test_gsub_data[30]);
  objs[10].tail = &(test_gsub_data[38]);
  objs[10].num_real_links = 1;
  objs[10].num_virtual_links = 0;
  objs[10].real_links = malloc (sizeof (link_t));
  objs[10].real_links[0].width = 2;
  objs[10].real_links[0].position = 6;
  objs[10].real_links[0].objidx = 10;
  objs[10].virtual_links = NULL;

  objs[11].head = &(test_gsub_data[22]);
  objs[11].tail = &(test_gsub_data[30]);
  objs[11].num_real_links = 0;
  objs[11].num_virtual_links = 0;
  objs[11].real_links = NULL;
  objs[11].virtual_links = NULL;

  objs[12].head = &(test_gsub_data[18]);
  objs[12].tail = &(test_gsub_data[22]);
  objs[12].num_real_links = 1;
  objs[12].num_virtual_links = 0;
  objs[12].real_links = malloc (sizeof (link_t));
  objs[12].real_links[0].width = 2;
  objs[12].real_links[0].position = 0;
  objs[12].real_links[0].objidx = 12;
  objs[12].virtual_links = NULL;

  objs[13].head = &(test_gsub_data[10]);
  objs[13].tail = &(test_gsub_data[18]);
  objs[13].num_real_links = 1;
  objs[13].num_virtual_links = 0;
  objs[13].real_links = malloc (sizeof (link_t));
  objs[13].real_links[0].width = 2;
  objs[13].real_links[0].position = 6;
  objs[13].real_links[0].objidx = 13;
  objs[13].virtual_links = NULL;

  objs[14].head = &(test_gsub_data[0]);
  objs[14].tail = &(test_gsub_data[10]);
  objs[14].num_real_links = 3;
  objs[14].num_virtual_links = 0;
  objs[14].real_links = calloc (3, sizeof (link_t));
  objs[14].real_links[0].width = 2;
  objs[14].real_links[0].position = 8;
  objs[14].real_links[0].objidx = 9;
  objs[14].real_links[1].width = 2;
  objs[14].real_links[1].position = 6;
  objs[14].real_links[1].objidx = 11;
  objs[14].real_links[2].width = 2;
  objs[14].real_links[2].position = 4;
  objs[14].real_links[2].objidx = 14;
  objs[14].virtual_links = NULL;

  blob_t *result = subset_repack_or_fail (HB_TAG_NONE, objs, 15);

  face_t *face_expected = test_open_font_file ("fonts/repacker_expected.otf");
  blob_t *expected_blob = face_reference_table (face_expected, HB_TAG ('G','S','U','B'));
  fprintf(stderr, "expected %d bytes, actual %d bytes\n", blob_get_length(expected_blob), blob_get_length (result));

  if (blob_get_length (expected_blob) != 0 ||
      blob_get_length (result) != 0)
    test_assert_blobs_equal (expected_blob, result);

  face_destroy (face_expected);
  blob_destroy (expected_blob);
  blob_destroy (result);

  for (unsigned i = 0 ; i < 15; i++)
  {
    if (objs[i].real_links != NULL)
      free (objs[i].real_links);
  }

  free (objs);
}


int
main (int argc, char **argv)
{
  test_init (&argc, &argv);

  test_add (test_repack_with_cy_struct);

  return test_run();
}
#else
int main (int argc HB_UNUSED, char **argv HB_UNUSED)
{
  return 0;
}
#endif
