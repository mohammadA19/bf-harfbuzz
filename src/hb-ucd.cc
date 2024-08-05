/*
 * Copyright (C) 2012 Grigori Goronzy <greg@kinoho.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "hb.hh"
#include "hb-unicode.hh"
#include "hb-machinery.hh"

#include "hb-ucd-table.hh"

static unicode_combining_class_t
ucd_combining_class (unicode_funcs_t *ufuncs HB_UNUSED,
			codepoint_t unicode,
			void *user_data HB_UNUSED)
{
  return (unicode_combining_class_t) _ucd_ccc (unicode);
}

static unicode_general_category_t
ucd_general_category (unicode_funcs_t *ufuncs HB_UNUSED,
			 codepoint_t unicode,
			 void *user_data HB_UNUSED)
{
  return (unicode_general_category_t) _ucd_gc (unicode);
}

static codepoint_t
ucd_mirroring (unicode_funcs_t *ufuncs HB_UNUSED,
		  codepoint_t unicode,
		  void *user_data HB_UNUSED)
{
  return unicode + _ucd_bmg (unicode);
}

static script_t
ucd_script (unicode_funcs_t *ufuncs HB_UNUSED,
	       codepoint_t unicode,
	       void *user_data HB_UNUSED)
{
  return _ucd_sc_map[_ucd_sc (unicode)];
}


#define SBASE 0xAC00u
#define LBASE 0x1100u
#define VBASE 0x1161u
#define TBASE 0x11A7u
#define SCOUNT 11172u
#define LCOUNT 19u
#define VCOUNT 21u
#define TCOUNT 28u
#define NCOUNT (VCOUNT * TCOUNT)

static inline bool
_ucd_decompose_hangul (codepoint_t ab, codepoint_t *a, codepoint_t *b)
{
  unsigned si = ab - SBASE;

  if (si >= SCOUNT)
    return false;

  if (si % TCOUNT)
  {
    /* LV,T */
    *a = SBASE + (si / TCOUNT) * TCOUNT;
    *b = TBASE + (si % TCOUNT);
    return true;
  } else {
    /* L,V */
    *a = LBASE + (si / NCOUNT);
    *b = VBASE + (si % NCOUNT) / TCOUNT;
    return true;
  }
}

static inline bool
_ucd_compose_hangul (codepoint_t a, codepoint_t b, codepoint_t *ab)
{
  if (a >= SBASE && a < (SBASE + SCOUNT) && b > TBASE && b < (TBASE + TCOUNT) &&
    !((a - SBASE) % TCOUNT))
  {
    /* LV,T */
    *ab = a + (b - TBASE);
    return true;
  }
  else if (a >= LBASE && a < (LBASE + LCOUNT) && b >= VBASE && b < (VBASE + VCOUNT))
  {
    /* L,V */
    int li = a - LBASE;
    int vi = b - VBASE;
    *ab = SBASE + li * NCOUNT + vi * TCOUNT;
    return true;
  }
  else
    return false;
}

static int
_cmp_pair (const void *_key, const void *_item)
{
  uint64_t& a = * (uint64_t*) _key;
  uint64_t b = (* (uint64_t*) _item) & HB_CODEPOINT_ENCODE3(0x1FFFFFu, 0x1FFFFFu, 0);

  return a < b ? -1 : a > b ? +1 : 0;
}
static int
_cmp_pair_11_7_14 (const void *_key, const void *_item)
{
  uint32_t& a = * (uint32_t*) _key;
  uint32_t b = (* (uint32_t*) _item) & HB_CODEPOINT_ENCODE3_11_7_14(0x1FFFFFu, 0x1FFFFFu, 0);

  return a < b ? -1 : a > b ? +1 : 0;
}

