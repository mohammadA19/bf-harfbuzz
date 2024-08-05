/*
 * Copyright © 1998-2004  David Turner and Werner Lemberg
 * Copyright © 2004,2007,2009,2010  Red Hat, Inc.
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

#include "hb-buffer.hh"
#include "hb-utf.hh"


/**
 * SECTION: hb-buffer
 * @title: hb-buffer
 * @short_description: Input and output buffers
 * @include: hb.h
 *
 * Buffers serve a dual role in HarfBuzz; before shaping, they hold
 * the input characters that are passed to shape(), and after
 * shaping they hold the output glyphs.
 *
 * The input buffer is a sequence of Unicode codepoints, with
 * associated attributes such as direction and script.  The output
 * buffer is a sequence of glyphs, with associated attributes such
 * as position and cluster.
 **/


/**
 * segment_properties_equal:
 * @a: first #segment_properties_t to compare.
 * @b: second #segment_properties_t to compare.
 *
 * Checks the equality of two #segment_properties_t's.
 *
 * Return value:
 * `true` if all properties of @a equal those of @b, `false` otherwise.
 *
 * Since: 0.9.7
 **/
bool_t
segment_properties_equal (const segment_properties_t *a,
			     const segment_properties_t *b)
{
  return a->direction == b->direction &&
	 a->script    == b->script    &&
	 a->language  == b->language  &&
	 a->reserved1 == b->reserved1 &&
	 a->reserved2 == b->reserved2;

}

/**
 * segment_properties_hash:
 * @p: #segment_properties_t to hash.
 *
 * Creates a hash representing @p.
 *
 * Return value:
 * A hash of @p.
 *
 * Since: 0.9.7
 **/
unsigned int
segment_properties_hash (const segment_properties_t *p)
{
  return ((unsigned int) p->direction * 31 +
	  (unsigned int) p->script) * 31 +
	 (intptr_t) (p->language);
}

/**
 * segment_properties_overlay:
 * @p: #segment_properties_t to fill in.
 * @src: #segment_properties_t to fill in from.
 *
 * Fills in missing fields of @p from @src in a considered manner.
 *
 * First, if @p does not have direction set, direction is copied from @src.
 *
 * Next, if @p and @src have the same direction (which can be unset), if @p
 * does not have script set, script is copied from @src.
 *
 * Finally, if @p and @src have the same direction and script (which either
 * can be unset), if @p does not have language set, language is copied from
 * @src.
 *
 * Since: 3.3.0
 **/
void
segment_properties_overlay (segment_properties_t *p,
			       const segment_properties_t *src)
{
  if (unlikely (!p || !src))
    return;

  if (!p->direction)
    p->direction = src->direction;

  if (p->direction != src->direction)
    return;

  if (!p->script)
    p->script = src->script;

  if (p->script != src->script)
    return;

  if (!p->language)
    p->language = src->language;
}

/* Here is how the buffer works internally:
 *
 * There are two info pointers: info and out_info.  They always have
 * the same allocated size, but different lengths.
 *
 * As an optimization, both info and out_info may point to the
 * same piece of memory, which is owned by info.  This remains the
 * case as long as out_len doesn't exceed i at any time.
 * In that case, sync() is mostly no-op and the glyph operations
 * operate mostly in-place.
 *
 * As soon as out_info gets longer than info, out_info is moved over
 * to an alternate buffer (which we reuse the pos buffer for), and its
 * current contents (out_len entries) are copied to the new place.
 *
 * This should all remain transparent to the user.  sync() then
 * switches info over to out_info and does housekeeping.
 */



/* Internal API */

bool
buffer_t::enlarge (unsigned int size)
{
  if (unlikely (!successful))
    return false;
  if (unlikely (size > max_len))
  {
    successful = false;
    return false;
  }

  unsigned int new_allocated = allocated;
  glyph_position_t *new_pos = nullptr;
  glyph_info_t *new_info = nullptr;
  bool separate_out = out_info != info;

  if (unlikely (unsigned_mul_overflows (size, sizeof (info[0]))))
    goto done;

  while (size >= new_allocated)
    new_allocated += (new_allocated >> 1) + 32;

  unsigned new_bytes;
  if (unlikely (unsigned_mul_overflows (new_allocated, sizeof (info[0]), &new_bytes)))
    goto done;

  static_assert (sizeof (info[0]) == sizeof (pos[0]), "");
  new_pos = (glyph_position_t *) realloc (pos, new_bytes);
  new_info = (glyph_info_t *) realloc (info, new_bytes);

done:
  if (unlikely (!new_pos || !new_info))
    successful = false;

  if (likely (new_pos))
    pos = new_pos;

  if (likely (new_info))
    info = new_info;

  out_info = separate_out ? (glyph_info_t *) pos : info;
  if (likely (successful))
    allocated = new_allocated;

  return likely (successful);
}

bool
buffer_t::make_room_for (unsigned int num_in,
			    unsigned int num_out)
{
  if (unlikely (!ensure (out_len + num_out))) return false;

  if (out_info == info &&
      out_len + num_out > idx + num_in)
  {
    assert (have_output);

    out_info = (glyph_info_t *) pos;
    memcpy (out_info, info, out_len * sizeof (out_info[0]));
  }

  return true;
}

bool
buffer_t::shift_forward (unsigned int count)
{
  assert (have_output);
  if (unlikely (!ensure (len + count))) return false;

  memmove (info + idx + count, info + idx, (len - idx) * sizeof (info[0]));
  if (idx + count > len)
  {
    /* Under memory failure we might expose this area.  At least
     * clean it up.  Oh well...
     *
     * Ideally, we should at least set Default_Ignorable bits on
     * these, as well as consistent cluster values.  But the former
     * is layering violation... */
    memset (info + len, 0, (idx + count - len) * sizeof (info[0]));
  }
  len += count;
  idx += count;

  return true;
}

buffer_t::scratch_buffer_t *
buffer_t::get_scratch_buffer (unsigned int *size)
{
  have_output = false;
  have_positions = false;

  out_len = 0;
  out_info = info;

  assert ((uintptr_t) pos % sizeof (scratch_buffer_t) == 0);
  *size = allocated * sizeof (pos[0]) / sizeof (scratch_buffer_t);
  return (scratch_buffer_t *) (void *) pos;
}



/* HarfBuzz-Internal API */

void
buffer_t::similar (const buffer_t &src)
{
  unicode_funcs_destroy (unicode);
  unicode = unicode_funcs_reference (src.unicode);
  flags = src.flags;
  cluster_level = src.cluster_level;
  replacement = src.replacement;
  invisible = src.invisible;
  not_found = src.not_found;
}

void
buffer_t::reset ()
{
  unicode_funcs_destroy (unicode);
  unicode = unicode_funcs_reference (unicode_funcs_get_default ());
  flags = HB_BUFFER_FLAG_DEFAULT;
  cluster_level = HB_BUFFER_CLUSTER_LEVEL_DEFAULT;
  replacement = HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT;
  invisible = 0;
  not_found = 0;

  clear ();
}

void
buffer_t::clear ()
{
  content_type = HB_BUFFER_CONTENT_TYPE_INVALID;
  segment_properties_t default_props = HB_SEGMENT_PROPERTIES_DEFAULT;
  props = default_props;

  successful = true;
  shaping_failed = false;
  have_output = false;
  have_positions = false;

  idx = 0;
  len = 0;
  out_len = 0;
  out_info = info;

  memset (context, 0, sizeof context);
  memset (context_len, 0, sizeof context_len);

  deallocate_var_all ();
  serial = 0;
  random_state = 1;
  scratch_flags = HB_BUFFER_SCRATCH_FLAG_DEFAULT;
}

