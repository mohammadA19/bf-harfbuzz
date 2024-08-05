/*
 * Copyright © 2007,2008,2009  Red Hat, Inc.
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
 * Red Hat Author(s): Behdad Esfahbod
 */

#if !defined(HB_OT_H_IN) && !defined(HB_NO_SINGLE_HEADER_ERROR)
#error "Include <hb-ot.h> instead."
#endif

#ifndef HB_OT_LAYOUT_H
#define HB_OT_LAYOUT_H

#include "hb.h"

#include "hb-ot-name.h"

HB_BEGIN_DECLS


/**
 * HB_OT_TAG_BASE:
 *
 * OpenType [Baseline Table](https://docs.microsoft.com/en-us/typography/opentype/spec/base).
 */
#define HB_OT_TAG_BASE HB_TAG('B','A','S','E')
/**
 * HB_OT_TAG_GDEF:
 *
 * OpenType [Glyph Definition Table](https://docs.microsoft.com/en-us/typography/opentype/spec/gdef).
 */
#define HB_OT_TAG_GDEF HB_TAG('G','D','E','F')
/**
 * HB_OT_TAG_GSUB:
 *
 * OpenType [Glyph Substitution Table](https://docs.microsoft.com/en-us/typography/opentype/spec/gsub).
 */
#define HB_OT_TAG_GSUB HB_TAG('G','S','U','B')
/**
 * HB_OT_TAG_GPOS:
 *
 * OpenType [Glyph Positioning Table](https://docs.microsoft.com/en-us/typography/opentype/spec/gpos).
 */
#define HB_OT_TAG_GPOS HB_TAG('G','P','O','S')
/**
 * HB_OT_TAG_JSTF:
 *
 * OpenType [Justification Table](https://docs.microsoft.com/en-us/typography/opentype/spec/jstf).
 */
#define HB_OT_TAG_JSTF HB_TAG('J','S','T','F')


/*
 * Script & Language tags.
 */

/**
 * HB_OT_TAG_DEFAULT_SCRIPT:
 *
 * OpenType script tag, `DFLT`, for features that are not script-specific.
 *
 */
#define HB_OT_TAG_DEFAULT_SCRIPT	HB_TAG ('D', 'F', 'L', 'T')
/**
 * HB_OT_TAG_DEFAULT_LANGUAGE:
 *
 * OpenType language tag, `dflt`. Not a valid language tag, but some fonts
 * mistakenly use it.
 */
#define HB_OT_TAG_DEFAULT_LANGUAGE	HB_TAG ('d', 'f', 'l', 't')

/**
 * HB_OT_MAX_TAGS_PER_SCRIPT:
 *
 * Maximum number of OpenType tags that can correspond to a give #script_t.
 *
 * Since: 2.0.0
 **/
#define HB_OT_MAX_TAGS_PER_SCRIPT	3u
/**
 * HB_OT_MAX_TAGS_PER_LANGUAGE:
 *
 * Maximum number of OpenType tags that can correspond to a give #language_t.
 *
 * Since: 2.0.0
 **/
#define HB_OT_MAX_TAGS_PER_LANGUAGE	3u

HB_EXTERN void
ot_tags_from_script_and_language (script_t   script,
				     language_t language,
				     unsigned int *script_count /* IN/OUT */,
				     tag_t     *script_tags /* OUT */,
				     unsigned int *language_count /* IN/OUT */,
				     tag_t     *language_tags /* OUT */);

HB_EXTERN script_t
ot_tag_to_script (tag_t tag);

HB_EXTERN language_t
ot_tag_to_language (tag_t tag);

HB_EXTERN void
ot_tags_to_script_and_language (tag_t       script_tag,
				   tag_t       language_tag,
				   script_t   *script /* OUT */,
				   language_t *language /* OUT */);


/*
 * GDEF
 */

HB_EXTERN bool_t
ot_layout_has_glyph_classes (face_t *face);

/**
 * ot_layout_glyph_class_t:
 * @HB_OT_LAYOUT_GLYPH_CLASS_UNCLASSIFIED: Glyphs not matching the other classifications
 * @HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH: Spacing, single characters, capable of accepting marks
 * @HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE: Glyphs that represent ligation of multiple characters
 * @HB_OT_LAYOUT_GLYPH_CLASS_MARK: Non-spacing, combining glyphs that represent marks
 * @HB_OT_LAYOUT_GLYPH_CLASS_COMPONENT: Spacing glyphs that represent part of a single character
 *
 * The GDEF classes defined for glyphs.
 *
 **/
