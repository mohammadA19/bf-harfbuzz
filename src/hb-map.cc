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

#include "hb-map.hh"


/**
 * SECTION:hb-map
 * @title: hb-map
 * @short_description: Object representing integer to integer mapping
 * @include: hb.h
 *
 * Map objects are integer-to-integer hash-maps.  Currently they are
 * not used in the HarfBuzz public API, but are provided for client's
 * use if desired.
 **/


/**
 * map_create:
 *
 * Creates a new, initially empty map.
 *
 * Return value: (transfer full): The new #map_t
 *
 * Since: 1.7.7
 **/
map_t *
map_create ()
{
  map_t *map;

  if (!(map = object_create<map_t> ()))
    return map_get_empty ();

  return map;
}

/**
 * map_get_empty:
 *
 * Fetches the singleton empty #map_t.
 *
 * Return value: (transfer full): The empty #map_t
 *
 * Since: 1.7.7
 **/
map_t *
map_get_empty ()
{
  return const_cast<map_t *> (&Null (map_t));
}

/**
 * map_reference: (skip)
 * @map: A map
 *
 * Increases the reference count on a map.
 *
 * Return value: (transfer full): The map
 *
 * Since: 1.7.7
 **/
map_t *
map_reference (map_t *map)
{
  return object_reference (map);
}

/**
 * map_destroy: (skip)
 * @map: A map
 *
 * Decreases the reference count on a map. When
 * the reference count reaches zero, the map is
 * destroyed, freeing all memory.
 *
 * Since: 1.7.7
 **/
void
map_destroy (map_t *map)
{
  if (!object_destroy (map)) return;

  free (map);
}

/**
 * map_set_user_data: (skip)
 * @map: A map
 * @key: The user-data key to set
 * @data: A pointer to the user data to set
 * @destroy: (nullable): A callback to call when @data is not needed anymore
 * @replace: Whether to replace an existing data with the same key
 *
 * Attaches a user-data key/data pair to the specified map.
 *
 * Return value: `true` if success, `false` otherwise
 *
 * Since: 1.7.7
 **/
bool_t
map_set_user_data (map_t           *map,
		      user_data_key_t *key,
		      void *              data,
		      destroy_func_t   destroy,
		      bool_t           replace)
{
  return object_set_user_data (map, key, data, destroy, replace);
}

/**
 * map_get_user_data: (skip)
 * @map: A map
 * @key: The user-data key to query
 *
 * Fetches the user data associated with the specified key,
 * attached to the specified map.
 *
 * Return value: (transfer none): A pointer to the user data
 *
 * Since: 1.7.7
 **/
void *
map_get_user_data (const map_t     *map,
		      user_data_key_t *key)
{
  return object_get_user_data (map, key);
}


/**
 * map_allocation_successful:
 * @map: A map
 *
 * Tests whether memory allocation for a set was successful.
 *
 * Return value: `true` if allocation succeeded, `false` otherwise
 *
 * Since: 1.7.7
 **/
bool_t
map_allocation_successful (const map_t  *map)
{
  return map->successful;
}

/**
 * map_copy:
 * @map: A map
 *
 * Allocate a copy of @map.
 *
 * Return value: (transfer full): Newly-allocated map.
 *
 * Since: 4.4.0
 **/
map_t *
map_copy (const map_t *map)
{
  map_t *copy = map_create ();
  if (unlikely (copy->in_error ()))
    return map_get_empty ();

  *copy = *map;
  return copy;
}

/**
 * map_set:
 * @map: A map
 * @key: The key to store in the map
 * @value: The value to store for @key
 *
 * Stores @key:@value in the map.
 *
 * Since: 1.7.7
 **/
void
map_set (map_t       *map,
	    codepoint_t  key,
	    codepoint_t  value)
{
  /* Immutable-safe. */
  map->set (key, value);
}

/**
 * map_get:
 * @map: A map
 * @key: The key to query
 *
 * Fetches the value stored for @key in @map.
 *
 * Since: 1.7.7
 **/
codepoint_t
map_get (const map_t *map,
	    codepoint_t  key)
{
  return map->get (key);
}

/**
 * map_del:
 * @map: A map
 * @key: The key to delete
 *
 * Removes @key and its stored value from @map.
 *
 * Since: 1.7.7
 **/
void
map_del (map_t       *map,
	    codepoint_t  key)
{
  /* Immutable-safe. */
  map->del (key);
}

/**
 * map_has:
 * @map: A map
 * @key: The key to query
 *
 * Tests whether @key is an element of @map.
 *
 * Return value: `true` if @key is found in @map, `false` otherwise
 *
 * Since: 1.7.7
 **/
bool_t
map_has (const map_t *map,
	    codepoint_t  key)
{
  return map->has (key);
}


/**
 * map_clear:
 * @map: A map
 *
 * Clears out the contents of @map.
 *
 * Since: 1.7.7
 **/
void
map_clear (map_t *map)
{
  return map->clear ();
}

/**
 * map_is_empty:
 * @map: A map
 *
 * Tests whether @map is empty (contains no elements).
 *
 * Return value: `true` if @map is empty
 *
 * Since: 1.7.7
 **/
bool_t
map_is_empty (const map_t *map)
{
  return map->is_empty ();
}

/**
 * map_get_population:
 * @map: A map
 *
 * Returns the number of key-value pairs in the map.
 *
 * Return value: The population of @map
 *
 * Since: 1.7.7
 **/
unsigned int
map_get_population (const map_t *map)
{
  return map->get_population ();
}

/**
 * map_is_equal:
 * @map: A map
 * @other: Another map
 *
 * Tests whether @map and @other are equal (contain the same
 * elements).
 *
 * Return value: `true` if the two maps are equal, `false` otherwise.
 *
 * Since: 4.3.0
 **/
bool_t
map_is_equal (const map_t *map,
		 const map_t *other)
{
  return map->is_equal (*other);
}

/**
 * map_hash:
 * @map: A map
 *
 * Creates a hash representing @map.
 *
 * Return value:
 * A hash of @map.
 *
 * Since: 4.4.0
 **/
unsigned int
map_hash (const map_t *map)
{
  return map->hash ();
}

/**
 * map_update:
 * @map: A map
 * @other: Another map
 *
 * Add the contents of @other to @map.
 *
 * Since: 7.0.0
 **/
HB_EXTERN void
map_update (map_t *map,
	       const map_t *other)
{
  map->update (*other);
}

/**
 * map_next:
 * @map: A map
 * @idx: (inout): Iterator internal state
 * @key: (out): Key retrieved
 * @value: (out): Value retrieved
 *
 * Fetches the next key/value pair in @map.
 *
 * Set @idx to -1 to get started.
 *
 * If the map is modified during iteration, the behavior is undefined.
 *
 * The order in which the key/values are returned is undefined.
 *
 * Return value: `true` if there was a next value, `false` otherwise
 *
 * Since: 7.0.0
 **/
bool_t
map_next (const map_t *map,
	     int *idx,
	     codepoint_t *key,
	     codepoint_t *value)
{
  return map->next (idx, key, value);
}

/**
 * map_keys:
 * @map: A map
 * @keys: A set
 *
 * Add the keys of @map to @keys.
 *
 * Since: 7.0.0
 **/
void
map_keys (const map_t *map,
	     set_t *keys)
{
  copy (map->keys() , *keys);
}

/**
 * map_values:
 * @map: A map
 * @values: A set
 *
 * Add the values of @map to @values.
 *
 * Since: 7.0.0
 **/
void
map_values (const map_t *map,
	       set_t *values)
{
  copy (map->values() , *values);
}