void
buffer_t::enter ()
{
  deallocate_var_all ();
  serial = 0;
  shaping_failed = false;
  scratch_flags = HB_BUFFER_SCRATCH_FLAG_DEFAULT;
  unsigned mul;
  if (likely (!unsigned_mul_overflows (len, HB_BUFFER_MAX_LEN_FACTOR, &mul)))
  {
    max_len = max (mul, (unsigned) HB_BUFFER_MAX_LEN_MIN);
  }
  if (likely (!unsigned_mul_overflows (len, HB_BUFFER_MAX_OPS_FACTOR, &mul)))
  {
    max_ops = max (mul, (unsigned) HB_BUFFER_MAX_OPS_MIN);
  }
}
void
buffer_t::leave ()
{
  max_len = HB_BUFFER_MAX_LEN_DEFAULT;
  max_ops = HB_BUFFER_MAX_OPS_DEFAULT;
  deallocate_var_all ();
  serial = 0;
  // Intentionally not reseting shaping_failed, such that it can be inspected.
}


void
buffer_t::add (codepoint_t  codepoint,
		  unsigned int    cluster)
{
  glyph_info_t *glyph;

  if (unlikely (!ensure (len + 1))) return;

  glyph = &info[len];

  memset (glyph, 0, sizeof (*glyph));
  glyph->codepoint = codepoint;
  glyph->mask = 0;
  glyph->cluster = cluster;

  len++;
}

void
buffer_t::add_info (const glyph_info_t &glyph_info)
{
  if (unlikely (!ensure (len + 1))) return;

  info[len] = glyph_info;

  len++;
}


void
buffer_t::clear_output ()
{
  have_output = true;
  have_positions = false;

  idx = 0;
  out_len = 0;
  out_info = info;
}

void
buffer_t::clear_positions ()
{
  have_output = false;
  have_positions = true;

  out_len = 0;
  out_info = info;

  memset (pos, 0, sizeof (pos[0]) * len);
}

bool
buffer_t::sync ()
{
  bool ret = false;

  assert (have_output);

  assert (idx <= len);

  if (unlikely (!successful || !next_glyphs (len - idx)))
    goto reset;

  if (out_info != info)
  {
    pos = (glyph_position_t *) info;
    info = out_info;
  }
  len = out_len;
  ret = true;

reset:
  have_output = false;
  out_len = 0;
  out_info = info;
  idx = 0;

  return ret;
}

int
buffer_t::sync_so_far ()
{
  bool had_output = have_output;
  unsigned out_i = out_len;
  unsigned i = idx;
  unsigned old_idx = idx;

  if (sync ())
    idx = out_i;
  else
    idx = i;

  if (had_output)
  {
    have_output = true;
    out_len = idx;
  }

  assert (idx <= len);

  return idx - old_idx;
}

bool
buffer_t::move_to (unsigned int i)
{
  if (!have_output)
  {
    assert (i <= len);
    idx = i;
    return true;
  }
  if (unlikely (!successful))
    return false;

  assert (i <= out_len + (len - idx));

  if (out_len < i)
  {
    unsigned int count = i - out_len;
    if (unlikely (!make_room_for (count, count))) return false;

    memmove (out_info + out_len, info + idx, count * sizeof (out_info[0]));
    idx += count;
    out_len += count;
  }
  else if (out_len > i)
  {
    /* Tricky part: rewinding... */
    unsigned int count = out_len - i;

    /* This will blow in our face if memory allocation fails later
     * in this same lookup...
     *
     * We used to shift with extra 32 items.
     * But that would leave empty slots in the buffer in case of allocation
     * failures.  See comments in shift_forward().  This can cause O(N^2)
     * behavior more severely than adding 32 empty slots can... */
    if (unlikely (idx < count && !shift_forward (count - idx))) return false;

    assert (idx >= count);

    idx -= count;
    out_len -= count;
    memmove (info + idx, out_info + out_len, count * sizeof (out_info[0]));
  }

  return true;
}


void
buffer_t::set_masks (mask_t    value,
			mask_t    mask,
			unsigned int cluster_start,
			unsigned int cluster_end)
{
  if (!mask)
    return;

  mask_t not_mask = ~mask;
  value &= mask;

  unsigned int count = len;
  for (unsigned int i = 0; i < count; i++)
    if (cluster_start <= info[i].cluster && info[i].cluster < cluster_end)
      info[i].mask = (info[i].mask & not_mask) | value;
}

void
buffer_t::merge_clusters_impl (unsigned int start,
				  unsigned int end)
{
  if (cluster_level == HB_BUFFER_CLUSTER_LEVEL_CHARACTERS)
  {
    unsafe_to_break (start, end);
    return;
  }

  unsigned int cluster = info[start].cluster;

  for (unsigned int i = start + 1; i < end; i++)
    cluster = min (cluster, info[i].cluster);

  /* Extend end */
  if (cluster != info[end - 1].cluster)
    while (end < len && info[end - 1].cluster == info[end].cluster)
      end++;

  /* Extend start */
  if (cluster != info[start].cluster)
    while (idx < start && info[start - 1].cluster == info[start].cluster)
      start--;

  /* If we hit the start of buffer, continue in out-buffer. */
  if (idx == start && info[start].cluster != cluster)
    for (unsigned int i = out_len; i && out_info[i - 1].cluster == info[start].cluster; i--)
      set_cluster (out_info[i - 1], cluster);

  for (unsigned int i = start; i < end; i++)
    set_cluster (info[i], cluster);
}
void
buffer_t::merge_out_clusters (unsigned int start,
				 unsigned int end)
{
  if (cluster_level == HB_BUFFER_CLUSTER_LEVEL_CHARACTERS)
    return;

  if (unlikely (end - start < 2))
    return;

  unsigned int cluster = out_info[start].cluster;

  for (unsigned int i = start + 1; i < end; i++)
    cluster = min (cluster, out_info[i].cluster);

  /* Extend start */
  while (start && out_info[start - 1].cluster == out_info[start].cluster)
    start--;

  /* Extend end */
  while (end < out_len && out_info[end - 1].cluster == out_info[end].cluster)
    end++;

  /* If we hit the end of out-buffer, continue in buffer. */
  if (end == out_len)
    for (unsigned int i = idx; i < len && info[i].cluster == out_info[end - 1].cluster; i++)
      set_cluster (info[i], cluster);

  for (unsigned int i = start; i < end; i++)
    set_cluster (out_info[i], cluster);
}
void
buffer_t::delete_glyph ()
{
  /* The logic here is duplicated in ot_hide_default_ignorables(). */

  unsigned int cluster = info[idx].cluster;
  if ((idx + 1 < len && cluster == info[idx + 1].cluster) ||
      (out_len && cluster == out_info[out_len - 1].cluster))
  {
    /* Cluster survives; do nothing. */
    goto done;
  }

  if (out_len)
  {
    /* Merge cluster backward. */
    if (cluster < out_info[out_len - 1].cluster)
    {
      unsigned int mask = info[idx].mask;
      unsigned int old_cluster = out_info[out_len - 1].cluster;
      for (unsigned i = out_len; i && out_info[i - 1].cluster == old_cluster; i--)
	set_cluster (out_info[i - 1], cluster, mask);
    }
    goto done;
  }

  if (idx + 1 < len)
  {
    /* Merge cluster forward. */
    merge_clusters (idx, idx + 2);
    goto done;
  }

done:
  skip_glyph ();
}

