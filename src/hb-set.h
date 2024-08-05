/*
 * Copyright © 2012  Google, Inc.
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
 * Google Author(s): Behdad Esfahbod
 */

#if !defined(HB_H_IN) && !defined(HB_NO_SINGLE_HEADER_ERROR)
#error "Include <hb.h> instead."
#endif

#ifndef HB_SET_H
#define HB_SET_H

#include "hb-common.h"

HB_BEGIN_DECLS


/**
 * HB_SET_VALUE_INVALID:
 *
 * Unset #set_t value.
 *
 * Since: 0.9.21
 */
#define HB_SET_VALUE_INVALID HB_CODEPOINT_INVALID

/**
 * set_t:
 *
 * Data type for holding a set of integers. #set_t's are
 * used to gather and contain glyph IDs, Unicode code
 * points, and various other collections of discrete 
 * values.
 *
 **/
typedef struct set_t set_t;


HB_EXTERN set_t *
set_create (void);

HB_EXTERN set_t *
set_get_empty (void);

HB_EXTERN set_t *
set_reference (set_t *set);

HB_EXTERN void
set_destroy (set_t *set);

HB_EXTERN bool_t
set_set_user_data (set_t           *set,
		      user_data_key_t *key,
		      void *              data,
		      destroy_func_t   destroy,
		      bool_t           replace);

HB_EXTERN void *
set_get_user_data (const set_t     *set,
		      user_data_key_t *key);


/* Returns false if allocation has failed before */
HB_EXTERN bool_t
set_allocation_successful (const set_t *set);

HB_EXTERN set_t *
set_copy (const set_t *set);

HB_EXTERN void
set_clear (set_t *set);

HB_EXTERN bool_t
set_is_empty (const set_t *set);

HB_EXTERN void
set_invert (set_t *set);

HB_EXTERN bool_t
set_is_inverted (const set_t *set);

HB_EXTERN bool_t
set_has (const set_t *set,
	    codepoint_t  codepoint);

HB_EXTERN void
set_add (set_t       *set,
	    codepoint_t  codepoint);

HB_EXTERN void
set_add_range (set_t       *set,
		  codepoint_t  first,
		  codepoint_t  last);

HB_EXTERN void
set_add_sorted_array (set_t             *set,
		         const codepoint_t *sorted_codepoints,
		         unsigned int          num_codepoints);

HB_EXTERN void
set_del (set_t       *set,
	    codepoint_t  codepoint);

HB_EXTERN void
set_del_range (set_t       *set,
		  codepoint_t  first,
		  codepoint_t  last);

HB_EXTERN bool_t
set_is_equal (const set_t *set,
		 const set_t *other);

HB_EXTERN unsigned int
set_hash (const set_t *set);

HB_EXTERN bool_t
set_is_subset (const set_t *set,
		  const set_t *larger_set);

HB_EXTERN void
set_set (set_t       *set,
	    const set_t *other);

HB_EXTERN void
set_union (set_t       *set,
	      const set_t *other);

HB_EXTERN void
set_intersect (set_t       *set,
		  const set_t *other);

HB_EXTERN void
set_subtract (set_t       *set,
		 const set_t *other);

HB_EXTERN void
set_symmetric_difference (set_t       *set,
			     const set_t *other);

HB_EXTERN unsigned int
set_get_population (const set_t *set);

/* Returns HB_SET_VALUE_INVALID if set empty. */
HB_EXTERN codepoint_t
set_get_min (const set_t *set);

/* Returns HB_SET_VALUE_INVALID if set empty. */
HB_EXTERN codepoint_t
set_get_max (const set_t *set);

/* Pass HB_SET_VALUE_INVALID in to get started. */
HB_EXTERN bool_t
set_next (const set_t *set,
	     codepoint_t *codepoint);

/* Pass HB_SET_VALUE_INVALID in to get started. */
HB_EXTERN bool_t
set_previous (const set_t *set,
		 codepoint_t *codepoint);

/* Pass HB_SET_VALUE_INVALID for first and last to get started. */
HB_EXTERN bool_t
set_next_range (const set_t *set,
		   codepoint_t *first,
		   codepoint_t *last);

/* Pass HB_SET_VALUE_INVALID for first and last to get started. */
HB_EXTERN bool_t
set_previous_range (const set_t *set,
		       codepoint_t *first,
		       codepoint_t *last);

/* Pass HB_SET_VALUE_INVALID in to get started. */
HB_EXTERN unsigned int
set_next_many (const set_t *set,
		  codepoint_t  codepoint,
		  codepoint_t *out,
		  unsigned int    size);

HB_END_DECLS

#endif /* HB_SET_H */
