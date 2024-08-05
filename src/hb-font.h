/*
 * Copyright © 2009  Red Hat, Inc.
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
 */

#if !defined(HB_H_IN) && !defined(HB_NO_SINGLE_HEADER_ERROR)
#error "Include <hb.h> instead."
#endif

#ifndef HB_FONT_H
#define HB_FONT_H

#include "hb-common.h"
#include "hb-face.h"
#include "hb-draw.h"
#include "hb-paint.h"

HB_BEGIN_DECLS

/*
 * font_funcs_t
 */

/**
 * font_funcs_t:
 *
 * Data type containing a set of virtual methods used for
 * working on #font_t font objects.
 *
 * HarfBuzz provides a lightweight default function for each of 
 * the methods in #font_funcs_t. Client programs can implement
 * their own replacements for the individual font functions, as
 * needed, and replace the default by calling the setter for a
 * method.
 *
 **/
typedef struct font_funcs_t font_funcs_t;

HB_EXTERN font_funcs_t *
font_funcs_create (void);

HB_EXTERN font_funcs_t *
font_funcs_get_empty (void);

HB_EXTERN font_funcs_t *
font_funcs_reference (font_funcs_t *ffuncs);

HB_EXTERN void
font_funcs_destroy (font_funcs_t *ffuncs);

HB_EXTERN bool_t
font_funcs_set_user_data (font_funcs_t    *ffuncs,
			     user_data_key_t *key,
			     void *              data,
			     destroy_func_t   destroy,
			     bool_t           replace);


HB_EXTERN void *
font_funcs_get_user_data (const font_funcs_t *ffuncs,
			     user_data_key_t    *key);


HB_EXTERN void
font_funcs_make_immutable (font_funcs_t *ffuncs);

HB_EXTERN bool_t
font_funcs_is_immutable (font_funcs_t *ffuncs);


/* font extents */

/**
 * font_extents_t:
 * @ascender: The height of typographic ascenders.
 * @descender: The depth of typographic descenders.
 * @line_gap: The suggested line-spacing gap.
 *
 * Font-wide extent values, measured in font units.
 *
 * Note that typically @ascender is positive and @descender
 * negative, in coordinate systems that grow up.
 **/
typedef struct font_extents_t {
  position_t ascender;
  position_t descender;
  position_t line_gap;
  /*< private >*/
  position_t reserved9;
  position_t reserved8;
  position_t reserved7;
  position_t reserved6;
  position_t reserved5;
  position_t reserved4;
  position_t reserved3;
  position_t reserved2;
  position_t reserved1;
} font_extents_t;

/* func types */

/**
 * font_get_font_extents_func_t:
 * @font: #font_t to work upon
 * @font_data: @font user data pointer
 * @extents: (out): The font extents retrieved
 * @user_data: User data pointer passed by the caller
 *
 * This method should retrieve the extents for a font.
 *
 **/
typedef bool_t (*font_get_font_extents_func_t) (font_t *font, void *font_data,
						       font_extents_t *extents,
						       void *user_data);

/**
 * font_get_font_h_extents_func_t:
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the extents for a font, for horizontal-direction
 * text segments. Extents must be returned in an #glyph_extents output
 * parameter.
 * 
 **/
typedef font_get_font_extents_func_t font_get_font_h_extents_func_t;

/**
 * font_get_font_v_extents_func_t:
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the extents for a font, for vertical-direction
 * text segments. Extents must be returned in an #glyph_extents output
 * parameter.
 * 
 **/
typedef font_get_font_extents_func_t font_get_font_v_extents_func_t;


/**
 * font_get_nominal_glyph_func_t:
 * @font: #font_t to work upon
 * @font_data: @font user data pointer
 * @unicode: The Unicode code point to query
 * @glyph: (out): The glyph ID retrieved
 * @user_data: User data pointer passed by the caller
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the nominal glyph ID for a specified Unicode code
 * point. Glyph IDs must be returned in a #codepoint_t output parameter.
 * 
 * Return value: `true` if data found, `false` otherwise
 *
 **/
typedef bool_t (*font_get_nominal_glyph_func_t) (font_t *font, void *font_data,
						       codepoint_t unicode,
						       codepoint_t *glyph,
						       void *user_data);

