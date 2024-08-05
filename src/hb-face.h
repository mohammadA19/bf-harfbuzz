/*
 * Copyright © 2009  Red Hat, Inc.
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

#if !defined(HB_H_IN) && !defined(HB_NO_SINGLE_HEADER_ERROR)
#error "Include <hb.h> instead."
#endif

#ifndef HB_FACE_H
#define HB_FACE_H

#include "hb-common.h"
#include "hb-blob.h"
#include "hb-map.h"
#include "hb-set.h"

HB_BEGIN_DECLS


HB_EXTERN unsigned int
face_count (blob_t *blob);


/*
 * face_t
 */

/**
 * face_t:
 *
 * Data type for holding font faces.
 *
 **/
typedef struct face_t face_t;

HB_EXTERN face_t *
face_create (blob_t    *blob,
		unsigned int  index);

/**
 * reference_table_func_t:
 * @face: an #face_t to reference table for
 * @tag: the tag of the table to reference
 * @user_data: User data pointer passed by the caller
 *
 * Callback function for face_create_for_tables().
 *
 * Return value: (transfer full): A pointer to the @tag table within @face
 *
 * Since: 0.9.2
 */

typedef blob_t * (*reference_table_func_t)  (face_t *face, tag_t tag, void *user_data);

/* calls destroy() when not needing user_data anymore */
HB_EXTERN face_t *
face_create_for_tables (reference_table_func_t  reference_table_func,
			   void                      *user_data,
			   destroy_func_t          destroy);

HB_EXTERN face_t *
face_get_empty (void);

HB_EXTERN face_t *
face_reference (face_t *face);

HB_EXTERN void
face_destroy (face_t *face);

HB_EXTERN bool_t
face_set_user_data (face_t          *face,
		       user_data_key_t *key,
		       void *              data,
		       destroy_func_t   destroy,
		       bool_t           replace);

HB_EXTERN void *
face_get_user_data (const face_t    *face,
		       user_data_key_t *key);

HB_EXTERN void
face_make_immutable (face_t *face);

HB_EXTERN bool_t
face_is_immutable (const face_t *face);


HB_EXTERN blob_t *
face_reference_table (const face_t *face,
			 tag_t tag);

HB_EXTERN blob_t *
face_reference_blob (face_t *face);

HB_EXTERN void
face_set_index (face_t    *face,
		   unsigned int  index);

HB_EXTERN unsigned int
face_get_index (const face_t *face);

HB_EXTERN void
face_set_upem (face_t    *face,
		  unsigned int  upem);

HB_EXTERN unsigned int
face_get_upem (const face_t *face);

HB_EXTERN void
face_set_glyph_count (face_t    *face,
			 unsigned int  glyph_count);

HB_EXTERN unsigned int
face_get_glyph_count (const face_t *face);

HB_EXTERN unsigned int
face_get_table_tags (const face_t *face,
			unsigned int  start_offset,
			unsigned int *table_count, /* IN/OUT */
			tag_t     *table_tags /* OUT */);


/*
 * Character set.
 */

HB_EXTERN void
face_collect_unicodes (face_t *face,
			  set_t  *out);

HB_EXTERN void
face_collect_nominal_glyph_mapping (face_t *face,
				       map_t  *mapping,
				       set_t  *unicodes);

HB_EXTERN void
face_collect_variation_selectors (face_t *face,
				     set_t  *out);

HB_EXTERN void
face_collect_variation_unicodes (face_t *face,
				    codepoint_t variation_selector,
				    set_t  *out);


/*
 * Builder face.
 */

HB_EXTERN face_t *
face_builder_create (void);

HB_EXTERN bool_t
face_builder_add_table (face_t *face,
			   tag_t   tag,
			   blob_t *blob);

HB_EXTERN void
face_builder_sort_tables (face_t *face,
                             const tag_t  *tags);


HB_END_DECLS

#endif /* HB_FACE_H */
