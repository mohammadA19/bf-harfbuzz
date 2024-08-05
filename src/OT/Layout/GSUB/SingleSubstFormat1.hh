#ifndef OT_LAYOUT_GSUB_SINGLESUBSTFORMAT1_HH
#define OT_LAYOUT_GSUB_SINGLESUBSTFORMAT1_HH

#include "Common.hh"

namespace OT {
namespace Layout {
namespace GSUB_impl {

template <typename Types>
struct SingleSubstFormat1_3
{
  protected:
  HBUINT16      format;                 /* Format identifier--format = 1 */
  typename Types::template OffsetTo<Coverage>
                coverage;               /* Offset to Coverage table--from
                                         * beginning of Substitution table */
  typename Types::HBUINT
                deltaGlyphID;           /* Add to original GlyphID to get
                                         * substitute GlyphID, modulo 0x10000 */

  public:
  DEFINE_SIZE_STATIC (2 + 2 * Types::size);

  bool sanitize (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  coverage.sanitize (c, this) &&
                  /* The coverage  table may use a range to represent a set
                   * of glyphs, which means a small number of bytes can
                   * generate a large glyph set. Manually modify the
                   * sanitizer max ops to take this into account.
                   *
                   * Note: This check *must* be right after coverage sanitize. */
                  c->check_ops ((this + coverage).get_population () >> 1));
  }

  codepoint_t get_mask () const
  { return (1 << (8 * Types::size)) - 1; }

  bool intersects (const set_t *glyphs) const
  { return (this+coverage).intersects (glyphs); }

  bool may_have_non_1to1 () const
  { return false; }

  void closure (closure_context_t *c) const
  {
    codepoint_t d = deltaGlyphID;
    codepoint_t mask = get_mask ();

    /* Help fuzzer avoid this function as much. */
    unsigned pop = (this+coverage).get_population ();
    if (pop >= mask)
      return;

    set_t intersection;
    (this+coverage).intersect_set (c->parent_active_glyphs (), intersection);

    /* In degenerate fuzzer-found fonts, but not real fonts,
     * this table can keep adding new glyphs in each round of closure.
     * Refuse to close-over, if it maps glyph range to overlapping range. */
    codepoint_t min_before = intersection.get_min ();
    codepoint_t max_before = intersection.get_max ();
    codepoint_t min_after = (min_before + d) & mask;
    codepoint_t max_after = (max_before + d) & mask;
    if (intersection.get_population () == max_before - min_before + 1 &&
	((min_before <= min_after && min_after <= max_before) ||
	 (min_before <= max_after && max_after <= max_before)))
      return;

    + iter (intersection)
    | map ([d, mask] (codepoint_t g) { return (g + d) & mask; })
    | sink (c->output)
    ;
  }

  void closure_lookups (closure_lookups_context_t *c) const {}

  void collect_glyphs (collect_glyphs_context_t *c) const
  {
    if (unlikely (!(this+coverage).collect_coverage (c->input))) return;
    codepoint_t d = deltaGlyphID;
    codepoint_t mask = get_mask ();

    + iter (this+coverage)
    | map ([d, mask] (codepoint_t g) { return (g + d) & mask; })
    | sink (c->output)
    ;
  }

  const Coverage &get_coverage () const { return this+coverage; }

  bool would_apply (would_apply_context_t *c) const
  { return c->len == 1 && (this+coverage).get_coverage (c->glyphs[0]) != NOT_COVERED; }

  unsigned
  get_glyph_alternates (codepoint_t  glyph_id,
                        unsigned        start_offset,
                        unsigned       *alternate_count  /* IN/OUT.  May be NULL. */,
                        codepoint_t *alternate_glyphs /* OUT.     May be NULL. */) const
  {
    unsigned int index = (this+coverage).get_coverage (glyph_id);
    if (likely (index == NOT_COVERED))
    {
      if (alternate_count)
        *alternate_count = 0;
      return 0;
    }

    if (alternate_count && *alternate_count)
    {
      codepoint_t d = deltaGlyphID;
      codepoint_t mask = get_mask ();

      glyph_id = (glyph_id + d) & mask;

      *alternate_glyphs = glyph_id;
      *alternate_count = 1;
    }

    return 1;
  }

  bool apply (ot_apply_context_t *c) const
  {
    TRACE_APPLY (this);
    codepoint_t glyph_id = c->buffer->cur().codepoint;
    unsigned int index = (this+coverage).get_coverage (glyph_id);
    if (likely (index == NOT_COVERED)) return_trace (false);

    codepoint_t d = deltaGlyphID;
    codepoint_t mask = get_mask ();

    glyph_id = (glyph_id + d) & mask;

    if (HB_BUFFER_MESSAGE_MORE && c->buffer->messaging ())
    {
      c->buffer->sync_so_far ();
      c->buffer->message (c->font,
			  "replacing glyph at %u (single substitution)",
			  c->buffer->idx);
    }

    c->replace_glyph (glyph_id);

    if (HB_BUFFER_MESSAGE_MORE && c->buffer->messaging ())
    {
      c->buffer->message (c->font,
			  "replaced glyph at %u (single substitution)",
			  c->buffer->idx - 1u);
    }

    return_trace (true);
  }

  template<typename Iterator,
           requires (is_sorted_source_of (Iterator, codepoint_t))>
  bool serialize (serialize_context_t *c,
                  Iterator glyphs,
                  unsigned delta)
  {
    TRACE_SERIALIZE (this);
    if (unlikely (!c->extend_min (this))) return_trace (false);
    if (unlikely (!coverage.serialize_serialize (c, glyphs))) return_trace (false);
    c->check_assign (deltaGlyphID, delta, HB_SERIALIZE_ERROR_INT_OVERFLOW);
    return_trace (true);
  }

  bool subset (subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    const set_t &glyphset = *c->plan->glyphset_gsub ();
    const map_t &glyph_map = *c->plan->glyph_map;

    codepoint_t d = deltaGlyphID;
    codepoint_t mask = get_mask ();

    set_t intersection;
    (this+coverage).intersect_set (glyphset, intersection);

    auto it =
    + iter (intersection)
    | map_retains_sorting ([d, mask] (codepoint_t g) {
                                return codepoint_pair_t (g,
                                                            (g + d) & mask); })
    | filter (glyphset, second)
    | map_retains_sorting ([&] (codepoint_pair_t p) -> codepoint_pair_t
                              { return pair (glyph_map[p.first], glyph_map[p.second]); })
    ;

    bool ret = bool (it);
    SingleSubst_serialize (c->serializer, it);
    return_trace (ret);
  }
};

}
}
}


#endif /* OT_LAYOUT_GSUB_SINGLESUBSTFORMAT1_HH */
