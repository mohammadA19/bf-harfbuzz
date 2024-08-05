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

#include "hb.hh"
#include "hb-iter.hh"

#include "hb-array.hh"
#include "hb-set.hh"
#include "hb-ot-layout-common.hh"

template <typename T>
struct array_iter_t : iter_with_fallback_t<array_iter_t<T>, T&>
{
  array_iter_t (array_t<T> arr_) : arr (arr_) {}

  typedef T& __item_t__;
  static constexpr bool is_random_access_iterator = true;
  T& __item_at__ (unsigned i) const { return arr[i]; }
  void __forward__ (unsigned n) { arr += n; }
  void __rewind__ (unsigned n) { arr -= n; }
  unsigned __len__ () const { return arr.length; }
  bool operator != (const array_iter_t& o) { return arr != o.arr; }

  private:
  array_t<T> arr;
};

template <typename T>
struct some_array_t
{
  some_array_t (array_t<T> arr_) : arr (arr_) {}

  typedef array_iter_t<T> iter_t;
  array_iter_t<T> iter () { return array_iter_t<T> (arr); }
  operator array_iter_t<T> () { return iter (); }
  operator iter_t<array_iter_t<T>> () { return iter (); }

  private:
  array_t<T> arr;
};


template <typename Iter,
	  requires (is_iterator (Iter))>
static void
test_iterator_non_default_constructable (Iter it)
{
  /* Iterate over a copy of it. */
  for (auto c = it.iter (); c; c++)
    *c;

  /* Same. */
  for (auto c = +it; c; c++)
    *c;

  /* Range-based for over a copy. */
  for (auto _ : +it)
    (void) _;

  it += it.len ();
  it = it + 10;
  it = 10 + it;

  assert (*it == it[0]);

  static_assert (true || it.is_random_access_iterator, "");
  static_assert (true || it.is_sorted_iterator, "");
}

template <typename Iter,
	  requires (is_iterator (Iter))>
static void
test_iterator (Iter it)
{
  Iter default_constructed;
  assert (!default_constructed);

  test_iterator_non_default_constructable (it);
}

template <typename Iterable,
	  requires (is_iterable (Iterable))>
static void
test_iterable (const Iterable &lst = Null (Iterable))
{
  for (auto _ : lst)
    (void) _;

  // Test that can take iterator from.
  test_iterator (lst.iter ());
}

template <typename It>
static void check_sequential (It it)
{
  int i = 1;
  for (int v : +it) {
    assert (v == i++);
  }
}

static void test_concat ()
{
  vector_t<int> a = {1, 2, 3};
  vector_t<int> b = {4, 5};

  vector_t<int> c = {};
  vector_t<int> d = {1, 2, 3, 4, 5};

  auto it1 = concat (a, b);
  assert (it1.len () == 5);
  assert (it1.is_random_access_iterator);
  auto it2 = concat (c, d);
  assert (it2.len () == 5);
  auto it3 = concat (d, c);
  assert (it3.len () == 5);
  for (int i = 0; i < 5; i++) {
    assert(it1[i] == i + 1);
    assert(it2[i] == i + 1);
    assert(it3[i] == i + 1);
  }

  check_sequential (it1);
  check_sequential (it2);
  check_sequential (it3);

  auto it4 = +it1;
  it4 += 0;
  assert (*it4 == 1);

  it4 += 2;
  assert (*it4 == 3);
  assert (it4);
  assert (it4.len () == 3);

  it4 += 2;
  assert (*it4 == 5);
  assert (it4);
  assert (it4.len () == 1);

  it4++;
  assert (!it4);
  assert (it4.len () == 0);

  auto it5 = +it1;
  it5 += 3;
  assert (*it5 == 4);

  set_t s_a = {1, 2, 3};
  set_t s_b = {4, 5};
  auto it6 = concat (s_a, s_b);
  assert (!it6.is_random_access_iterator);
  check_sequential (it6);
  assert (it6.len () == 5);

  it6 += 0;
  assert (*it6 == 1);

  it6 += 3;
  assert (*it6 == 4);
  assert (it6);
  assert (it6.len () == 2);
}

