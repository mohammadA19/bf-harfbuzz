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

#include "hb-set.hh"


/**
 * SECTION:hb-set
 * @title: hb-set
 * @short_description: Objects representing a set of integers
 * @include: hb.h
 *
 * Set objects represent a mathematical set of integer values.  They are
 * used in non-shaping APIs to query certain sets of characters or glyphs,
 * or other integer values.
 **/


/**
 * set_create:
 *
 * Creates a new, initially empty set.
 *
 * Return value: (transfer full): The new #set_t
 *
 * Since: 0.9.2
 **/
set_t *
set_create ()
{
  set_t *set;

  if (!(set = object_create<set_t> ()))
    return set_get_empty ();

  return set;
}

/**
 * set_get_empty:
 *
 * Fetches the singleton empty #set_t.
 *
 * Return value: (transfer full): The empty #set_t
 *
 * Since: 0.9.2
 **/
set_t *
set_get_empty ()
{
  return const_cast<set_t *> (&Null (set_t));
}

/**
 * set_reference: (skip)
 * @set: A set
 *
 * Increases the reference count on a set.
 *
 * Return value: (transfer full): The set
 *
 * Since: 0.9.2
 **/
set_t *
set_reference (set_t *set)
{
  return object_reference (set);
}

/**
 * set_destroy: (skip)
 * @set: A set
 *
 * Decreases the reference count on a set. When
 * the reference count reaches zero, the set is
 * destroyed, freeing all memory.
 *
 * Since: 0.9.2
 **/
void
set_destroy (set_t *set)
{
  if (!object_destroy (set)) return;

  free (set);
}

/**
 * set_set_user_data: (skip)
 * @set: A set
 * @key: The user-data key to set
 * @data: A pointer to the user data to set
 * @destroy: (nullable): A callback to call when @data is not needed anymore
 * @replace: Whether to replace an existing data with the same key
 *
 * Attaches a user-data key/data pair to the specified set.
 *
 * Return value: `true` if success, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
set_set_user_data (set_t           *set,
		      user_data_key_t *key,
		      void *              data,
		      destroy_func_t   destroy,
		      bool_t           replace)
{
  return object_set_user_data (set, key, data, destroy, replace);
}

/**
 * set_get_user_data: (skip)
 * @set: A set
 * @key: The user-data key to query
 *
 * Fetches the user data associated with the specified key,
 * attached to the specified set.
 *
 * Return value: (transfer none): A pointer to the user data
 *
 * Since: 0.9.2
 **/
void *
set_get_user_data (const set_t     *set,
		      user_data_key_t *key)
{
  return object_get_user_data (set, key);
}


/**
 * set_allocation_successful:
 * @set: A set
 *
 * Tests whether memory allocation for a set was successful.
 *
 * Return value: `true` if allocation succeeded, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
set_allocation_successful (const set_t  *set)
{
  return !set->in_error ();
}

/**
 * set_copy:
 * @set: A set
 *
 * Allocate a copy of @set.
 *
 * Return value: (transfer full): Newly-allocated set.
 *
 * Since: 2.8.2
 **/
set_t *
set_copy (const set_t *set)
{
  set_t *copy = set_create ();
  if (unlikely (copy->in_error ()))
    return set_get_empty ();

  copy->set (*set);
  return copy;
}

/**
 * set_clear:
 * @set: A set
 *
 * Clears out the contents of a set.
 *
 * Since: 0.9.2
 **/
void
set_clear (set_t *set)
{
  /* Immutable-safe. */
  set->clear ();
}

/**
 * set_is_empty:
 * @set: a set.
 *
 * Tests whether a set is empty (contains no elements).
 *
 * Return value: `true` if @set is empty
 *
 * Since: 0.9.7
 **/
bool_t
set_is_empty (const set_t *set)
{
  return set->is_empty ();
}

/**
 * set_has:
 * @set: A set
 * @codepoint: The element to query
 *
 * Tests whether @codepoint belongs to @set.
 *
 * Return value: `true` if @codepoint is in @set, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
set_has (const set_t *set,
	    codepoint_t  codepoint)
{
  return set->has (codepoint);
}

/**
 * set_add:
 * @set: A set
 * @codepoint: The element to add to @set
 *
 * Adds @codepoint to @set.
 *
 * Since: 0.9.2
 **/
void
set_add (set_t       *set,
	    codepoint_t  codepoint)
{
  /* Immutable-safe. */
  set->add (codepoint);
}