static bool_t
ucd_compose (unicode_funcs_t *ufuncs HB_UNUSED,
		codepoint_t a, codepoint_t b, codepoint_t *ab,
		void *user_data HB_UNUSED)
{
  // Hangul is handled algorithmically.
  if (_ucd_compose_hangul (a, b, ab)) return true;

  codepoint_t u = 0;

  if ((a & 0xFFFFF800u) == 0x0000u && (b & 0xFFFFFF80) == 0x0300u)
  {
    /* If "a" is small enough and "b" is in the U+0300 range,
     * the composition data is encoded in a 32bit array sorted
     * by "a,b" pair. */
    uint32_t k = HB_CODEPOINT_ENCODE3_11_7_14 (a, b, 0);
    const uint32_t *v = bsearch (k,
				    _ucd_dm2_u32_map,
				    ARRAY_LENGTH (_ucd_dm2_u32_map),
				    sizeof (*_ucd_dm2_u32_map),
				    _cmp_pair_11_7_14);
    if (likely (!v)) return false;
    u = HB_CODEPOINT_DECODE3_11_7_14_3 (*v);
  }
  else
  {
    /* Otherwise it is stored in a 64bit array sorted by
     * "a,b" pair. */
    uint64_t k = HB_CODEPOINT_ENCODE3 (a, b, 0);
    const uint64_t *v = bsearch (k,
				    _ucd_dm2_u64_map,
				    ARRAY_LENGTH (_ucd_dm2_u64_map),
				    sizeof (*_ucd_dm2_u64_map),
				    _cmp_pair);
    if (likely (!v)) return false;
    u = HB_CODEPOINT_DECODE3_3 (*v);
  }

  if (unlikely (!u)) return false;
  *ab = u;
  return true;
}

static bool_t
ucd_decompose (unicode_funcs_t *ufuncs HB_UNUSED,
		  codepoint_t ab, codepoint_t *a, codepoint_t *b,
		  void *user_data HB_UNUSED)
{
  if (_ucd_decompose_hangul (ab, a, b)) return true;

  unsigned i = _ucd_dm (ab);

  /* If no data, there's no decomposition. */
  if (likely (!i)) return false;
  i--;

  /* Check if it's a single-character decomposition. */
  if (i < ARRAY_LENGTH (_ucd_dm1_p0_map) + ARRAY_LENGTH (_ucd_dm1_p2_map))
  {
    /* Single-character decompositions currently are only in plane 0 or plane 2. */
    if (i < ARRAY_LENGTH (_ucd_dm1_p0_map))
    {
      /* Plane 0. */
      *a = _ucd_dm1_p0_map[i];
    }
    else
    {
      /* Plane 2. */
      i -= ARRAY_LENGTH (_ucd_dm1_p0_map);
      *a = 0x20000 | _ucd_dm1_p2_map[i];
    }
    *b = 0;
    return true;
  }
  i -= ARRAY_LENGTH (_ucd_dm1_p0_map) + ARRAY_LENGTH (_ucd_dm1_p2_map);

  /* Otherwise they are encoded either in a 32bit array or a 64bit array. */
  if (i < ARRAY_LENGTH (_ucd_dm2_u32_map))
  {
    /* 32bit array. */
    uint32_t v = _ucd_dm2_u32_map[i];
    *a = HB_CODEPOINT_DECODE3_11_7_14_1 (v);
    *b = HB_CODEPOINT_DECODE3_11_7_14_2 (v);
    return true;
  }
  i -= ARRAY_LENGTH (_ucd_dm2_u32_map);

  /* 64bit array. */
  uint64_t v = _ucd_dm2_u64_map[i];
  *a = HB_CODEPOINT_DECODE3_1 (v);
  *b = HB_CODEPOINT_DECODE3_2 (v);
  return true;
}


static void free_static_ucd_funcs ();

static struct ucd_unicode_funcs_lazy_loader_t : unicode_funcs_lazy_loader_t<ucd_unicode_funcs_lazy_loader_t>
{
  static unicode_funcs_t *create ()
  {
    unicode_funcs_t *funcs = unicode_funcs_create (nullptr);

    unicode_funcs_set_combining_class_func (funcs, ucd_combining_class, nullptr, nullptr);
    unicode_funcs_set_general_category_func (funcs, ucd_general_category, nullptr, nullptr);
    unicode_funcs_set_mirroring_func (funcs, ucd_mirroring, nullptr, nullptr);
    unicode_funcs_set_script_func (funcs, ucd_script, nullptr, nullptr);
    unicode_funcs_set_compose_func (funcs, ucd_compose, nullptr, nullptr);
    unicode_funcs_set_decompose_func (funcs, ucd_decompose, nullptr, nullptr);

    unicode_funcs_make_immutable (funcs);

    atexit (free_static_ucd_funcs);

    return funcs;
  }
} static_ucd_funcs;

static inline
void free_static_ucd_funcs ()
{
  static_ucd_funcs.free_instance ();
}

unicode_funcs_t *
ucd_get_unicode_funcs ()
{
#ifdef HB_NO_UCD
  return unicode_funcs_get_empty ();
#endif
  return static_ucd_funcs.get_unconst ();
}