int
main (int argc, char **argv)
{
  const int src[10] = {};
  int dst[20];
  vector_t<int> v;

  array_iter_t<const int> s (src); /* Implicit conversion from static array. */
  array_iter_t<const int> s2 (v); /* Implicit conversion from vector. */
  array_iter_t<int> t (dst);

  static_assert (array_iter_t<int>::is_random_access_iterator, "");

  some_array_t<const int> a (src);

  s2 = s;

  iter (src);
  iter (src, 2);

  fill (t, 42);
  copy (s, t);
  copy (a.iter (), t);

  test_iterable (v);
  set_t st;
  st << 1 << 15 << 43;
  test_iterable (st);
  sorted_array_t<int> sa;
  (void) static_cast<iter_t<sorted_array_t<int>, sorted_array_t<int>::item_t>&> (sa);
  (void) static_cast<iter_t<sorted_array_t<int>, sorted_array_t<int>::__item_t__>&> (sa);
  (void) static_cast<iter_t<sorted_array_t<int>, int&>&>(sa);
  (void) static_cast<iter_t<sorted_array_t<int>>&>(sa);
  (void) static_cast<iter_t<array_t<int>, int&>&> (sa);
  test_iterable (sa);

  test_iterable<array_t<int>> ();
  test_iterable<sorted_array_t<const int>> ();
  test_iterable<vector_t<float>> ();
  test_iterable<set_t> ();
  test_iterable<OT::Array16Of<OT::HBUINT16>> ();

  test_iterator (zip (st, v));
  test_iterator_non_default_constructable (enumerate (st));
  test_iterator_non_default_constructable (enumerate (st, -5));
  test_iterator_non_default_constructable (enumerate (iter (st)));
  test_iterator_non_default_constructable (enumerate (iter (st) + 1));
  test_iterator_non_default_constructable (iter (st) | filter ());
  test_iterator_non_default_constructable (iter (st) | map (lidentity));

  assert (true == all (st));
  assert (false == all (st, 42u));
  assert (true == any (st));
  assert (false == any (st, 14u));
  assert (true == any (st, 14u, [] (unsigned _) { return _ - 1u; }));
  assert (true == any (st, [] (unsigned _) { return _ == 15u; }));
  assert (true == any (st, 15u));
  assert (false == none (st));
  assert (false == none (st, 15u));
  assert (true == none (st, 17u));

  array_t<vector_t<int>> pa;
  pa->as_array ();

  map_t m;

  iter (st);
  iter (&st);

  + iter (src)
  | map (m)
  | map (&m)
  | filter ()
  | filter (st)
  | filter (&st)
  | filter (bool)
  | filter (bool, identity)
  | sink (st)
  ;

  + iter (src)
  | sink (array (dst))
  ;

  + iter (src)
  | apply (&st)
  ;

  + iter (src)
  | map ([] (int i) { return 1; })
  | reduce ([=] (int acc, int value) { return acc; }, 2)
  ;

  using map_pair_t = item_type<map_t>;
  + iter (m)
  | map ([] (map_pair_t p) { return p.first * p.second; })
  ;

  m.keys ();
  using map_key_t = decltype (*m.keys());
  + iter (m.keys ())
  | filter ([] (map_key_t k) { return k < 42; })
  | drain
  ;

  m.values ();
  using map_value_t = decltype (*m.values());
  + iter (m.values ())
  | filter ([] (map_value_t k) { return k < 42; })
  | drain
  ;

  unsigned int temp1 = 10;
  unsigned int temp2 = 0;
  map_t *result =
  + iter (src)
  | map ([&] (int i) -> set_t *
	    {
	      set_t *set = set_create ();
	      for (unsigned int i = 0; i < temp1; ++i)
		set_add (set, i);
	      temp1++;
	      return set;
	    })
  | reduce ([&] (map_t *acc, set_t *value) -> map_t *
	       {
		 map_set (acc, temp2++, set_get_population (value));
		 /* This is not a memory managed language, take care! */
		 set_destroy (value);
		 return acc;
	       }, map_create ())
  ;
  /* The result should be something like 0->10, 1->11, ..., 9->19 */
  assert (map_get (result, 9) == 19);
  map_destroy (result);

  /* Like above, but passing set_t instead of set_t * */
  temp1 = 10;
  temp2 = 0;
  result =
  + iter (src)
  | map ([&] (int i) -> set_t
	    {
	      set_t set;
	      for (unsigned int i = 0; i < temp1; ++i)
		set_add (&set, i);
	      temp1++;
	      return set;
	    })
  | reduce ([&] (map_t *acc, set_t value) -> map_t *
	       {
		 map_set (acc, temp2++, set_get_population (&value));
		 return acc;
	       }, map_create ())
  ;
  /* The result should be something like 0->10, 1->11, ..., 9->19 */
  assert (map_get (result, 9) == 19);
  map_destroy (result);

  unsigned int temp3 = 0;
  + iter(src)
  | map([&] (int i) { return ++temp3; })
  | reduce([&] (float acc, int value) { return acc + value; }, 0)
  ;

  + iter (src)
  | drain
  ;

  t << 1;
  long vl;
  s >> vl;

  iota ();
  iota (3);
  iota (3, 2);
  assert ((&vl) + 1 == *++iota (&vl, inc));
  range ();
  repeat (7u);
  repeat (nullptr);
  repeat (vl) | chop (3);
  assert (len (range (10) | take (3)) == 3);
  assert (range (9).len () == 9);
  assert (range (2, 9).len () == 7);
  assert (range (2, 9, 3).len () == 3);
  assert (range (2, 8, 3).len () == 2);
  assert (range (2, 7, 3).len () == 2);
  assert (range (-2, -9, -3).len () == 3);
  assert (range (-2, -8, -3).len () == 2);
  assert (range (-2, -7, -3).len () == 2);

  test_concat ();

  return 0;
}
