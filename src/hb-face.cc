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
#include "hb-blob.hh"
#include "hb-open-file.hh"
#include "hb-ot-face.hh"
#include "hb-ot-cmap-table.hh"


/**
 * SECTION:hb-face
 * @title: hb-face
 * @short_description: Font face objects
 * @include: hb.h
 *
 * A font face is an object that represents a single face from within a
 * font family.
 *
 * More precisely, a font face represents a single face in a binary font file.
 * Font faces are typically built from a binary blob and a face index.
 * Font faces are used to create fonts.
 *
 * A font face can be created from a binary blob using face_create().
 * The face index is used to select a face from a binary blob that contains
 * multiple faces.  For example, a binary blob that contains both a regular
 * and a bold face can be used to create two font faces, one for each face
 * index.
 **/


/**
 * face_count:
 * @blob: a blob.
 *
 * Fetches the number of faces in a blob.
 *
 * Return value: Number of faces in @blob
 *
 * Since: 1.7.7
 **/
unsigned int
face_count (blob_t *blob)
{
  if (unlikely (!blob))
    return 0;

  /* TODO We shouldn't be sanitizing blob.  Port to run sanitizer and return if not sane. */
  /* Make API signature const after. */
  blob_t *sanitized = sanitize_context_t ().sanitize_blob<OT::OpenTypeFontFile> (blob_reference (blob));
  const OT::OpenTypeFontFile& ot = *sanitized->as<OT::OpenTypeFontFile> ();
  unsigned int ret = ot.get_face_count ();
  blob_destroy (sanitized);

  return ret;
}

/*
 * face_t
 */

DEFINE_NULL_INSTANCE (face_t) =
{
  HB_OBJECT_HEADER_STATIC,

  nullptr, /* reference_table_func */
  nullptr, /* user_data */
  nullptr, /* destroy */

  0,    /* index */
  1000, /* upem */
  0,    /* num_glyphs */

  /* Zero for the rest is fine. */
};


/**
 * face_create_for_tables:
 * @reference_table_func: (closure user_data) (destroy destroy) (scope notified): Table-referencing function
 * @user_data: A pointer to the user data
 * @destroy: (nullable): A callback to call when @data is not needed anymore
 *
 * Variant of face_create(), built for those cases where it is more
 * convenient to provide data for individual tables instead of the whole font
 * data. With the caveat that face_get_table_tags() does not currently work
 * with faces created this way.
 *
 * Creates a new face object from the specified @user_data and @reference_table_func,
 * with the @destroy callback.
 *
 * Return value: (transfer full): The new face object
 *
 * Since: 0.9.2
 **/
face_t *
face_create_for_tables (reference_table_func_t  reference_table_func,
			   void                      *user_data,
			   destroy_func_t          destroy)
{
  face_t *face;

  if (!reference_table_func || !(face = object_create<face_t> ())) {
    if (destroy)
      destroy (user_data);
    return face_get_empty ();
  }

  face->reference_table_func = reference_table_func;
  face->user_data = user_data;
  face->destroy = destroy;

  face->num_glyphs = -1;

  face->data.init0 (face);
  face->table.init0 (face);

  return face;
}


typedef struct face_for_data_closure_t {
  blob_t *blob;
  uint16_t  index;
} face_for_data_closure_t;

static face_for_data_closure_t *
_face_for_data_closure_create (blob_t *blob, unsigned int index)
{
  face_for_data_closure_t *closure;

  closure = (face_for_data_closure_t *) calloc (1, sizeof (face_for_data_closure_t));
  if (unlikely (!closure))
    return nullptr;

  closure->blob = blob;
  closure->index = (uint16_t) (index & 0xFFFFu);

  return closure;
}

static void
_face_for_data_closure_destroy (void *data)
{
  face_for_data_closure_t *closure = (face_for_data_closure_t *) data;

  blob_destroy (closure->blob);
  free (closure);
}

