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

#include "hb.hh"
#include "hb-shape-plan.hh"
#include "hb-shaper.hh"
#include "hb-font.hh"
#include "hb-buffer.hh"


#ifndef HB_NO_SHAPER

/**
 * SECTION:hb-shape-plan
 * @title: hb-shape-plan
 * @short_description: Object representing a shaping plan
 * @include: hb.h
 *
 * Shape plans are an internal mechanism. Each plan contains state
 * describing how HarfBuzz will shape a particular text segment, based on
 * the combination of segment properties and the capabilities in the
 * font face in use.
 *
 * Shape plans are not used for shaping directly, but can be queried to
 * access certain information about how shaping will perform, given a set
 * of specific input parameters (script, language, direction, features,
 * etc.).
 *
 * Most client programs will not need to deal with shape plans directly.
 **/


/*
 * shape_plan_key_t
 */

bool
shape_plan_key_t::init (bool                           copy,
			   face_t                     *face,
			   const segment_properties_t *props,
			   const feature_t            *user_features,
			   unsigned int                   num_user_features,
			   const int                     *coords,
			   unsigned int                   num_coords,
			   const char * const            *shaper_list)
{
  feature_t *features = nullptr;
  if (copy && num_user_features && !(features = (feature_t *) calloc (num_user_features, sizeof (feature_t))))
    goto bail;

  this->props = *props;
  this->num_user_features = num_user_features;
  this->user_features = copy ? features : user_features;
  if (copy && num_user_features)
  {
    memcpy (features, user_features, num_user_features * sizeof (feature_t));
    /* Make start/end uniform to easier catch bugs. */
    for (unsigned int i = 0; i < num_user_features; i++)
    {
      if (features[0].start != HB_FEATURE_GLOBAL_START)
	features[0].start = 1;
      if (features[0].end   != HB_FEATURE_GLOBAL_END)
	features[0].end   = 2;
    }
  }
  this->shaper_func = nullptr;
  this->shaper_name = nullptr;
#ifndef HB_NO_OT_SHAPE
  this->ot.init (face, coords, num_coords);
#endif

  /*
   * Choose shaper.
   */

#define HB_SHAPER_PLAN(shaper) \
	HB_STMT_START { \
	  if (face->data.shaper) \
	  { \
	    this->shaper_func = _##shaper##_shape; \
	    this->shaper_name = #shaper; \
	    return true; \
	  } \
	} HB_STMT_END

  if (unlikely (shaper_list))
  {
    for (; *shaper_list; shaper_list++)
      if (false)
	;
#define HB_SHAPER_IMPLEMENT(shaper) \
      else if (0 == strcmp (*shaper_list, #shaper)) \
	HB_SHAPER_PLAN (shaper);
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT
  }
  else
  {
    const HB_UNUSED shaper_entry_t *shapers = _shapers_get ();
    for (unsigned int i = 0; i < HB_SHAPERS_COUNT; i++)
      if (false)
	;
#define HB_SHAPER_IMPLEMENT(shaper) \
      else if (shapers[i].func == _##shaper##_shape) \
	HB_SHAPER_PLAN (shaper);
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT
  }
#undef HB_SHAPER_PLAN

bail:
  ::free (features);
  return false;
}

bool
shape_plan_key_t::user_features_match (const shape_plan_key_t *other)
{
  if (this->num_user_features != other->num_user_features)
    return false;
  for (unsigned int i = 0; i < num_user_features; i++)
  {
    if (this->user_features[i].tag   != other->user_features[i].tag   ||
	this->user_features[i].value != other->user_features[i].value ||
	(this->user_features[i].start == HB_FEATURE_GLOBAL_START &&
	 this->user_features[i].end   == HB_FEATURE_GLOBAL_END) !=
	(other->user_features[i].start == HB_FEATURE_GLOBAL_START &&
	 other->user_features[i].end   == HB_FEATURE_GLOBAL_END))
      return false;
  }
  return true;
}

bool
shape_plan_key_t::equal (const shape_plan_key_t *other)
{
  return segment_properties_equal (&this->props, &other->props) &&
	 this->user_features_match (other) &&
#ifndef HB_NO_OT_SHAPE
	 this->ot.equal (&other->ot) &&
#endif
	 this->shaper_func == other->shaper_func;
}


/*
 * shape_plan_t
 */


/**
 * shape_plan_create:
 * @face: #face_t to use
 * @props: The #segment_properties_t of the segment
 * @user_features: (array length=num_user_features): The list of user-selected features
 * @num_user_features: The number of user-selected features
 * @shaper_list: (array zero-terminated=1): List of shapers to try
 *
 * Constructs a shaping plan for a combination of @face, @user_features, @props,
 * and @shaper_list.
 *
 * Return value: (transfer full): The shaping plan
 *
 * Since: 0.9.7
 **/
shape_plan_t *
shape_plan_create (face_t                     *face,
		      const segment_properties_t *props,
		      const feature_t            *user_features,
		      unsigned int                   num_user_features,
		      const char * const            *shaper_list)
{
  return shape_plan_create2 (face, props,
				user_features, num_user_features,
				nullptr, 0,
				shaper_list);
}

/**
 * shape_plan_create2:
 * @face: #face_t to use
 * @props: The #segment_properties_t of the segment
 * @user_features: (array length=num_user_features): The list of user-selected features
 * @num_user_features: The number of user-selected features
 * @coords: (array length=num_coords): The list of variation-space coordinates
 * @num_coords: The number of variation-space coordinates
 * @shaper_list: (array zero-terminated=1): List of shapers to try
 *
 * The variable-font version of #shape_plan_create. 
 * Constructs a shaping plan for a combination of @face, @user_features, @props,
 * and @shaper_list, plus the variation-space coordinates @coords.
 *
 * Return value: (transfer full): The shaping plan
 *
 * Since: 1.4.0
 **/
shape_plan_t *
shape_plan_create2 (face_t                     *face,
		       const segment_properties_t *props,
		       const feature_t            *user_features,
		       unsigned int                   num_user_features,
		       const int                     *coords,
		       unsigned int                   num_coords,
		       const char * const            *shaper_list)
{
  DEBUG_MSG_FUNC (SHAPE_PLAN, nullptr,
		  "face=%p num_features=%u num_coords=%u shaper_list=%p",
		  face,
		  num_user_features,
		  num_coords,
		  shaper_list);

  if (unlikely (props->direction == HB_DIRECTION_INVALID))
    return shape_plan_get_empty ();

  shape_plan_t *shape_plan;

  if (unlikely (!props))
    goto bail;
  if (!(shape_plan = object_create<shape_plan_t> ()))
    goto bail;

  if (unlikely (!face))
    face = face_get_empty ();
  face_make_immutable (face);
  shape_plan->face_unsafe = face;

  if (unlikely (!shape_plan->key.init (true,
				       face,
				       props,
				       user_features,
				       num_user_features,
				       coords,
				       num_coords,
				       shaper_list)))
    goto bail2;
#ifndef HB_NO_OT_SHAPE
  if (unlikely (!shape_plan->ot.init0 (face, &shape_plan->key)))
    goto bail3;
#endif

  return shape_plan;

#ifndef HB_NO_OT_SHAPE
bail3:
#endif
  shape_plan->key.fini ();
bail2:
  free (shape_plan);
bail:
  return shape_plan_get_empty ();
}

/**
 * shape_plan_get_empty:
 *
 * Fetches the singleton empty shaping plan.
 *
 * Return value: (transfer full): The empty shaping plan
 *
 * Since: 0.9.7
 **/
shape_plan_t *
shape_plan_get_empty ()
{
  return const_cast<shape_plan_t *> (&Null (shape_plan_t));
}

/**
 * shape_plan_reference: (skip)
 * @shape_plan: A shaping plan
 *
 * Increases the reference count on the given shaping plan.
 *
 * Return value: (transfer full): @shape_plan
 *
 * Since: 0.9.7
 **/
shape_plan_t *
shape_plan_reference (shape_plan_t *shape_plan)
{
  return object_reference (shape_plan);
}

/**
 * shape_plan_destroy: (skip)
 * @shape_plan: A shaping plan
 *
 * Decreases the reference count on the given shaping plan. When the
 * reference count reaches zero, the shaping plan is destroyed,
 * freeing all memory.
 *
 * Since: 0.9.7
 **/
void
shape_plan_destroy (shape_plan_t *shape_plan)
{
  if (!object_destroy (shape_plan)) return;

  free (shape_plan);
}

/**
 * shape_plan_set_user_data: (skip)
 * @shape_plan: A shaping plan
 * @key: The user-data key to set
 * @data: A pointer to the user data
 * @destroy: (nullable): A callback to call when @data is not needed anymore
 * @replace: Whether to replace an existing data with the same key
 *
 * Attaches a user-data key/data pair to the given shaping plan. 
 *
 * Return value: `true` if success, `false` otherwise.
 *
 * Since: 0.9.7
 **/
bool_t
shape_plan_set_user_data (shape_plan_t    *shape_plan,
			     user_data_key_t *key,
			     void *              data,
			     destroy_func_t   destroy,
			     bool_t           replace)
{
  return object_set_user_data (shape_plan, key, data, destroy, replace);
}

/**
 * shape_plan_get_user_data: (skip)
 * @shape_plan: A shaping plan
 * @key: The user-data key to query
 *
 * Fetches the user data associated with the specified key, 
 * attached to the specified shaping plan.
 *
 * Return value: (transfer none): A pointer to the user data
 *
 * Since: 0.9.7
 **/
void *
shape_plan_get_user_data (const shape_plan_t *shape_plan,
			     user_data_key_t    *key)
{
  return object_get_user_data (shape_plan, key);
}

/**
 * shape_plan_get_shaper:
 * @shape_plan: A shaping plan
 *
 * Fetches the shaper from a given shaping plan.
 *
 * Return value: (transfer none): The shaper
 *
 * Since: 0.9.7
 **/
const char *
shape_plan_get_shaper (shape_plan_t *shape_plan)
{
  return shape_plan->key.shaper_name;
}


static bool
_shape_plan_execute_internal (shape_plan_t    *shape_plan,
				 font_t          *font,
				 buffer_t        *buffer,
				 const feature_t *features,
				 unsigned int        num_features)
{
  DEBUG_MSG_FUNC (SHAPE_PLAN, shape_plan,
		  "num_features=%u shaper_func=%p, shaper_name=%s",
		  num_features,
		  shape_plan->key.shaper_func,
		  shape_plan->key.shaper_name);

  if (unlikely (!buffer->len))
    return true;

  assert (!object_is_immutable (buffer));

  buffer->assert_unicode ();

  if (unlikely (!object_is_valid (shape_plan)))
    return false;

  assert (shape_plan->face_unsafe == font->face);
  assert (segment_properties_equal (&shape_plan->key.props, &buffer->props));

#define HB_SHAPER_EXECUTE(shaper) \
	HB_STMT_START { \
	  return font->data.shaper && \
		 _##shaper##_shape (shape_plan, font, buffer, features, num_features); \
	} HB_STMT_END

  if (false)
    ;
#define HB_SHAPER_IMPLEMENT(shaper) \
  else if (shape_plan->key.shaper_func == _##shaper##_shape) \
    HB_SHAPER_EXECUTE (shaper);
#include "hb-shaper-list.hh"
#undef HB_SHAPER_IMPLEMENT

#undef HB_SHAPER_EXECUTE

  return false;
}
/**
 * shape_plan_execute:
 * @shape_plan: A shaping plan
 * @font: The #font_t to use
 * @buffer: The #buffer_t to work upon
 * @features: (array length=num_features): Features to enable
 * @num_features: The number of features to enable
 *
 * Executes the given shaping plan on the specified buffer, using
 * the given @font and @features.
 *
 * Return value: `true` if success, `false` otherwise.
 *
 * Since: 0.9.7
 **/
bool_t
shape_plan_execute (shape_plan_t    *shape_plan,
		       font_t          *font,
		       buffer_t        *buffer,
		       const feature_t *features,
		       unsigned int        num_features)
{
  bool ret = _shape_plan_execute_internal (shape_plan, font, buffer,
					      features, num_features);

  if (ret && buffer->content_type == HB_BUFFER_CONTENT_TYPE_UNICODE)
    buffer->content_type = HB_BUFFER_CONTENT_TYPE_GLYPHS;

  return ret;
}


/*
 * Caching
 */

/**
 * shape_plan_create_cached:
 * @face: #face_t to use
 * @props: The #segment_properties_t of the segment
 * @user_features: (array length=num_user_features): The list of user-selected features
 * @num_user_features: The number of user-selected features
 * @shaper_list: (array zero-terminated=1): List of shapers to try
 *
 * Creates a cached shaping plan suitable for reuse, for a combination
 * of @face, @user_features, @props, and @shaper_list.
 *
 * Return value: (transfer full): The shaping plan
 *
 * Since: 0.9.7
 **/
shape_plan_t *
shape_plan_create_cached (face_t                     *face,
			     const segment_properties_t *props,
			     const feature_t            *user_features,
			     unsigned int                   num_user_features,
			     const char * const            *shaper_list)
{
  return shape_plan_create_cached2 (face, props,
				       user_features, num_user_features,
				       nullptr, 0,
				       shaper_list);
}

/**
 * shape_plan_create_cached2:
 * @face: #face_t to use
 * @props: The #segment_properties_t of the segment
 * @user_features: (array length=num_user_features): The list of user-selected features
 * @num_user_features: The number of user-selected features
 * @coords: (array length=num_coords): The list of variation-space coordinates
 * @num_coords: The number of variation-space coordinates
 * @shaper_list: (array zero-terminated=1): List of shapers to try
 *
 * The variable-font version of #shape_plan_create_cached. 
 * Creates a cached shaping plan suitable for reuse, for a combination
 * of @face, @user_features, @props, and @shaper_list, plus the
 * variation-space coordinates @coords.
 *
 * Return value: (transfer full): The shaping plan
 *
 * Since: 1.4.0
 **/
shape_plan_t *
shape_plan_create_cached2 (face_t                     *face,
			      const segment_properties_t *props,
			      const feature_t            *user_features,
			      unsigned int                   num_user_features,
			      const int                     *coords,
			      unsigned int                   num_coords,
			      const char * const            *shaper_list)
{
  DEBUG_MSG_FUNC (SHAPE_PLAN, nullptr,
		  "face=%p num_features=%u shaper_list=%p",
		  face,
		  num_user_features,
		  shaper_list);

retry:
  face_t::plan_node_t *cached_plan_nodes = face->shape_plans;

  bool dont_cache = !object_is_valid (face);

  if (likely (!dont_cache))
  {
    shape_plan_key_t key;
    if (!key.init (false,
		   face,
		   props,
		   user_features,
		   num_user_features,
		   coords,
		   num_coords,
		   shaper_list))
      return shape_plan_get_empty ();

    for (face_t::plan_node_t *node = cached_plan_nodes; node; node = node->next)
      if (node->shape_plan->key.equal (&key))
      {
	DEBUG_MSG_FUNC (SHAPE_PLAN, node->shape_plan, "fulfilled from cache");
	return shape_plan_reference (node->shape_plan);
      }
  }

  shape_plan_t *shape_plan = shape_plan_create2 (face, props,
						       user_features, num_user_features,
						       coords, num_coords,
						       shaper_list);

  if (unlikely (dont_cache))
    return shape_plan;

  face_t::plan_node_t *node = (face_t::plan_node_t *) calloc (1, sizeof (face_t::plan_node_t));
  if (unlikely (!node))
    return shape_plan;

  node->shape_plan = shape_plan;
  node->next = cached_plan_nodes;

  if (unlikely (!face->shape_plans.cmpexch (cached_plan_nodes, node)))
  {
    shape_plan_destroy (shape_plan);
    free (node);
    goto retry;
  }
  DEBUG_MSG_FUNC (SHAPE_PLAN, shape_plan, "inserted into cache");

  return shape_plan_reference (shape_plan);
}


#endif