void
buffer_t::delete_glyphs_inplace (bool (*filter) (const glyph_info_t *info))
{
  /* Merge clusters and delete filtered glyphs.
   * NOTE! We can't use out-buffer as we have positioning data. */
  unsigned int j = 0;
  unsigned int count = len;
  for (unsigned int i = 0; i < count; i++)
  {
    if (filter (&info[i]))
    {
      /* Merge clusters.
       * Same logic as delete_glyph(), but for in-place removal. */

      unsigned int cluster = info[i].cluster;
      if (i + 1 < count && cluster == info[i + 1].cluster)
	continue; /* Cluster survives; do nothing. */

      if (j)
      {
	/* Merge cluster backward. */
	if (cluster < info[j - 1].cluster)
	{
	  unsigned int mask = info[i].mask;
	  unsigned int old_cluster = info[j - 1].cluster;
	  for (unsigned k = j; k && info[k - 1].cluster == old_cluster; k--)
	    set_cluster (info[k - 1], cluster, mask);
	}
	continue;
      }

      if (i + 1 < count)
	merge_clusters (i, i + 2); /* Merge cluster forward. */

      continue;
    }

    if (j != i)
    {
      info[j] = info[i];
      pos[j] = pos[i];
    }
    j++;
  }
  len = j;
}

void
buffer_t::guess_segment_properties ()
{
  assert_unicode ();

  /* If script is set to INVALID, guess from buffer contents */
  if (props.script == HB_SCRIPT_INVALID) {
    for (unsigned int i = 0; i < len; i++) {
      script_t script = unicode->script (info[i].codepoint);
      if (likely (script != HB_SCRIPT_COMMON &&
		  script != HB_SCRIPT_INHERITED &&
		  script != HB_SCRIPT_UNKNOWN)) {
	props.script = script;
	break;
      }
    }
  }

  /* If direction is set to INVALID, guess from script */
  if (props.direction == HB_DIRECTION_INVALID) {
    props.direction = script_get_horizontal_direction (props.script);
    if (props.direction == HB_DIRECTION_INVALID)
      props.direction = HB_DIRECTION_LTR;
  }

  /* If language is not set, use default language from locale */
  if (props.language == HB_LANGUAGE_INVALID) {
    /* TODO get_default_for_script? using $LANGUAGE */
    props.language = language_get_default ();
  }
}


/* Public API */

DEFINE_NULL_INSTANCE (buffer_t) =
{
  HB_OBJECT_HEADER_STATIC,

  const_cast<unicode_funcs_t *> (&_Null_unicode_funcs_t),
  HB_BUFFER_FLAG_DEFAULT,
  HB_BUFFER_CLUSTER_LEVEL_DEFAULT,
  HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT,
  0, /* invisible */
  0, /* not_found */


  HB_BUFFER_CONTENT_TYPE_INVALID,
  HB_SEGMENT_PROPERTIES_DEFAULT,

  false, /* successful */
  true, /* shaping_failed */
  false, /* have_output */
  true  /* have_positions */

  /* Zero is good enough for everything else. */
};


/**
 * buffer_create:
 *
 * Creates a new #buffer_t with all properties to defaults.
 *
 * Return value: (transfer full):
 * A newly allocated #buffer_t with a reference count of 1. The initial
 * reference count should be released with buffer_destroy() when you are done
 * using the #buffer_t. This function never returns `NULL`. If memory cannot
 * be allocated, a special #buffer_t object will be returned on which
 * buffer_allocation_successful() returns `false`.
 *
 * Since: 0.9.2
 **/
buffer_t *
buffer_create ()
{
  buffer_t *buffer;

  if (!(buffer = object_create<buffer_t> ()))
    return buffer_get_empty ();

  buffer->max_len = HB_BUFFER_MAX_LEN_DEFAULT;
  buffer->max_ops = HB_BUFFER_MAX_OPS_DEFAULT;

  buffer->reset ();

  return buffer;
}

/**
 * buffer_create_similar:
 * @src: An #buffer_t
 *
 * Creates a new #buffer_t, similar to buffer_create(). The only
 * difference is that the buffer is configured similarly to @src.
 *
 * Return value: (transfer full):
 * A newly allocated #buffer_t, similar to buffer_create().
 *
 * Since: 3.3.0
 **/
buffer_t *
buffer_create_similar (const buffer_t *src)
{
  buffer_t *buffer = buffer_create ();

  buffer->similar (*src);

  return buffer;
}

/**
 * buffer_reset:
 * @buffer: An #buffer_t
 *
 * Resets the buffer to its initial status, as if it was just newly created
 * with buffer_create().
 *
 * Since: 0.9.2
 **/
void
buffer_reset (buffer_t *buffer)
{
  if (unlikely (object_is_immutable (buffer)))
    return;

  buffer->reset ();
}

/**
 * buffer_get_empty:
 *
 * Fetches an empty #buffer_t.
 *
 * Return value: (transfer full): The empty buffer
 *
 * Since: 0.9.2
 **/
buffer_t *
buffer_get_empty ()
{
  return const_cast<buffer_t *> (&Null (buffer_t));
}

/**
 * buffer_reference: (skip)
 * @buffer: An #buffer_t
 *
 * Increases the reference count on @buffer by one. This prevents @buffer from
 * being destroyed until a matching call to buffer_destroy() is made.
 *
 * Return value: (transfer full):
 * The referenced #buffer_t.
 *
 * Since: 0.9.2
 **/
buffer_t *
buffer_reference (buffer_t *buffer)
{
  return object_reference (buffer);
}

/**
 * buffer_destroy: (skip)
 * @buffer: An #buffer_t
 *
 * Deallocate the @buffer.
 * Decreases the reference count on @buffer by one. If the result is zero, then
 * @buffer and all associated resources are freed. See buffer_reference().
 *
 * Since: 0.9.2
 **/
void
buffer_destroy (buffer_t *buffer)
{
  if (!object_destroy (buffer)) return;

  unicode_funcs_destroy (buffer->unicode);

  free (buffer->info);
  free (buffer->pos);
#ifndef HB_NO_BUFFER_MESSAGE
  if (buffer->message_destroy)
    buffer->message_destroy (buffer->message_data);
#endif

  free (buffer);
}

/**
 * buffer_set_user_data: (skip)
 * @buffer: An #buffer_t
 * @key: The user-data key
 * @data: A pointer to the user data
 * @destroy: (nullable): A callback to call when @data is not needed anymore
 * @replace: Whether to replace an existing data with the same key
 *
 * Attaches a user-data key/data pair to the specified buffer. 
 *
 * Return value: `true` if success, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
buffer_set_user_data (buffer_t        *buffer,
			 user_data_key_t *key,
			 void *              data,
			 destroy_func_t   destroy,
			 bool_t           replace)
{
  return object_set_user_data (buffer, key, data, destroy, replace);
}

/**
 * buffer_get_user_data: (skip)
 * @buffer: An #buffer_t
 * @key: The user-data key to query
 *
 * Fetches the user data associated with the specified key,
 * attached to the specified buffer.
 *
 * Return value: (transfer none): A pointer to the user data
 *
 * Since: 0.9.2
 **/
void *
buffer_get_user_data (const buffer_t  *buffer,
			 user_data_key_t *key)
{
  return object_get_user_data (buffer, key);
}


/**
 * buffer_set_content_type:
 * @buffer: An #buffer_t
 * @content_type: The type of buffer contents to set
 *
 * Sets the type of @buffer contents. Buffers are either empty, contain
 * characters (before shaping), or contain glyphs (the result of shaping).
 *
 * You rarely need to call this function, since a number of other
 * functions transition the content type for you. Namely:
 *
 * - A newly created buffer starts with content type
 *   %HB_BUFFER_CONTENT_TYPE_INVALID. Calling buffer_reset(),
 *   buffer_clear_contents(), as well as calling buffer_set_length()
 *   with an argument of zero all set the buffer content type to invalid
 *   as well.
 *
 * - Calling buffer_add_utf8(), buffer_add_utf16(),
 *   buffer_add_utf32(), buffer_add_codepoints() and
 *   buffer_add_latin1() expect that buffer is either empty and
 *   have a content type of invalid, or that buffer content type is
 *   %HB_BUFFER_CONTENT_TYPE_UNICODE, and they also set the content
 *   type to Unicode if they added anything to an empty buffer.
 *
 * - Finally shape() and shape_full() expect that the buffer
 *   is either empty and have content type of invalid, or that buffer
 *   content type is %HB_BUFFER_CONTENT_TYPE_UNICODE, and upon
 *   success they set the buffer content type to
 *   %HB_BUFFER_CONTENT_TYPE_GLYPHS.
 *
 * The above transitions are designed such that one can use a buffer
 * in a loop of "reset : add-text : shape" without needing to ever
 * modify the content type manually.
 *
 * Since: 0.9.5
 **/
