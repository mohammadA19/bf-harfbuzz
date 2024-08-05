#include "benchmark/benchmark.h"
#include <cassert>
#include <cstring>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "hb.h"
#include "hb-ot.h"
#ifdef HAVE_FREETYPE
#include "hb-ft.h"
#endif


#define SUBSET_FONT_BASE_PATH "test/subset/data/fonts/"

struct test_input_t
{
  bool is_variable;
  const char *font_path;
} default_tests[] =
{
  {false, SUBSET_FONT_BASE_PATH "Roboto-Regular.ttf"},
  {true , SUBSET_FONT_BASE_PATH "RobotoFlex-Variable.ttf"},
  {false, SUBSET_FONT_BASE_PATH "SourceSansPro-Regular.otf"},
  {true , SUBSET_FONT_BASE_PATH "AdobeVFPrototype.otf"},
  {true , SUBSET_FONT_BASE_PATH "SourceSerifVariable-Roman.ttf"},
  {false, SUBSET_FONT_BASE_PATH "Comfortaa-Regular-new.ttf"},
  {false, SUBSET_FONT_BASE_PATH "NotoNastaliqUrdu-Regular.ttf"},
  {false, SUBSET_FONT_BASE_PATH "NotoSerifMyanmar-Regular.otf"},
};

static test_input_t *tests = default_tests;
static unsigned num_tests = sizeof (default_tests) / sizeof (default_tests[0]);

enum backend_t { HARFBUZZ, FREETYPE };

enum operation_t
{
  nominal_glyphs,
  glyph_h_advances,
  glyph_extents,
  draw_glyph,
  paint_glyph,
  load_face_and_shape,
};

static void
_move_to (draw_funcs_t *, void *draw_data, draw_state_t *, float x, float y, void *)
{
  float &i = * (float *) draw_data;
  i += x + y;
}

static void
_line_to (draw_funcs_t *, void *draw_data, draw_state_t *, float x, float y, void *)
{
  float &i = * (float *) draw_data;
  i += x + y;
}

static void
_quadratic_to (draw_funcs_t *, void *draw_data, draw_state_t *, float cx, float cy, float x, float y, void *)
{
  float &i = * (float *) draw_data;
  i += cx + cy + x + y;
}

static void
_cubic_to (draw_funcs_t *, void *draw_data, draw_state_t *, float cx1, float cy1, float cx2, float cy2, float x, float y, void *)
{
  float &i = * (float *) draw_data;
  i += cx1 + cy1 + cx2 + cy2 + x + y;
}

static void
_close_path (draw_funcs_t *, void *draw_data, draw_state_t *, void *)
{
  float &i = * (float *) draw_data;
  i += 1.0;
}

static draw_funcs_t *
_draw_funcs_create (void)
{
  draw_funcs_t *draw_funcs = draw_funcs_create ();
  draw_funcs_set_move_to_func (draw_funcs, _move_to, nullptr, nullptr);
  draw_funcs_set_line_to_func (draw_funcs, _line_to, nullptr, nullptr);
  draw_funcs_set_quadratic_to_func (draw_funcs, _quadratic_to, nullptr, nullptr);
  draw_funcs_set_cubic_to_func (draw_funcs, _cubic_to, nullptr, nullptr);
  draw_funcs_set_close_path_func (draw_funcs, _close_path, nullptr, nullptr);
  return draw_funcs;
}

