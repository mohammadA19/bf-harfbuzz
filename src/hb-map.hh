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

#ifndef HB_MAP_HH
#define HB_MAP_HH

#include "hb.hh"

#include "hb-set.hh"


/*
 * hashmap_t
 */

extern HB_INTERNAL const codepoint_t minus_1;

template <typename K, typename V,
	  bool minus_one = false>
struct hashmap_t
{
  static constexpr bool realloc_move = true;

  hashmap_t ()  { init (); }
  ~hashmap_t () { fini (); }

  hashmap_t (const hashmap_t& o) : hashmap_t ()
  {
    if (unlikely (!o.mask)) return;

    if (item_t::is_trivial)
    {
      items = (item_t *) malloc (sizeof (item_t) * (o.mask + 1));
      if (unlikely (!items))
      {
	successful = false;
	return;
      }
      population = o.population;
      occupancy = o.occupancy;
      mask = o.mask;
      prime = o.prime;
      max_chain_length = o.max_chain_length;
      memcpy (items, o.items, sizeof (item_t) * (mask + 1));
      return;
    }

    alloc (o.population); copy (o, *this);
  }
  hashmap_t (hashmap_t&& o)  noexcept : hashmap_t () { swap (*this, o); }
  hashmap_t& operator= (const hashmap_t& o)  { reset (); alloc (o.population); copy (o, *this); return *this; }
  hashmap_t& operator= (hashmap_t&& o)   noexcept { swap (*this, o); return *this; }

  hashmap_t (std::initializer_list<pair_t<K, V>> lst) : hashmap_t ()
  {
    for (auto&& item : lst)
      set (item.first, item.second);
  }
  template <typename Iterable,
	    requires (is_iterable (Iterable))>
  hashmap_t (const Iterable &o) : hashmap_t ()
  {
    auto iter = iter (o);
    if (iter.is_random_access_iterator || iter.has_fast_len)
      alloc (len (iter));
    copy (iter, *this);
  }

  struct item_t
  {
    K key;
    uint32_t is_real_ : 1;
    uint32_t is_used_ : 1;
    uint32_t hash : 30;
    V value;

    item_t () : key (),
		is_real_ (false), is_used_ (false),
		hash (0),
		value () {}

    // Needed for https://github.com/harfbuzz/harfbuzz/issues/4138
    K& get_key () { return key; }
    V& get_value () { return value; }

    bool is_used () const { return is_used_; }
    void set_used (bool is_used) { is_used_ = is_used; }
    void set_real (bool is_real) { is_real_ = is_real; }
    bool is_real () const { return is_real_; }

    template <bool v = minus_one,
	      enable_if (v == false)>
    static inline const V& default_value () { return Null(V); };
    template <bool v = minus_one,
	      enable_if (v == true)>
    static inline const V& default_value ()
    {
      static_assert (is_same (V, codepoint_t), "");
      return minus_1;
    };

    bool operator == (const K &o) const { return deref (key) == deref (o); }
    bool operator == (const item_t &o) const { return *this == o.key; }
    pair_t<K, V> get_pair() const { return pair_t<K, V> (key, value); }
    pair_t<const K &, V &> get_pair_ref() { return pair_t<const K &, V &> (key, value); }

    uint32_t total_hash () const
    { return (hash * 31u) + hash (value); }

    static constexpr bool is_trivial = is_trivially_constructible(K) &&
				       is_trivially_destructible(K) &&
				       is_trivially_constructible(V) &&
				       is_trivially_destructible(V);
  };

  object_header_t header;
  bool successful; /* Allocations successful */
  unsigned short max_chain_length;
  unsigned int population; /* Not including tombstones. */
  unsigned int occupancy; /* Including tombstones. */
  unsigned int mask;
  unsigned int prime;
  item_t *items;

  friend void swap (hashmap_t& a, hashmap_t& b) noexcept
  {
    if (unlikely (!a.successful || !b.successful))
      return;
    swap (a.max_chain_length, b.max_chain_length);
    swap (a.population, b.population);
    swap (a.occupancy, b.occupancy);
    swap (a.mask, b.mask);
    swap (a.prime, b.prime);
    swap (a.items, b.items);
  }
  void init ()
  {
    object_init (this);

    successful = true;
    max_chain_length = 0;
    population = occupancy = 0;
    mask = 0;
    prime = 0;
    items = nullptr;
  }
  void fini ()
  {
    object_fini (this);

    if (likely (items))
    {
      unsigned size = mask + 1;
      if (!item_t::is_trivial)
	for (unsigned i = 0; i < size; i++)
	  items[i].~item_t ();
      free (items);
      items = nullptr;
    }
    population = occupancy = 0;
  }

  void reset ()
  {
    successful = true;
    clear ();
  }

  bool in_error () const { return !successful; }