void
buffer_set_content_type (buffer_t              *buffer,
			    buffer_content_type_t  content_type)
{
  buffer->content_type = content_type;
}

/**
 * buffer_get_content_type:
 * @buffer: An #buffer_t
 *
 * Fetches the type of @buffer contents. Buffers are either empty, contain
 * characters (before shaping), or contain glyphs (the result of shaping).
 *
 * Return value:
 * The type of @buffer contents
 *
 * Since: 0.9.5
 **/
buffer_content_type_t
buffer_get_content_type (const buffer_t *buffer)
{
  return buffer->content_type;
}


/**
 * buffer_set_unicode_funcs:
 * @buffer: An #buffer_t
 * @unicode_funcs: The Unicode-functions structure
 *
 * Sets the Unicode-functions structure of a buffer to
 * @unicode_funcs.
 *
 * Since: 0.9.2
 **/
void
buffer_set_unicode_funcs (buffer_t        *buffer,
			     unicode_funcs_t *unicode_funcs)
{
  if (unlikely (object_is_immutable (buffer)))
    return;

  if (!unicode_funcs)
    unicode_funcs = unicode_funcs_get_default ();

  unicode_funcs_reference (unicode_funcs);
  unicode_funcs_destroy (buffer->unicode);
  buffer->unicode = unicode_funcs;
}

/**
 * buffer_get_unicode_funcs:
 * @buffer: An #buffer_t
 *
 * Fetches the Unicode-functions structure of a buffer.
 *
 * Return value: The Unicode-functions structure
 *
 * Since: 0.9.2
 **/
unicode_funcs_t *
buffer_get_unicode_funcs (const buffer_t *buffer)
{
  return buffer->unicode;
}

/**
 * buffer_set_direction:
 * @buffer: An #buffer_t
 * @direction: the #direction_t of the @buffer
 *
 * Set the text flow direction of the buffer. No shaping can happen without
 * setting @buffer direction, and it controls the visual direction for the
 * output glyphs; for RTL direction the glyphs will be reversed. Many layout
 * features depend on the proper setting of the direction, for example,
 * reversing RTL text before shaping, then shaping with LTR direction is not
 * the same as keeping the text in logical order and shaping with RTL
 * direction.
 *
 * Since: 0.9.2
 **/
void
buffer_set_direction (buffer_t    *buffer,
			 direction_t  direction)
{
  if (unlikely (object_is_immutable (buffer)))
    return;

  buffer->props.direction = direction;
}

/**
 * buffer_get_direction:
 * @buffer: An #buffer_t
 *
 * See buffer_set_direction()
 *
 * Return value:
 * The direction of the @buffer.
 *
 * Since: 0.9.2
 **/
direction_t
buffer_get_direction (const buffer_t *buffer)
{
  return buffer->props.direction;
}

/**
 * buffer_set_script:
 * @buffer: An #buffer_t
 * @script: An #script_t to set.
 *
 * Sets the script of @buffer to @script.
 *
 * Script is crucial for choosing the proper shaping behaviour for scripts that
 * require it (e.g. Arabic) and the which OpenType features defined in the font
 * to be applied.
 *
 * You can pass one of the predefined #script_t values, or use
 * script_from_string() or script_from_iso15924_tag() to get the
 * corresponding script from an ISO 15924 script tag.
 *
 * Since: 0.9.2
 **/
void
buffer_set_script (buffer_t *buffer,
		      script_t  script)
{
  if (unlikely (object_is_immutable (buffer)))
    return;

  buffer->props.script = script;
}

/**
 * buffer_get_script:
 * @buffer: An #buffer_t
 *
 * Fetches the script of @buffer.
 *
 * Return value:
 * The #script_t of the @buffer
 *
 * Since: 0.9.2
 **/
script_t
buffer_get_script (const buffer_t *buffer)
{
  return buffer->props.script;
}

/**
 * buffer_set_language:
 * @buffer: An #buffer_t
 * @language: An language_t to set
 *
 * Sets the language of @buffer to @language.
 *
 * Languages are crucial for selecting which OpenType feature to apply to the
 * buffer which can result in applying language-specific behaviour. Languages
 * are orthogonal to the scripts, and though they are related, they are
 * different concepts and should not be confused with each other.
 *
 * Use language_from_string() to convert from BCP 47 language tags to
 * #language_t.
 *
 * Since: 0.9.2
 **/
void
buffer_set_language (buffer_t   *buffer,
			language_t  language)
{
  if (unlikely (object_is_immutable (buffer)))
    return;

  buffer->props.language = language;
}

/**
 * buffer_get_language:
 * @buffer: An #buffer_t
 *
 * See buffer_set_language().
 *
 * Return value: (transfer none):
 * The #language_t of the buffer. Must not be freed by the caller.
 *
 * Since: 0.9.2
 **/
language_t
buffer_get_language (const buffer_t *buffer)
{
  return buffer->props.language;
}

/**
 * buffer_set_segment_properties:
 * @buffer: An #buffer_t
 * @props: An #segment_properties_t to use
 *
 * Sets the segment properties of the buffer, a shortcut for calling
 * buffer_set_direction(), buffer_set_script() and
 * buffer_set_language() individually.
 *
 * Since: 0.9.7
 **/
void
buffer_set_segment_properties (buffer_t *buffer,
				  const segment_properties_t *props)
{
  if (unlikely (object_is_immutable (buffer)))
    return;

  buffer->props = *props;
}

/**
 * buffer_get_segment_properties:
 * @buffer: An #buffer_t
 * @props: (out): The output #segment_properties_t
 *
 * Sets @props to the #segment_properties_t of @buffer.
 *
 * Since: 0.9.7
 **/
void
buffer_get_segment_properties (const buffer_t *buffer,
				  segment_properties_t *props)
{
  *props = buffer->props;
}


/**
 * buffer_set_flags:
 * @buffer: An #buffer_t
 * @flags: The buffer flags to set
 *
 * Sets @buffer flags to @flags. See #buffer_flags_t.
 *
 * Since: 0.9.7
 **/
void
buffer_set_flags (buffer_t       *buffer,
		     buffer_flags_t  flags)
{
  if (unlikely (object_is_immutable (buffer)))
    return;

  buffer->flags = flags;
}

/**
 * buffer_get_flags:
 * @buffer: An #buffer_t
 *
 * Fetches the #buffer_flags_t of @buffer.
 *
 * Return value:
 * The @buffer flags
 *
 * Since: 0.9.7
 **/
buffer_flags_t
buffer_get_flags (const buffer_t *buffer)
{
  return buffer->flags;
}

/**
 * buffer_set_cluster_level:
 * @buffer: An #buffer_t
 * @cluster_level: The cluster level to set on the buffer
 *
 * Sets the cluster level of a buffer. The #buffer_cluster_level_t
 * dictates one aspect of how HarfBuzz will treat non-base characters 
 * during shaping.
 *
 * Since: 0.9.42
 **/
void
buffer_set_cluster_level (buffer_t               *buffer,
			     buffer_cluster_level_t  cluster_level)
{
  if (unlikely (object_is_immutable (buffer)))
    return;

  buffer->cluster_level = cluster_level;
}

/**
 * buffer_get_cluster_level:
 * @buffer: An #buffer_t
 *
 * Fetches the cluster level of a buffer. The #buffer_cluster_level_t
 * dictates one aspect of how HarfBuzz will treat non-base characters 
 * during shaping.
 *
 * Return value: The cluster level of @buffer
 *
 * Since: 0.9.42
 **/
