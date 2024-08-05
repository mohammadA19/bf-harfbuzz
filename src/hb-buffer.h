/*
 * Copyright © 1998-2004  David Turner and Werner Lemberg
 * Copyright © 2004,2007,2009  Red Hat, Inc.
 * Copyright © 2011,2012  Google, Inc.
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
 * Red Hat Author(s): Owen Taylor, Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#if !defined(HB_H_IN) && !defined(HB_NO_SINGLE_HEADER_ERROR)
#error "Include <hb.h> instead."
#endif

#ifndef HB_BUFFER_H
#define HB_BUFFER_H

#include "hb-common.h"
#include "hb-unicode.h"
#include "hb-font.h"

HB_BEGIN_DECLS

/**
 * glyph_info_t:
 * @codepoint: either a Unicode code point (before shaping) or a glyph index
 *             (after shaping).
 * @cluster: the index of the character in the original text that corresponds
 *           to this #glyph_info_t, or whatever the client passes to
 *           buffer_add(). More than one #glyph_info_t can have the same
 *           @cluster value, if they resulted from the same character (e.g. one
 *           to many glyph substitution), and when more than one character gets
 *           merged in the same glyph (e.g. many to one glyph substitution) the
 *           #glyph_info_t will have the smallest cluster value of them.
 *           By default some characters are merged into the same cluster
 *           (e.g. combining marks have the same cluster as their bases)
 *           even if they are separate glyphs, buffer_set_cluster_level()
 *           allow selecting more fine-grained cluster handling.
 *
 * The #glyph_info_t is the structure that holds information about the
 * glyphs and their relation to input text.
 */
typedef struct glyph_info_t {
  codepoint_t codepoint;
  /*< private >*/
  mask_t      mask;
  /*< public >*/
  uint32_t       cluster;

  /*< private >*/
  var_int_t   var1;
  var_int_t   var2;
} glyph_info_t;