/**
 * set_add_sorted_array:
 * @set: A set
 * @sorted_codepoints: (array length=num_codepoints): Array of codepoints to add
 * @num_codepoints: Length of @sorted_codepoints
 *
 * Adds @num_codepoints codepoints to a set at once.
 * The codepoints array must be in increasing order,
 * with size at least @num_codepoints.
 *
 * Since: 4.1.0
 */
HB_EXTERN void
set_add_sorted_array (set_t             *set,
		         const codepoint_t *sorted_codepoints,
		         unsigned int          num_codepoints)
{
  /* Immutable-safe. */
  set->add_sorted_array (sorted_codepoints,
		         num_codepoints,
		         sizeof(codepoint_t));
}

/**
 * set_add_range:
 * @set: A set
 * @first: The first element to add to @set
 * @last: The final element to add to @set
 *
 * Adds all of the elements from @first to @last
 * (inclusive) to @set.
 *
 * Since: 0.9.7
 **/
void
set_add_range (set_t       *set,
		  codepoint_t  first,
		  codepoint_t  last)
{
  /* Immutable-safe. */
  set->add_range (first, last);
}

/**
 * set_del:
 * @set: A set
 * @codepoint: Removes @codepoint from @set
 *
 * Removes @codepoint from @set.
 *
 * Since: 0.9.2
 **/
void
set_del (set_t       *set,
	    codepoint_t  codepoint)
{
  /* Immutable-safe. */
  set->del (codepoint);
}

/**
 * set_del_range:
 * @set: A set
 * @first: The first element to remove from @set
 * @last: The final element to remove from @set
 *
 * Removes all of the elements from @first to @last
 * (inclusive) from @set.
 *
 * If @last is #HB_SET_VALUE_INVALID, then all values
 * greater than or equal to @first are removed.
 *
 * Since: 0.9.7
 **/
void
set_del_range (set_t       *set,
		  codepoint_t  first,
		  codepoint_t  last)
{
  /* Immutable-safe. */
  set->del_range (first, last);
}

/**
 * set_is_equal:
 * @set: A set
 * @other: Another set
 *
 * Tests whether @set and @other are equal (contain the same
 * elements).
 *
 * Return value: `true` if the two sets are equal, `false` otherwise.
 *
 * Since: 0.9.7
 **/
bool_t
set_is_equal (const set_t *set,
		 const set_t *other)
{
  return set->is_equal (*other);
}

/**
 * set_hash:
 * @set: A set
 *
 * Creates a hash representing @set.
 *
 * Return value:
 * A hash of @set.
 *
 * Since: 4.4.0
 **/
HB_EXTERN unsigned int
set_hash (const set_t *set)
{
  return set->hash ();
}

/**
 * set_is_subset:
 * @set: A set
 * @larger_set: Another set
 *
 * Tests whether @set is a subset of @larger_set.
 *
 * Return value: `true` if the @set is a subset of (or equal to) @larger_set, `false` otherwise.
 *
 * Since: 1.8.1
 **/
bool_t
set_is_subset (const set_t *set,
		  const set_t *larger_set)
{
  return set->is_subset (*larger_set);
}

/**
 * set_set:
 * @set: A set
 * @other: Another set
 *
 * Makes the contents of @set equal to the contents of @other.
 *
 * Since: 0.9.2
 **/
void
set_set (set_t       *set,
	    const set_t *other)
{
  /* Immutable-safe. */
  set->set (*other);
}

/**
 * set_union:
 * @set: A set
 * @other: Another set
 *
 * Makes @set the union of @set and @other.
 *
 * Since: 0.9.2
 **/
void
set_union (set_t       *set,
	      const set_t *other)
{
  /* Immutable-safe. */
  set->union_ (*other);
}

/**
 * set_intersect:
 * @set: A set
 * @other: Another set
 *
 * Makes @set the intersection of @set and @other.
 *
 * Since: 0.9.2
 **/
void
set_intersect (set_t       *set,
		  const set_t *other)
{
  /* Immutable-safe. */
  set->intersect (*other);
}

/**
 * set_subtract:
 * @set: A set
 * @other: Another set
 *
 * Subtracts the contents of @other from @set.
 *
 * Since: 0.9.2
 **/
void
set_subtract (set_t       *set,
		 const set_t *other)
{
  /* Immutable-safe. */
  set->subtract (*other);
}

/**
 * set_symmetric_difference:
 * @set: A set
 * @other: Another set
 *
 * Makes @set the symmetric difference of @set
 * and @other.
 *
 * Since: 0.9.2
 **/
void
set_symmetric_difference (set_t       *set,
			     const set_t *other)
{
  /* Immutable-safe. */
  set->symmetric_difference (*other);
}

