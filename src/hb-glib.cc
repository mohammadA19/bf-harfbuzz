/*
 * Copyright © 2009  Red Hat, Inc.
 * Copyright © 2011  Google, Inc.
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

#ifdef HAVE_GLIB

#include "hb-glib.h"

#include "hb-machinery.hh"


/**
 * SECTION:hb-glib
 * @title: hb-glib
 * @short_description: GLib integration
 * @include: hb-glib.h
 *
 * Functions for using HarfBuzz with the GLib library.
 *
 * HarfBuzz supports using GLib to provide Unicode data, by attaching
 * GLib functions to the virtual methods in a #unicode_funcs_t function
 * structure.
 **/


/**
 * glib_script_to_script:
 * @script: The GUnicodeScript identifier to query
 *
 * Fetches the #script_t script that corresponds to the
 * specified GUnicodeScript identifier.
 *
 * Return value: the #script_t script found
 *
 * Since: 0.9.38
 **/
script_t
glib_script_to_script (GUnicodeScript script)
{
  return (script_t) g_unicode_script_to_iso15924 (script);
}

/**
 * glib_script_from_script:
 * @script: The #script_t to query
 *
 * Fetches the GUnicodeScript identifier that corresponds to the
 * specified #script_t script.
 *
 * Return value: the GUnicodeScript identifier found
 *
 * Since: 0.9.38
 **/
GUnicodeScript
glib_script_from_script (script_t script)
{
  return g_unicode_script_from_iso15924 (script);
}


static unicode_combining_class_t
glib_unicode_combining_class (unicode_funcs_t *ufuncs HB_UNUSED,
				 codepoint_t      unicode,
				 void               *user_data HB_UNUSED)

{
  return (unicode_combining_class_t) g_unichar_combining_class (unicode);
}

static unicode_general_category_t
glib_unicode_general_category (unicode_funcs_t *ufuncs HB_UNUSED,
				  codepoint_t      unicode,
				  void               *user_data HB_UNUSED)

{
  /* unicode_general_category_t and GUnicodeType are identical */
  return (unicode_general_category_t) g_unichar_type (unicode);
}

static codepoint_t
glib_unicode_mirroring (unicode_funcs_t *ufuncs HB_UNUSED,
			   codepoint_t      unicode,
			   void               *user_data HB_UNUSED)
{
  g_unichar_get_mirror_char (unicode, &unicode);
  return unicode;
}

static script_t
glib_unicode_script (unicode_funcs_t *ufuncs HB_UNUSED,
			codepoint_t      unicode,
			void               *user_data HB_UNUSED)
{
  return glib_script_to_script (g_unichar_get_script (unicode));
}

static bool_t
glib_unicode_compose (unicode_funcs_t *ufuncs HB_UNUSED,
			 codepoint_t      a,
			 codepoint_t      b,
			 codepoint_t     *ab,
			 void               *user_data HB_UNUSED)
{
#if GLIB_CHECK_VERSION(2,29,12)
  return g_unichar_compose (a, b, ab);
#else
  return false;
#endif
}

static bool_t
glib_unicode_decompose (unicode_funcs_t *ufuncs HB_UNUSED,
			   codepoint_t      ab,
			   codepoint_t     *a,
			   codepoint_t     *b,
			   void               *user_data HB_UNUSED)
{
#if GLIB_CHECK_VERSION(2,29,12)
  return g_unichar_decompose (ab, a, b);
#else
  return false;
#endif
}


static inline void free_static_glib_funcs ();

static struct glib_unicode_funcs_lazy_loader_t : unicode_funcs_lazy_loader_t<glib_unicode_funcs_lazy_loader_t>
{
  static unicode_funcs_t *create ()
  {
    unicode_funcs_t *funcs = unicode_funcs_create (nullptr);

    unicode_funcs_set_combining_class_func (funcs, glib_unicode_combining_class, nullptr, nullptr);
    unicode_funcs_set_general_category_func (funcs, glib_unicode_general_category, nullptr, nullptr);
    unicode_funcs_set_mirroring_func (funcs, glib_unicode_mirroring, nullptr, nullptr);
    unicode_funcs_set_script_func (funcs, glib_unicode_script, nullptr, nullptr);
    unicode_funcs_set_compose_func (funcs, glib_unicode_compose, nullptr, nullptr);
    unicode_funcs_set_decompose_func (funcs, glib_unicode_decompose, nullptr, nullptr);

    unicode_funcs_make_immutable (funcs);

    atexit (free_static_glib_funcs);

    return funcs;
  }
} static_glib_funcs;

static inline
void free_static_glib_funcs ()
{
  static_glib_funcs.free_instance ();
}

/**
 * glib_get_unicode_funcs:
 *
 * Fetches a Unicode-functions structure that is populated
 * with the appropriate GLib function for each method.
 *
 * Return value: (transfer none): a pointer to the #unicode_funcs_t Unicode-functions structure
 *
 * Since: 0.9.38
 **/
unicode_funcs_t *
glib_get_unicode_funcs ()
{
  return static_glib_funcs.get_unconst ();
}



#if GLIB_CHECK_VERSION(2,31,10)

static void
_g_bytes_unref (void *data)
{
  g_bytes_unref ((GBytes *) data);
}

/**
 * glib_blob_create:
 * @gbytes: the GBytes structure to work upon
 *
 * Creates an #blob_t blob from the specified
 * GBytes data structure.
 *
 * Return value: (transfer full): the new #blob_t blob object
 *
 * Since: 0.9.38
 **/
blob_t *
glib_blob_create (GBytes *gbytes)
{
  gsize size = 0;
  gconstpointer data = g_bytes_get_data (gbytes, &size);
  return blob_create ((const char *) data,
			 size,
			 HB_MEMORY_MODE_READONLY,
			 g_bytes_ref (gbytes),
			 _g_bytes_unref);
}
#endif


#endif