buffer_cluster_level_t
buffer_get_cluster_level (const buffer_t *buffer)
{
  return buffer->cluster_level;
}


/**
 * buffer_set_replacement_codepoint:
 * @buffer: An #buffer_t
 * @replacement: the replacement #codepoint_t
 *
 * Sets the #codepoint_t that replaces invalid entries for a given encoding
 * when adding text to @buffer.
 *
 * Default is #HB_BUFFER_REPLACEMENT_CODEPOINT_DEFAULT.
 *
 * Since: 0.9.31
 **/
void
buffer_set_replacement_codepoint (buffer_t    *buffer,
				     codepoint_t  replacement)
{
  if (unlikely (object_is_immutable (buffer)))
    return;

  buffer->replacement = replacement;
}

/**
 * buffer_get_replacement_codepoint:
 * @buffer: An #buffer_t
 *
 * Fetches the #codepoint_t that replaces invalid entries for a given encoding
 * when adding text to @buffer.
 *
 * Return value:
 * The @buffer replacement #codepoint_t
 *
 * Since: 0.9.31
 **/
codepoint_t
buffer_get_replacement_codepoint (const buffer_t *buffer)
{
  return buffer->replacement;
}


/**
 * buffer_set_invisible_glyph:
 * @buffer: An #buffer_t
 * @invisible: the invisible #codepoint_t
 *
 * Sets the #codepoint_t that replaces invisible characters in
 * the shaping result.  If set to zero (default), the glyph for the
 * U+0020 SPACE character is used.  Otherwise, this value is used
 * verbatim.
 *
 * Since: 2.0.0
 **/
void
buffer_set_invisible_glyph (buffer_t    *buffer,
			       codepoint_t  invisible)
{
  if (unlikely (object_is_immutable (buffer)))
    return;

  buffer->invisible = invisible;
}

/**
 * buffer_get_invisible_glyph:
 * @buffer: An #buffer_t
 *
 * See buffer_set_invisible_glyph().
 *
 * Return value:
 * The @buffer invisible #codepoint_t
 *
 * Since: 2.0.0
 **/
codepoint_t
buffer_get_invisible_glyph (const buffer_t *buffer)
{
  return buffer->invisible;
}

/**
 * buffer_set_not_found_glyph:
 * @buffer: An #buffer_t
 * @not_found: the not-found #codepoint_t
 *
 * Sets the #codepoint_t that replaces characters not found in
 * the font during shaping.
 *
 * The not-found glyph defaults to zero, sometimes known as the
 * ".notdef" glyph.  This API allows for differentiating the two.
 *
 * Since: 3.1.0
 **/
void
buffer_set_not_found_glyph (buffer_t    *buffer,
			       codepoint_t  not_found)
{
  if (unlikely (object_is_immutable (buffer)))
    return;

  buffer->not_found = not_found;
}

/**
 * buffer_get_not_found_glyph:
 * @buffer: An #buffer_t
 *
 * See buffer_set_not_found_glyph().
 *
 * Return value:
 * The @buffer not-found #codepoint_t
 *
 * Since: 3.1.0
 **/
codepoint_t
buffer_get_not_found_glyph (const buffer_t *buffer)
{
  return buffer->not_found;
}

/**
 * buffer_set_random_state:
 * @buffer: An #buffer_t
 * @state: the new random state
 *
 * Sets the random state of the buffer. The state changes
 * every time a glyph uses randomness (eg. the `rand`
 * OpenType feature). This function together with
 * buffer_get_random_state() allow for transferring
 * the current random state to a subsequent buffer, to
 * get better randomness distribution.
 *
 * Defaults to 1 and when buffer contents are cleared.
 * A value of 0 disables randomness during shaping.
 *
 * Since: 8.4.0
 **/
void
buffer_set_random_state (buffer_t    *buffer,
			    unsigned        state)
{
  if (unlikely (object_is_immutable (buffer)))
    return;

  buffer->random_state = state;
}

/**
 * buffer_get_random_state:
 * @buffer: An #buffer_t
 *
 * See buffer_set_random_state().
 *
 * Return value:
 * The @buffer random state
 *
 * Since: 8.4.0
 **/
unsigned
buffer_get_random_state (const buffer_t *buffer)
{
  return buffer->random_state;
}

/**
 * buffer_clear_contents:
 * @buffer: An #buffer_t
 *
 * Similar to buffer_reset(), but does not clear the Unicode functions and
 * the replacement code point.
 *
 * Since: 0.9.11
 **/
void
buffer_clear_contents (buffer_t *buffer)
{
  if (unlikely (object_is_immutable (buffer)))
    return;

  buffer->clear ();
}

/**
 * buffer_pre_allocate:
 * @buffer: An #buffer_t
 * @size: Number of items to pre allocate.
 *
 * Pre allocates memory for @buffer to fit at least @size number of items.
 *
 * Return value:
 * `true` if @buffer memory allocation succeeded, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
buffer_pre_allocate (buffer_t *buffer, unsigned int size)
{
  return buffer->ensure (size);
}

/**
 * buffer_allocation_successful:
 * @buffer: An #buffer_t
 *
 * Check if allocating memory for the buffer succeeded.
 *
 * Return value:
 * `true` if @buffer memory allocation succeeded, `false` otherwise.
 *
 * Since: 0.9.2
 **/
bool_t
buffer_allocation_successful (buffer_t  *buffer)
{
  return buffer->successful;
}

/**
 * buffer_add:
 * @buffer: An #buffer_t
 * @codepoint: A Unicode code point.
 * @cluster: The cluster value of @codepoint.
 *
 * Appends a character with the Unicode value of @codepoint to @buffer, and
 * gives it the initial cluster value of @cluster. Clusters can be any thing
 * the client wants, they are usually used to refer to the index of the
 * character in the input text stream and are output in
 * #glyph_info_t.cluster field.
 *
 * This function does not check the validity of @codepoint, it is up to the
 * caller to ensure it is a valid Unicode code point.
 *
 * Since: 0.9.7
 **/
void
buffer_add (buffer_t    *buffer,
	       codepoint_t  codepoint,
	       unsigned int    cluster)
{
  buffer->add (codepoint, cluster);
  buffer->clear_context (1);
}

/**
 * buffer_set_length:
 * @buffer: An #buffer_t
 * @length: The new length of @buffer
 *
 * Similar to buffer_pre_allocate(), but clears any new items added at the
 * end.
 *
 * Return value:
 * `true` if @buffer memory allocation succeeded, `false` otherwise.
 *
 * Since: 0.9.2
 **/
bool_t
buffer_set_length (buffer_t  *buffer,
		      unsigned int  length)
{
  if (unlikely (object_is_immutable (buffer)))
    return length == 0;

  if (unlikely (!buffer->ensure (length)))
    return false;

  /* Wipe the new space */
  if (length > buffer->len) {
    memset (buffer->info + buffer->len, 0, sizeof (buffer->info[0]) * (length - buffer->len));
    if (buffer->have_positions)
      memset (buffer->pos + buffer->len, 0, sizeof (buffer->pos[0]) * (length - buffer->len));
  }

  buffer->len = length;

  if (!length)
  {
    buffer->content_type = HB_BUFFER_CONTENT_TYPE_INVALID;
    buffer->clear_context (0);
  }
  buffer->clear_context (1);

  return true;
}

/**
 * buffer_get_length:
 * @buffer: An #buffer_t
 *
 * Returns the number of items in the buffer.
 *
 * Return value:
 * The @buffer length.
 * The value valid as long as buffer has not been modified.
 *
 * Since: 0.9.2
 **/
unsigned int
buffer_get_length (const buffer_t *buffer)
{
  return buffer->len;
}

