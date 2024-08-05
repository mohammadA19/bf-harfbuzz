/*
 * Copyright © 2016  Igalia S.L.
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
 * Igalia Author(s): Frédéric Wang
 */

#ifndef HB_OT_MATH_TABLE_HH
#define HB_OT_MATH_TABLE_HH

#include "hb-open-type.hh"
#include "hb-ot-layout-common.hh"
#include "hb-ot-math.h"

namespace OT {


struct MathValueRecord
{
  position_t get_x_value (font_t *font, const void *base) const
  { return font->em_scale_x (value) + (base+deviceTable).get_x_delta (font); }
  position_t get_y_value (font_t *font, const void *base) const
  { return font->em_scale_y (value) + (base+deviceTable).get_y_delta (font); }

  MathValueRecord* copy (serialize_context_t *c, const void *base) const
  {
    TRACE_SERIALIZE (this);
    auto *out = c->embed (this);
    if (unlikely (!out)) return_trace (nullptr);
    out->deviceTable.serialize_copy (c, deviceTable, base, 0, serialize_context_t::Head);

    return_trace (out);
  }

  bool sanitize (sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && deviceTable.sanitize (c, base));
  }

  protected:
  HBINT16		value;		/* The X or Y value in design units */
  Offset16To<Device>	deviceTable;	/* Offset to the device table - from the
					 * beginning of parent table.  May be NULL.
					 * Suggested format for device table is 1. */

  public:
  DEFINE_SIZE_STATIC (4);
};

struct MathConstants
{
  MathConstants* copy (serialize_context_t *c) const
  {
    TRACE_SERIALIZE (this);
    auto *out = c->start_embed (this);

    HBINT16 *p = c->allocate_size<HBINT16> (HBINT16::static_size * 2);
    if (unlikely (!p)) return_trace (nullptr);
    memcpy (p, percentScaleDown, HBINT16::static_size * 2);

    HBUINT16 *m = c->allocate_size<HBUINT16> (HBUINT16::static_size * 2);
    if (unlikely (!m)) return_trace (nullptr);
    memcpy (m, minHeight, HBUINT16::static_size * 2);

    unsigned count = ARRAY_LENGTH (mathValueRecords);
    for (unsigned i = 0; i < count; i++)
      if (!c->copy (mathValueRecords[i], this))
        return_trace (nullptr);

    if (!c->embed (radicalDegreeBottomRaisePercent)) return_trace (nullptr);
    return_trace (out);
  }

  bool sanitize_math_value_records (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);

    unsigned int count = ARRAY_LENGTH (mathValueRecords);
    for (unsigned int i = 0; i < count; i++)
      if (!mathValueRecords[i].sanitize (c, this))
	return_trace (false);