typedef enum {
  HB_OT_LAYOUT_GLYPH_CLASS_UNCLASSIFIED	= 0,
  HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH	= 1,
  HB_OT_LAYOUT_GLYPH_CLASS_LIGATURE	= 2,
  HB_OT_LAYOUT_GLYPH_CLASS_MARK		= 3,
  HB_OT_LAYOUT_GLYPH_CLASS_COMPONENT	= 4
} ot_layout_glyph_class_t;

HB_EXTERN ot_layout_glyph_class_t
ot_layout_get_glyph_class (face_t      *face,
			      codepoint_t  glyph);

HB_EXTERN void
ot_layout_get_glyphs_in_class (face_t                  *face,
				  ot_layout_glyph_class_t  klass,
				  set_t                   *glyphs /* OUT */);

/* Not that useful.  Provides list of attach points for a glyph that a
 * client may want to cache */
HB_EXTERN unsigned int
ot_layout_get_attach_points (face_t      *face,
				codepoint_t  glyph,
				unsigned int    start_offset,
				unsigned int   *point_count /* IN/OUT */,
				unsigned int   *point_array /* OUT */);

/* Ligature caret positions */
HB_EXTERN unsigned int
ot_layout_get_ligature_carets (font_t      *font,
				  direction_t  direction,
				  codepoint_t  glyph,
				  unsigned int    start_offset,
				  unsigned int   *caret_count /* IN/OUT */,
				  position_t  *caret_array /* OUT */);


/*
 * GSUB/GPOS feature query and enumeration interface
 */

/**
 * HB_OT_LAYOUT_NO_SCRIPT_INDEX:
 *
 * Special value for script index indicating unsupported script.
 */
#define HB_OT_LAYOUT_NO_SCRIPT_INDEX		0xFFFFu
/**
 * HB_OT_LAYOUT_NO_FEATURE_INDEX:
 *
 * Special value for feature index indicating unsupported feature.
 */
#define HB_OT_LAYOUT_NO_FEATURE_INDEX		0xFFFFu
/**
 * HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX:
 *
 * Special value for language index indicating default or unsupported language.
 */
#define HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX	0xFFFFu
/**
 * HB_OT_LAYOUT_NO_VARIATIONS_INDEX:
 *
 * Special value for variations index indicating unsupported variation.
 */
#define HB_OT_LAYOUT_NO_VARIATIONS_INDEX	0xFFFFFFFFu

HB_EXTERN unsigned int
ot_layout_table_get_script_tags (face_t    *face,
				    tag_t      table_tag,
				    unsigned int  start_offset,
				    unsigned int *script_count /* IN/OUT */,
				    tag_t     *script_tags /* OUT */);

HB_EXTERN bool_t
ot_layout_table_find_script (face_t    *face,
				tag_t      table_tag,
				tag_t      script_tag,
				unsigned int *script_index /* OUT */);

HB_EXTERN bool_t
ot_layout_table_select_script (face_t      *face,
				  tag_t        table_tag,
				  unsigned int    script_count,
				  const tag_t *script_tags,
				  unsigned int   *script_index /* OUT */,
				  tag_t       *chosen_script /* OUT */);

HB_EXTERN unsigned int
ot_layout_table_get_feature_tags (face_t    *face,
				     tag_t      table_tag,
				     unsigned int  start_offset,
				     unsigned int *feature_count /* IN/OUT */,
				     tag_t     *feature_tags /* OUT */);

HB_EXTERN unsigned int
ot_layout_script_get_language_tags (face_t    *face,
				       tag_t      table_tag,
				       unsigned int  script_index,
				       unsigned int  start_offset,
				       unsigned int *language_count /* IN/OUT */,
				       tag_t     *language_tags /* OUT */);

HB_EXTERN bool_t
ot_layout_script_select_language (face_t      *face,
				     tag_t        table_tag,
				     unsigned int    script_index,
				     unsigned int    language_count,
				     const tag_t *language_tags,
				     unsigned int   *language_index /* OUT */);