/**
 * buffer_get_glyph_infos:
 * @buffer: An #buffer_t
 * @length: (out): The output-array length.
 *
 * Returns @buffer glyph information array.  Returned pointer
 * is valid as long as @buffer contents are not modified.
 *
 * Return value: (transfer none) (array length=length):
 * The @buffer glyph information array.
 * The value valid as long as buffer has not been modified.
 *
 * Since: 0.9.2
 **/
glyph_info_t *
buffer_get_glyph_infos (buffer_t  *buffer,
			   unsigned int *length)
{
  if (length)
    *length = buffer->len;

  return (glyph_info_t *) buffer->info;
}

/**
 * buffer_get_glyph_positions:
 * @buffer: An #buffer_t
 * @length: (out): The output length
 *
 * Returns @buffer glyph position array.  Returned pointer
 * is valid as long as @buffer contents are not modified.
 *
 * If buffer did not have positions before, the positions will be
 * initialized to zeros, unless this function is called from
 * within a buffer message callback (see buffer_set_message_func()),
 * in which case `NULL` is returned.
 *
 * Return value: (transfer none) (array length=length):
 * The @buffer glyph position array.
 * The value valid as long as buffer has not been modified.
 *
 * Since: 0.9.2
 **/
glyph_position_t *
buffer_get_glyph_positions (buffer_t  *buffer,
			       unsigned int *length)
{
  if (length)
    *length = buffer->len;

  if (!buffer->have_positions)
  {
    if (unlikely (buffer->message_depth))
      return nullptr;

    buffer->clear_positions ();
  }

  return (glyph_position_t *) buffer->pos;
}

/**
 * buffer_has_positions:
 * @buffer: an #buffer_t.
 *
 * Returns whether @buffer has glyph position data.
 * A buffer gains position data when buffer_get_glyph_positions() is called on it,
 * and cleared of position data when buffer_clear_contents() is called.
 *
 * Return value:
 * `true` if the @buffer has position array, `false` otherwise.
 *
 * Since: 2.7.3
 **/
HB_EXTERN bool_t
buffer_has_positions (buffer_t  *buffer)
{
  return buffer->have_positions;
}

/**
 * glyph_info_get_glyph_flags:
 * @info: a #glyph_info_t
 *
 * Returns glyph flags encoded within a #glyph_info_t.
 *
 * Return value:
 * The #glyph_flags_t encoded within @info
 *
 * Since: 1.5.0
 **/
glyph_flags_t
(glyph_info_get_glyph_flags) (const glyph_info_t *info)
{
  return glyph_info_get_glyph_flags (info);
}

/**
 * buffer_reverse:
 * @buffer: An #buffer_t
 *
 * Reverses buffer contents.
 *
 * Since: 0.9.2
 **/
void
buffer_reverse (buffer_t *buffer)
{
  buffer->reverse ();
}

/**
 * buffer_reverse_range:
 * @buffer: An #buffer_t
 * @start: start index
 * @end: end index
 *
 * Reverses buffer contents between @start and @end.
 *
 * Since: 0.9.41
 **/
void
buffer_reverse_range (buffer_t *buffer,
			 unsigned int start, unsigned int end)
{
  buffer->reverse_range (start, end);
}

/**
 * buffer_reverse_clusters:
 * @buffer: An #buffer_t
 *
 * Reverses buffer clusters.  That is, the buffer contents are
 * reversed, then each cluster (consecutive items having the
 * same cluster number) are reversed again.
 *
 * Since: 0.9.2
 **/
void
buffer_reverse_clusters (buffer_t *buffer)
{
  buffer->reverse_clusters ();
}

/**
 * buffer_guess_segment_properties:
 * @buffer: An #buffer_t
 *
 * Sets unset buffer segment properties based on buffer Unicode
 * contents.  If buffer is not empty, it must have content type
 * #HB_BUFFER_CONTENT_TYPE_UNICODE.
 *
 * If buffer script is not set (ie. is #HB_SCRIPT_INVALID), it
 * will be set to the Unicode script of the first character in
 * the buffer that has a script other than #HB_SCRIPT_COMMON,
 * #HB_SCRIPT_INHERITED, and #HB_SCRIPT_UNKNOWN.
 *
 * Next, if buffer direction is not set (ie. is #HB_DIRECTION_INVALID),
 * it will be set to the natural horizontal direction of the
 * buffer script as returned by script_get_horizontal_direction().
 * If script_get_horizontal_direction() returns #HB_DIRECTION_INVALID,
 * then #HB_DIRECTION_LTR is used.
 *
 * Finally, if buffer language is not set (ie. is #HB_LANGUAGE_INVALID),
 * it will be set to the process's default language as returned by
 * language_get_default().  This may change in the future by
 * taking buffer script into consideration when choosing a language.
 * Note that language_get_default() is NOT threadsafe the first time
 * it is called.  See documentation for that function for details.
 *
 * Since: 0.9.7
 **/
void
buffer_guess_segment_properties (buffer_t *buffer)
{
  buffer->guess_segment_properties ();
}

template <typename utf_t>
static inline void
buffer_add_utf (buffer_t  *buffer,
		   const typename utf_t::codepoint_t *text,
		   int           text_length,
		   unsigned int  item_offset,
		   int           item_length)
{
  typedef typename utf_t::codepoint_t T;
  const codepoint_t replacement = buffer->replacement;

  buffer->assert_unicode ();

  if (unlikely (object_is_immutable (buffer)))
    return;

  if (text_length == -1)
    text_length = utf_t::strlen (text);

  if (item_length == -1)
    item_length = text_length - item_offset;

  if (unlikely (item_length < 0 ||
		item_length > INT_MAX / 8 ||
		!buffer->ensure (buffer->len + item_length * sizeof (T) / 4)))
    return;

  /* If buffer is empty and pre-context provided, install it.
   * This check is written this way, to make sure people can
   * provide pre-context in one add_utf() call, then provide
   * text in a follow-up call.  See:
   *
   * https://bugzilla.mozilla.org/show_bug.cgi?id=801410#c13
   */
  if (!buffer->len && item_offset > 0)
  {
    /* Add pre-context */
    buffer->clear_context (0);
    const T *prev = text + item_offset;
    const T *start = text;
    while (start < prev && buffer->context_len[0] < buffer->CONTEXT_LENGTH)
    {
      codepoint_t u;
      prev = utf_t::prev (prev, start, &u, replacement);
      buffer->context[0][buffer->context_len[0]++] = u;
    }
  }

  const T *next = text + item_offset;
  const T *end = next + item_length;
  while (next < end)
  {
    codepoint_t u;
    const T *old_next = next;
    next = utf_t::next (next, end, &u, replacement);
    buffer->add (u, old_next - (const T *) text);
  }

  /* Add post-context */
  buffer->clear_context (1);
  end = text + text_length;
  while (next < end && buffer->context_len[1] < buffer->CONTEXT_LENGTH)
  {
    codepoint_t u;
    next = utf_t::next (next, end, &u, replacement);
    buffer->context[1][buffer->context_len[1]++] = u;
  }

  buffer->content_type = HB_BUFFER_CONTENT_TYPE_UNICODE;
}

/**
 * buffer_add_utf8:
 * @buffer: An #buffer_t
 * @text: (array length=text_length) (element-type uint8_t): An array of UTF-8
 *               characters to append.
 * @text_length: The length of the @text, or -1 if it is `NULL` terminated.
 * @item_offset: The offset of the first character to add to the @buffer.
 * @item_length: The number of characters to add to the @buffer, or -1 for the
 *               end of @text (assuming it is `NULL` terminated).
 *
 * See buffer_add_codepoints().
 *
 * Replaces invalid UTF-8 characters with the @buffer replacement code point,
 * see buffer_set_replacement_codepoint().
 *
 * Since: 0.9.2
 **/
void
buffer_add_utf8 (buffer_t  *buffer,
		    const char   *text,
		    int           text_length,
		    unsigned int  item_offset,
		    int           item_length)
{
  buffer_add_utf<utf8_t> (buffer, (const uint8_t *) text, text_length, item_offset, item_length);
}