    return_trace (true);
  }

  bool sanitize (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) && sanitize_math_value_records (c));
  }

  position_t get_value (ot_math_constant_t constant,
			   font_t *font) const
  {
    switch (constant) {

    case HB_OT_MATH_CONSTANT_SCRIPT_PERCENT_SCALE_DOWN:
    case HB_OT_MATH_CONSTANT_SCRIPT_SCRIPT_PERCENT_SCALE_DOWN:
      return percentScaleDown[constant - HB_OT_MATH_CONSTANT_SCRIPT_PERCENT_SCALE_DOWN];

    case HB_OT_MATH_CONSTANT_DELIMITED_SUB_FORMULA_MIN_HEIGHT:
    case HB_OT_MATH_CONSTANT_DISPLAY_OPERATOR_MIN_HEIGHT:
      return font->em_scale_y (minHeight[constant - HB_OT_MATH_CONSTANT_DELIMITED_SUB_FORMULA_MIN_HEIGHT]);

    case HB_OT_MATH_CONSTANT_RADICAL_KERN_AFTER_DEGREE:
    case HB_OT_MATH_CONSTANT_RADICAL_KERN_BEFORE_DEGREE:
    case HB_OT_MATH_CONSTANT_SKEWED_FRACTION_HORIZONTAL_GAP:
    case HB_OT_MATH_CONSTANT_SPACE_AFTER_SCRIPT:
      return mathValueRecords[constant - HB_OT_MATH_CONSTANT_MATH_LEADING].get_x_value (font, this);

    case HB_OT_MATH_CONSTANT_ACCENT_BASE_HEIGHT:
    case HB_OT_MATH_CONSTANT_AXIS_HEIGHT:
    case HB_OT_MATH_CONSTANT_FLATTENED_ACCENT_BASE_HEIGHT:
    case HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_DISPLAY_STYLE_SHIFT_DOWN:
    case HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_GAP_MIN:
    case HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_SHIFT_DOWN:
    case HB_OT_MATH_CONSTANT_FRACTION_DENOM_DISPLAY_STYLE_GAP_MIN:
    case HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_DISPLAY_STYLE_SHIFT_UP:
    case HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_GAP_MIN:
    case HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_SHIFT_UP:
    case HB_OT_MATH_CONSTANT_FRACTION_NUM_DISPLAY_STYLE_GAP_MIN:
    case HB_OT_MATH_CONSTANT_FRACTION_RULE_THICKNESS:
    case HB_OT_MATH_CONSTANT_LOWER_LIMIT_BASELINE_DROP_MIN:
    case HB_OT_MATH_CONSTANT_LOWER_LIMIT_GAP_MIN:
    case HB_OT_MATH_CONSTANT_MATH_LEADING:
    case HB_OT_MATH_CONSTANT_OVERBAR_EXTRA_ASCENDER:
    case HB_OT_MATH_CONSTANT_OVERBAR_RULE_THICKNESS:
    case HB_OT_MATH_CONSTANT_OVERBAR_VERTICAL_GAP:
    case HB_OT_MATH_CONSTANT_RADICAL_DISPLAY_STYLE_VERTICAL_GAP:
    case HB_OT_MATH_CONSTANT_RADICAL_EXTRA_ASCENDER:
    case HB_OT_MATH_CONSTANT_RADICAL_RULE_THICKNESS:
    case HB_OT_MATH_CONSTANT_RADICAL_VERTICAL_GAP:
    case HB_OT_MATH_CONSTANT_SKEWED_FRACTION_VERTICAL_GAP:
    case HB_OT_MATH_CONSTANT_STACK_BOTTOM_DISPLAY_STYLE_SHIFT_DOWN:
    case HB_OT_MATH_CONSTANT_STACK_BOTTOM_SHIFT_DOWN:
    case HB_OT_MATH_CONSTANT_STACK_DISPLAY_STYLE_GAP_MIN:
    case HB_OT_MATH_CONSTANT_STACK_GAP_MIN:
    case HB_OT_MATH_CONSTANT_STACK_TOP_DISPLAY_STYLE_SHIFT_UP:
    case HB_OT_MATH_CONSTANT_STACK_TOP_SHIFT_UP:
    case HB_OT_MATH_CONSTANT_STRETCH_STACK_BOTTOM_SHIFT_DOWN:
    case HB_OT_MATH_CONSTANT_STRETCH_STACK_GAP_ABOVE_MIN:
    case HB_OT_MATH_CONSTANT_STRETCH_STACK_GAP_BELOW_MIN:
    case HB_OT_MATH_CONSTANT_STRETCH_STACK_TOP_SHIFT_UP:
    case HB_OT_MATH_CONSTANT_SUBSCRIPT_BASELINE_DROP_MIN:
    case HB_OT_MATH_CONSTANT_SUBSCRIPT_SHIFT_DOWN:
    case HB_OT_MATH_CONSTANT_SUBSCRIPT_TOP_MAX:
    case HB_OT_MATH_CONSTANT_SUB_SUPERSCRIPT_GAP_MIN:
    case HB_OT_MATH_CONSTANT_SUPERSCRIPT_BASELINE_DROP_MAX:
    case HB_OT_MATH_CONSTANT_SUPERSCRIPT_BOTTOM_MAX_WITH_SUBSCRIPT:
    case HB_OT_MATH_CONSTANT_SUPERSCRIPT_BOTTOM_MIN:
    case HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP:
    case HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP_CRAMPED:
    case HB_OT_MATH_CONSTANT_UNDERBAR_EXTRA_DESCENDER:
    case HB_OT_MATH_CONSTANT_UNDERBAR_RULE_THICKNESS:
    case HB_OT_MATH_CONSTANT_UNDERBAR_VERTICAL_GAP:
    case HB_OT_MATH_CONSTANT_UPPER_LIMIT_BASELINE_RISE_MIN:
    case HB_OT_MATH_CONSTANT_UPPER_LIMIT_GAP_MIN:
      return mathValueRecords[constant - HB_OT_MATH_CONSTANT_MATH_LEADING].get_y_value (font, this);

    case HB_OT_MATH_CONSTANT_RADICAL_DEGREE_BOTTOM_RAISE_PERCENT:
      return radicalDegreeBottomRaisePercent;

    default:
      return 0;
    }
  }

  protected:
  HBINT16 percentScaleDown[2];
  HBUINT16 minHeight[2];
  MathValueRecord mathValueRecords[51];
  HBINT16 radicalDegreeBottomRaisePercent;

  public:
  DEFINE_SIZE_STATIC (214);
};

struct MathItalicsCorrectionInfo
{
  bool subset (subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    const set_t &glyphset = c->plan->_glyphset_mathed;
    const map_t &glyph_map = *c->plan->glyph_map;

    auto *out = c->serializer->start_embed (*this);
    if (unlikely (!c->serializer->extend_min (out))) return_trace (false);

    sorted_vector_t<codepoint_t> new_coverage;
    + zip (this+coverage, italicsCorrection)
    | filter (glyphset, first)
    | filter (serialize_math_record_array (c->serializer, out->italicsCorrection, this), second)
    | map (first)
    | map (glyph_map)
    | sink (new_coverage)
    ;

    out->coverage.serialize_serialize (c->serializer, new_coverage.iter ());
    return_trace (true);
  }

