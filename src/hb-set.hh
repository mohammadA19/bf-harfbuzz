/*
 * Copyright © 2012,2017  Google, Inc.
 * Copyright © 2021 Behdad Esfahbod
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

#ifndef HB_SET_HH
#define HB_SET_HH

#include "hb.hh"
#include "hb-bit-set-invertible.hh"


template <typename impl_t>
struct sparseset_t
{
  static constexpr bool realloc_move = true;

  object_header_t header;
  impl_t s;

  sparseset_t () { init (); }
  ~sparseset_t () { fini (); }

  sparseset_t (const sparseset_t& other) : sparseset_t () { set (other); }
  sparseset_t (sparseset_t&& other)  noexcept : sparseset_t () { s = std::move (other.s); }
  sparseset_t& operator = (const sparseset_t& other) { set (other); return *this; }
  sparseset_t& operator = (sparseset_t&& other)  noexcept { s = std::move (other.s); return *this; }
  friend void swap (sparseset_t& a, sparseset_t& b)  noexcept { swap (a.s, b.s); }

  sparseset_t (std::initializer_list<codepoint_t> lst) : sparseset_t ()
  {
    for (auto&& item : lst)
      add (item);
  }
  template <typename Iterable,
           requires (is_iterable (Iterable))>
  sparseset_t (const Iterable &o) : sparseset_t ()
  {
    copy (o, *this);
  }

  void init ()
  {
    object_init (this);
    s.init ();
  }
  void fini ()
  {
    object_fini (this);
    s.fini ();
  }

  explicit operator bool () const { return !is_empty (); }

  void err () { s.err (); }
  bool in_error () const { return s.in_error (); }

  void alloc (unsigned sz) { s.alloc (sz); }
  void reset () { s.reset (); }
  void clear () { s.clear (); }
  void invert () { s.invert (); }
  bool is_inverted () const { return s.is_inverted (); }
  bool is_empty () const { return s.is_empty (); }
  uint32_t hash () const { return s.hash (); }

  void add (codepoint_t g) { s.add (g); }
  bool add_range (codepoint_t first, codepoint_t last) { return s.add_range (first, last); }

  template <typename T>
  void add_array (const T *array, unsigned int count, unsigned int stride=sizeof(T))
  { s.add_array (array, count, stride); }
  template <typename T>
  void add_array (const array_t<const T>& arr) { add_array (&arr, arr.len ()); }

  /* Might return false if array looks unsorted.
   * Used for faster rejection of corrupt data. */
  template <typename T>
  bool add_sorted_array (const T *array, unsigned int count, unsigned int stride=sizeof(T))
  { return s.add_sorted_array (array, count, stride); }
  template <typename T>
  bool add_sorted_array (const sorted_array_t<const T>& arr) { return add_sorted_array (&arr, arr.len ()); }

  void del (codepoint_t g) { s.del (g); }
  void del_range (codepoint_t a, codepoint_t b) { s.del_range (a, b); }

  bool get (codepoint_t g) const { return s.get (g); }

  /* Has interface. */
  bool operator [] (codepoint_t k) const { return get (k); }
  bool has (codepoint_t k) const { return (*this)[k]; }

  /* Predicate. */
  bool operator () (codepoint_t k) const { return has (k); }

  /* Sink interface. */
  sparseset_t& operator << (codepoint_t v)
  { add (v); return *this; }
  sparseset_t& operator << (const codepoint_pair_t& range)
  { add_range (range.first, range.second); return *this; }

  bool intersects (codepoint_t first, codepoint_t last) const
  { return s.intersects (first, last); }

  void set (const sparseset_t &other) { s.set (other.s); }

  bool is_equal (const sparseset_t &other) const { return s.is_equal (other.s); }
  bool operator == (const set_t &other) const { return is_equal (other); }
  bool operator != (const set_t &other) const { return !is_equal (other); }

  bool is_subset (const sparseset_t &larger_set) const { return s.is_subset (larger_set.s); }

  void union_ (const sparseset_t &other) { s.union_ (other.s); }
  void intersect (const sparseset_t &other) { s.intersect (other.s); }
  void subtract (const sparseset_t &other) { s.subtract (other.s); }
  void symmetric_difference (const sparseset_t &other) { s.symmetric_difference (other.s); }

  bool next (codepoint_t *codepoint) const { return s.next (codepoint); }
  bool previous (codepoint_t *codepoint) const { return s.previous (codepoint); }
  bool next_range (codepoint_t *first, codepoint_t *last) const
  { return s.next_range (first, last); }
  bool previous_range (codepoint_t *first, codepoint_t *last) const
  { return s.previous_range (first, last); }
  unsigned int next_many (codepoint_t codepoint, codepoint_t *out, unsigned int size) const
  { return s.next_many (codepoint, out, size); }

  unsigned int get_population () const { return s.get_population (); }
  codepoint_t get_min () const { return s.get_min (); }
  codepoint_t get_max () const { return s.get_max (); }

  static constexpr codepoint_t INVALID = impl_t::INVALID;

  /*
   * Iterator implementation.
   */
  using iter_t = typename impl_t::iter_t;
  iter_t iter () const { return iter_t (this->s); }
  operator iter_t () const { return iter (); }
};

struct set_t : sparseset_t<bit_set_invertible_t>
{
  using sparseset = sparseset_t<bit_set_invertible_t>;

  ~set_t () = default;
  set_t () : sparseset () {};
  set_t (const set_t &o) : sparseset ((sparseset &) o) {};
  set_t (set_t&& o)  noexcept : sparseset (std::move ((sparseset &) o)) {}
  set_t& operator = (const set_t&) = default;
  set_t& operator = (set_t&&) = default;
  set_t (std::initializer_list<codepoint_t> lst) : sparseset (lst) {}
  template <typename Iterable,
	    requires (is_iterable (Iterable))>
  set_t (const Iterable &o) : sparseset (o) {}

  set_t& operator << (codepoint_t v)
  { sparseset::operator<< (v); return *this; }
  set_t& operator << (const codepoint_pair_t& range)
  { sparseset::operator<< (range); return *this; }
};

static_assert (set_t::INVALID == HB_SET_VALUE_INVALID, "");


#endif /* HB_SET_HH */