/**
 * buffer_add_utf16:
 * @buffer: An #buffer_t
 * @text: (array length=text_length): An array of UTF-16 characters to append
 * @text_length: The length of the @text, or -1 if it is `NULL` terminated
 * @item_offset: The offset of the first character to add to the @buffer
 * @item_length: The number of characters to add to the @buffer, or -1 for the
 *               end of @text (assuming it is `NULL` terminated)
 *
 * See buffer_add_codepoints().
 *
 * Replaces invalid UTF-16 characters with the @buffer replacement code point,
 * see buffer_set_replacement_codepoint().
 *
 * Since: 0.9.2
 **/
void
buffer_add_utf16 (buffer_t    *buffer,
		     const uint16_t *text,
		     int             text_length,
		     unsigned int    item_offset,
		     int             item_length)
{
  buffer_add_utf<utf16_t> (buffer, text, text_length, item_offset, item_length);
}

/**
 * buffer_add_utf32:
 * @buffer: An #buffer_t
 * @text: (array length=text_length): An array of UTF-32 characters to append
 * @text_length: The length of the @text, or -1 if it is `NULL` terminated
 * @item_offset: The offset of the first character to add to the @buffer
 * @item_length: The number of characters to add to the @buffer, or -1 for the
 *               end of @text (assuming it is `NULL` terminated)
 *
 * See buffer_add_codepoints().
 *
 * Replaces invalid UTF-32 characters with the @buffer replacement code point,
 * see buffer_set_replacement_codepoint().
 *
 * Since: 0.9.2
 **/
void
buffer_add_utf32 (buffer_t    *buffer,
		     const uint32_t *text,
		     int             text_length,
		     unsigned int    item_offset,
		     int             item_length)
{
  buffer_add_utf<utf32_t> (buffer, text, text_length, item_offset, item_length);
}

/**
 * buffer_add_latin1:
 * @buffer: An #buffer_t
 * @text: (array length=text_length) (element-type uint8_t): an array of UTF-8
 *               characters to append
 * @text_length: the length of the @text, or -1 if it is `NULL` terminated
 * @item_offset: the offset of the first character to add to the @buffer
 * @item_length: the number of characters to add to the @buffer, or -1 for the
 *               end of @text (assuming it is `NULL` terminated)
 *
 * Similar to buffer_add_codepoints(), but allows only access to first 256
 * Unicode code points that can fit in 8-bit strings.
 *
 * <note>Has nothing to do with non-Unicode Latin-1 encoding.</note>
 *
 * Since: 0.9.39
 **/
void
buffer_add_latin1 (buffer_t   *buffer,
		      const uint8_t *text,
		      int            text_length,
		      unsigned int   item_offset,
		      int            item_length)
{
  buffer_add_utf<latin1_t> (buffer, text, text_length, item_offset, item_length);
}

/**
 * buffer_add_codepoints:
 * @buffer: a #buffer_t to append characters to.
 * @text: (array length=text_length): an array of Unicode code points to append.
 * @text_length: the length of the @text, or -1 if it is `NULL` terminated.
 * @item_offset: the offset of the first code point to add to the @buffer.
 * @item_length: the number of code points to add to the @buffer, or -1 for the
 *               end of @text (assuming it is `NULL` terminated).
 *
 * Appends characters from @text array to @buffer. The @item_offset is the
 * position of the first character from @text that will be appended, and
 * @item_length is the number of character. When shaping part of a larger text
 * (e.g. a run of text from a paragraph), instead of passing just the substring
 * corresponding to the run, it is preferable to pass the whole
 * paragraph and specify the run start and length as @item_offset and
 * @item_length, respectively, to give HarfBuzz the full context to be able,
 * for example, to do cross-run Arabic shaping or properly handle combining
 * marks at stat of run.
 *
 * This function does not check the validity of @text, it is up to the caller
 * to ensure it contains a valid Unicode scalar values.  In contrast,
 * buffer_add_utf32() can be used that takes similar input but performs
 * sanity-check on the input.
 *
 * Since: 0.9.31
 **/
void
buffer_add_codepoints (buffer_t          *buffer,
			  const codepoint_t *text,
			  int                   text_length,
			  unsigned int          item_offset,
			  int                   item_length)
{
  buffer_add_utf<utf32_novalidate_t> (buffer, text, text_length, item_offset, item_length);
}


/**
 * buffer_append:
 * @buffer: An #buffer_t
 * @source: source #buffer_t
 * @start: start index into source buffer to copy.  Use 0 to copy from start of buffer.
 * @end: end index into source buffer to copy.  Use @HB_FEATURE_GLOBAL_END to copy to end of buffer.
 *
 * Append (part of) contents of another buffer to this buffer.
 *
 * Since: 1.5.0
 **/
HB_EXTERN void
buffer_append (buffer_t *buffer,
		  const buffer_t *source,
		  unsigned int start,
		  unsigned int end)
{
  assert (!buffer->have_output && !source->have_output);
  assert (buffer->have_positions == source->have_positions ||
	  !buffer->len || !source->len);
  assert (buffer->content_type == source->content_type ||
	  !buffer->len || !source->len);

  if (end > source->len)
    end = source->len;
  if (start > end)
    start = end;
  if (start == end)
    return;

  if (buffer->len + (end - start) < buffer->len) /* Overflows. */
  {
    buffer->successful = false;
    return;
  }

  unsigned int orig_len = buffer->len;
  buffer_set_length (buffer, buffer->len + (end - start));
  if (unlikely (!buffer->successful))
    return;

  if (!orig_len)
    buffer->content_type = source->content_type;
  if (!buffer->have_positions && source->have_positions)
    buffer->clear_positions ();

  segment_properties_overlay (&buffer->props, &source->props);

  memcpy (buffer->info + orig_len, source->info + start, (end - start) * sizeof (buffer->info[0]));
  if (buffer->have_positions)
    memcpy (buffer->pos + orig_len, source->pos + start, (end - start) * sizeof (buffer->pos[0]));

  if (source->content_type == HB_BUFFER_CONTENT_TYPE_UNICODE)
  {
    /* See similar logic in add_utf. */

    /* pre-context */
    if (!orig_len && start + source->context_len[0] > 0)
    {
      buffer->clear_context (0);
      while (start > 0 && buffer->context_len[0] < buffer->CONTEXT_LENGTH)
	buffer->context[0][buffer->context_len[0]++] = source->info[--start].codepoint;
      for (auto i = 0u; i < source->context_len[0] && buffer->context_len[0] < buffer->CONTEXT_LENGTH; i++)
	buffer->context[0][buffer->context_len[0]++] = source->context[0][i];
    }

    /* post-context */
    buffer->clear_context (1);
    while (end < source->len && buffer->context_len[1] < buffer->CONTEXT_LENGTH)
      buffer->context[1][buffer->context_len[1]++] = source->info[end++].codepoint;
    for (auto i = 0u; i < source->context_len[1] && buffer->context_len[1] < buffer->CONTEXT_LENGTH; i++)
      buffer->context[1][buffer->context_len[1]++] = source->context[1][i];
  }
}


static int
compare_info_codepoint (const glyph_info_t *pa,
			const glyph_info_t *pb)
{
  return (int) pb->codepoint - (int) pa->codepoint;
}