  bool sanitize (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  coverage.sanitize (c, this) &&
		  italicsCorrection.sanitize (c, this));
  }

  position_t get_value (codepoint_t glyph,
			   font_t *font) const
  {
    unsigned int index = (this+coverage).get_coverage (glyph);
    return italicsCorrection[index].get_x_value (font, this);
  }

  protected:
  Offset16To<Coverage>       coverage;		/* Offset to Coverage table -
						 * from the beginning of
						 * MathItalicsCorrectionInfo
						 * table. */
  Array16Of<MathValueRecord> italicsCorrection;	/* Array of MathValueRecords
						 * defining italics correction
						 * values for each
						 * covered glyph. */

  public:
  DEFINE_SIZE_ARRAY (4, italicsCorrection);
};

struct MathTopAccentAttachment
{
  bool subset (subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    const set_t &glyphset = c->plan->_glyphset_mathed;
    const map_t &glyph_map = *c->plan->glyph_map;

    auto *out = c->serializer->start_embed (*this);
    if (unlikely (!c->serializer->extend_min (out))) return_trace (false);

    sorted_vector_t<codepoint_t> new_coverage;
    + zip (this+topAccentCoverage, topAccentAttachment)
    | filter (glyphset, first)
    | filter (serialize_math_record_array (c->serializer, out->topAccentAttachment, this), second)
    | map (first)
    | map (glyph_map)
    | sink (new_coverage)
    ;

    out->topAccentCoverage.serialize_serialize (c->serializer, new_coverage.iter ());
    return_trace (true);
  }

  bool sanitize (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  topAccentCoverage.sanitize (c, this) &&
		  topAccentAttachment.sanitize (c, this));
  }

  position_t get_value (codepoint_t glyph,
			   font_t *font) const
  {
    unsigned int index = (this+topAccentCoverage).get_coverage (glyph);
    if (index == NOT_COVERED)
      return font->get_glyph_h_advance (glyph) / 2;
    return topAccentAttachment[index].get_x_value (font, this);
  }

  protected:
  Offset16To<Coverage>       topAccentCoverage;   /* Offset to Coverage table -
						 * from the beginning of
						 * MathTopAccentAttachment
						 * table. */
  Array16Of<MathValueRecord> topAccentAttachment; /* Array of MathValueRecords
						 * defining top accent
						 * attachment points for each
						 * covered glyph. */

  public:
  DEFINE_SIZE_ARRAY (2 + 2, topAccentAttachment);
};

struct MathKern
{
  MathKern* copy (serialize_context_t *c) const
  {
    TRACE_SERIALIZE (this);
    auto *out = c->start_embed (this);

    if (unlikely (!c->embed (heightCount))) return_trace (nullptr);

    unsigned count = 2 * heightCount + 1;
    for (unsigned i = 0; i < count; i++)
      if (!c->copy (mathValueRecordsZ.arrayZ[i], this))
        return_trace (nullptr);

    return_trace (out);
  }

  bool sanitize_math_value_records (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    unsigned int count = 2 * heightCount + 1;
    for (unsigned int i = 0; i < count; i++)
      if (!mathValueRecordsZ.arrayZ[i].sanitize (c, this)) return_trace (false);
    return_trace (true);
  }

  bool sanitize (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  barrier () &&
		  c->check_array (mathValueRecordsZ.arrayZ, 2 * heightCount + 1) &&
		  sanitize_math_value_records (c));
  }

  position_t get_value (position_t correction_height, font_t *font) const
  {
    const MathValueRecord* correctionHeight = mathValueRecordsZ.arrayZ;
    const MathValueRecord* kernValue = mathValueRecordsZ.arrayZ + heightCount;
    int sign = font->y_scale < 0 ? -1 : +1;

    /* According to OpenType spec (v1.9), except for the boundary cases, the index
     * chosen for kern value should be i such that
     *    correctionHeight[i-1] <= correction_height < correctionHeight[i]
     * We can use the binary search algorithm of std::upper_bound(). Or, we can
     * use the internal bsearch_impl.
     */
    unsigned int pos;
    auto cmp = +[](const void* key, const void* p,
                   int sign, font_t* font, const MathKern* mathKern) -> int {
      return sign * *(position_t*)key - sign * ((MathValueRecord*)p)->get_y_value(font, mathKern);
    };
    unsigned int i = bsearch_impl(&pos, correction_height, correctionHeight,
                                     heightCount, MathValueRecord::static_size,
                                     cmp, sign, font, this) ? pos + 1 : pos;
    return kernValue[i].get_x_value (font, this);
  }

  unsigned int get_entries (unsigned int start_offset,
			    unsigned int *entries_count, /* IN/OUT */
			    ot_math_kern_entry_t *kern_entries, /* OUT */
			    font_t *font) const
  {
    const MathValueRecord* correctionHeight = mathValueRecordsZ.arrayZ;
    const MathValueRecord* kernValue = mathValueRecordsZ.arrayZ + heightCount;
    const unsigned int entriesCount = heightCount + 1;

    if (entries_count)
    {
      unsigned int start = min (start_offset, entriesCount);
      unsigned int end = min (start + *entries_count, entriesCount);
      *entries_count = end - start;

      for (unsigned int i = 0; i < *entries_count; i++) {
	unsigned int j = start + i;

	position_t max_height;
	if (j == heightCount) {
	  max_height = INT32_MAX;
	} else {
	  max_height = correctionHeight[j].get_y_value (font, this);
	}

	kern_entries[i] = {max_height, kernValue[j].get_x_value (font, this)};
      }
    }
    return entriesCount;
  }

  protected:
  HBUINT16	heightCount;
  UnsizedArrayOf<MathValueRecord>
		mathValueRecordsZ;
				/* Array of correction heights at
				 * which the kern value changes.
				 * Sorted by the height value in
				 * design units (heightCount entries),
				 * Followed by:
				 * Array of kern values corresponding
				 * to heights. (heightCount+1 entries).
				 */

  public:
  DEFINE_SIZE_ARRAY (2, mathValueRecordsZ);
};