/**
 * set_invert:
 * @set: A set
 *
 * Inverts the contents of @set.
 *
 * Since: 3.0.0
 **/
void
set_invert (set_t *set)
{
  /* Immutable-safe. */
  set->invert ();
}

/**
 * set_is_inverted:
 * @set: A set
 *
 * Returns whether the set is inverted.
 *
 * Return value: `true` if the set is inverted, `false` otherwise
 *
 * Since: 7.0.0
 **/
bool_t
set_is_inverted (const set_t *set)
{
  return set->is_inverted ();
}

/**
 * set_get_population:
 * @set: A set
 *
 * Returns the number of elements in the set.
 *
 * Return value: The population of @set
 *
 * Since: 0.9.7
 **/
unsigned int
set_get_population (const set_t *set)
{
  return set->get_population ();
}

/**
 * set_get_min:
 * @set: A set
 *
 * Finds the smallest element in the set.
 *
 * Return value: minimum of @set, or #HB_SET_VALUE_INVALID if @set is empty.
 *
 * Since: 0.9.7
 **/
codepoint_t
set_get_min (const set_t *set)
{
  return set->get_min ();
}

/**
 * set_get_max:
 * @set: A set
 *
 * Finds the largest element in the set.
 *
 * Return value: maximum of @set, or #HB_SET_VALUE_INVALID if @set is empty.
 *
 * Since: 0.9.7
 **/
codepoint_t
set_get_max (const set_t *set)
{
  return set->get_max ();
}

/**
 * set_next:
 * @set: A set
 * @codepoint: (inout): Input = Code point to query
 *             Output = Code point retrieved
 *
 * Fetches the next element in @set that is greater than current value of @codepoint.
 *
 * Set @codepoint to #HB_SET_VALUE_INVALID to get started.
 *
 * Return value: `true` if there was a next value, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
set_next (const set_t *set,
	     codepoint_t *codepoint)
{
  return set->next (codepoint);
}

/**
 * set_previous:
 * @set: A set
 * @codepoint: (inout): Input = Code point to query
 *             Output = Code point retrieved
 *
 * Fetches the previous element in @set that is lower than current value of @codepoint.
 *
 * Set @codepoint to #HB_SET_VALUE_INVALID to get started.
 *
 * Return value: `true` if there was a previous value, `false` otherwise
 *
 * Since: 1.8.0
 **/
bool_t
set_previous (const set_t *set,
		 codepoint_t *codepoint)
{
  return set->previous (codepoint);
}

/**
 * set_next_range:
 * @set: A set
 * @first: (out): The first code point in the range
 * @last: (inout): Input = The current last code point in the range
 *         Output = The last code point in the range
 *
 * Fetches the next consecutive range of elements in @set that
 * are greater than current value of @last.
 *
 * Set @last to #HB_SET_VALUE_INVALID to get started.
 *
 * Return value: `true` if there was a next range, `false` otherwise
 *
 * Since: 0.9.7
 **/
bool_t
set_next_range (const set_t *set,
		   codepoint_t *first,
		   codepoint_t *last)
{
  return set->next_range (first, last);
}

/**
 * set_previous_range:
 * @set: A set
 * @first: (inout): Input = The current first code point in the range
 *         Output = The first code point in the range
 * @last: (out): The last code point in the range
 *
 * Fetches the previous consecutive range of elements in @set that
 * are greater than current value of @last.
 *
 * Set @first to #HB_SET_VALUE_INVALID to get started.
 *
 * Return value: `true` if there was a previous range, `false` otherwise
 *
 * Since: 1.8.0
 **/
bool_t
set_previous_range (const set_t *set,
		       codepoint_t *first,
		       codepoint_t *last)
{
  return set->previous_range (first, last);
}

/**
 * set_next_many:
 * @set: A set
 * @codepoint: Outputting codepoints starting after this one.
 *             Use #HB_SET_VALUE_INVALID to get started.
 * @out: (array length=size): An array of codepoints to write to.
 * @size: The maximum number of codepoints to write out.
 *
 * Finds the next element in @set that is greater than @codepoint. Writes out
 * codepoints to @out, until either the set runs out of elements, or @size
 * codepoints are written, whichever comes first.
 *
 * Return value: the number of values written.
 *
 * Since: 4.2.0
 **/
unsigned int
set_next_many (const set_t *set,
		  codepoint_t  codepoint,
		  codepoint_t *out,
		  unsigned int    size)
{
  return set->next_many (codepoint, out, size);
}