/**
 * glyph_flags_t:
 * @HB_GLYPH_FLAG_UNSAFE_TO_BREAK: Indicates that if input text is broken at the
 * 				   beginning of the cluster this glyph is part of,
 * 				   then both sides need to be re-shaped, as the
 * 				   result might be different.
 * 				   On the flip side, it means that when this
 * 				   flag is not present, then it is safe to break
 * 				   the glyph-run at the beginning of this
 * 				   cluster, and the two sides will represent the
 * 				   exact same result one would get if breaking
 * 				   input text at the beginning of this cluster
 * 				   and shaping the two sides separately.
 * 				   This can be used to optimize paragraph
 * 				   layout, by avoiding re-shaping of each line
 * 				   after line-breaking.
 * @HB_GLYPH_FLAG_UNSAFE_TO_CONCAT: Indicates that if input text is changed on one
 * 				   side of the beginning of the cluster this glyph
 * 				   is part of, then the shaping results for the
 * 				   other side might change.
 * 				   Note that the absence of this flag will NOT by
 * 				   itself mean that it IS safe to concat text.
 * 				   Only two pieces of text both of which clear of
 * 				   this flag can be concatenated safely.
 * 				   This can be used to optimize paragraph
 * 				   layout, by avoiding re-shaping of each line
 * 				   after line-breaking, by limiting the
 * 				   reshaping to a small piece around the
 * 				   breaking position only, even if the breaking
 * 				   position carries the
 * 				   #HB_GLYPH_FLAG_UNSAFE_TO_BREAK or when
 * 				   hyphenation or other text transformation
 * 				   happens at line-break position, in the following
 * 				   way:
 * 				   1. Iterate back from the line-break position
 * 				   until the first cluster start position that is
 * 				   NOT unsafe-to-concat, 2. shape the segment from
 * 				   there till the end of line, 3. check whether the
 * 				   resulting glyph-run also is clear of the
 * 				   unsafe-to-concat at its start-of-text position;
 * 				   if it is, just splice it into place and the line
 * 				   is shaped; If not, move on to a position further
 * 				   back that is clear of unsafe-to-concat and retry
 * 				   from there, and repeat.
 * 				   At the start of next line a similar algorithm can
 * 				   be implemented. That is: 1. Iterate forward from
 * 				   the line-break position until the first cluster
 * 				   start position that is NOT unsafe-to-concat, 2.
 * 				   shape the segment from beginning of the line to
 * 				   that position, 3. check whether the resulting
 * 				   glyph-run also is clear of the unsafe-to-concat
 * 				   at its end-of-text position; if it is, just splice
 * 				   it into place and the beginning is shaped; If not,
 * 				   move on to a position further forward that is clear
 * 				   of unsafe-to-concat and retry up to there, and repeat.
 * 				   A slight complication will arise in the
 * 				   implementation of the algorithm above,
 * 				   because while our buffer API has a way to
 * 				   return flags for position corresponding to
 * 				   start-of-text, there is currently no position
 * 				   corresponding to end-of-text.  This limitation
 * 				   can be alleviated by shaping more text than needed
 * 				   and looking for unsafe-to-concat flag within text
 * 				   clusters.
 * 				   The #HB_GLYPH_FLAG_UNSAFE_TO_BREAK flag will
 * 				   always imply this flag.
 *				   To use this flag, you must enable the buffer flag
 *				   @HB_BUFFER_FLAG_PRODUCE_UNSAFE_TO_CONCAT during
 *				   shaping, otherwise the buffer flag will not be
 *				   reliably produced.
 * 				   Since: 4.0.0
 * @HB_GLYPH_FLAG_SAFE_TO_INSERT_TATWEEL: In scripts that use elongation (Arabic,
				   Mongolian, Syriac, etc.), this flag signifies
				   that it is safe to insert a U+0640 TATWEEL
				   character before this cluster for elongation.
				   This flag does not determine the
				   script-specific elongation places, but only
				   when it is safe to do the elongation without
				   interrupting text shaping.
				   Since: 5.1.0
 * @HB_GLYPH_FLAG_DEFINED: All the currently defined flags.
 *
 * Flags for #glyph_info_t.
 *
 * Since: 1.5.0
 */
typedef enum { /*< flags >*/
  HB_GLYPH_FLAG_UNSAFE_TO_BREAK			= 0x00000001,
  HB_GLYPH_FLAG_UNSAFE_TO_CONCAT		= 0x00000002,
  HB_GLYPH_FLAG_SAFE_TO_INSERT_TATWEEL		= 0x00000004,

  HB_GLYPH_FLAG_DEFINED				= 0x00000007 /* OR of all defined flags */
} glyph_flags_t;

HB_EXTERN glyph_flags_t
glyph_info_get_glyph_flags (const glyph_info_t *info);

#define glyph_info_get_glyph_flags(info) \
	((glyph_flags_t) ((unsigned int) (info)->mask & HB_GLYPH_FLAG_DEFINED))


/**
 * glyph_position_t:
 * @x_advance: how much the line advances after drawing this glyph when setting
 *             text in horizontal direction.
 * @y_advance: how much the line advances after drawing this glyph when setting
 *             text in vertical direction.
 * @x_offset: how much the glyph moves on the X-axis before drawing it, this
 *            should not affect how much the line advances.
 * @y_offset: how much the glyph moves on the Y-axis before drawing it, this
 *            should not affect how much the line advances.
 *
 * The #glyph_position_t is the structure that holds the positions of the
 * glyph in both horizontal and vertical directions. All positions in
 * #glyph_position_t are relative to the current point.
 *
 */
typedef struct glyph_position_t {
  position_t  x_advance;
  position_t  y_advance;
  position_t  x_offset;
  position_t  y_offset;

  /*< private >*/
  var_int_t   var;
} glyph_position_t;