struct MathKernInfoRecord
{
  MathKernInfoRecord* copy (serialize_context_t *c, const void *base) const
  {
    TRACE_SERIALIZE (this);
    auto *out = c->embed (this);
    if (unlikely (!out)) return_trace (nullptr);

    unsigned count = ARRAY_LENGTH (mathKern);
    for (unsigned i = 0; i < count; i++)
      out->mathKern[i].serialize_copy (c, mathKern[i], base, 0, serialize_context_t::Head);

    return_trace (out);
  }

  bool sanitize (sanitize_context_t *c, const void *base) const
  {
    TRACE_SANITIZE (this);

    unsigned int count = ARRAY_LENGTH (mathKern);
    for (unsigned int i = 0; i < count; i++)
      if (unlikely (!mathKern[i].sanitize (c, base)))
	return_trace (false);

    return_trace (true);
  }

  position_t get_kerning (ot_math_kern_t kern,
			     position_t correction_height,
			     font_t *font,
			     const void *base) const
  {
    unsigned int idx = kern;
    if (unlikely (idx >= ARRAY_LENGTH (mathKern))) return 0;
    return (base+mathKern[idx]).get_value (correction_height, font);
  }

  unsigned int get_kernings (ot_math_kern_t kern,
			     unsigned int start_offset,
			     unsigned int *entries_count, /* IN/OUT */
			     ot_math_kern_entry_t *kern_entries, /* OUT */
			     font_t *font,
			     const void *base) const
  {
    unsigned int idx = kern;
    if (unlikely (idx >= ARRAY_LENGTH (mathKern)) || !mathKern[idx]) {
      if (entries_count) *entries_count = 0;
      return 0;
    }
    return (base+mathKern[idx]).get_entries (start_offset,
					     entries_count,
					     kern_entries,
					     font);
  }

  protected:
  /* Offset to MathKern table for each corner -
   * from the beginning of MathKernInfo table.  May be NULL. */
  Offset16To<MathKern> mathKern[4];

  public:
  DEFINE_SIZE_STATIC (8);
};

struct MathKernInfo
{
  bool subset (subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    const set_t &glyphset = c->plan->_glyphset_mathed;
    const map_t &glyph_map = *c->plan->glyph_map;

    auto *out = c->serializer->start_embed (*this);
    if (unlikely (!c->serializer->extend_min (out))) return_trace (false);

    sorted_vector_t<codepoint_t> new_coverage;
    + zip (this+mathKernCoverage, mathKernInfoRecords)
    | filter (glyphset, first)
    | filter (serialize_math_record_array (c->serializer, out->mathKernInfoRecords, this), second)
    | map (first)
    | map (glyph_map)
    | sink (new_coverage)
    ;

    out->mathKernCoverage.serialize_serialize (c->serializer, new_coverage.iter ());
    return_trace (true);
  }

  bool sanitize (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  mathKernCoverage.sanitize (c, this) &&
		  mathKernInfoRecords.sanitize (c, this));
  }

  position_t get_kerning (codepoint_t glyph,
			     ot_math_kern_t kern,
			     position_t correction_height,
			     font_t *font) const
  {
    unsigned int index = (this+mathKernCoverage).get_coverage (glyph);
    return mathKernInfoRecords[index].get_kerning (kern, correction_height, font, this);
  }

  unsigned int get_kernings (codepoint_t glyph,
			     ot_math_kern_t kern,
			     unsigned int start_offset,
			     unsigned int *entries_count, /* IN/OUT */
			     ot_math_kern_entry_t *kern_entries, /* OUT */
			     font_t *font) const
  {
    unsigned int index = (this+mathKernCoverage).get_coverage (glyph);
    return mathKernInfoRecords[index].get_kernings (kern,
						    start_offset,
						    entries_count,
						    kern_entries,
						    font,
						    this);
  }

  protected:
  Offset16To<Coverage>
		mathKernCoverage;
				/* Offset to Coverage table -
				 * from the beginning of the
				 * MathKernInfo table. */
  Array16Of<MathKernInfoRecord>
		mathKernInfoRecords;
				/* Array of MathKernInfoRecords,
				 * per-glyph information for
				 * mathematical positioning
				 * of subscripts and
				 * superscripts. */

  public:
  DEFINE_SIZE_ARRAY (4, mathKernInfoRecords);
};

