/*
 * Copyright © 2024  Behdad Esfahbod
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
#include <math.h>

#include <hb.h>

typedef struct draw_data_t
{
  unsigned move_to_count;
  unsigned line_to_count;
  unsigned quad_to_count;
  unsigned cubic_to_count;
  unsigned close_path_count;
} draw_data_t;

/* Our modified itoa, why not using libc's? it is going to be used
   in harfbuzzjs where libc isn't available */
static void _reverse (char *buf, unsigned int len)
{
  unsigned start = 0, end = len - 1;
  while (start < end)
  {
    char c = buf[end];
    buf[end] = buf[start];
    buf[start] = c;
    start++; end--;
  }
}
static unsigned _itoa (float fnum, char *buf)
{
  int32_t num = (int32_t) floorf (fnum + .5f);
  unsigned int i = 0;
  bool_t is_negative = num < 0;
  if (is_negative) num = -num;
  do
  {
    buf[i++] = '0' + num % 10;
    num /= 10;
  } while (num);
  if (is_negative) buf[i++] = '-';
  _reverse (buf, i);
  buf[i] = '\0';
  return i;
}

#define ITOA_BUF_SIZE 12 // 10 digits in int32, 1 for negative sign, 1 for \0

static void
test_itoa (void)
{
  char s[] = "12345";
  _reverse (s, 5);
  g_assert_cmpmem (s, 5, "54321", 5);

  {
    unsigned num = 12345;
    char buf[ITOA_BUF_SIZE];
    unsigned len = _itoa (num, buf);
    g_assert_cmpmem (buf, len, "12345", 5);
  }

  {
    unsigned num = 3152;
    char buf[ITOA_BUF_SIZE];
    unsigned len = _itoa (num, buf);
    g_assert_cmpmem (buf, len, "3152", 4);
  }

  {
    int num = -6457;
    char buf[ITOA_BUF_SIZE];
    unsigned len = _itoa (num, buf);
    g_assert_cmpmem (buf, len, "-6457", 5);
  }
}

static void
move_to (HB_UNUSED draw_funcs_t *dfuncs, draw_data_t *draw_data,
	 HB_UNUSED draw_state_t *st,
	 HB_UNUSED float to_x, HB_UNUSED float to_y,
	 HB_UNUSED void *user_data)
{
  draw_data->move_to_count++;
}

static void
line_to (HB_UNUSED draw_funcs_t *dfuncs, draw_data_t *draw_data,
	 HB_UNUSED draw_state_t *st,
	 HB_UNUSED float to_x, HB_UNUSED float to_y,
	 HB_UNUSED void *user_data)
{
  draw_data->line_to_count++;
}

static void
quadratic_to (HB_UNUSED draw_funcs_t *dfuncs, draw_data_t *draw_data,
	      HB_UNUSED draw_state_t *st,
	      HB_UNUSED float control_x, HB_UNUSED float control_y,
	      HB_UNUSED float to_x, HB_UNUSED float to_y,
	      HB_UNUSED void *user_data)
{
  draw_data->quad_to_count++;
}

static void
cubic_to (HB_UNUSED draw_funcs_t *dfuncs, draw_data_t *draw_data,
	  HB_UNUSED draw_state_t *st,
	  HB_UNUSED float control1_x, HB_UNUSED float control1_y,
	  HB_UNUSED float control2_x, HB_UNUSED float control2_y,
	  HB_UNUSED float to_x, HB_UNUSED float to_y,
	  HB_UNUSED void *user_data)
{
  draw_data->cubic_to_count++;
}

static void
close_path (HB_UNUSED draw_funcs_t *dfuncs, draw_data_t *draw_data,
	    HB_UNUSED draw_state_t *st,
	    HB_UNUSED void *user_data)
{
  draw_data->close_path_count++;
}

static draw_funcs_t *funcs;

#ifdef HB_EXPERIMENTAL_API
static void
test_draw_varc_simple_hangul (void)
{
  face_t *face = test_open_font_file ("fonts/varc-ac00-ac01.ttf");
  font_t *font = font_create (face);
  face_destroy (face);

  draw_data_t draw_data0 = {0};
  draw_data_t draw_data;;
  unsigned gid = 0;

  font_get_nominal_glyph (font, 0xAC00u, &gid);
  draw_data = draw_data0;
  font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 3);

  font_get_nominal_glyph (font, 0xAC01u, &gid);
  draw_data = draw_data0;
  font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 4);

  variation_t var;
  var.tag = HB_TAG ('w','g','h','t');
  var.value = 800;
  font_set_variations (font, &var, 1);

  font_get_nominal_glyph (font, 0xAC00u, &gid);
  draw_data = draw_data0;
  font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 3);

  font_get_nominal_glyph (font, 0xAC01u, &gid);
  draw_data = draw_data0;
  font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 4);

  font_destroy (font);
}

static void
test_draw_varc_simple_hanzi (void)
{
  face_t *face = test_open_font_file ("fonts/varc-6868.ttf");
  font_t *font = font_create (face);
  face_destroy (face);

  draw_data_t draw_data0 = {0};
  draw_data_t draw_data;;
  unsigned gid = 0;

  font_get_nominal_glyph (font, 0x6868u, &gid);
  draw_data = draw_data0;
  font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 11);

  variation_t var;
  var.tag = HB_TAG ('w','g','h','t');
  var.value = 800;
  font_set_variations (font, &var, 1);

  font_get_nominal_glyph (font, 0x6868u, &gid);
  draw_data = draw_data0;
  font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 11);

  font_destroy (font);
}

static void
test_draw_varc_conditional (void)
{
  face_t *face = test_open_font_file ("fonts/varc-ac01-conditional.ttf");
  font_t *font = font_create (face);
  face_destroy (face);

  draw_data_t draw_data0 = {0};
  draw_data_t draw_data;;
  unsigned gid = 0;

  font_get_nominal_glyph (font, 0xAC01u, &gid);
  draw_data = draw_data0;
  font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 2);

  variation_t var;
  var.tag = HB_TAG ('w','g','h','t');
  var.value = 800;
  font_set_variations (font, &var, 1);

  font_get_nominal_glyph (font, 0xAC01u, &gid);
  draw_data = draw_data0;
  font_draw_glyph (font, gid, funcs, &draw_data);
  g_assert_cmpuint (draw_data.move_to_count, ==, 4);

  font_destroy (font);
}
#endif

int
main (int argc, char **argv)
{
  funcs = draw_funcs_create ();
  draw_funcs_set_move_to_func (funcs, (draw_move_to_func_t) move_to, NULL, NULL);
  draw_funcs_set_line_to_func (funcs, (draw_line_to_func_t) line_to, NULL, NULL);
  draw_funcs_set_quadratic_to_func (funcs, (draw_quadratic_to_func_t) quadratic_to, NULL, NULL);
  draw_funcs_set_cubic_to_func (funcs, (draw_cubic_to_func_t) cubic_to, NULL, NULL);
  draw_funcs_set_close_path_func (funcs, (draw_close_path_func_t) close_path, NULL, NULL);
  draw_funcs_make_immutable (funcs);

  test_init (&argc, &argv);
  test_add (test_itoa);
#ifdef HB_EXPERIMENTAL_API
  test_add (test_draw_varc_simple_hangul);
  test_add (test_draw_varc_simple_hanzi);
  test_add (test_draw_varc_conditional);
#endif
  unsigned result = test_run ();

  draw_funcs_destroy (funcs);
  return result;
}