HB_EXTERN bool_t
ot_layout_script_select_language2 (face_t      *face,
				     tag_t        table_tag,
				     unsigned int    script_index,
				     unsigned int    language_count,
				     const tag_t *language_tags,
				     unsigned int   *language_index /* OUT */,
				     tag_t       *chosen_language /* OUT */);

HB_EXTERN bool_t
ot_layout_language_get_required_feature_index (face_t    *face,
						  tag_t      table_tag,
						  unsigned int  script_index,
						  unsigned int  language_index,
						  unsigned int *feature_index /* OUT */);

HB_EXTERN bool_t
ot_layout_language_get_required_feature (face_t    *face,
					    tag_t      table_tag,
					    unsigned int  script_index,
					    unsigned int  language_index,
					    unsigned int *feature_index /* OUT */,
					    tag_t     *feature_tag /* OUT */);

HB_EXTERN unsigned int
ot_layout_language_get_feature_indexes (face_t    *face,
					   tag_t      table_tag,
					   unsigned int  script_index,
					   unsigned int  language_index,
					   unsigned int  start_offset,
					   unsigned int *feature_count /* IN/OUT */,
					   unsigned int *feature_indexes /* OUT */);

HB_EXTERN unsigned int
ot_layout_language_get_feature_tags (face_t    *face,
					tag_t      table_tag,
					unsigned int  script_index,
					unsigned int  language_index,
					unsigned int  start_offset,
					unsigned int *feature_count /* IN/OUT */,
					tag_t     *feature_tags /* OUT */);

HB_EXTERN bool_t
ot_layout_language_find_feature (face_t    *face,
				    tag_t      table_tag,
				    unsigned int  script_index,
				    unsigned int  language_index,
				    tag_t      feature_tag,
				    unsigned int *feature_index /* OUT */);

HB_EXTERN unsigned int
ot_layout_feature_get_lookups (face_t    *face,
				  tag_t      table_tag,
				  unsigned int  feature_index,
				  unsigned int  start_offset,
				  unsigned int *lookup_count /* IN/OUT */,
				  unsigned int *lookup_indexes /* OUT */);

HB_EXTERN unsigned int
ot_layout_table_get_lookup_count (face_t    *face,
				     tag_t      table_tag);

HB_EXTERN void
ot_layout_collect_features (face_t      *face,
			       tag_t        table_tag,
			       const tag_t *scripts,
			       const tag_t *languages,
			       const tag_t *features,
			       set_t       *feature_indexes /* OUT */);

HB_EXTERN void
ot_layout_collect_features_map (face_t      *face,
				   tag_t        table_tag,
				   unsigned        script_index,
				   unsigned        language_index,
				   map_t       *feature_map /* OUT */);

HB_EXTERN void
ot_layout_collect_lookups (face_t      *face,
			      tag_t        table_tag,
			      const tag_t *scripts,
			      const tag_t *languages,
			      const tag_t *features,
			      set_t       *lookup_indexes /* OUT */);

HB_EXTERN void
ot_layout_lookup_collect_glyphs (face_t    *face,
				    tag_t      table_tag,
				    unsigned int  lookup_index,
				    set_t     *glyphs_before, /* OUT.  May be NULL */
				    set_t     *glyphs_input,  /* OUT.  May be NULL */
				    set_t     *glyphs_after,  /* OUT.  May be NULL */
				    set_t     *glyphs_output  /* OUT.  May be NULL */);


/* Variations support */

HB_EXTERN bool_t
ot_layout_table_find_feature_variations (face_t    *face,
					    tag_t      table_tag,
					    const int    *coords,
					    unsigned int  num_coords,
					    unsigned int *variations_index /* out */);

HB_EXTERN unsigned int
ot_layout_feature_with_variations_get_lookups (face_t    *face,
						  tag_t      table_tag,
						  unsigned int  feature_index,
						  unsigned int  variations_index,
						  unsigned int  start_offset,
						  unsigned int *lookup_count /* IN/OUT */,
						  unsigned int *lookup_indexes /* OUT */);


/*
 * GSUB
 */

HB_EXTERN bool_t
ot_layout_has_substitution (face_t *face);