struct MathGlyphInfo
{
  bool subset (subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    auto *out = c->serializer->embed (*this);
    if (unlikely (!out)) return_trace (false);

    out->mathItalicsCorrectionInfo.serialize_subset (c, mathItalicsCorrectionInfo, this);
    out->mathTopAccentAttachment.serialize_subset (c, mathTopAccentAttachment, this);

    const set_t &glyphset = c->plan->_glyphset_mathed;
    const map_t &glyph_map = *c->plan->glyph_map;

    auto it =
    + iter (this+extendedShapeCoverage)
    | take (c->plan->source->get_num_glyphs ())
    | filter (glyphset)
    | map_retains_sorting (glyph_map)
    ;

    if (it) out->extendedShapeCoverage.serialize_serialize (c->serializer, it);
    else out->extendedShapeCoverage = 0;

    out->mathKernInfo.serialize_subset (c, mathKernInfo, this);
    return_trace (true);
  }

  bool sanitize (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  mathItalicsCorrectionInfo.sanitize (c, this) &&
		  mathTopAccentAttachment.sanitize (c, this) &&
		  extendedShapeCoverage.sanitize (c, this) &&
		  mathKernInfo.sanitize (c, this));
  }

  position_t
  get_italics_correction (codepoint_t  glyph, font_t *font) const
  { return (this+mathItalicsCorrectionInfo).get_value (glyph, font); }

  position_t
  get_top_accent_attachment (codepoint_t  glyph, font_t *font) const
  { return (this+mathTopAccentAttachment).get_value (glyph, font); }

  bool is_extended_shape (codepoint_t glyph) const
  { return (this+extendedShapeCoverage).get_coverage (glyph) != NOT_COVERED; }

  position_t get_kerning (codepoint_t glyph,
			     ot_math_kern_t kern,
			     position_t correction_height,
			     font_t *font) const
  { return (this+mathKernInfo).get_kerning (glyph, kern, correction_height, font); }

  position_t get_kernings (codepoint_t glyph,
			      ot_math_kern_t kern,
			      unsigned int start_offset,
			      unsigned int *entries_count, /* IN/OUT */
			      ot_math_kern_entry_t *kern_entries, /* OUT */
			      font_t *font) const
  { return (this+mathKernInfo).get_kernings (glyph,
					     kern,
					     start_offset,
					     entries_count,
					     kern_entries,
					     font); }

  protected:
  /* Offset to MathItalicsCorrectionInfo table -
   * from the beginning of MathGlyphInfo table. */
  Offset16To<MathItalicsCorrectionInfo> mathItalicsCorrectionInfo;

  /* Offset to MathTopAccentAttachment table -
   * from the beginning of MathGlyphInfo table. */
  Offset16To<MathTopAccentAttachment> mathTopAccentAttachment;

  /* Offset to coverage table for Extended Shape glyphs -
   * from the beginning of MathGlyphInfo table. When the left or right glyph of
   * a box is an extended shape variant, the (ink) box (and not the default
   * position defined by values in MathConstants table) should be used for
   * vertical positioning purposes.  May be NULL.. */
  Offset16To<Coverage> extendedShapeCoverage;

   /* Offset to MathKernInfo table -
    * from the beginning of MathGlyphInfo table. */
  Offset16To<MathKernInfo> mathKernInfo;

  public:
  DEFINE_SIZE_STATIC (8);
};

struct MathGlyphVariantRecord
{
  friend struct MathGlyphConstruction;

  bool subset (subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    auto *out = c->serializer->embed (this);
    if (unlikely (!out)) return_trace (false);

    const map_t& glyph_map = *c->plan->glyph_map;
    return_trace (c->serializer->check_assign (out->variantGlyph, glyph_map.get (variantGlyph), HB_SERIALIZE_ERROR_INT_OVERFLOW));
  }

  bool sanitize (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  void closure_glyphs (set_t *variant_glyphs) const
  { variant_glyphs->add (variantGlyph); }

  protected:
  HBGlyphID16 variantGlyph;       /* Glyph ID for the variant. */
  HBUINT16  advanceMeasurement; /* Advance width/height, in design units, of the
				 * variant, in the direction of requested
				 * glyph extension. */

  public:
  DEFINE_SIZE_STATIC (4);
};

struct PartFlags : HBUINT16
{
  enum Flags {
    Extender	= 0x0001u, /* If set, the part can be skipped or repeated. */

    Defined	= 0x0001u, /* All defined flags. */
  };

  public:
  DEFINE_SIZE_STATIC (2);
};

struct MathGlyphPartRecord
{
  bool subset (subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    auto *out = c->serializer->embed (this);
    if (unlikely (!out)) return_trace (false);

    const map_t& glyph_map = *c->plan->glyph_map;
    return_trace (c->serializer->check_assign (out->glyph, glyph_map.get (glyph), HB_SERIALIZE_ERROR_INT_OVERFLOW));
  }