/**
 * font_get_variation_glyph_func_t:
 * @font: #font_t to work upon
 * @font_data: @font user data pointer
 * @unicode: The Unicode code point to query
 * @variation_selector: The  variation-selector code point to query
 * @glyph: (out): The glyph ID retrieved
 * @user_data: User data pointer passed by the caller
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the glyph ID for a specified Unicode code point
 * followed by a specified Variation Selector code point. Glyph IDs must be
 * returned in a #codepoint_t output parameter.
 * 
 * Return value: `true` if data found, `false` otherwise
 *
 **/
typedef bool_t (*font_get_variation_glyph_func_t) (font_t *font, void *font_data,
							 codepoint_t unicode, codepoint_t variation_selector,
							 codepoint_t *glyph,
							 void *user_data);


/**
 * font_get_nominal_glyphs_func_t:
 * @font: #font_t to work upon
 * @font_data: @font user data pointer
 * @count: number of code points to query
 * @first_unicode: The first Unicode code point to query
 * @unicode_stride: The stride between successive code points
 * @first_glyph: (out): The first glyph ID retrieved
 * @glyph_stride: The stride between successive glyph IDs
 * @user_data: User data pointer passed by the caller
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the nominal glyph IDs for a sequence of
 * Unicode code points. Glyph IDs must be returned in a #codepoint_t
 * output parameter.
 *
 * Return value: the number of code points processed
 * 
 **/
typedef unsigned int (*font_get_nominal_glyphs_func_t) (font_t *font, void *font_data,
							   unsigned int count,
							   const codepoint_t *first_unicode,
							   unsigned int unicode_stride,
							   codepoint_t *first_glyph,
							   unsigned int glyph_stride,
							   void *user_data);

/**
 * font_get_glyph_advance_func_t:
 * @font: #font_t to work upon
 * @font_data: @font user data pointer
 * @glyph: The glyph ID to query
 * @user_data: User data pointer passed by the caller
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the advance for a specified glyph. The
 * method must return an #position_t.
 * 
 * Return value: The advance of @glyph within @font
 *
 **/
typedef position_t (*font_get_glyph_advance_func_t) (font_t *font, void *font_data,
							   codepoint_t glyph,
							   void *user_data);

/**
 * font_get_glyph_h_advance_func_t:
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the advance for a specified glyph, in
 * horizontal-direction text segments. Advances must be returned in
 * an #position_t output parameter.
 * 
 **/
typedef font_get_glyph_advance_func_t font_get_glyph_h_advance_func_t;

/**
 * font_get_glyph_v_advance_func_t:
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the advance for a specified glyph, in
 * vertical-direction text segments. Advances must be returned in
 * an #position_t output parameter.
 * 
 **/
typedef font_get_glyph_advance_func_t font_get_glyph_v_advance_func_t;

/**
 * font_get_glyph_advances_func_t:
 * @font: #font_t to work upon
 * @font_data: @font user data pointer
 * @count: The number of glyph IDs in the sequence queried
 * @first_glyph: The first glyph ID to query
 * @glyph_stride: The stride between successive glyph IDs
 * @first_advance: (out): The first advance retrieved
 * @advance_stride: The stride between successive advances
 * @user_data: User data pointer passed by the caller
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the advances for a sequence of glyphs.
 * 
 **/
typedef void (*font_get_glyph_advances_func_t) (font_t* font, void* font_data,
						   unsigned int count,
						   const codepoint_t *first_glyph,
						   unsigned glyph_stride,
						   position_t *first_advance,
						   unsigned advance_stride,
						   void *user_data);

/**
 * font_get_glyph_h_advances_func_t:
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the advances for a sequence of glyphs, in
 * horizontal-direction text segments.
 * 
 **/
typedef font_get_glyph_advances_func_t font_get_glyph_h_advances_func_t;

/**
 * font_get_glyph_v_advances_func_t:
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the advances for a sequence of glyphs, in
 * vertical-direction text segments.
 * 
 **/
typedef font_get_glyph_advances_func_t font_get_glyph_v_advances_func_t;

/**
 * font_get_glyph_origin_func_t:
 * @font: #font_t to work upon
 * @font_data: @font user data pointer
 * @glyph: The glyph ID to query
 * @x: (out): The X coordinate of the origin
 * @y: (out): The Y coordinate of the origin
 * @user_data: User data pointer passed by the caller
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the (X,Y) coordinates (in font units) of the
 * origin for a glyph. Each coordinate must be returned in an #position_t
 * output parameter.
 *
 * Return value: `true` if data found, `false` otherwise
 * 
 **/
