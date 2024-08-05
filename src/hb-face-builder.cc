/*
 * Copyright © 2009  Red Hat, Inc.
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
 * Red Hat Author(s): Behdad Esfahbod
 * Google Author(s): Behdad Esfahbod
 */

#include "hb.hh"

#include "hb-face.hh"

#include "hb-map.hh"
#include "hb-open-file.hh"
#include "hb-serialize.hh"


/*
 * face-builder: A face that has add_table().
 */

struct face_table_info_t
{
  blob_t* data;
  signed order;
};

struct face_builder_data_t
{
  hashmap_t<tag_t, face_table_info_t> tables;
};

static int compare_entries (const void* pa, const void* pb)
{
  const auto& a = * (const pair_t<tag_t, face_table_info_t> *) pa;
  const auto& b = * (const pair_t<tag_t, face_table_info_t> *) pb;

  /* Order by blob size first (smallest to largest) and then table tag */

  if (a.second.order != b.second.order)
    return a.second.order < b.second.order ? -1 : +1;

  if (a.second.data->length != b.second.data->length)
    return a.second.data->length < b.second.data->length ? -1 : +1;

  return a.first < b.first ? -1 : a.first == b.first ? 0 : +1;
}

static face_builder_data_t *
_face_builder_data_create ()
{
  face_builder_data_t *data = (face_builder_data_t *) calloc (1, sizeof (face_builder_data_t));
  if (unlikely (!data))
    return nullptr;

  data->tables.init ();

  return data;
}

static void
_face_builder_data_destroy (void *user_data)
{
  face_builder_data_t *data = (face_builder_data_t *) user_data;

  for (auto info : data->tables.values())
    blob_destroy (info.data);

  data->tables.fini ();

  free (data);
}

static blob_t *
_face_builder_data_reference_blob (face_builder_data_t *data)
{

  unsigned int table_count = data->tables.get_population ();
  unsigned int face_length = table_count * 16 + 12;

  for (auto info : data->tables.values())
    face_length += ceil_to_4 (blob_get_length (info.data));

  char *buf = (char *) malloc (face_length);
  if (unlikely (!buf))
    return nullptr;

  serialize_context_t c (buf, face_length);
  c.propagate_error (data->tables);
  OT::OpenTypeFontFile *f = c.start_serialize<OT::OpenTypeFontFile> ();

  bool is_cff = (data->tables.has (HB_TAG ('C','F','F',' '))
                 || data->tables.has (HB_TAG ('C','F','F','2')));
  tag_t sfnt_tag = is_cff ? OT::OpenTypeFontFile::CFFTag : OT::OpenTypeFontFile::TrueTypeTag;

  // Sort the tags so that produced face is deterministic.
  vector_t<pair_t <tag_t, face_table_info_t>> sorted_entries;
  data->tables.iter () | sink (sorted_entries);
  if (unlikely (sorted_entries.in_error ()))
  {
    free (buf);
    return nullptr;
  }

  sorted_entries.qsort (compare_entries);

  bool ret = f->serialize_single (&c,
                                  sfnt_tag,
                                  + sorted_entries.iter()
                                  | map ([&] (pair_t<tag_t, face_table_info_t> _) {
                                    return pair_t<tag_t, blob_t*> (_.first, _.second.data);
                                  }));

  c.end_serialize ();

  if (unlikely (!ret))
  {
    free (buf);
    return nullptr;
  }

  return blob_create (buf, face_length, HB_MEMORY_MODE_WRITABLE, buf, free);
}

static blob_t *
_face_builder_reference_table (face_t *face HB_UNUSED, tag_t tag, void *user_data)
{
  face_builder_data_t *data = (face_builder_data_t *) user_data;

  if (!tag)
    return _face_builder_data_reference_blob (data);

  return blob_reference (data->tables[tag].data);
}


/**
 * face_builder_create:
 *
 * Creates a #face_t that can be used with face_builder_add_table().
 * After tables are added to the face, it can be compiled to a binary
 * font file by calling face_reference_blob().
 *
 * Return value: (transfer full): New face.
 *
 * Since: 1.9.0
 **/
face_t *
face_builder_create ()
{
  face_builder_data_t *data = _face_builder_data_create ();
  if (unlikely (!data)) return face_get_empty ();

  return face_create_for_tables (_face_builder_reference_table,
				    data,
				    _face_builder_data_destroy);
}

/**
 * face_builder_add_table:
 * @face: A face object created with face_builder_create()
 * @tag: The #tag_t of the table to add
 * @blob: The blob containing the table data to add
 *
 * Add table for @tag with data provided by @blob to the face.  @face must
 * be created using face_builder_create().
 *
 * Since: 1.9.0
 **/
bool_t
face_builder_add_table (face_t *face, tag_t tag, blob_t *blob)
{
  if (unlikely (face->destroy != (destroy_func_t) _face_builder_data_destroy))
    return false;

  if (tag == HB_MAP_VALUE_INVALID)
    return false;

  face_builder_data_t *data = (face_builder_data_t *) face->user_data;

  blob_t* previous = data->tables.get (tag).data;
  if (!data->tables.set (tag, face_table_info_t {blob_reference (blob), -1}))
  {
    blob_destroy (blob);
    return false;
  }

  blob_destroy (previous);
  return true;
}

/**
 * face_builder_sort_tables:
 * @face: A face object created with face_builder_create()
 * @tags: (array zero-terminated=1): ordered list of table tags terminated by
 *   %HB_TAG_NONE
 *
 * Set the ordering of tables for serialization. Any tables not
 * specified in the tags list will be ordered after the tables in
 * tags, ordered by the default sort ordering.
 *
 * Since: 5.3.0
 **/
void
face_builder_sort_tables (face_t *face,
                             const tag_t  *tags)
{
  if (unlikely (face->destroy != (destroy_func_t) _face_builder_data_destroy))
    return;

  face_builder_data_t *data = (face_builder_data_t *) face->user_data;

  // Sort all unspecified tables after any specified tables.
  for (auto& info : data->tables.values_ref())
    info.order = (unsigned) -1;

  signed order = 0;
  for (const tag_t* tag = tags;
       *tag;
       tag++)
  {
    face_table_info_t* info;
    if (!data->tables.has (*tag, &info)) continue;
    info->order = order++;
  }
}