  bool sanitize (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this));
  }

  void extract (ot_math_glyph_part_t &out,
		int64_t mult,
		font_t *font) const
  {
    out.glyph			= glyph;

    out.start_connector_length	= font->em_mult (startConnectorLength, mult);
    out.end_connector_length	= font->em_mult (endConnectorLength, mult);
    out.full_advance		= font->em_mult (fullAdvance, mult);

    static_assert ((unsigned int) HB_OT_MATH_GLYPH_PART_FLAG_EXTENDER ==
		   (unsigned int) PartFlags::Extender, "");

    out.flags = (ot_math_glyph_part_flags_t)
		(unsigned int)
		(partFlags & PartFlags::Defined);
  }

  void closure_glyphs (set_t *variant_glyphs) const
  { variant_glyphs->add (glyph); }

  protected:
  HBGlyphID16	glyph;		/* Glyph ID for the part. */
  HBUINT16	startConnectorLength;
				/* Advance width/ height of the straight bar
				 * connector material, in design units, is at
				 * the beginning of the glyph, in the
				 * direction of the extension. */
  HBUINT16	endConnectorLength;
				/* Advance width/ height of the straight bar
				 * connector material, in design units, is at
				 * the end of the glyph, in the direction of
				 * the extension. */
  HBUINT16	fullAdvance;	/* Full advance width/height for this part,
				 * in the direction of the extension.
				 * In design units. */
  PartFlags	partFlags;	/* Part qualifiers. */

  public:
  DEFINE_SIZE_STATIC (10);
};

struct MathGlyphAssembly
{
  bool subset (subset_context_t *c) const
  {
    TRACE_SUBSET (this);

    if (!c->serializer->copy (italicsCorrection, this)) return_trace (false);
    if (!c->serializer->copy<HBUINT16> (partRecords.len)) return_trace (false);

    for (const auto& record : partRecords.iter ())
      if (!record.subset (c)) return_trace (false);
    return_trace (true);
  }

  bool sanitize (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  italicsCorrection.sanitize (c, this) &&
		  partRecords.sanitize (c));
  }

  unsigned int get_parts (direction_t direction,
			  font_t *font,
			  unsigned int start_offset,
			  unsigned int *parts_count, /* IN/OUT */
			  ot_math_glyph_part_t *parts /* OUT */,
			  position_t *italics_correction /* OUT */) const
  {
    if (parts_count)
    {
      int64_t mult = font->dir_mult (direction);
      for (auto _ : zip (partRecords.as_array ().sub_array (start_offset, parts_count),
			    array (parts, *parts_count)))
	_.first.extract (_.second, mult, font);
    }

    if (italics_correction)
      *italics_correction = italicsCorrection.get_x_value (font, this);

    return partRecords.len;
  }

  void closure_glyphs (set_t *variant_glyphs) const
  {
    for (const auto& _ : partRecords.iter ())
      _.closure_glyphs (variant_glyphs);
  }

  protected:
  MathValueRecord
		italicsCorrection;
				/* Italics correction of this
				 * MathGlyphAssembly. Should not
				 * depend on the assembly size. */
  Array16Of<MathGlyphPartRecord>
		partRecords;	/* Array of part records, from
				 * left to right and bottom to
				 * top. */

  public:
  DEFINE_SIZE_ARRAY (6, partRecords);
};

struct MathGlyphConstruction
{
  bool subset (subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    auto *out = c->serializer->start_embed (*this);
    if (unlikely (!c->serializer->extend_min (out))) return_trace (false);

    out->glyphAssembly.serialize_subset (c, glyphAssembly, this);

    if (!c->serializer->check_assign (out->mathGlyphVariantRecord.len, mathGlyphVariantRecord.len, HB_SERIALIZE_ERROR_INT_OVERFLOW))
      return_trace (false);
    for (const auto& record : mathGlyphVariantRecord.iter ())
      if (!record.subset (c)) return_trace (false);

    return_trace (true);
  }

  bool sanitize (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  glyphAssembly.sanitize (c, this) &&
		  mathGlyphVariantRecord.sanitize (c));
  }

  const MathGlyphAssembly &get_assembly () const { return this+glyphAssembly; }

  unsigned int get_variants (direction_t direction,
			     font_t *font,
			     unsigned int start_offset,
			     unsigned int *variants_count, /* IN/OUT */
			     ot_math_glyph_variant_t *variants /* OUT */) const
  {
    if (variants_count)
    {
      int64_t mult = font->dir_mult (direction);
      for (auto _ : zip (mathGlyphVariantRecord.as_array ().sub_array (start_offset, variants_count),
			    array (variants, *variants_count)))
	_.second = {_.first.variantGlyph, font->em_mult (_.first.advanceMeasurement, mult)};
    }
    return mathGlyphVariantRecord.len;
  }

  void closure_glyphs (set_t *variant_glyphs) const
  {
    (this+glyphAssembly).closure_glyphs (variant_glyphs);

    for (const auto& _ : mathGlyphVariantRecord.iter ())
      _.closure_glyphs (variant_glyphs);
  }

  protected:
  /* Offset to MathGlyphAssembly table for this shape - from the beginning of
     MathGlyphConstruction table.  May be NULL. */
  Offset16To<MathGlyphAssembly>	  glyphAssembly;

  /* MathGlyphVariantRecords for alternative variants of the glyphs. */
  Array16Of<MathGlyphVariantRecord> mathGlyphVariantRecord;

  public:
  DEFINE_SIZE_ARRAY (4, mathGlyphVariantRecord);
};