  bool alloc (unsigned new_population = 0)
  {
    if (unlikely (!successful)) return false;

    if (new_population != 0 && (new_population + new_population / 2) < mask) return true;

    unsigned int power = bit_storage (max ((unsigned) population, new_population) * 2 + 8);
    unsigned int new_size = 1u << power;
    item_t *new_items = (item_t *) malloc ((size_t) new_size * sizeof (item_t));
    if (unlikely (!new_items))
    {
      successful = false;
      return false;
    }
    if (!item_t::is_trivial)
      for (auto &_ : iter (new_items, new_size))
	new (&_) item_t ();
    else
      memset (new_items, 0, (size_t) new_size * sizeof (item_t));

    unsigned int old_size = size ();
    item_t *old_items = items;

    /* Switch to new, empty, array. */
    population = occupancy = 0;
    mask = new_size - 1;
    prime = prime_for (power);
    max_chain_length = power * 2;
    items = new_items;

    /* Insert back old items. */
    for (unsigned int i = 0; i < old_size; i++)
    {
      if (old_items[i].is_real ())
      {
	set_with_hash (std::move (old_items[i].key),
		       old_items[i].hash,
		       std::move (old_items[i].value));
      }
    }
    if (!item_t::is_trivial)
      for (unsigned int i = 0; i < old_size; i++)
	old_items[i].~item_t ();

    free (old_items);

    return true;
  }

  template <typename KK, typename VV>
  bool set_with_hash (KK&& key, uint32_t hash, VV&& value, bool overwrite = true)
  {
    if (unlikely (!successful)) return false;
    if (unlikely ((occupancy + occupancy / 2) >= mask && !alloc ())) return false;

    hash &= 0x3FFFFFFF; // We only store lower 30bit of hash
    unsigned int tombstone = (unsigned int) -1;
    unsigned int i = hash % prime;
    unsigned length = 0;
    unsigned step = 0;
    while (items[i].is_used ())
    {
      if ((std::is_integral<K>::value || items[i].hash == hash) &&
	  items[i] == key)
      {
        if (!overwrite)
	  return false;
        else
	  break;
      }
      if (!items[i].is_real () && tombstone == (unsigned) -1)
        tombstone = i;
      i = (i + ++step) & mask;
      length++;
    }

    item_t &item = items[tombstone == (unsigned) -1 ? i : tombstone];

    if (item.is_used ())
    {
      occupancy--;
      population -= item.is_real ();
    }

    item.key = std::forward<KK> (key);
    item.value = std::forward<VV> (value);
    item.hash = hash;
    item.set_used (true);
    item.set_real (true);

    occupancy++;
    population++;

    if (unlikely (length > max_chain_length) && occupancy * 8 > mask)
      alloc (mask - 8); // This ensures we jump to next larger size

    return true;
  }

  template <typename VV>
  bool set (const K &key, VV&& value, bool overwrite = true) { return set_with_hash (key, hash (key), std::forward<VV> (value), overwrite); }
  template <typename VV>
  bool set (K &&key, VV&& value, bool overwrite = true)
  {
    uint32_t hash = hash (key);
    return set_with_hash (std::move (key), hash, std::forward<VV> (value), overwrite);
  }
  bool add (const K &key)
  {
    uint32_t hash = hash (key);
    return set_with_hash (key, hash, item_t::default_value ());
  }

  const V& get_with_hash (const K &key, uint32_t hash) const
  {
    if (!items) return item_t::default_value ();
    auto *item = fetch_item (key, hash (key));
    if (item)
      return item->value;
    return item_t::default_value ();
  }
  const V& get (const K &key) const
  {
    if (!items) return item_t::default_value ();
    return get_with_hash (key, hash (key));
  }

  void del (const K &key)
  {
    if (!items) return;
    auto *item = fetch_item (key, hash (key));
    if (item)
    {
      item->set_real (false);
      population--;
    }
  }

  /* Has interface. */
  const V& operator [] (K k) const { return get (k); }
  template <typename VV=V>
  bool has (const K &key, VV **vp = nullptr) const
  {
    if (!items) return false;
    auto *item = fetch_item (key, hash (key));
    if (item)
    {
      if (vp) *vp = std::addressof (item->value);
      return true;
    }
    return false;
  }
  item_t *fetch_item (const K &key, uint32_t hash) const
  {
    hash &= 0x3FFFFFFF; // We only store lower 30bit of hash
    unsigned int i = hash % prime;
    unsigned step = 0;
    while (items[i].is_used ())
    {
      if ((std::is_integral<K>::value || items[i].hash == hash) &&
	  items[i] == key)
      {
	if (items[i].is_real ())
	  return &items[i];
	else
	  return nullptr;
      }
      i = (i + ++step) & mask;
    }
    return nullptr;
  }
  /* Projection. */
  const V& operator () (K k) const { return get (k); }

  unsigned size () const { return mask ? mask + 1 : 0; }