static blob_t *
_face_for_data_reference_table (face_t *face HB_UNUSED, tag_t tag, void *user_data)
{
  face_for_data_closure_t *data = (face_for_data_closure_t *) user_data;

  if (tag == HB_TAG_NONE)
    return blob_reference (data->blob);

  const OT::OpenTypeFontFile &ot_file = *data->blob->as<OT::OpenTypeFontFile> ();
  unsigned int base_offset;
  const OT::OpenTypeFontFace &ot_face = ot_file.get_face (data->index, &base_offset);

  const OT::OpenTypeTable &table = ot_face.get_table_by_tag (tag);

  blob_t *blob = blob_create_sub_blob (data->blob, base_offset + table.offset, table.length);

  return blob;
}

/**
 * face_create:
 * @blob: #blob_t to work upon
 * @index: The index of the face within @blob
 *
 * Constructs a new face object from the specified blob and
 * a face index into that blob.
 *
 * The face index is used for blobs of file formats such as TTC and
 * DFont that can contain more than one face.  Face indices within
 * such collections are zero-based.
 *
 * <note>Note: If the blob font format is not a collection, @index
 * is ignored.  Otherwise, only the lower 16-bits of @index are used.
 * The unmodified @index can be accessed via face_get_index().</note>
 *
 * <note>Note: The high 16-bits of @index, if non-zero, are used by
 * font_create() to load named-instances in variable fonts.  See
 * font_create() for details.</note>
 *
 * Return value: (transfer full): The new face object
 *
 * Since: 0.9.2
 **/
face_t *
face_create (blob_t    *blob,
		unsigned int  index)
{
  face_t *face;

  if (unlikely (!blob))
    blob = blob_get_empty ();

  blob = sanitize_context_t ().sanitize_blob<OT::OpenTypeFontFile> (blob_reference (blob));

  face_for_data_closure_t *closure = _face_for_data_closure_create (blob, index);

  if (unlikely (!closure))
  {
    blob_destroy (blob);
    return face_get_empty ();
  }

  face = face_create_for_tables (_face_for_data_reference_table,
				    closure,
				    _face_for_data_closure_destroy);

  face->index = index;

  return face;
}

/**
 * face_get_empty:
 *
 * Fetches the singleton empty face object.
 *
 * Return value: (transfer full): The empty face object
 *
 * Since: 0.9.2
 **/
face_t *
face_get_empty ()
{
  return const_cast<face_t *> (&Null (face_t));
}


/**
 * face_reference: (skip)
 * @face: A face object
 *
 * Increases the reference count on a face object.
 *
 * Return value: The @face object
 *
 * Since: 0.9.2
 **/
face_t *
face_reference (face_t *face)
{
  return object_reference (face);
}

/**
 * face_destroy: (skip)
 * @face: A face object
 *
 * Decreases the reference count on a face object. When the
 * reference count reaches zero, the face is destroyed,
 * freeing all memory.
 *
 * Since: 0.9.2
 **/
void
face_destroy (face_t *face)
{
  if (!object_destroy (face)) return;

#ifndef HB_NO_SHAPER
  for (face_t::plan_node_t *node = face->shape_plans; node; )
  {
    face_t::plan_node_t *next = node->next;
    shape_plan_destroy (node->shape_plan);
    free (node);
    node = next;
  }
#endif

  face->data.fini ();
  face->table.fini ();

  if (face->destroy)
    face->destroy (face->user_data);

  free (face);
}

/**
 * face_set_user_data: (skip)
 * @face: A face object
 * @key: The user-data key to set
 * @data: A pointer to the user data
 * @destroy: (nullable): A callback to call when @data is not needed anymore
 * @replace: Whether to replace an existing data with the same key
 *
 * Attaches a user-data key/data pair to the given face object.
 *
 * Return value: `true` if success, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
face_set_user_data (face_t          *face,
		       user_data_key_t *key,
		       void *              data,
		       destroy_func_t   destroy,
		       bool_t           replace)
{
  return object_set_user_data (face, key, data, destroy, replace);
}

/**
 * face_get_user_data: (skip)
 * @face: A face object
 * @key: The user-data key to query
 *
 * Fetches the user data associated with the specified key,
 * attached to the specified face object.
 *
 * Return value: (transfer none): A pointer to the user data
 *
 * Since: 0.9.2
 **/
