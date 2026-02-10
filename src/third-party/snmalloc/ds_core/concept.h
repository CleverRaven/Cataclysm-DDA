#pragma once

#include "snmalloc/stl/type_traits.h"

/**
 * C++20 concepts are referenced as if they were types in declarations within
 * template parameters (e.g. "template<FooConcept Foo> ...").  That is, they
 * take the place of the "typename"/"class" keyword on template parameters.
 * If the compiler understands concepts, this macro expands as its argument;
 * otherwise, it expands to the keyword "typename", so snmalloc templates that
 * use concept-qualified parameters should use this to remain compatible across
 * C++ versions: "template<SNMALLOC_CONCEPT(FooConcept) Foo>"
 */
#ifdef __cpp_concepts
#  define SNMALLOC_CONCEPT(c) c
#else
#  define SNMALLOC_CONCEPT(c) typename
#endif

#ifdef __cpp_concepts
namespace snmalloc
{
  template<typename T, typename U>
  concept ConceptSame = stl::is_same_v<T, U>;

  /**
   * Equivalence mod stl::remove_reference
   */
  template<typename T, typename U>
  concept ConceptSameModRef =
    ConceptSame<stl::remove_reference_t<T>, stl::remove_reference_t<U>>;

  /**
   * Some of the types in snmalloc are circular in their definition and use
   * templating as a lazy language to carefully tie knots and only pull on the
   * whole mess once it's assembled.  Unfortunately, concepts amount to eagerly
   * demanding the result of the computation.  If concepts come into play during
   * the circular definition, they may see an incomplete type and so fail (with
   * "incomplete type ... used in type trait expression" or similar).  However,
   * it turns out that SFINAE gives us a way to detect whether a template
   * parameter refers to an incomplete type, and short circuit evaluation means
   * we can bail on concept checking if we find ourselves in this situation.
   *
   * See https://devblogs.microsoft.com/oldnewthing/20190710-00/?p=102678
   *
   * Unfortunately, C++20 concepts are not first-order things and, in
   * particular, cannot themselves be template parameters.  So while we would
   * love to write a generic Lazy combinator,
   *
   *   template<template<typename> concept C, typename T>
   *   concept Lazy = !is_type_complete_v<T> || C<T>();
   *
   * this will instead have to be inlined at every definition (and referred to
   * explicitly at call sites) until C++23 or later.
   */
  template<typename, typename = void>
  constexpr bool is_type_complete_v{false};

  template<typename T>
  constexpr bool
    is_type_complete_v<T, stl::void_t<decltype(stl::is_base_of_v<size_t, T>)>>{
      false};
} // namespace snmalloc
#endif
