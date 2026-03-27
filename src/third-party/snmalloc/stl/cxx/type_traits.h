#pragma once

#include <type_traits>

namespace snmalloc
{
  namespace stl
  {
    using std::add_const_t;
    using std::add_lvalue_reference_t;
    using std::add_pointer_t;
    using std::add_rvalue_reference_t;
    using std::bool_constant;
    using std::conditional;
    using std::conditional_t;
    using std::enable_if;
    using std::enable_if_t;
    using std::false_type;
    using std::has_unique_object_representations_v;
    using std::integral_constant;
    using std::is_base_of_v;
    using std::is_copy_assignable_v;
    using std::is_copy_constructible_v;
    using std::is_integral;
    using std::is_integral_v;
    using std::is_move_assignable_v;
    using std::is_move_constructible_v;
    using std::is_same;
    using std::is_same_v;
    using std::is_trivially_copyable_v;
    using std::remove_all_extents_t;
    using std::remove_const_t;
    using std::remove_cv;
    using std::remove_cv_t;
    using std::remove_reference;
    using std::remove_reference_t;
    using std::true_type;
    using std::void_t;
  } // namespace stl
} // namespace snmalloc