/**
 * segment_properties_t:
 * @direction: the #direction_t of the buffer, see buffer_set_direction().
 * @script: the #script_t of the buffer, see buffer_set_script().
 * @language: the #language_t of the buffer, see buffer_set_language().
 *
 * The structure that holds various text properties of an #buffer_t. Can be
 * set and retrieved using buffer_set_segment_properties() and
 * buffer_get_segment_properties(), respectively.
 */
typedef struct segment_properties_t {
  direction_t  direction;
  script_t     script;
  language_t   language;
  /*< private >*/
  void           *reserved1;
  void           *reserved2;
} segment_properties_t;

/**
 * HB_SEGMENT_PROPERTIES_DEFAULT:
 *
 * The default #segment_properties_t of of freshly created #buffer_t.
 */
#define HB_SEGMENT_PROPERTIES_DEFAULT {HB_DIRECTION_INVALID, \
				       HB_SCRIPT_INVALID, \
				       HB_LANGUAGE_INVALID, \
				       (void *) 0, \
				       (void *) 0}

HB_EXTERN bool_t
segment_properties_equal (const segment_properties_t *a,
			     const segment_properties_t *b);

HB_EXTERN unsigned int
segment_properties_hash (const segment_properties_t *p);

HB_EXTERN void
segment_properties_overlay (segment_properties_t *p,
			       const segment_properties_t *src);


/**
 * buffer_t:
 *
 * The main structure holding the input text and its properties before shaping,
 * and output glyphs and their information after shaping.
 */

typedef struct buffer_t buffer_t;

HB_EXTERN buffer_t *
buffer_create (void);

HB_EXTERN buffer_t *
buffer_create_similar (const buffer_t *src);

HB_EXTERN void
buffer_reset (buffer_t *buffer);


HB_EXTERN buffer_t *
buffer_get_empty (void);

HB_EXTERN buffer_t *
buffer_reference (buffer_t *buffer);

HB_EXTERN void
buffer_destroy (buffer_t *buffer);

HB_EXTERN bool_t
buffer_set_user_data (buffer_t        *buffer,
			 user_data_key_t *key,
			 void *              data,
			 destroy_func_t   destroy,
			 bool_t           replace);

HB_EXTERN void *
buffer_get_user_data (const buffer_t  *buffer,
			 user_data_key_t *key);


/**
 * buffer_content_type_t:
 * @HB_BUFFER_CONTENT_TYPE_INVALID: Initial value for new buffer.
 * @HB_BUFFER_CONTENT_TYPE_UNICODE: The buffer contains input characters (before shaping).
 * @HB_BUFFER_CONTENT_TYPE_GLYPHS: The buffer contains output glyphs (after shaping).
 *
 * The type of #buffer_t contents.
 */
typedef enum {
  HB_BUFFER_CONTENT_TYPE_INVALID = 0,
  HB_BUFFER_CONTENT_TYPE_UNICODE,
  HB_BUFFER_CONTENT_TYPE_GLYPHS
} buffer_content_type_t;

HB_EXTERN void
buffer_set_content_type (buffer_t              *buffer,
			    buffer_content_type_t  content_type);

HB_EXTERN buffer_content_type_t
buffer_get_content_type (const buffer_t *buffer);


HB_EXTERN void
buffer_set_unicode_funcs (buffer_t        *buffer,
			     unicode_funcs_t *unicode_funcs);

HB_EXTERN unicode_funcs_t *
buffer_get_unicode_funcs (const buffer_t  *buffer);

HB_EXTERN void
buffer_set_direction (buffer_t    *buffer,
			 direction_t  direction);

HB_EXTERN direction_t
buffer_get_direction (const buffer_t *buffer);

HB_EXTERN void
buffer_set_script (buffer_t *buffer,
		      script_t  script);

HB_EXTERN script_t
buffer_get_script (const buffer_t *buffer);

HB_EXTERN void
buffer_set_language (buffer_t   *buffer,
			language_t  language);


HB_EXTERN language_t
buffer_get_language (const buffer_t *buffer);

