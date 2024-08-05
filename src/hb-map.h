/*
 * Copyright © 2018  Google, Inc.
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

#ifndef HB_MAP_H
#define HB_MAP_H

#include "hb-common.h"
#include "hb-set.h"

HB_BEGIN_DECLS


/**
 * HB_MAP_VALUE_INVALID:
 *
 * Unset #map_t value.
 *
 * Since: 1.7.7
 */
#define HB_MAP_VALUE_INVALID HB_CODEPOINT_INVALID

/**
 * map_t:
 *
 * Data type for holding integer-to-integer hash maps.
 *
 **/
typedef struct map_t map_t;


HB_EXTERN map_t *
map_create (void);

HB_EXTERN map_t *
map_get_empty (void);

HB_EXTERN map_t *
map_reference (map_t *map);

HB_EXTERN void
map_destroy (map_t *map);

HB_EXTERN bool_t
map_set_user_data (map_t           *map,
		      user_data_key_t *key,
		      void *              data,
		      destroy_func_t   destroy,
		      bool_t           replace);

HB_EXTERN void *
map_get_user_data (const map_t     *map,
		      user_data_key_t *key);


/* Returns false if allocation has failed before */
HB_EXTERN bool_t
map_allocation_successful (const map_t *map);

HB_EXTERN map_t *
map_copy (const map_t *map);

HB_EXTERN void
map_clear (map_t *map);

HB_EXTERN bool_t
map_is_empty (const map_t *map);

HB_EXTERN unsigned int
map_get_population (const map_t *map);

HB_EXTERN bool_t
map_is_equal (const map_t *map,
		 const map_t *other);

HB_EXTERN unsigned int
map_hash (const map_t *map);

HB_EXTERN void
map_set (map_t       *map,
	    codepoint_t  key,
	    codepoint_t  value);

HB_EXTERN codepoint_t
map_get (const map_t *map,
	    codepoint_t  key);

HB_EXTERN void
map_del (map_t       *map,
	    codepoint_t  key);

HB_EXTERN bool_t
map_has (const map_t *map,
	    codepoint_t  key);

HB_EXTERN void
map_update (map_t *map,
	       const map_t *other);

/* Pass -1 in for idx to get started. */
HB_EXTERN bool_t
map_next (const map_t *map,
	     int *idx,
	     codepoint_t *key,
	     codepoint_t *value);

HB_EXTERN void
map_keys (const map_t *map,
	     set_t *keys);

HB_EXTERN void
map_values (const map_t *map,
	       set_t *values);

HB_END_DECLS

#endif /* HB_MAP_H */