void *
face_get_user_data (const face_t    *face,
		       user_data_key_t *key)
{
  return object_get_user_data (face, key);
}

/**
 * face_make_immutable:
 * @face: A face object
 *
 * Makes the given face object immutable.
 *
 * Since: 0.9.2
 **/
void
face_make_immutable (face_t *face)
{
  if (object_is_immutable (face))
    return;

  object_make_immutable (face);
}

/**
 * face_is_immutable:
 * @face: A face object
 *
 * Tests whether the given face object is immutable.
 *
 * Return value: `true` is @face is immutable, `false` otherwise
 *
 * Since: 0.9.2
 **/
bool_t
face_is_immutable (const face_t *face)
{
  return object_is_immutable (face);
}


/**
 * face_reference_table:
 * @face: A face object
 * @tag: The #tag_t of the table to query
 *
 * Fetches a reference to the specified table within
 * the specified face.
 *
 * Return value: (transfer full): A pointer to the @tag table within @face
 *
 * Since: 0.9.2
 **/
blob_t *
face_reference_table (const face_t *face,
			 tag_t tag)
{
  if (unlikely (tag == HB_TAG_NONE))
    return blob_get_empty ();

  return face->reference_table (tag);
}

/**
 * face_reference_blob:
 * @face: A face object
 *
 * Fetches a pointer to the binary blob that contains the
 * specified face. Returns an empty blob if referencing face data is not
 * possible.
 *
 * Return value: (transfer full): A pointer to the blob for @face
 *
 * Since: 0.9.2
 **/
blob_t *
face_reference_blob (face_t *face)
{
  return face->reference_table (HB_TAG_NONE);
}

/**
 * face_set_index:
 * @face: A face object
 * @index: The index to assign
 *
 * Assigns the specified face-index to @face. Fails if the
 * face is immutable.
 *
 * <note>Note: changing the index has no effect on the face itself
 * This only changes the value returned by face_get_index().</note>
 *
 * Since: 0.9.2
 **/
void
face_set_index (face_t    *face,
		   unsigned int  index)
{
  if (object_is_immutable (face))
    return;

  face->index = index;
}

/**
 * face_get_index:
 * @face: A face object
 *
 * Fetches the face-index corresponding to the given face.
 *
 * <note>Note: face indices within a collection are zero-based.</note>
 *
 * Return value: The index of @face.
 *
 * Since: 0.9.2
 **/
unsigned int
face_get_index (const face_t *face)
{
  return face->index;
}

/**
 * face_set_upem:
 * @face: A face object
 * @upem: The units-per-em value to assign
 *
 * Sets the units-per-em (upem) for a face object to the specified value.
 *
 * This API is used in rare circumstances.
 *
 * Since: 0.9.2
 **/
void
face_set_upem (face_t    *face,
		  unsigned int  upem)
{
  if (object_is_immutable (face))
    return;

  face->upem = upem;
}

/**
 * face_get_upem:
 * @face: A face object
 *
 * Fetches the units-per-em (UPEM) value of the specified face object.
 *
 * Typical UPEM values for fonts are 1000, or 2048, but any value
 * in between 16 and 16,384 is allowed for OpenType fonts.
 *
 * Return value: The upem value of @face
 *
 * Since: 0.9.2
 **/
unsigned int
face_get_upem (const face_t *face)
{
  return face->get_upem ();
}

/**
 * face_set_glyph_count:
 * @face: A face object
 * @glyph_count: The glyph-count value to assign
 *
 * Sets the glyph count for a face object to the specified value.
 *
 * This API is used in rare circumstances.
 *
 * Since: 0.9.7
 **/