typedef bool_t (*font_get_glyph_origin_func_t) (font_t *font, void *font_data,
						      codepoint_t glyph,
						      position_t *x, position_t *y,
						      void *user_data);

/**
 * font_get_glyph_h_origin_func_t:
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the (X,Y) coordinates (in font units) of the
 * origin for a glyph, for horizontal-direction text segments. Each
 * coordinate must be returned in an #position_t output parameter.
 * 
 **/
typedef font_get_glyph_origin_func_t font_get_glyph_h_origin_func_t;

/**
 * font_get_glyph_v_origin_func_t:
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the (X,Y) coordinates (in font units) of the
 * origin for a glyph, for vertical-direction text segments. Each coordinate
 * must be returned in an #position_t output parameter.
 * 
 **/
typedef font_get_glyph_origin_func_t font_get_glyph_v_origin_func_t;

/**
 * font_get_glyph_kerning_func_t:
 * @font: #font_t to work upon
 * @font_data: @font user data pointer
 * @first_glyph: The glyph ID of the first glyph in the glyph pair
 * @second_glyph: The glyph ID of the second glyph in the glyph pair
 * @user_data: User data pointer passed by the caller
 *
 * This method should retrieve the kerning-adjustment value for a glyph-pair in
 * the specified font, for horizontal text segments.
 *
 **/
typedef position_t (*font_get_glyph_kerning_func_t) (font_t *font, void *font_data,
							   codepoint_t first_glyph, codepoint_t second_glyph,
							   void *user_data);
/**
 * font_get_glyph_h_kerning_func_t:
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the kerning-adjustment value for a glyph-pair in
 * the specified font, for horizontal text segments.
 *
 **/
typedef font_get_glyph_kerning_func_t font_get_glyph_h_kerning_func_t;


/**
 * font_get_glyph_extents_func_t:
 * @font: #font_t to work upon
 * @font_data: @font user data pointer
 * @glyph: The glyph ID to query
 * @extents: (out): The #glyph_extents_t retrieved
 * @user_data: User data pointer passed by the caller
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the extents for a specified glyph. Extents must be 
 * returned in an #glyph_extents output parameter.
 *
 * Return value: `true` if data found, `false` otherwise
 * 
 **/
typedef bool_t (*font_get_glyph_extents_func_t) (font_t *font, void *font_data,
						       codepoint_t glyph,
						       glyph_extents_t *extents,
						       void *user_data);

/**
 * font_get_glyph_contour_point_func_t:
 * @font: #font_t to work upon
 * @font_data: @font user data pointer
 * @glyph: The glyph ID to query
 * @point_index: The contour-point index to query
 * @x: (out): The X value retrieved for the contour point
 * @y: (out): The Y value retrieved for the contour point
 * @user_data: User data pointer passed by the caller
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the (X,Y) coordinates (in font units) for a
 * specified contour point in a glyph. Each coordinate must be returned as
 * an #position_t output parameter.
 * 
 * Return value: `true` if data found, `false` otherwise
 *
 **/
typedef bool_t (*font_get_glyph_contour_point_func_t) (font_t *font, void *font_data,
							     codepoint_t glyph, unsigned int point_index,
							     position_t *x, position_t *y,
							     void *user_data);


/**
 * font_get_glyph_name_func_t:
 * @font: #font_t to work upon
 * @font_data: @font user data pointer
 * @glyph: The glyph ID to query
 * @name: (out) (array length=size): Name string retrieved for the glyph ID
 * @size: Length of the glyph-name string retrieved
 * @user_data: User data pointer passed by the caller
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the glyph name that corresponds to a
 * glyph ID. The name should be returned in a string output parameter.
 * 
 * Return value: `true` if data found, `false` otherwise
 *
 **/
typedef bool_t (*font_get_glyph_name_func_t) (font_t *font, void *font_data,
						    codepoint_t glyph,
						    char *name, unsigned int size,
						    void *user_data);