struct MathVariants
{
  void closure_glyphs (const set_t *glyph_set,
                       set_t *variant_glyphs) const
  {
    const array_t<const Offset16To<MathGlyphConstruction>> glyph_construction_offsets = glyphConstruction.as_array (vertGlyphCount + horizGlyphCount);

    if (vertGlyphCoverage)
    {
      const auto vert_offsets = glyph_construction_offsets.sub_array (0, vertGlyphCount);
      + zip (this+vertGlyphCoverage, vert_offsets)
      | filter (glyph_set, first)
      | map (second)
      | map (add (this))
      | apply ([=] (const MathGlyphConstruction &_) { _.closure_glyphs (variant_glyphs); })
      ;
    }

    if (horizGlyphCoverage)
    {
      const auto hori_offsets = glyph_construction_offsets.sub_array (vertGlyphCount, horizGlyphCount);
      + zip (this+horizGlyphCoverage, hori_offsets)
      | filter (glyph_set, first)
      | map (second)
      | map (add (this))
      | apply ([=] (const MathGlyphConstruction &_) { _.closure_glyphs (variant_glyphs); })
      ;
    }
  }

  void collect_coverage_and_indices (sorted_vector_t<codepoint_t>& new_coverage,
                                     const Offset16To<Coverage>& coverage,
                                     unsigned i,
                                     unsigned end_index,
                                     set_t& indices,
                                     const set_t& glyphset,
                                     const map_t& glyph_map) const
  {
    if (!coverage) return;

    for (const auto _ : (this+coverage).iter ())
    {
      if (i >= end_index) return;
      if (glyphset.has (_))
      {
        unsigned new_gid = glyph_map.get (_);
        new_coverage.push (new_gid);
        indices.add (i);
      }
      i++;
    }
  }

  bool subset (subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    const set_t &glyphset = c->plan->_glyphset_mathed;
    const map_t &glyph_map = *c->plan->glyph_map;

    auto *out = c->serializer->start_embed (*this);
    if (unlikely (!c->serializer->extend_min (out))) return_trace (false);
    if (!c->serializer->check_assign (out->minConnectorOverlap, minConnectorOverlap, HB_SERIALIZE_ERROR_INT_OVERFLOW))
      return_trace (false);

    sorted_vector_t<codepoint_t> new_vert_coverage;
    sorted_vector_t<codepoint_t> new_hori_coverage;
    set_t indices;
    collect_coverage_and_indices (new_vert_coverage, vertGlyphCoverage, 0, vertGlyphCount, indices, glyphset, glyph_map);
    collect_coverage_and_indices (new_hori_coverage, horizGlyphCoverage, vertGlyphCount, vertGlyphCount + horizGlyphCount, indices, glyphset, glyph_map);

    if (!c->serializer->check_assign (out->vertGlyphCount, new_vert_coverage.length, HB_SERIALIZE_ERROR_INT_OVERFLOW))
      return_trace (false);
    if (!c->serializer->check_assign (out->horizGlyphCount, new_hori_coverage.length, HB_SERIALIZE_ERROR_INT_OVERFLOW))
      return_trace (false);

    for (unsigned i : indices.iter ())
    {
      auto *o = c->serializer->embed (glyphConstruction[i]);
      if (!o) return_trace (false);
      o->serialize_subset (c, glyphConstruction[i], this);
    }

    if (new_vert_coverage)
      out->vertGlyphCoverage.serialize_serialize (c->serializer, new_vert_coverage.iter ());

    if (new_hori_coverage)
    out->horizGlyphCoverage.serialize_serialize (c->serializer, new_hori_coverage.iter ());
    return_trace (true);
  }

  bool sanitize_offsets (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    unsigned int count = vertGlyphCount + horizGlyphCount;
    for (unsigned int i = 0; i < count; i++)
      if (!glyphConstruction.arrayZ[i].sanitize (c, this)) return_trace (false);
    return_trace (true);
  }

  bool sanitize (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (c->check_struct (this) &&
		  vertGlyphCoverage.sanitize (c, this) &&
		  horizGlyphCoverage.sanitize (c, this) &&
		  barrier () &&
		  c->check_array (glyphConstruction.arrayZ, vertGlyphCount + horizGlyphCount) &&
		  sanitize_offsets (c));
  }