static inline void
normalize_glyphs_cluster (buffer_t *buffer,
			  unsigned int start,
			  unsigned int end,
			  bool backward)
{
  glyph_position_t *pos = buffer->pos;

  /* Total cluster advance */
  position_t total_x_advance = 0, total_y_advance = 0;
  for (unsigned int i = start; i < end; i++)
  {
    total_x_advance += pos[i].x_advance;
    total_y_advance += pos[i].y_advance;
  }

  position_t x_advance = 0, y_advance = 0;
  for (unsigned int i = start; i < end; i++)
  {
    pos[i].x_offset += x_advance;
    pos[i].y_offset += y_advance;

    x_advance += pos[i].x_advance;
    y_advance += pos[i].y_advance;

    pos[i].x_advance = 0;
    pos[i].y_advance = 0;
  }

  if (backward)
  {
    /* Transfer all cluster advance to the last glyph. */
    pos[end - 1].x_advance = total_x_advance;
    pos[end - 1].y_advance = total_y_advance;

    stable_sort (buffer->info + start, end - start - 1, compare_info_codepoint, buffer->pos + start);
  } else {
    /* Transfer all cluster advance to the first glyph. */
    pos[start].x_advance += total_x_advance;
    pos[start].y_advance += total_y_advance;
    for (unsigned int i = start + 1; i < end; i++) {
      pos[i].x_offset -= total_x_advance;
      pos[i].y_offset -= total_y_advance;
    }
    stable_sort (buffer->info + start + 1, end - start - 1, compare_info_codepoint, buffer->pos + start + 1);
  }
}

/**
 * buffer_normalize_glyphs:
 * @buffer: An #buffer_t
 *
 * Reorders a glyph buffer to have canonical in-cluster glyph order / position.
 * The resulting clusters should behave identical to pre-reordering clusters.
 *
 * <note>This has nothing to do with Unicode normalization.</note>
 *
 * Since: 0.9.2
 **/
void
buffer_normalize_glyphs (buffer_t *buffer)
{
  assert (buffer->have_positions);

  buffer->assert_glyphs ();

  bool backward = HB_DIRECTION_IS_BACKWARD (buffer->props.direction);

  foreach_cluster (buffer, start, end)
    normalize_glyphs_cluster (buffer, start, end, backward);
}

void
buffer_t::sort (unsigned int start, unsigned int end, int(*compar)(const glyph_info_t *, const glyph_info_t *))
{
  assert (!have_positions);
  for (unsigned int i = start + 1; i < end; i++)
  {
    unsigned int j = i;
    while (j > start && compar (&info[j - 1], &info[i]) > 0)
      j--;
    if (i == j)
      continue;
    /* Move item i to occupy place for item j, shift what's in between. */
    merge_clusters (j, i + 1);
    {
      glyph_info_t t = info[i];
      memmove (&info[j + 1], &info[j], (i - j) * sizeof (glyph_info_t));
      info[j] = t;
    }
  }
}


/*
 * Comparing buffers.
 */

/**
 * buffer_diff:
 * @buffer: a buffer.
 * @reference: other buffer to compare to.
 * @dottedcircle_glyph: glyph id of U+25CC DOTTED CIRCLE, or (codepoint_t) -1.
 * @position_fuzz: allowed absolute difference in position values.
 *
 * If dottedcircle_glyph is (codepoint_t) -1 then #HB_BUFFER_DIFF_FLAG_DOTTED_CIRCLE_PRESENT
 * and #HB_BUFFER_DIFF_FLAG_NOTDEF_PRESENT are never returned.  This should be used by most
 * callers if just comparing two buffers is needed.
 *
 * Since: 1.5.0
 **/
buffer_diff_flags_t
buffer_diff (buffer_t *buffer,
		buffer_t *reference,
		codepoint_t dottedcircle_glyph,
		unsigned int position_fuzz)
{
  if (buffer->content_type != reference->content_type && buffer->len && reference->len)
    return HB_BUFFER_DIFF_FLAG_CONTENT_TYPE_MISMATCH;

  buffer_diff_flags_t result = HB_BUFFER_DIFF_FLAG_EQUAL;
  bool contains = dottedcircle_glyph != (codepoint_t) -1;

  unsigned int count = reference->len;

  if (buffer->len != count)
  {
    /*
     * we can't compare glyph-by-glyph, but we do want to know if there
     * are .notdef or dottedcircle glyphs present in the reference buffer
     */
    const glyph_info_t *info = reference->info;
    unsigned int i;
    for (i = 0; i < count; i++)
    {
      if (contains && info[i].codepoint == dottedcircle_glyph)
	result |= HB_BUFFER_DIFF_FLAG_DOTTED_CIRCLE_PRESENT;
      if (contains && info[i].codepoint == 0)
	result |= HB_BUFFER_DIFF_FLAG_NOTDEF_PRESENT;
    }
    result |= HB_BUFFER_DIFF_FLAG_LENGTH_MISMATCH;
    return buffer_diff_flags_t (result);
  }

  if (!count)
    return buffer_diff_flags_t (result);

  const glyph_info_t *buf_info = buffer->info;
  const glyph_info_t *ref_info = reference->info;
  for (unsigned int i = 0; i < count; i++)
  {
    if (buf_info->codepoint != ref_info->codepoint)
      result |= HB_BUFFER_DIFF_FLAG_CODEPOINT_MISMATCH;
    if (buf_info->cluster != ref_info->cluster)
      result |= HB_BUFFER_DIFF_FLAG_CLUSTER_MISMATCH;
    if ((buf_info->mask ^ ref_info->mask) & HB_GLYPH_FLAG_DEFINED)
      result |= HB_BUFFER_DIFF_FLAG_GLYPH_FLAGS_MISMATCH;
    if (contains && ref_info->codepoint == dottedcircle_glyph)
      result |= HB_BUFFER_DIFF_FLAG_DOTTED_CIRCLE_PRESENT;
    if (contains && ref_info->codepoint == 0)
      result |= HB_BUFFER_DIFF_FLAG_NOTDEF_PRESENT;
    buf_info++;
    ref_info++;
  }

  if (buffer->content_type == HB_BUFFER_CONTENT_TYPE_GLYPHS)
  {
    assert (buffer->have_positions);
    const glyph_position_t *buf_pos = buffer->pos;
    const glyph_position_t *ref_pos = reference->pos;
    for (unsigned int i = 0; i < count; i++)
    {
      if ((unsigned int) abs (buf_pos->x_advance - ref_pos->x_advance) > position_fuzz ||
	  (unsigned int) abs (buf_pos->y_advance - ref_pos->y_advance) > position_fuzz ||
	  (unsigned int) abs (buf_pos->x_offset - ref_pos->x_offset) > position_fuzz ||
	  (unsigned int) abs (buf_pos->y_offset - ref_pos->y_offset) > position_fuzz)
      {
	result |= HB_BUFFER_DIFF_FLAG_POSITION_MISMATCH;
	break;
      }
      buf_pos++;
      ref_pos++;
    }
  }

  return result;
}


/*
 * Debugging.
 */

#ifndef HB_NO_BUFFER_MESSAGE
/**
 * buffer_set_message_func:
 * @buffer: An #buffer_t
 * @func: (closure user_data) (destroy destroy) (scope notified): Callback function
 * @user_data: (nullable): Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #buffer_message_func_t.
 *
 * Since: 1.1.3
 **/
void
buffer_set_message_func (buffer_t *buffer,
			    buffer_message_func_t func,
			    void *user_data, destroy_func_t destroy)
{
  if (unlikely (object_is_immutable (buffer)))
  {
    if (destroy)
      destroy (user_data);
    return;
  }

  if (buffer->message_destroy)
    buffer->message_destroy (buffer->message_data);

  if (func) {
    buffer->message_func = func;
    buffer->message_data = user_data;
    buffer->message_destroy = destroy;
  } else {
    buffer->message_func = nullptr;
    buffer->message_data = nullptr;
    buffer->message_destroy = nullptr;
  }
}
bool
buffer_t::message_impl (font_t *font, const char *fmt, va_list ap)
{
  assert (!have_output || (out_info == info && out_len == idx));

  message_depth++;

  char buf[100];
  vsnprintf (buf, sizeof (buf), fmt, ap);
  bool ret = (bool) this->message_func (this, font, buf, this->message_data);

  message_depth--;

  return ret;
}
#endif