/**
 * font_get_glyph_from_name_func_t:
 * @font: #font_t to work upon
 * @font_data: @font user data pointer
 * @name: (array length=len): The name string to query
 * @len: The length of the name queried
 * @glyph: (out): The glyph ID retrieved
 * @user_data: User data pointer passed by the caller
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * This method should retrieve the glyph ID that corresponds to a glyph-name
 * string. 
 * 
 * Return value: `true` if data found, `false` otherwise
 *
 **/
typedef bool_t (*font_get_glyph_from_name_func_t) (font_t *font, void *font_data,
							 const char *name, int len, /* -1 means nul-terminated */
							 codepoint_t *glyph,
							 void *user_data);

/**
 * font_draw_glyph_func_t:
 * @font: #font_t to work upon
 * @font_data: @font user data pointer
 * @glyph: The glyph ID to query
 * @draw_funcs: The draw functions to send the shape data to
 * @draw_data: The data accompanying the draw functions
 * @user_data: User data pointer passed by the caller
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * Since: 7.0.0
 *
 **/
typedef void (*font_draw_glyph_func_t) (font_t *font, void *font_data,
                                           codepoint_t glyph,
                                           draw_funcs_t *draw_funcs, void *draw_data,
                                           void *user_data);

/**
 * font_paint_glyph_func_t:
 * @font: #font_t to work upon
 * @font_data: @font user data pointer
 * @glyph: The glyph ID to query
 * @paint_funcs: The paint functions to use
 * @paint_data: The data accompanying the paint functions
 * @palette_index: The color palette to use
 * @foreground: The foreground color
 * @user_data: User data pointer passed by the caller
 *
 * A virtual method for the #font_funcs_t of an #font_t object.
 *
 * Since: 7.0.0
 */
typedef void (*font_paint_glyph_func_t) (font_t *font, void *font_data,
                                            codepoint_t glyph,
                                            paint_funcs_t *paint_funcs, void *paint_data,
                                            unsigned int palette_index,
                                            color_t foreground,
                                            void *user_data);

/* func setters */

/**
 * font_funcs_set_font_h_extents_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_get_font_h_extents_func_t.
 *
 * Since: 1.1.2
 **/
