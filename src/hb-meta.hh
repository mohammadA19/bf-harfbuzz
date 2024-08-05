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

#ifndef HB_META_HH
#define HB_META_HH

#include "hb.hh"

#include <memory>
#include <type_traits>
#include <utility>


/*
 * C++ template meta-programming & fundamentals used with them.
 */

/* Void!  For when we need a expression-type of void. */
struct empty_t {};

/* https://en.cppreference.com/w/cpp/types/void_t */
template<typename... Ts> struct _void_t { typedef void type; };
template<typename... Ts> using void_t = typename _void_t<Ts...>::type;

template<typename Head, typename... Ts> struct _head_t { typedef Head type; };
template<typename... Ts> using head_t = typename _head_t<Ts...>::type;

template <typename T, T v> struct integral_constant { static constexpr T value = v; };
template <bool b> using bool_constant = integral_constant<bool, b>;
using true_type = bool_constant<true>;
using false_type = bool_constant<false>;

/* Static-assert as expression. */
template <bool cond> struct static_assert_expr;
template <> struct static_assert_expr<true> : false_type {};
#define static_assert_expr(C) static_assert_expr<C>::value

/* Basic type SFINAE. */

template <bool B, typename T = void> struct enable_if {};
template <typename T>                struct enable_if<true, T> { typedef T type; };
#define enable_if(Cond) typename enable_if<(Cond)>::type* = nullptr
/* Concepts/Requires alias: */
#define requires(Cond) enable_if((Cond))

template <typename T, typename T2> struct is_same : false_type {};
template <typename T>              struct is_same<T, T> : true_type {};
#define is_same(T, T2) is_same<T, T2>::value

/* Function overloading SFINAE and priority. */

#define HB_RETURN(Ret, E) -> head_t<Ret, decltype ((E))> { return (E); }
#define HB_AUTO_RETURN(E) -> decltype ((E)) { return (E); }
#define HB_VOID_RETURN(E) -> void_t<decltype ((E))> { (E); }

template <unsigned Pri> struct priority : priority<Pri - 1> {};
template <>             struct priority<0> {};
#define prioritize priority<16> ()

#define HB_FUNCOBJ(x) static_const x HB_UNUSED


template <typename T> struct type_identity_t { typedef T type; };
template <typename T> using type_identity = typename type_identity_t<T>::type;

template <typename T> static inline T declval ();
#define declval(T) (declval<T> ())

template <typename T> struct match_const		: type_identity_t<T>, false_type	{};
template <typename T> struct match_const<const T>	: type_identity_t<T>, true_type	{};
template <typename T> using remove_const = typename match_const<T>::type;

template <typename T> struct match_reference		: type_identity_t<T>, false_type	{};
template <typename T> struct match_reference<T &>	: type_identity_t<T>, true_type	{};
template <typename T> struct match_reference<T &&>	: type_identity_t<T>, true_type	{};
template <typename T> using remove_reference = typename match_reference<T>::type;
template <typename T> auto _try_add_lvalue_reference (priority<1>) -> type_identity<T&>;
template <typename T> auto _try_add_lvalue_reference (priority<0>) -> type_identity<T>;
template <typename T> using add_lvalue_reference = decltype (_try_add_lvalue_reference<T> (prioritize));
template <typename T> auto _try_add_rvalue_reference (priority<1>) -> type_identity<T&&>;
template <typename T> auto _try_add_rvalue_reference (priority<0>) -> type_identity<T>;
template <typename T> using add_rvalue_reference = decltype (_try_add_rvalue_reference<T> (prioritize));

template <typename T> struct match_pointer		: type_identity_t<T>, false_type	{};
template <typename T> struct match_pointer<T *>	: type_identity_t<T>, true_type	{};
template <typename T> using remove_pointer = typename match_pointer<T>::type;
template <typename T> auto _try_add_pointer (priority<1>) -> type_identity<remove_reference<T>*>;
template <typename T> auto _try_add_pointer (priority<1>) -> type_identity<T>;
template <typename T> using add_pointer = decltype (_try_add_pointer<T> (prioritize));


template <typename T> using decay = typename std::decay<T>::type;

#define is_convertible(From,To) std::is_convertible<From, To>::value

template <typename From, typename To>
using is_cr_convertible = bool_constant<
  is_same (decay<From>, decay<To>) &&
  (!std::is_const<From>::value || std::is_const<To>::value) &&
  (!std::is_reference<To>::value || std::is_const<To>::value || std::is_reference<To>::value)
>;
#define is_cr_convertible(From,To) is_cr_convertible<From, To>::value


struct
{
  template <typename T> constexpr auto
  operator () (T&& v) const HB_AUTO_RETURN (std::forward<T> (v))

  template <typename T> constexpr auto
  operator () (T *v) const HB_AUTO_RETURN (*v)

  template <typename T> constexpr auto
  operator () (const hb::shared_ptr<T>& v) const HB_AUTO_RETURN (*v)

  template <typename T> constexpr auto
  operator () (hb::shared_ptr<T>& v) const HB_AUTO_RETURN (*v)
  