static void BM_Font (benchmark::State &state,
		     bool is_var, backend_t backend, operation_t operation,
		     const test_input_t &test_input)
{
  font_t *font;
  unsigned num_glyphs;
  {
    blob_t *blob = blob_create_from_file_or_fail (test_input.font_path);
    assert (blob);
    face_t *face = face_create (blob, 0);
    blob_destroy (blob);
    num_glyphs = face_get_glyph_count (face);
    font = font_create (face);
    face_destroy (face);
  }

  if (is_var)
  {
    variation_t wght = {HB_TAG ('w','g','h','t'), 500};
    font_set_variations (font, &wght, 1);
  }

  switch (backend)
  {
    case HARFBUZZ:
      ot_font_set_funcs (font);
      break;

    case FREETYPE:
#ifdef HAVE_FREETYPE
      ft_font_set_funcs (font);
#endif
      break;
  }

  switch (operation)
  {
    case nominal_glyphs:
    {
      set_t *set = set_create ();
      face_collect_unicodes (font_get_face (font), set);
      unsigned pop = set_get_population (set);
      codepoint_t *unicodes = (codepoint_t *) calloc (pop, sizeof (codepoint_t));
      codepoint_t *glyphs = (codepoint_t *) calloc (pop, sizeof (codepoint_t));

      codepoint_t *p = unicodes;
      for (codepoint_t u = HB_SET_VALUE_INVALID;
	   set_next (set, &u);)
        *p++ = u;
      assert (p == unicodes + pop);

      for (auto _ : state)
	font_get_nominal_glyphs (font,
				    pop,
				    unicodes, sizeof (*unicodes),
				    glyphs, sizeof (*glyphs));

      free (glyphs);
      free (unicodes);
      set_destroy (set);
      break;
    }
    case glyph_h_advances:
    {
      codepoint_t *glyphs = (codepoint_t *) calloc (num_glyphs, sizeof (codepoint_t));
      position_t *advances = (position_t *) calloc (num_glyphs, sizeof (codepoint_t));

      for (unsigned g = 0; g < num_glyphs; g++)
        glyphs[g] = g;

      for (auto _ : state)
	font_get_glyph_h_advances (font,
				      num_glyphs,
				      glyphs, sizeof (*glyphs),
				      advances, sizeof (*advances));

      free (advances);
      free (glyphs);
      break;
    }
    case glyph_extents:
    {
      glyph_extents_t extents;
      for (auto _ : state)
	for (unsigned gid = 0; gid < num_glyphs; ++gid)
	  font_get_glyph_extents (font, gid, &extents);
      break;
    }
    case draw_glyph:
    {
      draw_funcs_t *draw_funcs = _draw_funcs_create ();
      for (auto _ : state)
      {
	float i = 0;
	for (unsigned gid = 0; gid < num_glyphs; ++gid)
	  font_draw_glyph (font, gid, draw_funcs, &i);
      }
      draw_funcs_destroy (draw_funcs);
      break;
    }
    case paint_glyph:
    {
      paint_funcs_t *paint_funcs = paint_funcs_create ();
      for (auto _ : state)
      {
	for (unsigned gid = 0; gid < num_glyphs; ++gid)
	  font_paint_glyph (font, gid, paint_funcs, nullptr, 0, 0);
      }
      paint_funcs_destroy (paint_funcs);
      break;
    }
    case load_face_and_shape:
    {
      for (auto _ : state)
      {
	blob_t *blob = blob_create_from_file_or_fail (test_input.font_path);
	assert (blob);
	face_t *face = face_create (blob, 0);
	blob_destroy (blob);
	font_t *font = font_create (face);
	face_destroy (face);

	switch (backend)
	{
	  case HARFBUZZ:
	    ot_font_set_funcs (font);
	    break;

	  case FREETYPE:
#ifdef HAVE_FREETYPE
	    ft_font_set_funcs (font);
#endif
	    break;
	}

	buffer_t *buffer = buffer_create ();
	buffer_add_utf8 (buffer, " ", -1, 0, -1);
	buffer_guess_segment_properties (buffer);

	shape (font, buffer, nullptr, 0);

	buffer_destroy (buffer);
	font_destroy (font);
      }
      break;
    }
  }


  font_destroy (font);
}

static void test_backend (backend_t backend,
			  const char *backend_name,
			  bool variable,
			  operation_t op,
			  const char *op_name,
			  benchmark::TimeUnit time_unit,
			  const test_input_t &test_input)
{
  char name[1024] = "BM_Font/";
  strcat (name, op_name);
  strcat (name, "/");
  const char *p = strrchr (test_input.font_path, '/');
  strcat (name, p ? p + 1 : test_input.font_path);
  strcat (name, variable ? "/var" : "");
  strcat (name, "/");
  strcat (name, backend_name);

  benchmark::RegisterBenchmark (name, BM_Font, variable, backend, op, test_input)
   ->Unit(time_unit);
}

static void test_operation (operation_t op,
			    const char *op_name,
			    benchmark::TimeUnit time_unit)
{
  for (unsigned i = 0; i < num_tests; i++)
  {
    auto& test_input = tests[i];
    for (int variable = 0; variable < int (test_input.is_variable) + 1; variable++)
    {
      bool is_var = (bool) variable;

      test_backend (HARFBUZZ, "hb", is_var, op, op_name, time_unit, test_input);
#ifdef HAVE_FREETYPE
      test_backend (FREETYPE, "ft", is_var, op, op_name, time_unit, test_input);
#endif
    }
  }
}

int main(int argc, char** argv)
{
  benchmark::Initialize(&argc, argv);

  if (argc > 1)
  {
    num_tests = argc - 1;
    tests = (test_input_t *) calloc (num_tests, sizeof (test_input_t));
    for (unsigned i = 0; i < num_tests; i++)
    {
      tests[i].is_variable = true;
      tests[i].font_path = argv[i + 1];
    }
  }

#define TEST_OPERATION(op, time_unit) test_operation (op, #op, time_unit)

  TEST_OPERATION (nominal_glyphs, benchmark::kMicrosecond);
  TEST_OPERATION (glyph_h_advances, benchmark::kMicrosecond);
  TEST_OPERATION (glyph_extents, benchmark::kMicrosecond);
  TEST_OPERATION (draw_glyph, benchmark::kMicrosecond);
  TEST_OPERATION (paint_glyph, benchmark::kMillisecond);
  TEST_OPERATION (load_face_and_shape, benchmark::kMicrosecond);

#undef TEST_OPERATION

  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();

  if (tests != default_tests)
    free (tests);
}