HB_EXTERN void
font_funcs_set_font_h_extents_func (font_funcs_t *ffuncs,
				       font_get_font_h_extents_func_t func,
				       void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_font_v_extents_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_get_font_v_extents_func_t.
 *
 * Since: 1.1.2
 **/
HB_EXTERN void
font_funcs_set_font_v_extents_func (font_funcs_t *ffuncs,
				       font_get_font_v_extents_func_t func,
				       void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_nominal_glyph_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_get_nominal_glyph_func_t.
 *
 * Since: 1.2.3
 **/
HB_EXTERN void
font_funcs_set_nominal_glyph_func (font_funcs_t *ffuncs,
				      font_get_nominal_glyph_func_t func,
				      void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_nominal_glyphs_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_get_nominal_glyphs_func_t.
 *
 * Since: 2.0.0
 **/
HB_EXTERN void
font_funcs_set_nominal_glyphs_func (font_funcs_t *ffuncs,
				       font_get_nominal_glyphs_func_t func,
				       void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_variation_glyph_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_get_variation_glyph_func_t.
 *
 * Since: 1.2.3
 **/
HB_EXTERN void
font_funcs_set_variation_glyph_func (font_funcs_t *ffuncs,
					font_get_variation_glyph_func_t func,
					void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_glyph_h_advance_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_get_glyph_h_advance_func_t.
 *
 * Since: 0.9.2
 **/
HB_EXTERN void
font_funcs_set_glyph_h_advance_func (font_funcs_t *ffuncs,
					font_get_glyph_h_advance_func_t func,
					void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_glyph_v_advance_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_get_glyph_v_advance_func_t.
 *
 * Since: 0.9.2
 **/
HB_EXTERN void
font_funcs_set_glyph_v_advance_func (font_funcs_t *ffuncs,
					font_get_glyph_v_advance_func_t func,
					void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_glyph_h_advances_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_get_glyph_h_advances_func_t.
 *
 * Since: 1.8.6
 **/
HB_EXTERN void
font_funcs_set_glyph_h_advances_func (font_funcs_t *ffuncs,
					font_get_glyph_h_advances_func_t func,
					void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_glyph_v_advances_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_get_glyph_v_advances_func_t.
 *
 * Since: 1.8.6
 **/
HB_EXTERN void
font_funcs_set_glyph_v_advances_func (font_funcs_t *ffuncs,
					font_get_glyph_v_advances_func_t func,
					void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_glyph_h_origin_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_get_glyph_h_origin_func_t.
 *
 * Since: 0.9.2
 **/
HB_EXTERN void
font_funcs_set_glyph_h_origin_func (font_funcs_t *ffuncs,
				       font_get_glyph_h_origin_func_t func,
				       void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_glyph_v_origin_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_get_glyph_v_origin_func_t.
 *
 * Since: 0.9.2
 **/
HB_EXTERN void
font_funcs_set_glyph_v_origin_func (font_funcs_t *ffuncs,
				       font_get_glyph_v_origin_func_t func,
				       void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_glyph_h_kerning_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_get_glyph_h_kerning_func_t.
 *
 * Since: 0.9.2
 **/
HB_EXTERN void
font_funcs_set_glyph_h_kerning_func (font_funcs_t *ffuncs,
					font_get_glyph_h_kerning_func_t func,
					void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_glyph_extents_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_get_glyph_extents_func_t.
 *
 * Since: 0.9.2
 **/
HB_EXTERN void
font_funcs_set_glyph_extents_func (font_funcs_t *ffuncs,
				      font_get_glyph_extents_func_t func,
				      void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_glyph_contour_point_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_get_glyph_contour_point_func_t.
 *
 * Since: 0.9.2
 **/
HB_EXTERN void
font_funcs_set_glyph_contour_point_func (font_funcs_t *ffuncs,
					    font_get_glyph_contour_point_func_t func,
					    void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_glyph_name_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_get_glyph_name_func_t.
 *
 * Since: 0.9.2
 **/
HB_EXTERN void
font_funcs_set_glyph_name_func (font_funcs_t *ffuncs,
				   font_get_glyph_name_func_t func,
				   void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_glyph_from_name_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_get_glyph_from_name_func_t.
 *
 * Since: 0.9.2
 **/
HB_EXTERN void
font_funcs_set_glyph_from_name_func (font_funcs_t *ffuncs,
					font_get_glyph_from_name_func_t func,
					void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_draw_glyph_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is not needed anymore
 *
 * Sets the implementation function for #font_draw_glyph_func_t.
 *
 * Since: 7.0.0
 **/
HB_EXTERN void
font_funcs_set_draw_glyph_func (font_funcs_t *ffuncs,
                                   font_draw_glyph_func_t func,
                                   void *user_data, destroy_func_t destroy);

/**
 * font_funcs_set_paint_glyph_func:
 * @ffuncs: A font-function structure
 * @func: (closure user_data) (destroy destroy) (scope notified): The callback function to assign
 * @user_data: Data to pass to @func
 * @destroy: (nullable): The function to call when @user_data is no longer needed
 *
 * Sets the implementation function for #font_paint_glyph_func_t.
 *
 * Since: 7.0.0
 */
HB_EXTERN void
font_funcs_set_paint_glyph_func (font_funcs_t *ffuncs,
                                    font_paint_glyph_func_t func,
                                    void *user_data, destroy_func_t destroy);

/* func dispatch */

HB_EXTERN bool_t
font_get_h_extents (font_t *font,
		       font_extents_t *extents);
HB_EXTERN bool_t
font_get_v_extents (font_t *font,
		       font_extents_t *extents);

HB_EXTERN bool_t
font_get_nominal_glyph (font_t *font,
			   codepoint_t unicode,
			   codepoint_t *glyph);
HB_EXTERN bool_t
font_get_variation_glyph (font_t *font,
			     codepoint_t unicode, codepoint_t variation_selector,
			     codepoint_t *glyph);

HB_EXTERN unsigned int
font_get_nominal_glyphs (font_t *font,
			    unsigned int count,
			    const codepoint_t *first_unicode,
			    unsigned int unicode_stride,
			    codepoint_t *first_glyph,
			    unsigned int glyph_stride);

HB_EXTERN position_t
font_get_glyph_h_advance (font_t *font,
			     codepoint_t glyph);
HB_EXTERN position_t
font_get_glyph_v_advance (font_t *font,
			     codepoint_t glyph);

HB_EXTERN void
font_get_glyph_h_advances (font_t* font,
			      unsigned int count,
			      const codepoint_t *first_glyph,
			      unsigned glyph_stride,
			      position_t *first_advance,
			      unsigned advance_stride);
HB_EXTERN void
font_get_glyph_v_advances (font_t* font,
			      unsigned int count,
			      const codepoint_t *first_glyph,
			      unsigned glyph_stride,
			      position_t *first_advance,
			      unsigned advance_stride);

HB_EXTERN bool_t
font_get_glyph_h_origin (font_t *font,
			    codepoint_t glyph,
			    position_t *x, position_t *y);
HB_EXTERN bool_t
font_get_glyph_v_origin (font_t *font,
			    codepoint_t glyph,
			    position_t *x, position_t *y);

HB_EXTERN position_t
font_get_glyph_h_kerning (font_t *font,
			     codepoint_t left_glyph, codepoint_t right_glyph);

HB_EXTERN bool_t
font_get_glyph_extents (font_t *font,
			   codepoint_t glyph,
			   glyph_extents_t *extents);

HB_EXTERN bool_t
font_get_glyph_contour_point (font_t *font,
				 codepoint_t glyph, unsigned int point_index,
				 position_t *x, position_t *y);

HB_EXTERN bool_t
font_get_glyph_name (font_t *font,
			codepoint_t glyph,
			char *name, unsigned int size);
HB_EXTERN bool_t
font_get_glyph_from_name (font_t *font,
			     const char *name, int len, /* -1 means nul-terminated */
			     codepoint_t *glyph);

HB_EXTERN void
font_draw_glyph (font_t *font,
                    codepoint_t glyph,
                    draw_funcs_t *dfuncs, void *draw_data);

HB_EXTERN void
font_paint_glyph (font_t *font,
                     codepoint_t glyph,
                     paint_funcs_t *pfuncs, void *paint_data,
                     unsigned int palette_index,
                     color_t foreground);

/* high-level funcs, with fallback */

/* Calls either font_get_nominal_glyph() if variation_selector is 0,
 * otherwise calls font_get_variation_glyph(). */
HB_EXTERN bool_t
font_get_glyph (font_t *font,
		   codepoint_t unicode, codepoint_t variation_selector,
		   codepoint_t *glyph);

HB_EXTERN void
font_get_extents_for_direction (font_t *font,
				   direction_t direction,
				   font_extents_t *extents);
HB_EXTERN void
font_get_glyph_advance_for_direction (font_t *font,
					 codepoint_t glyph,
					 direction_t direction,
					 position_t *x, position_t *y);
HB_EXTERN void
font_get_glyph_advances_for_direction (font_t* font,
					  direction_t direction,
					  unsigned int count,
					  const codepoint_t *first_glyph,
					  unsigned glyph_stride,
					  position_t *first_advance,
					  unsigned advance_stride);
HB_EXTERN void
font_get_glyph_origin_for_direction (font_t *font,
					codepoint_t glyph,
					direction_t direction,
					position_t *x, position_t *y);
HB_EXTERN void
font_add_glyph_origin_for_direction (font_t *font,
					codepoint_t glyph,
					direction_t direction,
					position_t *x, position_t *y);
HB_EXTERN void
font_subtract_glyph_origin_for_direction (font_t *font,
					     codepoint_t glyph,
					     direction_t direction,
					     position_t *x, position_t *y);

HB_EXTERN void
font_get_glyph_kerning_for_direction (font_t *font,
					 codepoint_t first_glyph, codepoint_t second_glyph,
					 direction_t direction,
					 position_t *x, position_t *y);

HB_EXTERN bool_t
font_get_glyph_extents_for_origin (font_t *font,
				      codepoint_t glyph,
				      direction_t direction,
				      glyph_extents_t *extents);

HB_EXTERN bool_t
font_get_glyph_contour_point_for_origin (font_t *font,
					    codepoint_t glyph, unsigned int point_index,
					    direction_t direction,
					    position_t *x, position_t *y);

/* Generates gidDDD if glyph has no name. */
HB_EXTERN void
font_glyph_to_string (font_t *font,
			 codepoint_t glyph,
			 char *s, unsigned int size);
/* Parses gidDDD and uniUUUU strings automatically. */
HB_EXTERN bool_t
font_glyph_from_string (font_t *font,
			   const char *s, int len, /* -1 means nul-terminated */
			   codepoint_t *glyph);


/*
 * font_t
 */

/* Fonts are very light-weight objects */

HB_EXTERN font_t *
font_create (face_t *face);

HB_EXTERN font_t *
font_create_sub_font (font_t *parent);

HB_EXTERN font_t *
font_get_empty (void);

HB_EXTERN font_t *
font_reference (font_t *font);

HB_EXTERN void
font_destroy (font_t *font);

HB_EXTERN bool_t
font_set_user_data (font_t          *font,
		       user_data_key_t *key,
		       void *              data,
		       destroy_func_t   destroy,
		       bool_t           replace);


HB_EXTERN void *
font_get_user_data (const font_t    *font,
		       user_data_key_t *key);

HB_EXTERN void
font_make_immutable (font_t *font);

HB_EXTERN bool_t
font_is_immutable (font_t *font);

HB_EXTERN unsigned int
font_get_serial (font_t *font);

HB_EXTERN void
font_changed (font_t *font);

HB_EXTERN void
font_set_parent (font_t *font,
		    font_t *parent);

HB_EXTERN font_t *
font_get_parent (font_t *font);

HB_EXTERN void
font_set_face (font_t *font,
		  face_t *face);

HB_EXTERN face_t *
font_get_face (font_t *font);


HB_EXTERN void
font_set_funcs (font_t         *font,
		   font_funcs_t   *klass,
		   void              *font_data,
		   destroy_func_t  destroy);

/* Be *very* careful with this function! */
HB_EXTERN void
font_set_funcs_data (font_t         *font,
			void              *font_data,
			destroy_func_t  destroy);


HB_EXTERN void
font_set_scale (font_t *font,
		   int x_scale,
		   int y_scale);

HB_EXTERN void
font_get_scale (font_t *font,
		   int *x_scale,
		   int *y_scale);

/*
 * A zero value means "no hinting in that direction"
 */
HB_EXTERN void
font_set_ppem (font_t *font,
		  unsigned int x_ppem,
		  unsigned int y_ppem);

HB_EXTERN void
font_get_ppem (font_t *font,
		  unsigned int *x_ppem,
		  unsigned int *y_ppem);

/*
 * Point size per EM.  Used for optical-sizing in CoreText.
 * A value of zero means "not set".
 */
HB_EXTERN void
font_set_ptem (font_t *font, float ptem);

HB_EXTERN float
font_get_ptem (font_t *font);

HB_EXTERN void
font_set_synthetic_bold (font_t *font,
			    float x_embolden, float y_embolden,
			    bool_t in_place);

HB_EXTERN void
font_get_synthetic_bold (font_t *font,
			    float *x_embolden, float *y_embolden,
			    bool_t *in_place);

HB_EXTERN void
font_set_synthetic_slant (font_t *font, float slant);

HB_EXTERN float
font_get_synthetic_slant (font_t *font);

HB_EXTERN void
font_set_variations (font_t *font,
			const variation_t *variations,
			unsigned int variations_length);

HB_EXTERN void
font_set_variation (font_t *font,
		       tag_t tag,
		       float    value);

HB_EXTERN void
font_set_var_coords_design (font_t *font,
			       const float *coords,
			       unsigned int coords_length);

HB_EXTERN const float *
font_get_var_coords_design (font_t *font,
			       unsigned int *length);

HB_EXTERN void
font_set_var_coords_normalized (font_t *font,
				   const int *coords, /* 2.14 normalized */
				   unsigned int coords_length);

HB_EXTERN const int *
font_get_var_coords_normalized (font_t *font,
				   unsigned int *length);

/**
 * HB_FONT_NO_VAR_NAMED_INSTANCE:
 *
 * Constant signifying that a font does not have any
 * named-instance index set.  This is the default of
 * a font.
 *
 * Since: 7.0.0
 */
#define HB_FONT_NO_VAR_NAMED_INSTANCE 0xFFFFFFFF

HB_EXTERN void
font_set_var_named_instance (font_t *font,
				unsigned int instance_index);

HB_EXTERN unsigned int
font_get_var_named_instance (font_t *font);

HB_END_DECLS

#endif /* HB_FONT_H */