HB_EXTERN void
buffer_set_segment_properties (buffer_t *buffer,
				  const segment_properties_t *props);

HB_EXTERN void
buffer_get_segment_properties (const buffer_t *buffer,
				  segment_properties_t *props);

HB_EXTERN void
buffer_guess_segment_properties (buffer_t *buffer);


/**
 * buffer_flags_t:
 * @HB_BUFFER_FLAG_DEFAULT: the default buffer flag.
 * @HB_BUFFER_FLAG_BOT: flag indicating that special handling of the beginning
 *                      of text paragraph can be applied to this buffer. Should usually
 *                      be set, unless you are passing to the buffer only part
 *                      of the text without the full context.
 * @HB_BUFFER_FLAG_EOT: flag indicating that special handling of the end of text
 *                      paragraph can be applied to this buffer, similar to
 *                      @HB_BUFFER_FLAG_BOT.
 * @HB_BUFFER_FLAG_PRESERVE_DEFAULT_IGNORABLES:
 *                      flag indication that character with Default_Ignorable
 *                      Unicode property should use the corresponding glyph
 *                      from the font, instead of hiding them (done by
 *                      replacing them with the space glyph and zeroing the
 *                      advance width.)  This flag takes precedence over
 *                      @HB_BUFFER_FLAG_REMOVE_DEFAULT_IGNORABLES.
 * @HB_BUFFER_FLAG_REMOVE_DEFAULT_IGNORABLES:
 *                      flag indication that character with Default_Ignorable
 *                      Unicode property should be removed from glyph string
 *                      instead of hiding them (done by replacing them with the
 *                      space glyph and zeroing the advance width.)
 *                      @HB_BUFFER_FLAG_PRESERVE_DEFAULT_IGNORABLES takes
 *                      precedence over this flag. Since: 1.8.0
 * @HB_BUFFER_FLAG_DO_NOT_INSERT_DOTTED_CIRCLE:
 *                      flag indicating that a dotted circle should
 *                      not be inserted in the rendering of incorrect
 *                      character sequences (such at <0905 093E>). Since: 2.4.0
 * @HB_BUFFER_FLAG_VERIFY:
 *                      flag indicating that the shape() call and its variants
 *                      should perform various verification processes on the results
 *                      of the shaping operation on the buffer.  If the verification
 *                      fails, then either a buffer message is sent, if a message
 *                      handler is installed on the buffer, or a message is written
 *                      to standard error.  In either case, the shaping result might
 *                      be modified to show the failed output. Since: 3.4.0
 * @HB_BUFFER_FLAG_PRODUCE_UNSAFE_TO_CONCAT:
 *                      flag indicating that the @HB_GLYPH_FLAG_UNSAFE_TO_CONCAT
 *                      glyph-flag should be produced by the shaper. By default
 *                      it will not be produced since it incurs a cost. Since: 4.0.0
 * @HB_BUFFER_FLAG_PRODUCE_SAFE_TO_INSERT_TATWEEL:
 *                      flag indicating that the @HB_GLYPH_FLAG_SAFE_TO_INSERT_TATWEEL
 *                      glyph-flag should be produced by the shaper. By default
 *                      it will not be produced. Since: 5.1.0
 * @HB_BUFFER_FLAG_DEFINED: All currently defined flags: Since: 4.4.0
 *
 * Flags for #buffer_t.
 *
 * Since: 0.9.20
 */