  position_t get_min_connector_overlap (direction_t direction,
						  font_t *font) const
  { return font->em_scale_dir (minConnectorOverlap, direction); }

  unsigned int get_glyph_variants (codepoint_t glyph,
				   direction_t direction,
				   font_t *font,
				   unsigned int start_offset,
				   unsigned int *variants_count, /* IN/OUT */
				   ot_math_glyph_variant_t *variants /* OUT */) const
  { return get_glyph_construction (glyph, direction, font)
	   .get_variants (direction, font, start_offset, variants_count, variants); }

  unsigned int get_glyph_parts (codepoint_t glyph,
				direction_t direction,
				font_t *font,
				unsigned int start_offset,
				unsigned int *parts_count, /* IN/OUT */
				ot_math_glyph_part_t *parts /* OUT */,
				position_t *italics_correction /* OUT */) const
  { return get_glyph_construction (glyph, direction, font)
	   .get_assembly ()
	   .get_parts (direction, font,
		       start_offset, parts_count, parts,
		       italics_correction); }

  private:
  const MathGlyphConstruction &
  get_glyph_construction (codepoint_t glyph,
			  direction_t direction,
			  font_t *font HB_UNUSED) const
  {
    bool vertical = HB_DIRECTION_IS_VERTICAL (direction);
    unsigned int count = vertical ? vertGlyphCount : horizGlyphCount;
    const Offset16To<Coverage> &coverage = vertical ? vertGlyphCoverage
						  : horizGlyphCoverage;

    unsigned int index = (this+coverage).get_coverage (glyph);
    if (unlikely (index >= count)) return Null (MathGlyphConstruction);

    if (!vertical)
      index += vertGlyphCount;

    return this+glyphConstruction[index];
  }

  protected:
  HBUINT16	minConnectorOverlap;
				/* Minimum overlap of connecting
				 * glyphs during glyph construction,
				 * in design units. */
  Offset16To<Coverage> vertGlyphCoverage;
				/* Offset to Coverage table -
				 * from the beginning of MathVariants
				 * table. */
  Offset16To<Coverage> horizGlyphCoverage;
				/* Offset to Coverage table -
				 * from the beginning of MathVariants
				 * table. */
  HBUINT16	vertGlyphCount;	/* Number of glyphs for which
				 * information is provided for
				 * vertically growing variants. */
  HBUINT16	horizGlyphCount;/* Number of glyphs for which
				 * information is provided for
				 * horizontally growing variants. */

  /* Array of offsets to MathGlyphConstruction tables - from the beginning of
     the MathVariants table, for shapes growing in vertical/horizontal
     direction. */
  UnsizedArrayOf<Offset16To<MathGlyphConstruction>>
			glyphConstruction;

  public:
  DEFINE_SIZE_ARRAY (10, glyphConstruction);
};


/*
 * MATH -- Mathematical typesetting
 * https://docs.microsoft.com/en-us/typography/opentype/spec/math
 */

struct MATH
{
  static constexpr tag_t tableTag = HB_OT_TAG_MATH;

  bool has_data () const { return version.to_int (); }

  void closure_glyphs (set_t *glyph_set) const
  {
    if (mathVariants)
    {
      set_t variant_glyphs;
      (this+mathVariants).closure_glyphs (glyph_set, &variant_glyphs);
      set_union (glyph_set, &variant_glyphs);
    }
  }

  bool subset (subset_context_t *c) const
  {
    TRACE_SUBSET (this);
    auto *out = c->serializer->embed (*this);
    if (unlikely (!out)) return_trace (false);

    out->mathConstants.serialize_copy (c->serializer, mathConstants, this, 0, serialize_context_t::Head);
    out->mathGlyphInfo.serialize_subset (c, mathGlyphInfo, this);
    out->mathVariants.serialize_subset (c, mathVariants, this);
    return_trace (true);
  }

  bool sanitize (sanitize_context_t *c) const
  {
    TRACE_SANITIZE (this);
    return_trace (version.sanitize (c) &&
		  likely (version.major == 1) &&
		  barrier () &&
		  mathConstants.sanitize (c, this) &&
		  mathGlyphInfo.sanitize (c, this) &&
		  mathVariants.sanitize (c, this));
  }

  position_t get_constant (ot_math_constant_t  constant,
			      font_t		   *font) const
  { return (this+mathConstants).get_value (constant, font); }

  const MathGlyphInfo &get_glyph_info () const { return this+mathGlyphInfo; }

  const MathVariants &get_variants () const    { return this+mathVariants; }

  protected:
  FixedVersion<>version;	/* Version of the MATH table
				 * initially set to 0x00010000u */
  Offset16To<MathConstants>
		mathConstants;	/* MathConstants table */
  Offset16To<MathGlyphInfo>
		mathGlyphInfo;	/* MathGlyphInfo table */
  Offset16To<MathVariants>
		mathVariants;	/* MathVariants table */

  public:
  DEFINE_SIZE_STATIC (10);
};

} /* namespace OT */


#endif /* HB_OT_MATH_TABLE_HH */