  template <typename T> constexpr auto
  operator () (const hb::unique_ptr<T>& v) const HB_AUTO_RETURN (*v)

  template <typename T> constexpr auto
  operator () (hb::unique_ptr<T>& v) const HB_AUTO_RETURN (*v)
}
HB_FUNCOBJ (deref);

template <typename T>
struct reference_wrapper
{
  reference_wrapper (T v) : v (v) {}
  bool operator == (const reference_wrapper& o) const { return v == o.v; }
  bool operator != (const reference_wrapper& o) const { return v != o.v; }
  operator T& () { return v; }
  T& get () { return v; }
  T v;
};
template <typename T>
struct reference_wrapper<T&>
{
  reference_wrapper (T& v) : v (std::addressof (v)) {}
  bool operator == (const reference_wrapper& o) const { return v == o.v; }
  bool operator != (const reference_wrapper& o) const { return v != o.v; }
  operator T& () { return *v; }
  T& get () { return *v; }
  T* v;
};


/* Type traits */

template <typename T> struct int_min;
template <> struct int_min<char>			: integral_constant<char,			CHAR_MIN>	{};
template <> struct int_min<signed char>		: integral_constant<signed char,		SCHAR_MIN>	{};
template <> struct int_min<unsigned char>		: integral_constant<unsigned char,		0>		{};
template <> struct int_min<signed short>		: integral_constant<signed short,		SHRT_MIN>	{};
template <> struct int_min<unsigned short>		: integral_constant<unsigned short,		0>		{};
template <> struct int_min<signed int>		: integral_constant<signed int,		INT_MIN>	{};
template <> struct int_min<unsigned int>		: integral_constant<unsigned int,		0>		{};
template <> struct int_min<signed long>		: integral_constant<signed long,		LONG_MIN>	{};
template <> struct int_min<unsigned long>		: integral_constant<unsigned long,		0>		{};
template <> struct int_min<signed long long>		: integral_constant<signed long long,	LLONG_MIN>	{};
template <> struct int_min<unsigned long long>	: integral_constant<unsigned long long,	0>		{};
template <typename T> struct int_min<T *>		: integral_constant<T *,			nullptr>	{};
#define int_min(T) int_min<T>::value
template <typename T> struct int_max;
template <> struct int_max<char>			: integral_constant<char,			CHAR_MAX>	{};
template <> struct int_max<signed char>		: integral_constant<signed char,		SCHAR_MAX>	{};
template <> struct int_max<unsigned char>		: integral_constant<unsigned char,		UCHAR_MAX>	{};
template <> struct int_max<signed short>		: integral_constant<signed short,		SHRT_MAX>	{};
template <> struct int_max<unsigned short>		: integral_constant<unsigned short,		USHRT_MAX>	{};
template <> struct int_max<signed int>		: integral_constant<signed int,		INT_MAX>	{};
template <> struct int_max<unsigned int>		: integral_constant<unsigned int,		UINT_MAX>	{};
template <> struct int_max<signed long>		: integral_constant<signed long,		LONG_MAX>	{};
template <> struct int_max<unsigned long>		: integral_constant<unsigned long,		ULONG_MAX>	{};
template <> struct int_max<signed long long>		: integral_constant<signed long long,	LLONG_MAX>	{};
template <> struct int_max<unsigned long long>	: integral_constant<unsigned long long,	ULLONG_MAX>	{};
#define int_max(T) int_max<T>::value

#if defined(__GNUC__) && __GNUC__ < 5 && !defined(__clang__)
#define is_trivially_copyable(T) __has_trivial_copy(T)
#define is_trivially_copy_assignable(T) __has_trivial_assign(T)
#define is_trivially_constructible(T) __has_trivial_constructor(T)
#define is_trivially_copy_constructible(T) __has_trivial_copy_constructor(T)
#define is_trivially_destructible(T) __has_trivial_destructor(T)
#else
#define is_trivially_copyable(T) std::is_trivially_copyable<T>::value
#define is_trivially_copy_assignable(T) std::is_trivially_copy_assignable<T>::value
#define is_trivially_constructible(T) std::is_trivially_constructible<T>::value
#define is_trivially_copy_constructible(T) std::is_trivially_copy_constructible<T>::value
#define is_trivially_destructible(T) std::is_trivially_destructible<T>::value
#endif

/* Class traits. */

#define HB_DELETE_COPY_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete; \
  void operator=(const TypeName&) = delete
#define HB_DELETE_CREATE_COPY_ASSIGN(TypeName) \
  TypeName() = delete; \
  TypeName(const TypeName&) = delete; \
  void operator=(const TypeName&) = delete

/* unwrap_type (T)
 * If T has no T::type, returns T. Otherwise calls itself on T::type recursively.
 */

template <typename T, typename>
struct _unwrap_type : type_identity_t<T> {};
template <typename T>
struct _unwrap_type<T, void_t<typename T::type>> : _unwrap_type<typename T::type, void> {};
template <typename T>
using unwrap_type = _unwrap_type<T, void>;
#define unwrap_type(T) typename unwrap_type<T>::type

#endif /* HB_META_HH */