void
face_set_glyph_count (face_t    *face,
			 unsigned int  glyph_count)
{
  if (object_is_immutable (face))
    return;

  face->num_glyphs = glyph_count;
}

/**
 * face_get_glyph_count:
 * @face: A face object
 *
 * Fetches the glyph-count value of the specified face object.
 *
 * Return value: The glyph-count value of @face
 *
 * Since: 0.9.7
 **/
unsigned int
face_get_glyph_count (const face_t *face)
{
  return face->get_num_glyphs ();
}

/**
 * face_get_table_tags:
 * @face: A face object
 * @start_offset: The index of first table tag to retrieve
 * @table_count: (inout): Input = the maximum number of table tags to return;
 *                Output = the actual number of table tags returned (may be zero)
 * @table_tags: (out) (array length=table_count): The array of table tags found
 *
 * Fetches a list of all table tags for a face, if possible. The list returned will
 * begin at the offset provided
 *
 * Return value: Total number of tables, or zero if it is not possible to list
 *
 * Since: 1.6.0
 **/
unsigned int
face_get_table_tags (const face_t *face,
			unsigned int  start_offset,
			unsigned int *table_count, /* IN/OUT */
			tag_t     *table_tags /* OUT */)
{
  if (face->destroy != (destroy_func_t) _face_for_data_closure_destroy)
  {
    if (table_count)
      *table_count = 0;
    return 0;
  }

  face_for_data_closure_t *data = (face_for_data_closure_t *) face->user_data;

  const OT::OpenTypeFontFile &ot_file = *data->blob->as<OT::OpenTypeFontFile> ();
  const OT::OpenTypeFontFace &ot_face = ot_file.get_face (data->index);

  return ot_face.get_table_tags (start_offset, table_count, table_tags);
}


/*
 * Character set.
 */


#ifndef HB_NO_FACE_COLLECT_UNICODES
/**
 * face_collect_unicodes:
 * @face: A face object
 * @out: (out): The set to add Unicode characters to
 *
 * Collects all of the Unicode characters covered by @face and adds
 * them to the #set_t set @out.
 *
 * Since: 1.9.0
 */
void
face_collect_unicodes (face_t *face,
			  set_t  *out)
{
  face->table.cmap->collect_unicodes (out, face->get_num_glyphs ());
}
/**
 * face_collect_nominal_glyph_mapping:
 * @face: A face object
 * @mapping: (out): The map to add Unicode-to-glyph mapping to
 * @unicodes: (nullable) (out): The set to add Unicode characters to, or `NULL`
 *
 * Collects the mapping from Unicode characters to nominal glyphs of the @face,
 * and optionally all of the Unicode characters covered by @face.
 *
 * Since: 7.0.0
 */
void
face_collect_nominal_glyph_mapping (face_t *face,
				       map_t  *mapping,
				       set_t  *unicodes)
{
  set_t stack_unicodes;
  if (!unicodes)
    unicodes = &stack_unicodes;
  face->table.cmap->collect_mapping (unicodes, mapping, face->get_num_glyphs ());
}
/**
 * face_collect_variation_selectors:
 * @face: A face object
 * @out: (out): The set to add Variation Selector characters to
 *
 * Collects all Unicode "Variation Selector" characters covered by @face and adds
 * them to the #set_t set @out.
 *
 * Since: 1.9.0
 */
void
face_collect_variation_selectors (face_t *face,
				     set_t  *out)
{
  face->table.cmap->collect_variation_selectors (out);
}
/**
 * face_collect_variation_unicodes:
 * @face: A face object
 * @variation_selector: The Variation Selector to query
 * @out: (out): The set to add Unicode characters to
 *
 * Collects all Unicode characters for @variation_selector covered by @face and adds
 * them to the #set_t set @out.
 *
 * Since: 1.9.0
 */
void
face_collect_variation_unicodes (face_t *face,
				    codepoint_t variation_selector,
				    set_t  *out)
{
  face->table.cmap->collect_variation_unicodes (variation_selector, out);
}
#endif