HB_EXTERN unsigned
ot_layout_lookup_get_glyph_alternates (face_t      *face,
					  unsigned        lookup_index,
					  codepoint_t  glyph,
					  unsigned        start_offset,
					  unsigned       *alternate_count /* IN/OUT */,
					  codepoint_t *alternate_glyphs /* OUT */);

HB_EXTERN bool_t
ot_layout_lookup_would_substitute (face_t            *face,
				      unsigned int          lookup_index,
				      const codepoint_t *glyphs,
				      unsigned int          glyphs_length,
				      bool_t             zero_context);

HB_EXTERN void
ot_layout_lookup_substitute_closure (face_t    *face,
					unsigned int  lookup_index,
					set_t     *glyphs
					/*TODO , bool_t  inclusive */);

HB_EXTERN void
ot_layout_lookups_substitute_closure (face_t      *face,
					 const set_t *lookups,
					 set_t       *glyphs);


/*
 * GPOS
 */

HB_EXTERN bool_t
ot_layout_has_positioning (face_t *face);

/* Optical 'size' feature info.  Returns true if found.
 * https://docs.microsoft.com/en-us/typography/opentype/spec/features_pt#size */
HB_EXTERN bool_t
ot_layout_get_size_params (face_t       *face,
			      unsigned int    *design_size,       /* OUT.  May be NULL */
			      unsigned int    *subfamily_id,      /* OUT.  May be NULL */
			      ot_name_id_t *subfamily_name_id, /* OUT.  May be NULL */
			      unsigned int    *range_start,       /* OUT.  May be NULL */
			      unsigned int    *range_end          /* OUT.  May be NULL */);

HB_EXTERN position_t
ot_layout_lookup_get_optical_bound (font_t      *font,
				       unsigned        lookup_index,
				       direction_t  direction,
				       codepoint_t  glyph);


/*
 * GSUB/GPOS
 */

HB_EXTERN bool_t
ot_layout_feature_get_name_ids (face_t       *face,
				   tag_t         table_tag,
				   unsigned int     feature_index,
				   ot_name_id_t *label_id             /* OUT.  May be NULL */,
				   ot_name_id_t *tooltip_id           /* OUT.  May be NULL */,
				   ot_name_id_t *sample_id            /* OUT.  May be NULL */,
				   unsigned int    *num_named_parameters /* OUT.  May be NULL */,
				   ot_name_id_t *first_param_id       /* OUT.  May be NULL */);


HB_EXTERN unsigned int
ot_layout_feature_get_characters (face_t      *face,
				     tag_t        table_tag,
				     unsigned int    feature_index,
				     unsigned int    start_offset,
				     unsigned int   *char_count    /* IN/OUT.  May be NULL */,
				     codepoint_t *characters    /* OUT.     May be NULL */);


/*
 * BASE
 */

HB_EXTERN bool_t
ot_layout_get_font_extents (font_t         *font,
			       direction_t     direction,
			       tag_t           script_tag,
			       tag_t           language_tag,
			       font_extents_t *extents);

HB_EXTERN bool_t
ot_layout_get_font_extents2 (font_t         *font,
				direction_t     direction,
				script_t        script,
				language_t      language,
				font_extents_t *extents);