typedef enum { /*< flags >*/
  HB_BUFFER_FLAG_DEFAULT			= 0x00000000u,
  HB_BUFFER_FLAG_BOT				= 0x00000001u, /* Beginning-of-text */
  HB_BUFFER_FLAG_EOT				= 0x00000002u, /* End-of-text */
  HB_BUFFER_FLAG_PRESERVE_DEFAULT_IGNORABLES	= 0x00000004u,
  HB_BUFFER_FLAG_REMOVE_DEFAULT_IGNORABLES	= 0x00000008u,
  HB_BUFFER_FLAG_DO_NOT_INSERT_DOTTED_CIRCLE	= 0x00000010u,
  HB_BUFFER_FLAG_VERIFY				= 0x00000020u,
  HB_BUFFER_FLAG_PRODUCE_UNSAFE_TO_CONCAT	= 0x00000040u,
  HB_BUFFER_FLAG_PRODUCE_SAFE_TO_INSERT_TATWEEL	= 0x00000080u,

  HB_BUFFER_FLAG_DEFINED			= 0x000000FFu
} buffer_flags_t;

HB_EXTERN void
buffer_set_flags (buffer_t       *buffer,
		     buffer_flags_t  flags);

HB_EXTERN buffer_flags_t
buffer_get_flags (const buffer_t *buffer);

/**
 * buffer_cluster_level_t:
 * @HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES: Return cluster values grouped by graphemes into
 *   monotone order.
 * @HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS: Return cluster values grouped into monotone order.
 * @HB_BUFFER_CLUSTER_LEVEL_CHARACTERS: Don't group cluster values.
 * @HB_BUFFER_CLUSTER_LEVEL_DEFAULT: Default cluster level,
 *   equal to @HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES.
 * 
 * Data type for holding HarfBuzz's clustering behavior options. The cluster level
 * dictates one aspect of how HarfBuzz will treat non-base characters 
 * during shaping.
 *
 * In @HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES, non-base
 * characters are merged into the cluster of the base character that precedes them.
 *
 * In @HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS, non-base characters are initially
 * assigned their own cluster values, which are not merged into preceding base
 * clusters. This allows HarfBuzz to perform additional operations like reorder
 * sequences of adjacent marks.
 *
 * @HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES is the default, because it maintains
 * backward compatibility with older versions of HarfBuzz. New client programs that
 * do not need to maintain such backward compatibility are recommended to use
 * @HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS instead of the default.
 *
 * Since: 0.9.42
 */
typedef enum {
  HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES	= 0,
  HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS	= 1,
  HB_BUFFER_CLUSTER_LEVEL_CHARACTERS		= 2,
  HB_BUFFER_CLUSTER_LEVEL_DEFAULT = HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES
} buffer_cluster_level_t;

HB_EXTERN void
buffer_set_cluster_level (buffer_t               *buffer,
			     buffer_cluster_level_t  cluster_level);

HB_EXTERN buffer_cluster_level_t
buffer_get_cluster_level (const buffer_t *buffer);

/**
 * HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT:
 *
 * The default code point for replacing invalid characters in a given encoding.
 * Set to U+FFFD REPLACEMENT CHARACTER.
 *
 * Since: 0.9.31
 */
#define HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT 0xFFFDu

HB_EXTERN void
buffer_set_replacement_codepoint (buffer_t    *buffer,
				     codepoint_t  replacement);

HB_EXTERN codepoint_t
buffer_get_replacement_codepoint (const buffer_t *buffer);

HB_EXTERN void
buffer_set_invisible_glyph (buffer_t    *buffer,
			       codepoint_t  invisible);

HB_EXTERN codepoint_t
buffer_get_invisible_glyph (const buffer_t *buffer);

HB_EXTERN void
buffer_set_not_found_glyph (buffer_t    *buffer,
			       codepoint_t  not_found);

HB_EXTERN codepoint_t
buffer_get_not_found_glyph (const buffer_t *buffer);

HB_EXTERN void
buffer_set_random_state (buffer_t    *buffer,
			    unsigned        state);

HB_EXTERN unsigned
buffer_get_random_state (const buffer_t *buffer);

/*
 * Content API.
 */

HB_EXTERN void
buffer_clear_contents (buffer_t *buffer);

HB_EXTERN bool_t
buffer_pre_allocate (buffer_t  *buffer,
			unsigned int  size);


HB_EXTERN bool_t
buffer_allocation_successful (buffer_t  *buffer);