  void clear ()
  {
    if (unlikely (!successful)) return;

    for (auto &_ : iter (items, size ()))
    {
      /* Reconstruct items. */
      _.~item_t ();
      new (&_) item_t ();
    }

    population = occupancy = 0;
  }

  bool is_empty () const { return population == 0; }
  explicit operator bool () const { return !is_empty (); }

  uint32_t hash () const
  {
    return
    + iter_items ()
    | reduce ([] (uint32_t h, const item_t &_) { return h ^ _.total_hash (); }, (uint32_t) 0u)
    ;
  }

  bool is_equal (const hashmap_t &other) const
  {
    if (population != other.population) return false;

    for (auto pair : iter ())
      if (other.get (pair.first) != pair.second)
        return false;

    return true;
  }
  bool operator == (const hashmap_t &other) const { return is_equal (other); }
  bool operator != (const hashmap_t &other) const { return !is_equal (other); }

  unsigned int get_population () const { return population; }

  void update (const hashmap_t &other)
  {
    if (unlikely (!successful)) return;

    copy (other, *this);
  }

  /*
   * Iterator
   */

  auto iter_items () const HB_AUTO_RETURN
  (
    + iter (items, this->size ())
    | filter (&item_t::is_real)
  )
  auto iter_ref () const HB_AUTO_RETURN
  (
    + this->iter_items ()
    | map (&item_t::get_pair_ref)
  )
  auto iter () const HB_AUTO_RETURN
  (
    + this->iter_items ()
    | map (&item_t::get_pair)
  )
  auto keys_ref () const HB_AUTO_RETURN
  (
    + this->iter_items ()
    | map (&item_t::get_key)
  )
  auto keys () const HB_AUTO_RETURN
  (
    + this->keys_ref ()
    | map (ridentity)
  )
  auto values_ref () const HB_AUTO_RETURN
  (
    + this->iter_items ()
    | map (&item_t::get_value)
  )
  auto values () const HB_AUTO_RETURN
  (
    + this->values_ref ()
    | map (ridentity)
  )

  /* C iterator. */
  bool next (int *idx,
	     K *key,
	     V *value) const
  {
    unsigned i = (unsigned) (*idx + 1);

    unsigned count = size ();
    while (i < count && !items[i].is_real ())
      i++;

    if (i >= count)
    {
      *idx = -1;
      return false;
    }

    *key = items[i].key;
    *value = items[i].value;

    *idx = (signed) i;
    return true;
  }

  /* Sink interface. */
  hashmap_t& operator << (const pair_t<K, V>& v)
  { set (v.first, v.second); return *this; }
  hashmap_t& operator << (const pair_t<K, V&&>& v)
  { set (v.first, std::move (v.second)); return *this; }
  hashmap_t& operator << (const pair_t<K&&, V>& v)
  { set (std::move (v.first), v.second); return *this; }
  hashmap_t& operator << (const pair_t<K&&, V&&>& v)
  { set (std::move (v.first), std::move (v.second)); return *this; }

  static unsigned int prime_for (unsigned int shift)
  {
    /* Following comment and table copied from glib. */
    /* Each table size has an associated prime modulo (the first prime
     * lower than the table size) used to find the initial bucket. Probing
     * then works modulo 2^n. The prime modulo is necessary to get a
     * good distribution with poor hash functions.
     */
    /* Not declaring static to make all kinds of compilers happy... */
    /*static*/ const unsigned int prime_mod [32] =
    {
      1,          /* For 1 << 0 */
      2,
      3,
      7,
      13,
      31,
      61,
      127,
      251,
      509,
      1021,
      2039,
      4093,
      8191,
      16381,
      32749,
      65521,      /* For 1 << 16 */
      131071,
      262139,
      524287,
      1048573,
      2097143,
      4194301,
      8388593,
      16777213,
      33554393,
      67108859,
      134217689,
      268435399,
      536870909,
      1073741789,
      2147483647  /* For 1 << 31 */
    };

    if (unlikely (shift >= ARRAY_LENGTH (prime_mod)))
      return prime_mod[ARRAY_LENGTH (prime_mod) - 1];

    return prime_mod[shift];
  }
};

/*
 * map_t
 */

struct map_t : hashmap_t<codepoint_t,
			       codepoint_t,
			       true>
{
  using hashmap = hashmap_t<codepoint_t,
			       codepoint_t,
			       true>;

  ~map_t () = default;
  map_t () : hashmap () {}
  map_t (const map_t &o) : hashmap ((hashmap &) o) {}
  map_t (map_t &&o)  noexcept : hashmap (std::move ((hashmap &) o)) {}
  map_t& operator= (const map_t&) = default;
  map_t& operator= (map_t&&) = default;
  map_t (std::initializer_list<codepoint_pair_t> lst) : hashmap (lst) {}
  template <typename Iterable,
	    requires (is_iterable (Iterable))>
  map_t (const Iterable &o) : hashmap (o) {}
};


#endif /* HB_MAP_HH */