/**
 * ot_layout_baseline_tag_t:
 * @HB_OT_LAYOUT_BASELINE_TAG_ROMAN: The baseline used by alphabetic scripts such as Latin, Cyrillic and Greek.
 * In vertical writing mode, the alphabetic baseline for characters rotated 90 degrees clockwise.
 * (This would not apply to alphabetic characters that remain upright in vertical writing mode, since these
 * characters are not rotated.)
 * @HB_OT_LAYOUT_BASELINE_TAG_HANGING: The hanging baseline. In horizontal direction, this is the horizontal
 * line from which syllables seem, to hang in Tibetan and other similar scripts. In vertical writing mode,
 * for Tibetan (or some other similar script) characters rotated 90 degrees clockwise.
 * @HB_OT_LAYOUT_BASELINE_TAG_IDEO_FACE_BOTTOM_OR_LEFT: Ideographic character face bottom or left edge,
 * if the direction is horizontal or vertical, respectively.
 * @HB_OT_LAYOUT_BASELINE_TAG_IDEO_FACE_TOP_OR_RIGHT: Ideographic character face top or right edge,
 * if the direction is horizontal or vertical, respectively.
 * @HB_OT_LAYOUT_BASELINE_TAG_IDEO_FACE_CENTRAL: The center of the ideographic character face. Since: 4.0.0
 * @HB_OT_LAYOUT_BASELINE_TAG_IDEO_EMBOX_BOTTOM_OR_LEFT: Ideographic em-box bottom or left edge,
 * if the direction is horizontal or vertical, respectively.
 * @HB_OT_LAYOUT_BASELINE_TAG_IDEO_EMBOX_TOP_OR_RIGHT: Ideographic em-box top or right edge baseline,
 * @HB_OT_LAYOUT_BASELINE_TAG_IDEO_EMBOX_CENTRAL: The center of the ideographic em-box. Since: 4.0.0
 * if the direction is horizontal or vertical, respectively.
 * @HB_OT_LAYOUT_BASELINE_TAG_MATH: The baseline about which mathematical characters are centered.
 * In vertical writing mode when mathematical characters rotated 90 degrees clockwise, are centered.
 *
 * Baseline tags from [Baseline Tags](https://docs.microsoft.com/en-us/typography/opentype/spec/baselinetags) registry.
 *
 * Since: 2.6.0
 */
typedef enum {
  HB_OT_LAYOUT_BASELINE_TAG_ROMAN			= HB_TAG ('r','o','m','n'),
  HB_OT_LAYOUT_BASELINE_TAG_HANGING			= HB_TAG ('h','a','n','g'),
  HB_OT_LAYOUT_BASELINE_TAG_IDEO_FACE_BOTTOM_OR_LEFT	= HB_TAG ('i','c','f','b'),
  HB_OT_LAYOUT_BASELINE_TAG_IDEO_FACE_TOP_OR_RIGHT	= HB_TAG ('i','c','f','t'),
  HB_OT_LAYOUT_BASELINE_TAG_IDEO_FACE_CENTRAL		= HB_TAG ('I','c','f','c'),
  HB_OT_LAYOUT_BASELINE_TAG_IDEO_EMBOX_BOTTOM_OR_LEFT	= HB_TAG ('i','d','e','o'),
  HB_OT_LAYOUT_BASELINE_TAG_IDEO_EMBOX_TOP_OR_RIGHT	= HB_TAG ('i','d','t','p'),
  HB_OT_LAYOUT_BASELINE_TAG_IDEO_EMBOX_CENTRAL		= HB_TAG ('I','d','c','e'),
  HB_OT_LAYOUT_BASELINE_TAG_MATH			= HB_TAG ('m','a','t','h'),

  /*< private >*/
  _HB_OT_LAYOUT_BASELINE_TAG_MAX_VALUE = HB_TAG_MAX_SIGNED /*< skip >*/
} ot_layout_baseline_tag_t;

HB_EXTERN ot_layout_baseline_tag_t
ot_layout_get_horizontal_baseline_tag_for_script (script_t script);

HB_EXTERN bool_t
ot_layout_get_baseline (font_t                   *font,
			   ot_layout_baseline_tag_t  baseline_tag,
			   direction_t               direction,
			   tag_t                     script_tag,
			   tag_t                     language_tag,
			   position_t               *coord        /* OUT.  May be NULL. */);

HB_EXTERN bool_t
ot_layout_get_baseline2 (font_t                   *font,
			    ot_layout_baseline_tag_t  baseline_tag,
			    direction_t               direction,
			    script_t                  script,
			    language_t                language,
			    position_t               *coord        /* OUT.  May be NULL. */);

HB_EXTERN void
ot_layout_get_baseline_with_fallback (font_t                   *font,
					 ot_layout_baseline_tag_t  baseline_tag,
					 direction_t               direction,
					 tag_t                     script_tag,
					 tag_t                     language_tag,
					 position_t               *coord        /* OUT */);

HB_EXTERN void
ot_layout_get_baseline_with_fallback2 (font_t                   *font,
					  ot_layout_baseline_tag_t  baseline_tag,
					  direction_t               direction,
					  script_t                  script,
					  language_t                language,
					  position_t               *coord        /* OUT */);

HB_END_DECLS

#endif /* HB_OT_LAYOUT_H */