HB_EXTERN void
buffer_reverse (buffer_t *buffer);

HB_EXTERN void
buffer_reverse_range (buffer_t *buffer,
			 unsigned int start, unsigned int end);

HB_EXTERN void
buffer_reverse_clusters (buffer_t *buffer);


/* Filling the buffer in */

HB_EXTERN void
buffer_add (buffer_t    *buffer,
	       codepoint_t  codepoint,
	       unsigned int    cluster);

HB_EXTERN void
buffer_add_utf8 (buffer_t  *buffer,
		    const char   *text,
		    int           text_length,
		    unsigned int  item_offset,
		    int           item_length);

HB_EXTERN void
buffer_add_utf16 (buffer_t    *buffer,
		     const uint16_t *text,
		     int             text_length,
		     unsigned int    item_offset,
		     int             item_length);

HB_EXTERN void
buffer_add_utf32 (buffer_t    *buffer,
		     const uint32_t *text,
		     int             text_length,
		     unsigned int    item_offset,
		     int             item_length);

HB_EXTERN void
buffer_add_latin1 (buffer_t   *buffer,
		      const uint8_t *text,
		      int            text_length,
		      unsigned int   item_offset,
		      int            item_length);

HB_EXTERN void
buffer_add_codepoints (buffer_t          *buffer,
			  const codepoint_t *text,
			  int                   text_length,
			  unsigned int          item_offset,
			  int                   item_length);

HB_EXTERN void
buffer_append (buffer_t *buffer,
		  const buffer_t *source,
		  unsigned int start,
		  unsigned int end);

HB_EXTERN bool_t
buffer_set_length (buffer_t  *buffer,
		      unsigned int  length);

HB_EXTERN unsigned int
buffer_get_length (const buffer_t *buffer);

/* Getting glyphs out of the buffer */

HB_EXTERN glyph_info_t *
buffer_get_glyph_infos (buffer_t  *buffer,
			   unsigned int *length);

HB_EXTERN glyph_position_t *
buffer_get_glyph_positions (buffer_t  *buffer,
			       unsigned int *length);

HB_EXTERN bool_t
buffer_has_positions (buffer_t  *buffer);


HB_EXTERN void
buffer_normalize_glyphs (buffer_t *buffer);


/*
 * Serialize
 */

/**
 * buffer_serialize_flags_t:
 * @HB_BUFFER_SERIALIZE_FLAG_DEFAULT: serialize glyph names, clusters and positions.
 * @HB_BUFFER_SERIALIZE_FLAG_NO_CLUSTERS: do not serialize glyph cluster.
 * @HB_BUFFER_SERIALIZE_FLAG_NO_POSITIONS: do not serialize glyph position information.
 * @HB_BUFFER_SERIALIZE_FLAG_NO_GLYPH_NAMES: do no serialize glyph name.
 * @HB_BUFFER_SERIALIZE_FLAG_GLYPH_EXTENTS: serialize glyph extents.
 * @HB_BUFFER_SERIALIZE_FLAG_GLYPH_FLAGS: serialize glyph flags. Since: 1.5.0
 * @HB_BUFFER_SERIALIZE_FLAG_NO_ADVANCES: do not serialize glyph advances,
 *  glyph offsets will reflect absolute glyph positions. Since: 1.8.0
 * @HB_BUFFER_SERIALIZE_FLAG_DEFINED: All currently defined flags. Since: 4.4.0
 *
 * Flags that control what glyph information are serialized in buffer_serialize_glyphs().
 *
 * Since: 0.9.20
 */
