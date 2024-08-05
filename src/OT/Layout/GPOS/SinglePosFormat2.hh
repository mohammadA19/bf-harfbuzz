#ifndef OT_LAYOUT_GPOS_SINGLEPOSFORMAT2_HH
#define OT_LAYOUT_GPOS_SINGLEPOSFORMAT2_HH

#include "Common.hh"

namespace OT {
namespace Layout {
namespace GPOS_impl {

struct SinglePosFormat2 : ValueBase
{
  protected:
  HBUINT16      format;                 /* Format identifier--format = 2 */
  Offset16To<Coverage>
                coverage;               /* Offset to Coverage table--from
                                         * beginning of subtable */
  ValueFormat   valueFormat;            /* Defines the types of data in the
                                         * ValueRecord */
  HBUINT16      valueCount;             /* Number of ValueRecords */
  ValueRecord   values;                 /* Array of ValueRecords--positioning
                                         * values applied to glyphs */
  public:
  DEFINE_SIZE_ARRAY (8, values);

  bool sanitize (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
                  coverage.sanitize (c, this) &&
                  valueFormat.sanitize_values (c, this, values, valueCount));
  }

  bool intersects (const set_t *glyphs) const
  { return (this+coverage).intersects (glyphs); }

  void closure_lookups (closure_lookups_context_t *c) const {}
  void collect_variation_indices (collect_variation_indices_context_t *c) const
  {
    if (!valueFormat.has_device ()) return;

    auto it =
    + zip (this+coverage, range ((unsigned) valueCount))
    | filter (c->glyph_set, first)
    ;

    if (!it) return;

    unsigned sub_length = valueFormat.get_len ();
    const array_t<const Value> values_array = values.as_array (valueCount * sub_length);

    for (unsigned i : + it
                      | map (second))
      valueFormat.collect_variation_indices (c, this, values_array.sub_array (i * sub_length, sub_length));

  }

  void collect_glyphs (collect_glyphs_context_t *c) const
  { if (unlikely (!(this+coverage).collect_coverage (c->input))) return; }

  const Coverage &get_coverage () const { return this+coverage; }

  ValueFormat get_value_format () const { return valueFormat; }

  bool apply (ot_apply_context_t *c) const
  {
    TRACE_APPLY (this);
    buffer_t *buffer = c->buffer;
    unsigned int index = (this+coverage).get_coverage  (buffer->cur().codepoint);
    if (likely (index == NOT_COVERED)) return_trace (false);

    if (unlikely (index >= valueCount)) return_trace (false);

    if (HB_BUFFER_MESSAGE_MORE && c->buffer->messaging ())
    {
      c->buffer->message (c->font,
			  "positioning glyph at %u",
			  c->buffer->idx);
    }

    valueFormat.apply_value (c, this,
                             &values[index * valueFormat.get_len ()],
                             buffer->cur_pos());

    if (HB_BUFFER_MESSAGE_MORE && c->buffer->messaging ())
    {
      c->buffer->message (c->font,
			  "positioned glyph at %u",
			  c->buffer->idx);
    }

    buffer->idx++;
    return_trace (true);
  }

  bool
  position_single (font_t           *font,
		   blob_t           *table_blob,
		   direction_t       direction,
		   codepoint_t       gid,
		   glyph_position_t &pos) const
  {
    unsigned int index = (this+coverage).get_coverage  (gid);
    if (likely (index == NOT_COVERED)) return false;
    if (unlikely (index >= valueCount)) return false;

    /* This is ugly... */
    buffer_t buffer;
    buffer.props.direction = direction;
    OT::ot_apply_context_t c (1, font, &buffer, table_blob);

    valueFormat.apply_value (&c, this,
                             &values[index * valueFormat.get_len ()],
                             pos);
    return true;
  }


  template<typename Iterator,
      typename SrcLookup,
      requires (is_iterator (Iterator))>
  void serialize (serialize_context_t *c,
                  const SrcLookup *src,
                  Iterator it,
                  ValueFormat newFormat,
                  const hashmap_t<unsigned, pair_t<unsigned, int>> *layout_variation_idx_delta_map)
  {
    auto out = c->extend_min (this);
    if (unlikely (!out)) return;
    if (unlikely (!c->check_assign (valueFormat, newFormat, HB_SERIALIZE_ERROR_INT_OVERFLOW))) return;
    if (unlikely (!c->check_assign (valueCount, it.len (), HB_SERIALIZE_ERROR_ARRAY_OVERFLOW))) return;

    + it
    | map (second)
    | apply ([&] (array_t<const Value> _)
    { src->get_value_format ().copy_values (c, newFormat, src, &_, layout_variation_idx_delta_map); })
    ;

    auto glyphs =
    + it
    | map_retains_sorting (first)
    ;

    coverage.serialize_serialize (c, glyphs);
  }

  template<typename Iterator,
      requires (is_iterator (Iterator))>
  unsigned compute_effective_format (const face_t *face,
                                     Iterator it,
                                     bool is_instancing, bool strip_hints,
                                     bool has_gdef_varstore,
                                     const hashmap_t<unsigned, pair_t<unsigned, int>> *varidx_delta_map) const
  {
    blob_t* blob = face_reference_table (face, HB_TAG ('f','v','a','r'));
    bool has_fvar = (blob != blob_get_empty ());
    blob_destroy (blob);

    unsigned new_format = 0;
    if (is_instancing)
    {
      new_format = new_format | valueFormat.get_effective_format (+ it | map (second), false, false, this, varidx_delta_map);
    }
    /* do not strip hints for VF */
    else if (strip_hints)
    {
      bool strip = !has_fvar;
      if (has_fvar && !has_gdef_varstore)
        strip = true;
      new_format = new_format | valueFormat.get_effective_format (+ it | map (second), strip, true, this, nullptr);
    }
    else
      new_format = valueFormat;

    return new_format;
  }

  bool subset (subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    const set_t &glyphset = *c->plan->glyphset_gsub ();
    const map_t &glyph_map = *c->plan->glyph_map;

    unsigned sub_length = valueFormat.get_len ();
    auto values_array = values.as_array (valueCount * sub_length);

    auto it =
    + zip (this+coverage, range ((unsigned) valueCount))
    | filter (glyphset, first)
    | map_retains_sorting ([&] (const pair_t<codepoint_t, unsigned>& _)
                              {
                                return pair (glyph_map[_.first],
                                                values_array.sub_array (_.second * sub_length,
                                                                        sub_length));
                              })
    ;

    unsigned new_format = compute_effective_format (c->plan->source, it,
                                                    bool (c->plan->normalized_coords),
                                                    bool (c->plan->flags & HB_SUBSET_FLAGS_NO_HINTING),
                                                    c->plan->has_gdef_varstore,
                                                    &c->plan->layout_variation_idx_delta_map);
    bool ret = bool (it);
    SinglePos_serialize (c->serializer, this, it, &c->plan->layout_variation_idx_delta_map, new_format);
    return_trace (ret);
  }
};


}
}
}

#endif /* OT_LAYOUT_GPOS_SINGLEPOSFORMAT2_HH */