typedef enum { /*< flags >*/
  HB_BUFFER_SERIALIZE_FLAG_DEFAULT		= 0x00000000u,
  HB_BUFFER_SERIALIZE_FLAG_NO_CLUSTERS		= 0x00000001u,
  HB_BUFFER_SERIALIZE_FLAG_NO_POSITIONS		= 0x00000002u,
  HB_BUFFER_SERIALIZE_FLAG_NO_GLYPH_NAMES	= 0x00000004u,
  HB_BUFFER_SERIALIZE_FLAG_GLYPH_EXTENTS	= 0x00000008u,
  HB_BUFFER_SERIALIZE_FLAG_GLYPH_FLAGS		= 0x00000010u,
  HB_BUFFER_SERIALIZE_FLAG_NO_ADVANCES		= 0x00000020u,

  HB_BUFFER_SERIALIZE_FLAG_DEFINED		= 0x0000003Fu
} buffer_serialize_flags_t;

/**
 * buffer_serialize_format_t:
 * @HB_BUFFER_SERIALIZE_FORMAT_TEXT: a human-readable, plain text format.
 * @HB_BUFFER_SERIALIZE_FORMAT_JSON: a machine-readable JSON format.
 * @HB_BUFFER_SERIALIZE_FORMAT_INVALID: invalid format.
 *
 * The buffer serialization and de-serialization format used in
 * buffer_serialize_glyphs() and buffer_deserialize_glyphs().
 *
 * Since: 0.9.2
 */
typedef enum {
  HB_BUFFER_SERIALIZE_FORMAT_TEXT	= HB_TAG('T','E','X','T'),
  HB_BUFFER_SERIALIZE_FORMAT_JSON	= HB_TAG('J','S','O','N'),
  HB_BUFFER_SERIALIZE_FORMAT_INVALID	= HB_TAG_NONE
} buffer_serialize_format_t;

HB_EXTERN buffer_serialize_format_t
buffer_serialize_format_from_string (const char *str, int len);

HB_EXTERN const char *
buffer_serialize_format_to_string (buffer_serialize_format_t format);

HB_EXTERN const char **
buffer_serialize_list_formats (void);

HB_EXTERN unsigned int
buffer_serialize_glyphs (buffer_t *buffer,
			    unsigned int start,
			    unsigned int end,
			    char *buf,
			    unsigned int buf_size,
			    unsigned int *buf_consumed,
			    font_t *font,
			    buffer_serialize_format_t format,
			    buffer_serialize_flags_t flags);

HB_EXTERN unsigned int
buffer_serialize_unicode (buffer_t *buffer,
			     unsigned int start,
			     unsigned int end,
			     char *buf,
			     unsigned int buf_size,
			     unsigned int *buf_consumed,
			     buffer_serialize_format_t format,
			     buffer_serialize_flags_t flags);

HB_EXTERN unsigned int
buffer_serialize (buffer_t *buffer,
		     unsigned int start,
		     unsigned int end,
		     char *buf,
		     unsigned int buf_size,
		     unsigned int *buf_consumed,
		     font_t *font,
		     buffer_serialize_format_t format,
		     buffer_serialize_flags_t flags);

HB_EXTERN bool_t
buffer_deserialize_glyphs (buffer_t *buffer,
			      const char *buf,
			      int buf_len,
			      const char **end_ptr,
			      font_t *font,
			      buffer_serialize_format_t format);

HB_EXTERN bool_t
buffer_deserialize_unicode (buffer_t *buffer,
			       const char *buf,
			       int buf_len,
			       const char **end_ptr,
			       buffer_serialize_format_t format);



/*
 * Compare buffers
 */

/**
 * buffer_diff_flags_t:
 * @HB_BUFFER_DIFF_FLAG_EQUAL: equal buffers.
 * @HB_BUFFER_DIFF_FLAG_CONTENT_TYPE_MISMATCH: buffers with different
 *     #buffer_content_type_t.
 * @HB_BUFFER_DIFF_FLAG_LENGTH_MISMATCH: buffers with differing length.
 * @HB_BUFFER_DIFF_FLAG_NOTDEF_PRESENT: `.notdef` glyph is present in the
 *     reference buffer.
 * @HB_BUFFER_DIFF_FLAG_DOTTED_CIRCLE_PRESENT: dotted circle glyph is present
 *     in the reference buffer.
 * @HB_BUFFER_DIFF_FLAG_CODEPOINT_MISMATCH: difference in #glyph_info_t.codepoint
 * @HB_BUFFER_DIFF_FLAG_CLUSTER_MISMATCH: difference in #glyph_info_t.cluster
 * @HB_BUFFER_DIFF_FLAG_GLYPH_FLAGS_MISMATCH: difference in #glyph_flags_t.
 * @HB_BUFFER_DIFF_FLAG_POSITION_MISMATCH: difference in #glyph_position_t.
 *
 * Flags from comparing two #buffer_t's.
 *
 * Buffer with different #buffer_content_type_t cannot be meaningfully
 * compared in any further detail.
 *
 * For buffers with differing length, the per-glyph comparison is not
 * attempted, though we do still scan reference buffer for dotted circle and
 * `.notdef` glyphs.
 *
 * If the buffers have the same length, we compare them glyph-by-glyph and
 * report which aspect(s) of the glyph info/position are different.
 *
 * Since: 1.5.0
 */
typedef enum { /*< flags >*/
  HB_BUFFER_DIFF_FLAG_EQUAL			= 0x0000,

  /* Buffers with different content_type cannot be meaningfully compared
   * in any further detail. */
  HB_BUFFER_DIFF_FLAG_CONTENT_TYPE_MISMATCH	= 0x0001,

  /* For buffers with differing length, the per-glyph comparison is not
   * attempted, though we do still scan reference for dottedcircle / .notdef
   * glyphs. */
  HB_BUFFER_DIFF_FLAG_LENGTH_MISMATCH		= 0x0002,

  /* We want to know if dottedcircle / .notdef glyphs are present in the
   * reference, as we may not care so much about other differences in this
   * case. */
  HB_BUFFER_DIFF_FLAG_NOTDEF_PRESENT		= 0x0004,
  HB_BUFFER_DIFF_FLAG_DOTTED_CIRCLE_PRESENT	= 0x0008,

  /* If the buffers have the same length, we compare them glyph-by-glyph
   * and report which aspect(s) of the glyph info/position are different. */
  HB_BUFFER_DIFF_FLAG_CODEPOINT_MISMATCH	= 0x0010,
  HB_BUFFER_DIFF_FLAG_CLUSTER_MISMATCH		= 0x0020,
  HB_BUFFER_DIFF_FLAG_GLYPH_FLAGS_MISMATCH	= 0x0040,
  HB_BUFFER_DIFF_FLAG_POSITION_MISMATCH		= 0x0080

} buffer_diff_flags_t;

/* Compare the contents of two buffers, report types of differences. */
HB_EXTERN buffer_diff_flags_t
buffer_diff (buffer_t *buffer,
		buffer_t *reference,
		codepoint_t dottedcircle_glyph,
		unsigned int position_fuzz);


/*
 * Tracing.
 */

/**
 * buffer_message_func_t:
 * @buffer: An #buffer_t to work upon
 * @font: The #font_t the @buffer is shaped with
 * @message: `NULL`-terminated message passed to the function
 * @user_data: User data pointer passed by the caller
 *
 * A callback method for #buffer_t. The method gets called with the
 * #buffer_t it was set on, the #font_t the buffer is shaped with and a
 * message describing what step of the shaping process will be performed.
 * Returning `false` from this method will skip this shaping step and move to
 * the next one.
 *
 * Return value: `true` to perform the shaping step, `false` to skip it.
 *
 * Since: 1.1.3
 */
typedef bool_t	(*buffer_message_func_t)	(buffer_t *buffer,
							 font_t   *font,
							 const char  *message,
							 void        *user_data);

HB_EXTERN void
buffer_set_message_func (buffer_t *buffer,
			    buffer_message_func_t func,
			    void *user_data, destroy_func_t destroy);


HB_END_DECLS

#endif /* HB_BUFFER_H */
