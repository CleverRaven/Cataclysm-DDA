// Copyright (c) 2026, Matthew Bentley (mattreecebentley@gmail.com) www.plflib.org

// zLib license (https://www.zlib.net/zlib_license.html):
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
// 	claim that you wrote the original software. If you use this software
// 	in a product, an acknowledgement in the product documentation would be
// 	appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
// 	misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.


#ifndef PLF_LIST_H
#define PLF_LIST_H


// Compiler-specific defines:

// Define default cases before possibly redefining:
#define PLF_NOEXCEPT throw()
#define PLF_NOEXCEPT_ALLOCATOR
#define PLF_CONSTEXPR
#define PLF_CONSTFUNC

#define PLF_EXCEPTIONS_SUPPORT

#if ((defined(__clang__) || defined(__GNUC__)) && !defined(__EXCEPTIONS)) || (defined(_MSC_VER) && !defined(_CPPUNWIND))
	#undef PLF_EXCEPTIONS_SUPPORT
	#include <exception> // std::terminate
#endif


#if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
	 // Suppress incorrect (unfixed MSVC bug) warnings re: constant expressions in constexpr-if statements
	#pragma warning ( push )
	#pragma warning ( disable : 4127 )

	#if _MSC_VER >= 1600
		#define PLF_MOVE_SEMANTICS_SUPPORT
	#endif
	#if _MSC_VER >= 1700
		#define PLF_TYPE_TRAITS_SUPPORT
		#define PLF_ALLOCATOR_TRAITS_SUPPORT
	#endif
	#if _MSC_VER >= 1800
		#define PLF_VARIADICS_SUPPORT // Variadics, in this context, means both variadic templates and variadic macros are supported
		#define PLF_DEFAULT_SUPPORT // Both support for default template arguments and defaulted class functions
		#define PLF_INITIALIZER_LIST_SUPPORT
	#endif
	#if _MSC_VER >= 1900
		#define PLF_ALIGNMENT_SUPPORT
		#undef PLF_NOEXCEPT
		#undef PLF_NOEXCEPT_ALLOCATOR
		#define PLF_NOEXCEPT noexcept
		#define PLF_NOEXCEPT_ALLOCATOR noexcept(noexcept(allocator_type()))
		#define PLF_IS_ALWAYS_EQUAL_SUPPORT
	#endif

	#if defined(_MSVC_LANG) && (_MSVC_LANG >= 201703L)
		#undef PLF_CONSTEXPR
		#define PLF_CONSTEXPR constexpr
	#endif

	#if defined(_MSVC_LANG) && (_MSVC_LANG >= 202002L) && _MSC_VER >= 1929
		#define PLF_CPP20_SUPPORT
		#undef PLF_CONSTFUNC
		#define PLF_CONSTFUNC constexpr
	#endif
#elif defined(__cplusplus) && __cplusplus >= 201103L // C++11 support, at least
	#if defined(__GNUC__) && defined(__GNUC_MINOR__) && !defined(__clang__) // If compiler is GCC/G++
		#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 3) || __GNUC__ > 4
			#define PLF_MOVE_SEMANTICS_SUPPORT
			#define PLF_VARIADICS_SUPPORT
		#endif
		#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 4) || __GNUC__ > 4
			#define PLF_DEFAULT_SUPPORT
			#define PLF_INITIALIZER_LIST_SUPPORT
		#endif
		#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4
			#undef PLF_NOEXCEPT
			#undef PLF_NOEXCEPT_ALLOCATOR
			#define PLF_NOEXCEPT noexcept
			#define PLF_NOEXCEPT_ALLOCATOR noexcept(noexcept(allocator_type()))
		#endif
		#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || __GNUC__ > 4
			#define PLF_ALLOCATOR_TRAITS_SUPPORT
		#endif
		#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 8) || __GNUC__ > 4
			#define PLF_ALIGNMENT_SUPPORT
		#endif
		#if __GNUC__ >= 5 // GCC v4.9 and below do not support std::is_trivially_copyable
			#define PLF_TYPE_TRAITS_SUPPORT
		#endif
		#if __GNUC__ > 6
			#define PLF_IS_ALWAYS_EQUAL_SUPPORT
		#endif
	#elif defined(__clang__) && !defined(__GLIBCXX__) && !defined(_LIBCPP_CXX03_LANG) && __clang_major__ >= 3
		#define PLF_DEFAULT_SUPPORT
		#define PLF_ALLOCATOR_TRAITS_SUPPORT
		#define PLF_TYPE_TRAITS_SUPPORT

		#if __has_feature(cxx_alignas) && __has_feature(cxx_alignof)
			#define PLF_ALIGNMENT_SUPPORT
		#endif
		#if __has_feature(cxx_noexcept)
			#undef PLF_NOEXCEPT
			#undef PLF_NOEXCEPT_ALLOCATOR
			#define PLF_NOEXCEPT noexcept
			#define PLF_NOEXCEPT_ALLOCATOR noexcept(noexcept(allocator_type()))
			#define PLF_IS_ALWAYS_EQUAL_SUPPORT
		#endif
		#if __has_feature(cxx_rvalue_references) && !defined(_LIBCPP_HAS_NO_RVALUE_REFERENCES)
			#define PLF_MOVE_SEMANTICS_SUPPORT
		#endif
		#if __has_feature(cxx_variadic_templates) && !defined(_LIBCPP_HAS_NO_VARIADICS)
			#define PLF_VARIADICS_SUPPORT
		#endif
		#if (__clang_major__ == 3 && __clang_minor__ >= 1) || __clang_major__ > 3
			#define PLF_INITIALIZER_LIST_SUPPORT
		#endif
	#elif defined(__GLIBCXX__) // Using another compiler type with libstdc++ - we are assuming full c++11 compliance for compiler - which may not be true
		#define PLF_DEFAULT_SUPPORT

		#if __GLIBCXX__ >= 20080606
			#define PLF_MOVE_SEMANTICS_SUPPORT
			#define PLF_VARIADICS_SUPPORT
		#endif
		#if __GLIBCXX__ >= 20090421
			#define PLF_INITIALIZER_LIST_SUPPORT
		#endif
		#if __GLIBCXX__ >= 20120322
			#define PLF_ALLOCATOR_TRAITS_SUPPORT
			#undef PLF_NOEXCEPT
			#undef PLF_NOEXCEPT_ALLOCATOR
			#define PLF_NOEXCEPT noexcept
			#define PLF_NOEXCEPT_ALLOCATOR noexcept(noexcept(allocator_type()))
		#endif
		#if __GLIBCXX__ >= 20130322
			#define PLF_ALIGNMENT_SUPPORT
		#endif
		#if __GLIBCXX__ >= 20150422 // libstdc++ v4.9 and below do not support std::is_trivially_copyable
			#define PLF_TYPE_TRAITS_SUPPORT
		#endif
		#if __GLIBCXX__ >= 20160111
			#define PLF_IS_ALWAYS_EQUAL_SUPPORT
		#endif
	#elif defined(_LIBCPP_CXX03_LANG) || defined(_LIBCPP_HAS_NO_RVALUE_REFERENCES) // Special case for checking C++11 support with libCPP
		#if !defined(_LIBCPP_HAS_NO_VARIADICS)
			#define PLF_VARIADICS_SUPPORT
		#endif
	#else // Assume type traits and initializer support for other compilers and standard library implementations
		#define PLF_DEFAULT_SUPPORT
		#define PLF_MOVE_SEMANTICS_SUPPORT
		#define PLF_VARIADICS_SUPPORT
		#define PLF_TYPE_TRAITS_SUPPORT
		#define PLF_ALLOCATOR_TRAITS_SUPPORT
		#define PLF_ALIGNMENT_SUPPORT
		#define PLF_INITIALIZER_LIST_SUPPORT
		#undef PLF_NOEXCEPT
		#undef PLF_NOEXCEPT_ALLOCATOR
		#define PLF_NOEXCEPT noexcept
		#define PLF_NOEXCEPT_ALLOCATOR noexcept(noexcept(allocator_type()))
		#define PLF_IS_ALWAYS_EQUAL_SUPPORT
	#endif

	#if __cplusplus >= 201703L && ((defined(__clang__) && ((__clang_major__ == 3 && __clang_minor__ == 9) || __clang_major__ > 3)) || (defined(__GNUC__) && __GNUC__ >= 7) || (!defined(__clang__) && !defined(__GNUC__))) // assume correct C++17 implementation for non-gcc/clang compilers
		#undef PLF_CONSTEXPR
		#define PLF_CONSTEXPR constexpr
	#endif

	#if __cplusplus > 201704L && ((((defined(__clang__) && !defined(__APPLE_CC__) && __clang_major__ >= 14) || (defined(__GNUC__) && (__GNUC__ > 11 || (__GNUC__ == 11 && __GNUC_MINOR__ > 0)))) && ((defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 14) || (defined(__GLIBCXX__) && __GLIBCXX__ >= 201806L))) || (!defined(__clang__) && !defined(__GNUC__)))
		#define PLF_CPP20_SUPPORT
		#undef PLF_CONSTFUNC
		#define PLF_CONSTFUNC constexpr
	#endif
#endif

#if defined(PLF_IS_ALWAYS_EQUAL_SUPPORT) && defined(PLF_MOVE_SEMANTICS_SUPPORT) && defined(PLF_ALLOCATOR_TRAITS_SUPPORT) && (__cplusplus >= 201703L || (defined(_MSVC_LANG) && (_MSVC_LANG >= 201703L)))
	#define PLF_NOEXCEPT_MOVE_ASSIGN(the_allocator) noexcept(std::allocator_traits<the_allocator>::propagate_on_container_move_assignment::value || std::allocator_traits<the_allocator>::is_always_equal::value)
	#define PLF_NOEXCEPT_SWAP(the_allocator) noexcept(std::allocator_traits<the_allocator>::propagate_on_container_swap::value || std::allocator_traits<the_allocator>::is_always_equal::value)
#else
	#define PLF_NOEXCEPT_MOVE_ASSIGN(the_allocator)
	#define PLF_NOEXCEPT_SWAP(the_allocator)
#endif

#ifdef PLF_ALLOCATOR_TRAITS_SUPPORT
	#ifdef PLF_VARIADICS_SUPPORT
		#define PLF_CONSTRUCT(the_allocator, allocator_instance, location, ...) std::allocator_traits<the_allocator>::construct(allocator_instance, location, __VA_ARGS__)
	#else
		#define PLF_CONSTRUCT(the_allocator, allocator_instance, location, data)	std::allocator_traits<the_allocator>::construct(allocator_instance, location, data)
	#endif

	#define PLF_DESTROY(the_allocator, allocator_instance, location)				std::allocator_traits<the_allocator>::destroy(allocator_instance, location)
	#define PLF_ALLOCATE(the_allocator, allocator_instance, size, hint)			std::allocator_traits<the_allocator>::allocate(allocator_instance, size, hint)
	#define PLF_DEALLOCATE(the_allocator, allocator_instance, location, size)	std::allocator_traits<the_allocator>::deallocate(allocator_instance, location, size)
#else
	#ifdef PLF_VARIADICS_SUPPORT
		#define PLF_CONSTRUCT(the_allocator, allocator_instance, location, ...) 	(allocator_instance).construct(location, __VA_ARGS__)
	#else
		#define PLF_CONSTRUCT(the_allocator, allocator_instance, location, data)	(allocator_instance).construct(location, data)
	#endif

	#define PLF_DESTROY(the_allocator, allocator_instance, location)				(allocator_instance).destroy(location)
	#define PLF_ALLOCATE(the_allocator, allocator_instance, size, hint)			(allocator_instance).allocate(size, hint)
	#define PLF_DEALLOCATE(the_allocator, allocator_instance, location, size)	(allocator_instance).deallocate(location, size)
#endif


#ifdef PLF_VARIADICS_SUPPORT
	#define PLF_CONSTRUCT_NODE(location, next, prev, element)	PLF_CONSTRUCT(node_allocator_type, node_allocator_pair, location, next, prev, element)
#else
	#define PLF_CONSTRUCT_NODE(location, next, prev, element)	PLF_CONSTRUCT(node_allocator_type, node_allocator_pair, location, node(next, prev, element))
#endif



#include <cstring> // memmove, memcpy
#include <cassert> // assert
#include <limits>   // std::numeric_limits
#include <memory>  // std::uninitialized_copy, std::allocator, std::to_address
#include <iterator>	// std::bidirectional_iterator_tag, iterator_traits, std::move_iterator, std::distance
#include <stdexcept> // std::length_error
#include <utility> // std::move, std::swap


#if !defined(PLF_SORT_FUNCTION) || defined(PLF_CPP20_SUPPORT)
	#include <algorithm> // std::sort, lexicographical_three_way_compare (C++20)
#endif

#ifdef PLF_TYPE_TRAITS_SUPPORT
	#include <type_traits> // std::is_trivially_destructible, etc
#endif


#ifdef PLF_INITIALIZER_LIST_SUPPORT
	#include <initializer_list>
#endif

#ifdef PLF_CPP20_SUPPORT
	#include <concepts>
	#include <compare> // std::strong_ordering
	#include <ranges>

	namespace plf
	{
		// For getting std:: overload for reverse_iterator to match plf::list iterators specifically (see bottom of header):
		template <class T>
		concept list_iterator_concept = requires { typename T::list_iterator_tag; };

		#ifndef PLF_RANGES
			#define PLF_RANGES

			// For matching ranges which return input_iterator's and match the container's element type:
			template <typename range_type, class element_type>
			concept compatible_range = std::ranges::input_range<range_type> && std::convertible_to<std::ranges::range_reference_t<range_type>, element_type>;

			// Until such point as standard libraries include std::ranges::from_range_t, including this so the rangesv3 constructor overloads will work unambiguously:
			namespace ranges
			{
				struct from_range_t {};
				constexpr from_range_t from_range;
			}
		#endif
	}
#endif



namespace plf
{


#ifndef PLF_TOOLS
	#define PLF_TOOLS

	// std:: tool replacements for C++03/98/11 support:
	template <bool condition, class T = void>
	struct enable_if
	{
		typedef T type;
	};

	template <class T>
	struct enable_if<false, T>
	{};



	template <bool flag, class is_true, class is_false> struct conditional;

	template <class is_true, class is_false> struct conditional<true, is_true, is_false>
	{
		typedef is_true type;
	};

	template <class is_true, class is_false> struct conditional<false, is_true, is_false>
	{
		typedef is_false type;
	};



	template <class element_type>
	struct less
	{
		bool operator() (const element_type &a, const element_type &b) const PLF_NOEXCEPT
		{
			return a < b;
		}
	};



	template<class element_type>
	struct equal_to
	{
		const element_type &value;

		explicit equal_to(const element_type &store_value) PLF_NOEXCEPT:
			value(store_value)
		{}

		bool operator() (const element_type &compare_value) const PLF_NOEXCEPT
		{
			return value == compare_value;
		}
	};



	// To enable conversion to void * when allocator supplies non-raw pointers:
	template <class source_pointer_type>
	static PLF_CONSTFUNC void * void_cast(const source_pointer_type source_pointer) PLF_NOEXCEPT
	{
		#ifdef PLF_CPP20_SUPPORT
			return static_cast<void *>(std::to_address(source_pointer));
		#else
			return static_cast<void *>(&*source_pointer);
		#endif
	}


	#ifdef PLF_MOVE_SEMANTICS_SUPPORT
		template <class iterator_type>
		static PLF_CONSTFUNC std::move_iterator<iterator_type> make_move_iterator(iterator_type it)
		{
			return std::move_iterator<iterator_type>(std::move(it));
		}
	#endif



	enum priority { performance = 1, memory_use = 4};

#endif



template <class element_type, class allocator_type = std::allocator<element_type> > class list : private allocator_type
{
public:
	// Standard container typedefs:
	typedef element_type														value_type;
	typedef unsigned short													group_size_type;

	#ifdef PLF_ALLOCATOR_TRAITS_SUPPORT
		typedef typename std::allocator_traits<allocator_type>::size_type			size_type;
		typedef typename std::allocator_traits<allocator_type>::difference_type difference_type;
		typedef element_type &														reference;
		typedef const element_type &												const_reference;
		typedef typename std::allocator_traits<allocator_type>::pointer 			pointer;
		typedef typename std::allocator_traits<allocator_type>::const_pointer	const_pointer;
	#else
		typedef typename allocator_type::size_type			size_type;
		typedef typename allocator_type::difference_type	difference_type;
		typedef typename allocator_type::reference			reference;
		typedef typename allocator_type::const_reference	const_reference;
		typedef typename allocator_type::pointer				pointer;
		typedef typename allocator_type::const_pointer		const_pointer;
	#endif


	// Iterator declarations:
	template <bool is_const> class	list_iterator;
	typedef list_iterator<false>		iterator;
	typedef list_iterator<true>		const_iterator;
	friend class list_iterator<false>; // Using 'iterator' typedef name here is illegal under C++03
	friend class list_iterator<true>;

	template <bool is_const> class			list_reverse_iterator;
	typedef list_reverse_iterator<false>	reverse_iterator;
	typedef list_reverse_iterator<true>		const_reverse_iterator;
	friend class list_reverse_iterator<false>;
	friend class list_reverse_iterator<true>;


private:
	struct group; // forward declarations for typedefs below
	struct node;

	#ifdef PLF_ALLOCATOR_TRAITS_SUPPORT // >= C++11
		typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<group>				group_allocator_type;
		typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<node>					node_allocator_type;
		typedef typename std::allocator_traits<group_allocator_type>::pointer 									group_pointer_type;
		typedef typename std::allocator_traits<node_allocator_type>::pointer										node_pointer_type;
		typedef typename std::allocator_traits<allocator_type>::template rebind_alloc<node_pointer_type>	node_pointer_allocator_type;
	#else
		typedef typename allocator_type::template rebind<group>::other			group_allocator_type;
		typedef typename allocator_type::template rebind<node>::other			node_allocator_type;
		typedef typename group_allocator_type::pointer 								group_pointer_type;
		typedef typename node_allocator_type::pointer								node_pointer_type;
		typedef typename allocator_type::template rebind<node_pointer_type>::other	node_pointer_allocator_type;
	#endif



	struct node_base
	{
		node_pointer_type next, previous;

		node_base() PLF_NOEXCEPT
		{}

		node_base(const node_pointer_type &n, const node_pointer_type &p) PLF_NOEXCEPT:
			next(n),
			previous(p)
		{}


		#ifdef PLF_MOVE_SEMANTICS_SUPPORT
			node_base(node_pointer_type &&n, node_pointer_type &&p) PLF_NOEXCEPT:
				next(std::move(n)),
				previous(std::move(p))
			{}
		#endif
	};



	struct node : public node_base
	{
		element_type element;

		node(const node_pointer_type next, const node_pointer_type previous, const element_type &source):
			node_base(next, previous),
			element(source)
		{}


		#ifdef PLF_MOVE_SEMANTICS_SUPPORT
			node(node_pointer_type &&next, node_pointer_type &&previous, element_type &&source) PLF_NOEXCEPT:
				node_base(std::move(next), std::move(previous)),
				element(std::move(source))
			{}
		#endif


		#ifdef PLF_VARIADICS_SUPPORT
			template<typename... arguments>
			node(const node_pointer_type next, const node_pointer_type previous, arguments&&... parameters):
				node_base(next, previous),
				element(std::forward<arguments>(parameters) ...)
			{}
		#endif
	};



	struct group : public node_allocator_type // Node memory block + metadata
	{
		node_pointer_type nodes;
		node_pointer_type free_list_head;
		node_pointer_type beyond_end;
		group_size_type number_of_elements;


		group() PLF_NOEXCEPT:
			nodes(NULL),
			free_list_head(NULL),
			beyond_end(NULL),
			number_of_elements(0)
		{}


		#if defined(PLF_VARIADICS_SUPPORT) || defined(PLF_MOVE_SEMANTICS_SUPPORT)
			group(const group_size_type group_size, const node_pointer_type previous = NULL):
				nodes(PLF_ALLOCATE(node_allocator_type, *this, group_size, previous)),
				free_list_head(NULL),
				beyond_end(nodes + group_size),
				number_of_elements(0)
			{}
		#else
			// This is a hack around the fact that allocator_type::construct only supports copy construction in C++03 and copy elision does not occur on the vast majority of compilers in this circumstance. And to avoid running out of memory (and performance loss) from allocating the same block twice, we're allocating in this constructor and moving data in the copy constructor.
			group(const group_size_type group_size, const node_pointer_type previous = NULL) PLF_NOEXCEPT:
				nodes(NULL),
				free_list_head(previous),
				beyond_end(NULL),
				number_of_elements(group_size)
			{}

			// Not a real copy constructor ie. actually a move constructor. Only used for allocator.construct in C++03 for reasons stated above:
			group(const group &source):
				node_allocator_type(source),
				nodes(PLF_ALLOCATE(node_allocator_type, *this, source.number_of_elements, source.free_list_head)),
				free_list_head(NULL),
				beyond_end(nodes + source.number_of_elements),
				number_of_elements(0)
			{}
		#endif


		group & operator = (const group &source) PLF_NOEXCEPT // Actually a move operator, used by c++03 in group_vector's remove, expand_capacity and append functions
		{
			nodes = source.nodes;
			free_list_head = source.free_list_head;
			beyond_end = source.beyond_end;
			number_of_elements = source.number_of_elements;
			return *this;
		}


		#ifdef PLF_MOVE_SEMANTICS_SUPPORT
			group(group &&source) PLF_NOEXCEPT:
				node_allocator_type(source),
				nodes(std::move(source.nodes)),
				free_list_head(std::move(source.free_list_head)),
				beyond_end(std::move(source.beyond_end)),
				number_of_elements(source.number_of_elements)
			{
				source.nodes = NULL;
				source.beyond_end = NULL;
			}


			group & operator = (group &&source) PLF_NOEXCEPT
			{
				nodes = std::move(source.nodes);
				free_list_head = std::move(source.free_list_head);
				beyond_end = std::move(source.beyond_end);
				number_of_elements = std::move(source.number_of_elements);
				source.nodes = NULL;
				source.beyond_end = NULL;
				return *this;
			}
		#endif


		~group() PLF_NOEXCEPT
		{
			PLF_DEALLOCATE(node_allocator_type, *this, nodes, static_cast<size_type>(beyond_end - nodes));
		}
	};




	class group_vector : private allocator_type // Simple vector of groups + associated functions
	{
	public:
		group_pointer_type last_endpoint_group, block_pointer, last_searched_group; // last_endpoint_group is the last -active- group in the block. Other -inactive- (previously used, now empty of elements) groups may be stored after this group for future usage (to reduce deallocation/reallocation of nodes). block_pointer + size - 1 == the last group in the block, regardless of whether or not the group is active.
		size_type size;

		struct ebco_pair2 : node_pointer_allocator_type // empty-base-class optimisation
		{
			size_type capacity; // Total element capacity of all initialized groups
			ebco_pair2(const size_type number_of_elements, const allocator_type &alloc) PLF_NOEXCEPT:
				node_pointer_allocator_type(alloc),
				capacity(number_of_elements)
			{};
		} node_pointer_allocator_pair;

		struct ebco_pair : group_allocator_type
		{
			size_type capacity; // Total number of groups
			ebco_pair(const size_type number_of_groups, const allocator_type &alloc) PLF_NOEXCEPT:
				group_allocator_type(alloc),
				capacity(number_of_groups)
			{};
		} group_allocator_pair;



		group_vector(const allocator_type &alloc) PLF_NOEXCEPT:
			allocator_type(alloc),
			last_endpoint_group(NULL),
			block_pointer(NULL),
			last_searched_group(NULL),
			size(0),
			node_pointer_allocator_pair(0, alloc),
			group_allocator_pair(0, alloc)
		{}



		void blank() PLF_NOEXCEPT
		{
			#ifdef PLF_IS_ALWAYS_EQUAL_SUPPORT // allocator_traits and type_traits always available when is_always_equal is available
				if PLF_CONSTEXPR (std::is_standard_layout<group_vector>::value && std::allocator_traits<allocator_type>::is_always_equal::value && std::is_trivially_destructible<group_pointer_type>::value)
				{
					std::memset(static_cast<void *>(this), 0, sizeof(group_vector));
				}
				else
			#endif
			{
				last_endpoint_group = NULL;
				block_pointer = NULL;
				last_searched_group = NULL;
				size = 0;
				node_pointer_allocator_pair.capacity = 0;
				group_allocator_pair.capacity = 0;
			}
		}



		#ifdef PLF_MOVE_SEMANTICS_SUPPORT
			group_vector(group_vector &&source) PLF_NOEXCEPT:
				allocator_type(source),
				last_endpoint_group(std::move(source.last_endpoint_group)),
				block_pointer(std::move(source.block_pointer)),
				last_searched_group(std::move(source.last_searched_group)),
				size(source.size),
				node_pointer_allocator_pair(source.node_pointer_allocator_pair.capacity, source),
				group_allocator_pair(source.group_allocator_pair.capacity, source)
			{
				source.blank();
			}



			group_vector & operator = (group_vector &&source) PLF_NOEXCEPT
			{
				#ifdef PLF_IS_ALWAYS_EQUAL_SUPPORT
					if PLF_CONSTEXPR ((std::is_trivially_copyable<allocator_type>::value || std::allocator_traits<allocator_type>::is_always_equal::value) && std::is_trivially_copyable<group_pointer_type>::value)
					{
						std::memcpy(static_cast<void *>(this), &source, sizeof(group_vector));
					}
					else
				#endif
				{
					last_endpoint_group = std::move(source.last_endpoint_group);
					block_pointer = std::move(source.block_pointer);
					last_searched_group = std::move(source.last_searched_group);
					size = source.size;
					node_pointer_allocator_pair.capacity = source.node_pointer_allocator_pair.capacity;
					group_allocator_pair.capacity = source.group_allocator_pair.capacity;

					#ifdef PLF_ALLOCATOR_TRAITS_SUPPORT
						if PLF_CONSTEXPR(std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value)
					#endif
					{
						static_cast<allocator_type &>(*this) = static_cast<allocator_type &>(source);
						// Reconstruct rebinds:
		  				static_cast<node_pointer_allocator_type &>(node_pointer_allocator_pair) = node_allocator_type(*this);
						static_cast<group_allocator_type &>(group_allocator_pair) = group_allocator_type(*this);
					}
				}

				source.blank();
				return *this;
			}
		#endif



		void destroy_all_data(const node_pointer_type last_endpoint_node) PLF_NOEXCEPT
		{
			if (block_pointer == NULL) return;

			#ifdef PLF_TYPE_TRAITS_SUPPORT
				if PLF_CONSTEXPR (!std::is_trivially_destructible<element_type>::value || !std::is_trivially_destructible<node_pointer_type>::value)
			#endif
			{
				if (last_endpoint_node != NULL) clear(last_endpoint_node);
			}

			const group_pointer_type end = block_pointer + size;
			for (group_pointer_type current_group = block_pointer; current_group != end; ++current_group)
			{
				PLF_DESTROY(group_allocator_type, group_allocator_pair, current_group);
			}

			PLF_DEALLOCATE(group_allocator_type, group_allocator_pair, block_pointer, group_allocator_pair.capacity);
			blank();
		}



		void destroy_group(const group_pointer_type current_group, const node_pointer_type end_node) PLF_NOEXCEPT
		{
			#ifdef PLF_TYPE_TRAITS_SUPPORT
				if PLF_CONSTEXPR (!std::is_trivially_destructible<element_type>::value || !std::is_trivially_destructible<node_pointer_type>::value)
			#endif
			{
				if ((end_node - current_group->nodes) != current_group->number_of_elements) // If there are erased nodes present in the group
				{
					for (node_pointer_type current_node = current_group->nodes; current_node != end_node; ++current_node)
					{
						#ifdef PLF_TYPE_TRAITS_SUPPORT
							if PLF_CONSTEXPR (!std::is_trivially_destructible<element_type>::value)
						#endif
						{
							if (current_node->next != NULL) // is not part of free list ie. element has not already had it's destructor called
							{
								PLF_DESTROY(allocator_type, *this, &(current_node->element));
							}
						}

						#ifdef PLF_TYPE_TRAITS_SUPPORT
							if PLF_CONSTEXPR (!std::is_trivially_destructible<node_pointer_type>::value)
						#endif
						{
							PLF_DESTROY(node_pointer_allocator_type, node_pointer_allocator_pair, &(current_node->next));
							PLF_DESTROY(node_pointer_allocator_type, node_pointer_allocator_pair, &(current_node->previous));
						}
					}
				}
				else
				{
					for (node_pointer_type current_node = current_group->nodes; current_node != end_node; ++current_node)
					{
						#ifdef PLF_TYPE_TRAITS_SUPPORT
							if PLF_CONSTEXPR (!std::is_trivially_destructible<element_type>::value)
						#endif
						{
							PLF_DESTROY(allocator_type, *this, &(current_node->element));
						}

						#ifdef PLF_TYPE_TRAITS_SUPPORT
							if PLF_CONSTEXPR (!std::is_trivially_destructible<node_pointer_type>::value)
						#endif
						{
							PLF_DESTROY(node_pointer_allocator_type, node_pointer_allocator_pair, &(current_node->next));
							PLF_DESTROY(node_pointer_allocator_type, node_pointer_allocator_pair, &(current_node->previous));
						}
					}
				}
			}

			current_group->free_list_head = NULL;
			current_group->number_of_elements = 0;
		}



		void clear(const node_pointer_type last_endpoint_node) PLF_NOEXCEPT
		{
			for (group_pointer_type current_group = block_pointer; current_group != last_endpoint_group; ++current_group)
			{
				destroy_group(current_group, current_group->beyond_end);
			}

			destroy_group(last_endpoint_group, last_endpoint_node);
			last_searched_group = last_endpoint_group = block_pointer;
		}



		void expand_capacity(const size_type new_capacity) // used by add_new and append
		{
			const group_pointer_type old_block = block_pointer;
			block_pointer = PLF_ALLOCATE(group_allocator_type, group_allocator_pair, new_capacity, 0);

			#ifdef PLF_TYPE_TRAITS_SUPPORT
				if PLF_CONSTEXPR (std::is_trivially_copyable<node_pointer_type>::value && std::is_trivially_destructible<node_pointer_type>::value)
				{
					std::memcpy(plf::void_cast(block_pointer), plf::void_cast(old_block), sizeof(group) * size);
				}
				#ifdef PLF_MOVE_SEMANTICS_SUPPORT
					else if PLF_CONSTEXPR (std::is_move_constructible<node_pointer_type>::value)
					{
						std::uninitialized_copy(plf::make_move_iterator(old_block), plf::make_move_iterator(old_block + size), block_pointer);
					}
				#endif
				else
			#endif
			{
				// If allocator supplies non-trivial pointers it becomes necessary to destroy the group. uninitialized_copy will not work in this context as the copy constructor for "group" is overriden in C++03/98. The = operator for "group" has been overriden to make the following work:
				const group_pointer_type end = old_block + size;

				for (group_pointer_type current_group = old_block, current_new_group = block_pointer; current_group != end; ++current_group)
				{
					*current_new_group++ = *current_group;

					current_group->nodes = NULL;
					current_group->beyond_end = NULL;
					PLF_DESTROY(group_allocator_type, group_allocator_pair, current_group);
				}
			}

			last_searched_group = block_pointer + (last_searched_group - old_block); // correct pointer post-reallocation
			PLF_DEALLOCATE(group_allocator_type, group_allocator_pair, old_block, group_allocator_pair.capacity);
			group_allocator_pair.capacity = new_capacity;
		}



		void add_new(const group_size_type group_size)
		{
			if (group_allocator_pair.capacity == size) expand_capacity(group_allocator_pair.capacity * 2);

			last_endpoint_group = block_pointer + size - 1;

			#ifdef PLF_VARIADICS_SUPPORT
				PLF_CONSTRUCT(group_allocator_type, group_allocator_pair, last_endpoint_group + 1, group_size, last_endpoint_group->nodes);
			#else
				PLF_CONSTRUCT(group_allocator_type, group_allocator_pair, last_endpoint_group + 1, group(group_size, last_endpoint_group->nodes));
			#endif

			++last_endpoint_group; // Doing this here instead of pre-construct to avoid need for a try-catch block
			node_pointer_allocator_pair.capacity += group_size;
			++size;
		}



		void initialize(const group_size_type first_group_capacity) // For adding first group *only* when group vector is completely empty and block_pointer is NULL
		{
			last_endpoint_group = block_pointer = last_searched_group = PLF_ALLOCATE(group_allocator_type, group_allocator_pair, 1, 0);
			group_allocator_pair.capacity = 1;

			#ifdef PLF_VARIADICS_SUPPORT
				PLF_CONSTRUCT(group_allocator_type, group_allocator_pair, last_endpoint_group, first_group_capacity);
			#else
				PLF_CONSTRUCT(group_allocator_type, group_allocator_pair, last_endpoint_group, group(first_group_capacity));
			#endif

			size = 1; // Doing these here instead of pre-construct to avoid need for a try-catch block
			node_pointer_allocator_pair.capacity = first_group_capacity;
		}



		#ifdef PLF_CPP20_SUPPORT
			#define PLF_TO_ADDRESS(pointer) std::to_address(pointer)
		#else
			#define PLF_TO_ADDRESS(pointer) &*(pointer)
		#endif



		void remove(const group_pointer_type group_to_erase) PLF_NOEXCEPT
		{
			if (last_searched_group >= group_to_erase && last_searched_group != block_pointer) --last_searched_group;

			node_pointer_allocator_pair.capacity -= static_cast<size_type>(group_to_erase->beyond_end - group_to_erase->nodes);

			PLF_DESTROY(group_allocator_type, group_allocator_pair, group_to_erase);

			#ifdef PLF_TYPE_TRAITS_SUPPORT
				if PLF_CONSTEXPR (std::is_trivially_copyable<node_pointer_type>::value && std::is_trivially_destructible<node_pointer_type>::value)
				{
					std::memmove(plf::void_cast(group_to_erase), plf::void_cast(group_to_erase + 1), sizeof(group) * (--size - static_cast<size_type>(PLF_TO_ADDRESS(group_to_erase) - PLF_TO_ADDRESS(block_pointer))));
				}
				#ifdef PLF_MOVE_SEMANTICS_SUPPORT
					else if PLF_CONSTEXPR (std::is_move_constructible<node_pointer_type>::value)
					{
						std::move(group_to_erase + 1, block_pointer + size--, group_to_erase);
					}
				#endif
				else
			#endif
			{
				group_pointer_type back = block_pointer + size--;
				std::copy(group_to_erase + 1, back--, group_to_erase);

				back->nodes = NULL;
				back->beyond_end = NULL;
				PLF_DESTROY(group_allocator_type, group_allocator_pair, back);
			}
		}



		void move_to_back(const group_pointer_type group_to_erase)
		{
			if (last_searched_group >= group_to_erase && last_searched_group != block_pointer) --last_searched_group;

			group *temp_group = PLF_ALLOCATE(group_allocator_type, group_allocator_pair, 1, NULL);

			#ifdef PLF_TYPE_TRAITS_SUPPORT
				if PLF_CONSTEXPR (std::is_trivially_copyable<node_pointer_type>::value && std::is_trivially_destructible<node_pointer_type>::value)
				{
					std::memcpy(plf::void_cast(temp_group), plf::void_cast(group_to_erase), sizeof(group));
					std::memmove(plf::void_cast(group_to_erase), plf::void_cast(group_to_erase + 1), sizeof(group) * ((size - 1) - static_cast<size_type>(PLF_TO_ADDRESS(group_to_erase) - PLF_TO_ADDRESS(block_pointer))));
					std::memcpy(plf::void_cast(block_pointer + size - 1), plf::void_cast(temp_group), sizeof(group));
				}
				#ifdef PLF_MOVE_SEMANTICS_SUPPORT
					else if PLF_CONSTEXPR (std::is_move_constructible<node_pointer_type>::value)
					{
						PLF_CONSTRUCT(group_allocator_type, group_allocator_pair, temp_group, std::move(*group_to_erase));
						std::move(group_to_erase + 1, block_pointer + size, group_to_erase);
						*(block_pointer + size - 1) = std::move(*temp_group);

						if PLF_CONSTEXPR (!std::is_trivially_destructible<node_pointer_type>::value)
						{
							PLF_DESTROY(group_allocator_type, group_allocator_pair, temp_group);
						}
					}
				#endif
				else
			#endif
			{
				PLF_CONSTRUCT(group_allocator_type, group_allocator_pair, temp_group, group());

				*temp_group = *group_to_erase;
				std::copy(group_to_erase + 1, block_pointer + size, group_to_erase);
				*(block_pointer + --size) = *temp_group;

				temp_group->nodes = NULL;
				PLF_DESTROY(group_allocator_type, group_allocator_pair, temp_group);
			}

			PLF_DEALLOCATE(group_allocator_type, group_allocator_pair, temp_group, 1);
		}



		group_pointer_type get_nearest_freelist_group(const node_pointer_type location_node) PLF_NOEXCEPT // In working implementation this cannot throw
		{
			const group_pointer_type beyond_end_group = last_endpoint_group + 1;
			group_pointer_type left = last_searched_group - 1, right = last_searched_group + 1, freelist_group = NULL;
			bool right_not_beyond_back = right < beyond_end_group;
			bool left_not_beyond_front = left >= block_pointer;


			if (location_node >= last_searched_group->nodes && location_node < last_searched_group->beyond_end) // ie. location is within last_search_group
			{
				if (last_searched_group->free_list_head != NULL) return last_searched_group; // if last_searched_group has previously-erased nodes
				// Else: search outwards using loop below this if/else block
			}
			else // search for the node group which location_node is located within, using last_searched_group as a starting point and searching left and right. Try and find the closest node group with reusable erased-element locations along the way:
			{
				group_pointer_type closest_freelist_left = (last_searched_group->free_list_head == NULL) ? NULL : last_searched_group;
				group_pointer_type closest_freelist_right = closest_freelist_left;

				while (true)
				{
					if (right_not_beyond_back)
					{
						if (location_node < right->beyond_end && location_node >= right->nodes) // location_node's group is found
						{
							last_searched_group = right;
							if (right->free_list_head != NULL) return right; // group has erased nodes, reuse them:
							difference_type left_distance;

							if (closest_freelist_right != NULL)
							{
								left_distance = right - closest_freelist_right;
								if (left_distance <= 2) return closest_freelist_right; // ie. this group is close enough to location_node's group
								freelist_group = closest_freelist_right;
							}
							else
							{
								left_distance = right - left;
							}


							// Otherwise find closest group with freelist - check an equal distance on the right to the distance we've checked on the left:
							const group_pointer_type right_plus_distance = right + left_distance;
							const group_pointer_type end_group = (right_plus_distance > beyond_end_group) ? beyond_end_group : right_plus_distance - 1;

							while (++right != end_group)
							{
								if (right->free_list_head != NULL) return right;
							}

							if (freelist_group != NULL) return freelist_group;

							right_not_beyond_back = right < beyond_end_group;
							break; // group with reusable erased nodes not found yet, continue searching in loop below
						}

						if (right->free_list_head != NULL) // location_node's group not found, but a reusable location found
						{
							if ((closest_freelist_right == NULL) & (closest_freelist_left == NULL)) closest_freelist_left = right;
							closest_freelist_right = right;
						}

						right_not_beyond_back = ++right < beyond_end_group;
					}


					if (left_not_beyond_front)
					{
						if (location_node >= left->nodes && location_node < left->beyond_end)
						{
							last_searched_group = left;
							if (left->free_list_head != NULL) return left;
							difference_type right_distance;

							if (closest_freelist_left != NULL)
							{
								right_distance = closest_freelist_left - left;
								if (right_distance <= 2) return closest_freelist_left;
								freelist_group = closest_freelist_left;
							}
							else
							{
								right_distance = right - left;
							}

							// Otherwise find closest group with freelist:
							const group_pointer_type left_minus_distance = left - right_distance;
							const group_pointer_type end_group = (left_minus_distance < block_pointer) ? block_pointer - 1 : left_minus_distance + 1;

							while (--left != end_group)
							{
								if (left->free_list_head != NULL) return left;
							}

							if (freelist_group != NULL) return freelist_group;
							left_not_beyond_front = left >= block_pointer;
							break;
						}

						if (left->free_list_head != NULL)
						{
							if ((closest_freelist_left == NULL) & (closest_freelist_right == NULL)) closest_freelist_right = left;
							closest_freelist_left = left;
						}

						left_not_beyond_front = --left >= block_pointer;
					}
				}
			}


			// The node group which location_node is located within, is known at this point. Continue searching outwards from this group until a group is found with a reusable location:
			while (true)
			{
				if (right_not_beyond_back)
				{
					if (right->free_list_head != NULL) return right;
					right_not_beyond_back = ++right < beyond_end_group;
				}

				if (left_not_beyond_front)
				{
					if (left->free_list_head != NULL) return left;
					left_not_beyond_front = --left >= block_pointer;
				}
			}

			// Will never reach here
		}



		void swap(group_vector &source) PLF_NOEXCEPT_SWAP(group_allocator_type)
		{
			#ifdef PLF_IS_ALWAYS_EQUAL_SUPPORT
				if PLF_CONSTEXPR (std::allocator_traits<allocator_type>::is_always_equal::value && std::is_trivially_copyable<group_pointer_type>::value) // if all pointer types are trivial we can just copy using memcpy - avoids constructors/destructors etc and is faster
				{
					char temp[sizeof(group_vector)];
					std::memcpy(static_cast<void *>(&temp), static_cast<void *>(this), sizeof(group_vector));
					std::memcpy(static_cast<void *>(this), static_cast<void *>(&source), sizeof(group_vector));
					std::memcpy(static_cast<void *>(&source), static_cast<void *>(&temp), sizeof(group_vector));
				}
				else
			#endif
			{
				// Otherwise, make the reads/writes as contiguous in memory as-possible (yes, it is faster than using std::swap with the individual variables):
				const group_pointer_type swap_last_endpoint_group = last_endpoint_group, swap_block_pointer = block_pointer, swap_last_searched_group = last_searched_group;
				const size_type swap_size = size, swap_element_capacity = node_pointer_allocator_pair.capacity, swap_capacity = group_allocator_pair.capacity;

				last_endpoint_group = source.last_endpoint_group;
				block_pointer = source.block_pointer;
				last_searched_group = source.last_searched_group;
				size = source.size;
				node_pointer_allocator_pair.capacity = source.node_pointer_allocator_pair.capacity;
				group_allocator_pair.capacity = source.group_allocator_pair.capacity;

				source.last_endpoint_group = swap_last_endpoint_group;
				source.block_pointer = swap_block_pointer;
				source.last_searched_group = swap_last_searched_group;
				source.size = swap_size;
				source.node_pointer_allocator_pair.capacity = swap_element_capacity;
				source.group_allocator_pair.capacity = swap_capacity;

				#ifdef PLF_IS_ALWAYS_EQUAL_SUPPORT
					if PLF_CONSTEXPR (std::allocator_traits<allocator_type>::propagate_on_container_swap::value && !std::allocator_traits<allocator_type>::is_always_equal::value)
				#endif
				{
					std::swap(static_cast<allocator_type &>(source), static_cast<allocator_type &>(*this));

					// Reconstruct rebinds for swapped allocators:
					static_cast<node_pointer_allocator_type &>(node_pointer_allocator_pair) = node_pointer_allocator_type(*this);
					static_cast<group_allocator_type &>(group_allocator_pair) = group_allocator_type(*this);
					static_cast<node_pointer_allocator_type &>(source.node_pointer_allocator_pair) = node_pointer_allocator_type(source);
					static_cast<group_allocator_type &>(source.group_allocator_pair) = group_allocator_type(source);
				} // else: undefined behaviour, as per standard
			}
		}



		void trim_unused_groups() PLF_NOEXCEPT // trim trailing groups previously allocated by reserve() or retained via erase()
		{
			const group_pointer_type end = block_pointer + size;

			for (group_pointer_type current_group = last_endpoint_group + 1; current_group != end; ++current_group)
			{
				node_pointer_allocator_pair.capacity -= static_cast<size_type>(current_group->beyond_end - current_group->nodes);
				PLF_DESTROY(group_allocator_type, group_allocator_pair, current_group);
			}

			size -= static_cast<size_type>(end - (last_endpoint_group + 1));
		}



		void append(group_vector &source)
		{
			source.trim_unused_groups();
			trim_unused_groups();

			if (size + source.size > group_allocator_pair.capacity) expand_capacity(size + source.size);

			#ifdef PLF_TYPE_TRAITS_SUPPORT
				if PLF_CONSTEXPR (std::is_trivially_copyable<node_pointer_type>::value && std::is_trivially_destructible<node_pointer_type>::value)
				{
					std::memcpy(plf::void_cast(block_pointer + size), plf::void_cast(source.block_pointer), sizeof(group) * source.size);
				}
				#ifdef PLF_MOVE_SEMANTICS_SUPPORT
					else if PLF_CONSTEXPR (std::is_move_constructible<node_pointer_type>::value)
					{
						std::uninitialized_copy(plf::make_move_iterator(source.block_pointer), plf::make_move_iterator(source.block_pointer + source.size), block_pointer + size);
					}
				#endif
				else
			#endif
			{
				const group_pointer_type end = source.block_pointer + source.size;

				for (group_pointer_type current_group = source.block_pointer, current_new_group = block_pointer + size; current_group != end; ++current_group)
				{
					*current_new_group++ = *current_group;

					current_group->nodes = NULL;
					current_group->beyond_end = NULL;
					PLF_DESTROY(group_allocator_type, source.group_allocator_pair, current_group);
				}
			}

			PLF_DEALLOCATE(group_allocator_type, source.group_allocator_pair, source.block_pointer, source.group_allocator_pair.capacity);
			size += source.size;
			last_endpoint_group = block_pointer + size - 1;
			node_pointer_allocator_pair.capacity += source.node_pointer_allocator_pair.capacity;
			source.blank();
		}
	};



private:

	group_vector groups; // Structure which contains all groups (structures containing node memory blocks + block metadata)
	node_base end_node; // The independent, content-less node which is returned by end()
	// When the list is empty, the previous and next pointers of end_node both point to end_node.
	node_pointer_type last_endpoint; // The node location which is one-past the last inserted element in the last group of the list. Is not affected by erasures to prior elements (these are handled using the group's freelist).
	// If last_endpoint is beyond the end of a memory block it means a new group must be created upon the next insertion if prior erased nodes are not available for re-use.
	// last_endpoint == NULL means total_size is zero, but there may still be groups available due to calling clear(), reserve() on an empty list, or having erased all elements in the list
	// groups.block_pointer == NULL means an uninitialized container ie. no groups or elements yet
	iterator end_iterator, begin_iterator; // Returned by begin() and end().
	// end_iterator always points to end_node. It is a convenience/optimization variable to save generating many temporary iterators from end_node during functions and during end().
	// When the list is empty of elements, begin_iterator == end_iterator so that program loops iterating from begin() to end() will function as expected.
	size_type total_size;

	struct ebco_pair3 : node_allocator_type
	{
		size_type number_of_erased_nodes;
		ebco_pair3(const size_type num_nodes, const allocator_type &alloc) PLF_NOEXCEPT:
			node_allocator_type(alloc),
			number_of_erased_nodes(num_nodes)
		{};
	}		node_allocator_pair;



public:

	// Default constructor:

	PLF_CONSTFUNC list() PLF_NOEXCEPT_ALLOCATOR:
		groups(*this),
		end_node(static_cast<node_pointer_type>(&end_node), static_cast<node_pointer_type>(&end_node)),
		last_endpoint(NULL),
		end_iterator(static_cast<node_pointer_type>(&end_node)),
		begin_iterator(static_cast<node_pointer_type>(&end_node)),
		total_size(0),
		node_allocator_pair(0, *this)
	{}



	// Allocator-extended constructor:

	explicit list(const allocator_type &alloc):
		allocator_type(alloc),
		groups(alloc),
		end_node(static_cast<node_pointer_type>(&end_node), static_cast<node_pointer_type>(&end_node)),
		last_endpoint(NULL),
		end_iterator(static_cast<node_pointer_type>(&end_node)),
		begin_iterator(static_cast<node_pointer_type>(&end_node)),
		total_size(0),
		node_allocator_pair(0, alloc)
	{}



	// Copy constructor:

	list(const list &source):
		#if (defined(__cplusplus) && __cplusplus >= 201103L) || _MSC_VER >= 1700
			allocator_type(std::allocator_traits<allocator_type>::select_on_container_copy_construction(source)),
		#else
			allocator_type(source),
		#endif
		groups(*this),
		end_node(static_cast<node_pointer_type>(&end_node), static_cast<node_pointer_type>(&end_node)),
		last_endpoint(NULL),
		end_iterator(static_cast<node_pointer_type>(&end_node)),
		begin_iterator(static_cast<node_pointer_type>(&end_node)),
		total_size(0),
		node_allocator_pair(0, *this)
	{
		range_insert(end_iterator, source.total_size, source.begin_iterator);
	}



	// Allocator-extended copy constructor:

	#ifdef PLF_CPP20_SUPPORT
		list(const list &source, const std::type_identity_t<allocator_type> &alloc):
	#else
		list(const list &source, const allocator_type &alloc):
	#endif
		allocator_type(alloc),
		groups(alloc),
		end_node(static_cast<node_pointer_type>(&end_node), static_cast<node_pointer_type>(&end_node)),
		last_endpoint(NULL),
		end_iterator(static_cast<node_pointer_type>(&end_node)),
		begin_iterator(static_cast<node_pointer_type>(&end_node)),
		total_size(0),
		node_allocator_pair(0, alloc)
	{
		range_insert(end_iterator, source.total_size, source.begin_iterator);
	}



private:

	void blank() PLF_NOEXCEPT
	{
		end_node.next = static_cast<node_pointer_type>(&end_node);
		end_node.previous = static_cast<node_pointer_type>(&end_node);
		last_endpoint = NULL;
		begin_iterator.node_pointer = end_iterator.node_pointer;
		total_size = 0;
		node_allocator_pair.number_of_erased_nodes = 0;
	}



	void reset() PLF_NOEXCEPT
	{
		groups.destroy_all_data(last_endpoint);
		blank();
	}



public:

	#ifdef PLF_MOVE_SEMANTICS_SUPPORT
		// Move constructor:

		list(list &&source) PLF_NOEXCEPT:
			allocator_type(static_cast<allocator_type &>(source)),
			groups(std::move(source.groups)),
			end_node(std::move(source.end_node)),
			last_endpoint(std::move(source.last_endpoint)),
			end_iterator(static_cast<node_pointer_type>(&end_node)),
			begin_iterator((source.begin_iterator.node_pointer == source.end_iterator.node_pointer) ? static_cast<node_pointer_type>(&end_node) : std::move(source.begin_iterator)),
			total_size(source.total_size),
			node_allocator_pair(source.node_allocator_pair.number_of_erased_nodes, source)
		{
			end_node.previous->next = begin_iterator.node_pointer->previous = end_iterator.node_pointer;
			source.groups.blank();
			source.blank();
		}



		// Allocator-extended move constructor:

		#ifdef PLF_CPP20_SUPPORT
			list(list &&source, const std::type_identity_t<allocator_type> &alloc):
		#else
			list(list &&source, const allocator_type &alloc):
		#endif
			allocator_type(alloc),
			groups(std::move(source.groups)),
			end_node(std::move(source.end_node)),
			last_endpoint(std::move(source.last_endpoint)),
			end_iterator(static_cast<node_pointer_type>(&end_node)),
			begin_iterator((source.begin_iterator.node_pointer == source.end_iterator.node_pointer) ? static_cast<node_pointer_type>(&end_node) : std::move(source.begin_iterator)),
			total_size(source.total_size),
			node_allocator_pair(source.node_allocator_pair.number_of_erased_nodes, alloc)
		{
			#ifdef PLF_IS_ALWAYS_EQUAL_SUPPORT
				if PLF_CONSTEXPR (!std::allocator_traits<allocator_type>::is_always_equal::value)
			#endif
			{
				if (alloc != static_cast<allocator_type &>(source))
				{
					blank();
					range_insert(end_iterator, source.total_size, source.begin_iterator);
					source.~list();
				}
			}

			end_node.previous->next = begin_iterator.node_pointer->previous = end_iterator.node_pointer;
			source.groups.blank();
			source.blank();
		}
	#endif



	// Fill constructor:

	list(const size_type fill_number, const element_type &element, const allocator_type &alloc = allocator_type()):
		allocator_type(alloc),
		groups(alloc),
		end_node(static_cast<node_pointer_type>(&end_node), static_cast<node_pointer_type>(&end_node)),
		last_endpoint(NULL),
		end_iterator(static_cast<node_pointer_type>(&end_node)),
		begin_iterator(static_cast<node_pointer_type>(&end_node)),
		total_size(0),
		node_allocator_pair(0, alloc)
	{
		insert(end_iterator, fill_number, element);
	}



	// Default element value fill constructor:

	list(const size_type fill_number, const allocator_type &alloc = allocator_type()):
		allocator_type(alloc),
		groups(alloc),
		end_node(static_cast<node_pointer_type>(&end_node), static_cast<node_pointer_type>(&end_node)),
		last_endpoint(NULL),
		end_iterator(static_cast<node_pointer_type>(&end_node)),
		begin_iterator(static_cast<node_pointer_type>(&end_node)),
		total_size(0),
		node_allocator_pair(0, alloc)
	{
		insert(end_iterator, fill_number, element_type());
	}



	// Range constructor:

	template<typename iterator_type>
	list(const typename plf::enable_if<!std::numeric_limits<iterator_type>::is_integer, iterator_type>::type &first, const iterator_type &last, const allocator_type &alloc = allocator_type()):
		allocator_type(alloc),
		groups(alloc),
		end_node(static_cast<node_pointer_type>(&end_node), static_cast<node_pointer_type>(&end_node)),
		last_endpoint(NULL),
		end_iterator(static_cast<node_pointer_type>(&end_node)),
		begin_iterator(static_cast<node_pointer_type>(&end_node)),
		total_size(0),
		node_allocator_pair(0, alloc)
	{
		insert<iterator_type>(end_iterator, first, last);
	}



	// Initializer-list constructor:

	#ifdef PLF_INITIALIZER_LIST_SUPPORT
		list(const std::initializer_list<element_type> &element_list, const allocator_type &alloc = allocator_type()):
			allocator_type(alloc),
			groups(alloc),
			end_node(static_cast<node_pointer_type>(&end_node), static_cast<node_pointer_type>(&end_node)),
			last_endpoint(NULL),
			end_iterator(static_cast<node_pointer_type>(&end_node)),
			begin_iterator(static_cast<node_pointer_type>(&end_node)),
			total_size(0),
			node_allocator_pair(0, alloc)
		{
			range_insert(end_iterator, static_cast<size_type>(element_list.size()), element_list.begin());
		}

	#endif



	#ifdef PLF_CPP20_SUPPORT
		// Ranges v3 constructor:

		template<compatible_range<element_type> range_type>
		list(plf::ranges::from_range_t, range_type &&rg, const allocator_type &alloc = allocator_type()):
			allocator_type(alloc),
			end_node(static_cast<node_pointer_type>(&end_node), static_cast<node_pointer_type>(&end_node)),
			last_endpoint(NULL),
			end_iterator(static_cast<node_pointer_type>(&end_node)),
			begin_iterator(static_cast<node_pointer_type>(&end_node)),
			total_size(0),
			node_allocator_pair(0, alloc)
		{
			range_insert(end_iterator, static_cast<size_type>(std::ranges::distance(rg)), std::ranges::begin(rg));
		}
	#endif



	~list() PLF_NOEXCEPT
	{
		groups.destroy_all_data(last_endpoint);
	}



	iterator begin() PLF_NOEXCEPT
	{
		return begin_iterator;
	}



	const_iterator begin() const PLF_NOEXCEPT
	{
		return begin_iterator;
	}



	iterator end() PLF_NOEXCEPT
	{
		return end_iterator;
	}



	const_iterator end() const PLF_NOEXCEPT
	{
		return end_iterator;
	}



	const_iterator cbegin() const PLF_NOEXCEPT
	{
		return const_iterator(begin_iterator.node_pointer);
	}



	const_iterator cend() const PLF_NOEXCEPT
	{
		return const_iterator(end_iterator.node_pointer);
	}



 	reverse_iterator rbegin() PLF_NOEXCEPT
 	{
 		return reverse_iterator(end_node.previous);
 	}



 	const_reverse_iterator rbegin() const PLF_NOEXCEPT
 	{
 		return const_reverse_iterator(end_node.previous);
 	}



 	reverse_iterator rend() PLF_NOEXCEPT
 	{
 		return reverse_iterator(end_iterator.node_pointer);
 	}



 	const_reverse_iterator rend() const PLF_NOEXCEPT
 	{
 		return const_reverse_iterator(end_iterator.node_pointer);
 	}



 	const_reverse_iterator crbegin() const PLF_NOEXCEPT
 	{
 		return const_reverse_iterator(end_node.previous);
 	}



 	const_reverse_iterator crend() const PLF_NOEXCEPT
 	{
 		return const_reverse_iterator(end_iterator.node_pointer);
 	}



	reference front()
	{
		assert(total_size != 0);
		return begin_iterator.node_pointer->element;
	}



	const_reference front() const
	{
		assert(total_size != 0);
		return begin_iterator.node_pointer->element;
	}



	reference back()
	{
		assert(total_size != 0);
		return end_node.previous->element;
	}



	const_reference back() const
	{
		assert(total_size != 0);
		return end_node.previous->element;
	}



	void clear() PLF_NOEXCEPT
	{
		if (last_endpoint == NULL) return; // already clear'ed or uninitialized
		if (total_size != 0) groups.clear(last_endpoint);
		blank();
	}



private:



	void add_group_if_necessary()
	{
		if (last_endpoint == groups.last_endpoint_group->beyond_end) // last_endpoint is beyond the end of a group
		{
			if (static_cast<size_type>(groups.last_endpoint_group - groups.block_pointer) == groups.size - 1) // ie. there are no reusable groups available at the back of group vector
			{
				groups.add_new((total_size < list_max_block_capacity()) ? static_cast<group_size_type>(total_size) : list_max_block_capacity());
			}
			else
			{
				++groups.last_endpoint_group;
			}

			last_endpoint = groups.last_endpoint_group->nodes;
		}
	}



	void update_sizes_and_iterators(const const_iterator it)
	{
		++(groups.last_endpoint_group->number_of_elements);
		++total_size;
		if (it.node_pointer == begin_iterator.node_pointer) begin_iterator.node_pointer = last_endpoint;
		it.node_pointer->previous->next = last_endpoint;
		it.node_pointer->previous = last_endpoint;
	}



	static PLF_CONSTFUNC group_size_type list_min_block_capacity() PLF_NOEXCEPT
	{
		return static_cast<group_size_type>((sizeof(node) * 8 > (sizeof(list) + sizeof(group)) * 2) ? 8 : (((sizeof(list) + sizeof(group)) * 2) / sizeof(node)) + 1);
	}



	static PLF_CONSTFUNC group_size_type list_max_block_capacity() PLF_NOEXCEPT
	{
		return 2048u;
	}



	void insert_initialize()
	{
		if (groups.block_pointer == NULL) groups.initialize(list_min_block_capacity()); // In case of prior reserve/clear call as opposed to being uninitialized

		groups.last_endpoint_group->number_of_elements = 1;
		end_node.next = end_node.previous = last_endpoint = begin_iterator.node_pointer = groups.last_endpoint_group->nodes;
		total_size = 1;
	}



public:


	iterator insert(const const_iterator it, const element_type &element)
	{
		if (last_endpoint != NULL) // ie. list is not empty
		{
			if (node_allocator_pair.number_of_erased_nodes == 0) // No erased nodes available for reuse
			{
				add_group_if_necessary();
				PLF_CONSTRUCT_NODE(last_endpoint, it.node_pointer, it.node_pointer->previous, element);
				update_sizes_and_iterators(it);
				return iterator(last_endpoint++);
			}
			else
			{
				const group_pointer_type node_group = groups.get_nearest_freelist_group((it.node_pointer != end_iterator.node_pointer) ? it.node_pointer : end_node.previous);
				const node_pointer_type selected_node = node_group->free_list_head;
				const node_pointer_type previous = node_group->free_list_head->previous;
				PLF_CONSTRUCT_NODE(selected_node, it.node_pointer, it.node_pointer->previous, element);

				node_group->free_list_head = previous;
				++(node_group->number_of_elements);
				++total_size;
				--node_allocator_pair.number_of_erased_nodes;

				it.node_pointer->previous->next = selected_node;
				it.node_pointer->previous = selected_node;

				if (it.node_pointer == begin_iterator.node_pointer) begin_iterator.node_pointer = selected_node;
				return iterator(selected_node);
			}
		}
		else // list is empty
		{
			insert_initialize();

			#ifndef PLF_EXCEPTIONS_SUPPORT
				PLF_CONSTRUCT_NODE(last_endpoint++, end_iterator.node_pointer, end_iterator.node_pointer, element);
	 		#else
				#ifdef PLF_TYPE_TRAITS_SUPPORT
					if PLF_CONSTEXPR (std::is_nothrow_copy_constructible<node>::value) // Avoid try-catch code generation
					{
						PLF_CONSTRUCT_NODE(last_endpoint++, end_iterator.node_pointer, end_iterator.node_pointer, element);
					}
					else
				#endif
				{
					try
					{
						PLF_CONSTRUCT_NODE(last_endpoint++, end_iterator.node_pointer, end_iterator.node_pointer, element);
					}
					catch (...)
					{
						reset();
						throw;
					}
				}
			#endif

			return begin_iterator;
		}
	}



	void push_back(const element_type &element)
	{
		insert(end_iterator, element);
	}



	void push_front(const element_type &element)
	{
		insert(begin_iterator, element);
	}



	#ifdef PLF_MOVE_SEMANTICS_SUPPORT
		iterator insert(const const_iterator it, element_type &&element) // This is almost identical to the insert implementation above with the only change being std::move of the element and the is_nothrow test
		{
			if (last_endpoint != NULL)
			{
				if (node_allocator_pair.number_of_erased_nodes == 0)
				{
					add_group_if_necessary();
					PLF_CONSTRUCT_NODE(last_endpoint, it.node_pointer, it.node_pointer->previous, std::move(element));
					update_sizes_and_iterators(it);
					return iterator(last_endpoint++);
				}
				else
				{
					const group_pointer_type node_group = groups.get_nearest_freelist_group((it.node_pointer != end_iterator.node_pointer) ? it.node_pointer : end_node.previous);
					const node_pointer_type selected_node = node_group->free_list_head;
					const node_pointer_type previous = node_group->free_list_head->previous;
					PLF_CONSTRUCT_NODE(selected_node, it.node_pointer, it.node_pointer->previous, std::move(element));

					node_group->free_list_head = previous;
					++(node_group->number_of_elements);
					++total_size;
					--node_allocator_pair.number_of_erased_nodes;

					it.node_pointer->previous->next = selected_node;
					it.node_pointer->previous = selected_node;

					if (it.node_pointer == begin_iterator.node_pointer) begin_iterator.node_pointer = selected_node;
					return iterator(selected_node);
				}
			}
			else
			{
				insert_initialize();

				#ifndef PLF_EXCEPTIONS_SUPPORT
					PLF_CONSTRUCT_NODE(last_endpoint++, end_iterator.node_pointer, end_iterator.node_pointer, std::move(element));
		 		#else
					#ifdef PLF_TYPE_TRAITS_SUPPORT
						if PLF_CONSTEXPR (std::is_nothrow_move_constructible<node>::value)
						{
							PLF_CONSTRUCT_NODE(last_endpoint++, end_iterator.node_pointer, end_iterator.node_pointer, std::move(element));
						}
						else
					#endif
					{
						try
						{
							PLF_CONSTRUCT_NODE(last_endpoint++, end_iterator.node_pointer, end_iterator.node_pointer, std::move(element));
						}
						catch (...)
						{
							reset();
							throw;
						}
					}
				#endif

				return begin_iterator;
			}
		}



		void push_back(element_type &&element)
		{
			insert(end_iterator, std::move(element));
		}



		void push_front(element_type &&element)
		{
			insert(begin_iterator, std::move(element));
		}
	#endif




	#ifdef PLF_VARIADICS_SUPPORT
		template<typename... arguments>
		iterator emplace(const const_iterator it, arguments &&... parameters) // This is almost identical to the insert implementations above with the only changes being std::forward of element parameters, removal of VARIADICS support checking, and is_nothrow_contructible
		{
			if (last_endpoint != NULL)
			{
				if (node_allocator_pair.number_of_erased_nodes == 0)
				{
					add_group_if_necessary();
					PLF_CONSTRUCT_NODE(last_endpoint, it.node_pointer, it.node_pointer->previous, std::forward<arguments>(parameters)...);
					update_sizes_and_iterators(it);
					return iterator(last_endpoint++);
				}
				else
				{
					const group_pointer_type node_group = groups.get_nearest_freelist_group((it.node_pointer != end_iterator.node_pointer) ? it.node_pointer : end_node.previous);
					const node_pointer_type selected_node = node_group->free_list_head;
					const node_pointer_type previous = node_group->free_list_head->previous;
					PLF_CONSTRUCT_NODE(selected_node, it.node_pointer, it.node_pointer->previous, std::forward<arguments>(parameters)...);

					node_group->free_list_head = previous;
					++(node_group->number_of_elements);
					++total_size;
					--node_allocator_pair.number_of_erased_nodes;

					it.node_pointer->previous->next = selected_node;
					it.node_pointer->previous = selected_node;

					if (it.node_pointer == begin_iterator.node_pointer) begin_iterator.node_pointer = selected_node;
					return iterator(selected_node);
				}
			}
			else
			{
			  	insert_initialize();

				#ifndef PLF_EXCEPTIONS_SUPPORT
					PLF_CONSTRUCT_NODE(last_endpoint++, end_iterator.node_pointer, end_iterator.node_pointer, std::forward<arguments>(parameters)...);
		 		#else
					#ifdef PLF_TYPE_TRAITS_SUPPORT
						if PLF_CONSTEXPR (std::is_nothrow_constructible<node>::value)
						{
							PLF_CONSTRUCT_NODE(last_endpoint++, end_iterator.node_pointer, end_iterator.node_pointer, std::forward<arguments>(parameters)...);
						}
						else
					#endif
					{
						try
						{
							PLF_CONSTRUCT_NODE(last_endpoint++, end_iterator.node_pointer, end_iterator.node_pointer, std::forward<arguments>(parameters)...);
						}
						catch (...)
						{
							reset();
							throw;
						}
					}
				#endif

				return begin_iterator;
			}
		}



		template<typename... arguments>
		reference emplace_back(arguments &&... parameters)
		{
			return (emplace(end_iterator, std::forward<arguments>(parameters)...)).node_pointer->element;
		}



		template<typename... arguments>
		reference emplace_front(arguments &&... parameters)
		{
			return (emplace(begin_iterator, std::forward<arguments>(parameters)...)).node_pointer->element;
		}


	#endif




private:

	void fill(const element_type &element, group_size_type number_of_elements, const node_pointer_type position)
	{
		position->previous->next = last_endpoint;
		groups.last_endpoint_group->number_of_elements = static_cast<group_size_type>(groups.last_endpoint_group->number_of_elements + number_of_elements);
		node_pointer_type previous = position->previous;

		do
		{
			#ifdef PLF_TYPE_TRAITS_SUPPORT
				if PLF_CONSTEXPR (std::is_nothrow_copy_constructible<element_type>::value)
				{
					PLF_CONSTRUCT_NODE(last_endpoint, last_endpoint + 1, previous, element);
				}
				else
			#endif
			{
				#ifdef PLF_EXCEPTIONS_SUPPORT
					try
					{
						PLF_CONSTRUCT_NODE(last_endpoint, last_endpoint + 1, previous, element);
					}
					catch (...)
					{
						previous->next = position;
						position->previous = --previous;
						groups.last_endpoint_group->number_of_elements = static_cast<group_size_type>(groups.last_endpoint_group->number_of_elements - (number_of_elements - (last_endpoint - position)));
						throw;
					}
				#else
					PLF_CONSTRUCT_NODE(last_endpoint, last_endpoint + 1, previous, element);
				#endif
			}

			previous = last_endpoint++;
		} while (--number_of_elements != 0);

		previous->next = position;
		position->previous = previous;
	}




	template <class iterator_type>
	void range_fill(iterator_type &it, group_size_type number_of_elements, const node_pointer_type position)
	{
		position->previous->next = last_endpoint;
		groups.last_endpoint_group->number_of_elements = static_cast<group_size_type>(groups.last_endpoint_group->number_of_elements + number_of_elements);
		node_pointer_type previous = position->previous;

		do
		{
			#ifdef PLF_TYPE_TRAITS_SUPPORT
				if PLF_CONSTEXPR (std::is_nothrow_copy_constructible<element_type>::value)
				{
					PLF_CONSTRUCT_NODE(last_endpoint, last_endpoint + 1, previous, *it++);
				}
				else
			#endif
			{
				#ifdef PLF_EXCEPTIONS_SUPPORT
					try
					{
						PLF_CONSTRUCT_NODE(last_endpoint, last_endpoint + 1, previous, *it++);
					}
					catch (...)
					{
						previous->next = position;
						position->previous = --previous;
						groups.last_endpoint_group->number_of_elements = static_cast<group_size_type>(groups.last_endpoint_group->number_of_elements - (number_of_elements - (last_endpoint - position)));
						throw;
					}
				#else
					PLF_CONSTRUCT_NODE(last_endpoint, last_endpoint + 1, previous, *it++);
				#endif
			}

			previous = last_endpoint++;
		} while (--number_of_elements != 0);

		previous->next = position;
		position->previous = previous;
	}



	// This function is near-identical to fill-insert, the only difference is it uses and iterates over an iterator:
	template <class iterator_type>
	iterator range_insert(const const_iterator position, const size_type number_of_elements, iterator_type it)
	{
		if (number_of_elements == 0)
		{
			return end_iterator;
		}
		else if (number_of_elements == 1)
		{
			return insert(position, *it);
		}

		reserve(total_size + number_of_elements);

		// Insert first element, then use up any erased nodes:
		size_type remainder = number_of_elements - 1;
		const iterator return_iterator = insert(position, *it++);

		while (node_allocator_pair.number_of_erased_nodes != 0)
		{
			insert(position, *it++);
			if (--remainder == 0) return return_iterator;
		}

		total_size += remainder;

		// then use up remainder of last_endpoint_group:
		const group_size_type remaining_nodes_in_group = static_cast<group_size_type>(groups.last_endpoint_group->beyond_end - last_endpoint);

		if (remaining_nodes_in_group != 0)
		{
			if (remaining_nodes_in_group < remainder)
			{
				range_fill(it, remaining_nodes_in_group, position.node_pointer);
				remainder -= remaining_nodes_in_group;
			}
			else
			{
				range_fill(it, static_cast<group_size_type>(remainder), position.node_pointer);
				return return_iterator;
			}
		}


		// Then start using trailing (reserved) groups:
		while (true)
		{
			last_endpoint = (++groups.last_endpoint_group)->nodes;
			const group_size_type group_size = static_cast<group_size_type>(groups.last_endpoint_group->beyond_end - groups.last_endpoint_group->nodes);

			if (group_size < remainder)
			{
				range_fill(it, group_size, position.node_pointer);
				remainder -= group_size;
			}
			else
			{
				range_fill(it, static_cast<group_size_type>(remainder), position.node_pointer);
				break;
			}
		}

		return return_iterator;
	}




public:

	// Fill insert

	iterator insert(const const_iterator position, const size_type number_of_elements, const element_type &element)
	{
		if (number_of_elements == 0)
		{
			return end_iterator;
		}
		else if (number_of_elements == 1)
		{
			return insert(position, element);
		}

		reserve(total_size + number_of_elements);

		// Insert first element, then use up any erased nodes:
		size_type remainder = number_of_elements - 1;
		const iterator return_iterator = insert(position, element);

		while (node_allocator_pair.number_of_erased_nodes != 0)
		{
			insert(position, element);
			if (--remainder == 0) return return_iterator;
		}

		total_size += remainder;

		// then use up remainder of last_endpoint_group:
		const group_size_type remaining_nodes_in_group = static_cast<group_size_type>(groups.last_endpoint_group->beyond_end - last_endpoint);

		if (remaining_nodes_in_group != 0)
		{
			if (remaining_nodes_in_group < remainder)
			{
				fill(element, remaining_nodes_in_group, position.node_pointer);
				remainder -= remaining_nodes_in_group;
			}
			else
			{
				fill(element, static_cast<group_size_type>(remainder), position.node_pointer);
				return return_iterator;
			}
		}


		// Then start using trailing (reserved) groups:
		while (true)
		{
			last_endpoint = (++groups.last_endpoint_group)->nodes;
			const group_size_type group_size = static_cast<group_size_type>(groups.last_endpoint_group->beyond_end - groups.last_endpoint_group->nodes);

			if (group_size < remainder)
			{
				fill(element, group_size, position.node_pointer);
				remainder -= group_size;
			}
			else
			{
				fill(element, static_cast<group_size_type>(remainder), position.node_pointer);
				break;
			}
		}

		return return_iterator;
	}



	// Range insert

	template <class iterator_type>
	iterator insert(const const_iterator position, typename plf::enable_if<!std::numeric_limits<iterator_type>::is_integer, iterator_type>::type first, const iterator_type last)
	{
		return range_insert(position, static_cast<size_type>(std::distance(first, last)), first);
	}



	// Range insert, move_iterator overload

	#ifdef PLF_MOVE_SEMANTICS_SUPPORT
		template <class iterator_type>
		iterator insert (const const_iterator position, const std::move_iterator<iterator_type> first, const std::move_iterator<iterator_type> last)
		{
			return range_insert(position, static_cast<size_type>(std::distance(first.base(),last.base())), first);
		}
	#endif



	// Initializer-list insert

	#ifdef PLF_INITIALIZER_LIST_SUPPORT
		iterator insert(const const_iterator it, const std::initializer_list<element_type> &element_list)
		{
			return range_insert(it, static_cast<size_type>(element_list.size()), element_list.begin());
		}
	#endif



	#ifdef PLF_CPP20_SUPPORT
		template<compatible_range<element_type> range_type>
		iterator insert_range(const const_iterator it, range_type &&the_range)
		{
			return range_insert(it, static_cast<size_type>(std::ranges::distance(the_range)), std::ranges::begin(the_range));
		}
	#endif



private:

	void destroy_all_node_pointers(const group_pointer_type group_to_process, const node_pointer_type beyond_end_node) PLF_NOEXCEPT
	{
		for (node_pointer_type current_node = group_to_process->nodes; current_node != beyond_end_node; ++current_node)
		{
			PLF_DESTROY(node_pointer_allocator_type, groups.node_pointer_allocator_pair, &(current_node->next));
			PLF_DESTROY(node_pointer_allocator_type, groups.node_pointer_allocator_pair, &(current_node->previous));
		}
	}



public:


	// Single erase:

	iterator erase(const const_iterator it) // if uninitialized/invalid iterator supplied, function could generate an exception, hence no noexcept
	{
		assert(total_size != 0);
		assert(it.node_pointer != NULL); // uninitialized iterator
		assert(it.node_pointer != end_iterator.node_pointer); // iterator points to end()

		#ifdef PLF_TYPE_TRAITS_SUPPORT
			if PLF_CONSTEXPR (!(std::is_trivially_destructible<element_type>::value))
		#endif
		{
			PLF_DESTROY(allocator_type, *this, &(it.node_pointer->element)); // Destruct element
		}

		--total_size;
		++node_allocator_pair.number_of_erased_nodes;


		// find the group this element is in, starting from the last group an element to-be-erased was found in (as erasures are, for most programs, likely to be closer in proximity to previous erasures):
		group_pointer_type node_group = groups.last_searched_group;

		if (it.node_pointer < node_group->nodes || it.node_pointer >= node_group->beyond_end)
		{
			// Search groups to the left and right of the last searched group, in the group vector:
			const group_pointer_type beyond_end_group = groups.last_endpoint_group + 1;
			group_pointer_type left = node_group - 1;
			bool right_not_beyond_back = ++node_group < beyond_end_group;
			bool left_not_beyond_front = left >= groups.block_pointer;

			while (true)
			{
				if (right_not_beyond_back)
				{
					if (it.node_pointer < node_group->beyond_end && it.node_pointer >= node_group->nodes) break; // element location found
					right_not_beyond_back = ++node_group < beyond_end_group;
				}

				if (left_not_beyond_front)
				{
					if (it.node_pointer >= left->nodes && it.node_pointer < left->beyond_end) // element location found
					{
						node_group = left;
						break;
					}

					left_not_beyond_front = --left >= groups.block_pointer;
				}
			}

			groups.last_searched_group = node_group;
		}

		// To avoid pointer aliasing and increase performance:
		const node_pointer_type previous = it.node_pointer->previous;
		const node_pointer_type next = it.node_pointer->next;
		next->previous = previous;
		previous->next = next;

		if (it.node_pointer == begin_iterator.node_pointer) begin_iterator.node_pointer = next;


		const iterator return_iterator(next);

		if (--(node_group->number_of_elements) != 0) // ie. group is not empty yet, add node to free list
		{
			it.node_pointer->next = NULL; // next == NULL so that destructor and other functions which linearly iterate over node memory chunks can detect this as a free list node, ie an erased node
			it.node_pointer->previous = node_group->free_list_head;
			node_group->free_list_head = it.node_pointer;
			return return_iterator;
		}
		else if (node_group != groups.last_endpoint_group--) // remove group (and decrement active back group)
		{
			const group_size_type group_size = static_cast<group_size_type>(node_group->beyond_end - node_group->nodes);
			node_allocator_pair.number_of_erased_nodes -= group_size;

			#ifdef PLF_TYPE_TRAITS_SUPPORT
				if PLF_CONSTEXPR (!std::is_trivially_destructible<node_pointer_type>::value)
			#endif
			{
				destroy_all_node_pointers(node_group, node_group->beyond_end);
			}

			node_group->free_list_head = NULL;

			if ((group_size == list_max_block_capacity()) | (node_group >= groups.last_endpoint_group - 1)) // Preserve only groups which are at the maximum possible size, or first/second/third-to-last active groups - seems to be best for performance under high-modification benchmarks
			{
				groups.move_to_back(node_group);
			}
			else
			{
				groups.remove(node_group);
			}

			return return_iterator;
		}
		else // clear back group, leave trailing
		{
			#ifdef PLF_TYPE_TRAITS_SUPPORT
				if PLF_CONSTEXPR (!std::is_trivially_destructible<node_pointer_type>::value)
			#endif
			{
				destroy_all_node_pointers(node_group, last_endpoint);
			}

			node_group->free_list_head = NULL;

			if (total_size != 0)
			{
				node_allocator_pair.number_of_erased_nodes -= static_cast<group_size_type>(last_endpoint - node_group->nodes);
	  			last_endpoint = groups.last_endpoint_group->beyond_end;
			}
			else
			{
				groups.last_endpoint_group = groups.block_pointer; // If number of elements is zero, it indicates that this was the only group in the vector. In which case the last_endpoint_group would be invalid at this point due to the decrement in the above else-if statement. So it needs to be reset, as it will not be reset in the function call below.
				blank();
			}

			return return_iterator;
		}
	}



	// Range-erase:

	iterator erase(const_iterator iterator1, const const_iterator iterator2)
	{
		while (iterator1 != iterator2)
		{
			iterator1 = erase(iterator1);
		}

		return iterator(iterator2.node_pointer);
	}



	void pop_back() // Exception will occur on empty list
	{
		erase(iterator(end_node.previous));
	}



	void pop_front()
	{
		erase(begin_iterator);
	}



	list & operator = (const list &source)
	{
		assert (&source != this);

		#ifdef PLF_ALLOCATOR_TRAITS_SUPPORT
			if PLF_CONSTEXPR (std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value)
		#endif
		{
			#ifdef PLF_IS_ALWAYS_EQUAL_SUPPORT
				if PLF_CONSTEXPR (!std::allocator_traits<allocator_type>::is_always_equal::value)
			#endif
			{
				if(static_cast<allocator_type &>(*this) != static_cast<const allocator_type &>(source))
				{ // Deallocate existing blocks as source allocator is not necessarily able to do so
					reset();
				}
			}

			static_cast<allocator_type &>(*this) = static_cast<const allocator_type &>(source);

			// Reconstruct rebinds:
			static_cast<node_allocator_type &>(node_allocator_pair) = node_allocator_type(*this);
		}

		range_assign(source.begin_iterator, source.total_size);
		return *this;
	}



	#ifdef PLF_MOVE_SEMANTICS_SUPPORT
	private:

		void move_assign(list &&source) PLF_NOEXCEPT
		{
			groups.destroy_all_data(last_endpoint);

			groups = std::move(source.groups);
			end_node = std::move(source.end_node);
			last_endpoint = std::move(source.last_endpoint);
			begin_iterator.node_pointer = (source.begin_iterator.node_pointer == source.end_iterator.node_pointer) ? end_iterator.node_pointer : std::move(source.begin_iterator.node_pointer);
			total_size = source.total_size;
			node_allocator_pair.number_of_erased_nodes = source.node_allocator_pair.number_of_erased_nodes;

			end_node.previous->next = begin_iterator.node_pointer->previous = end_iterator.node_pointer;
			source.groups.blank();

			#ifdef PLF_ALLOCATOR_TRAITS_SUPPORT
				if PLF_CONSTEXPR(std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value)
			#endif
			{
				static_cast<allocator_type &>(*this) = source;
				static_cast<node_allocator_type &>(node_allocator_pair) = node_allocator_type(*this);
			}

			source.blank();
		}



	public:

		// Move assignment
		list & operator = (list &&source) PLF_NOEXCEPT_MOVE_ASSIGN(allocator_type)
		{
			assert (&source != this);

			// Move source values across:
			#ifdef PLF_IS_ALWAYS_EQUAL_SUPPORT
				if PLF_CONSTEXPR (std::allocator_traits<allocator_type>::propagate_on_container_move_assignment::value || std::allocator_traits<allocator_type>::is_always_equal::value)
				{
					move_assign(std::move(source));
				}
				else
			#endif
			if (static_cast<allocator_type &>(*this) == static_cast<allocator_type &>(source))
			{
				move_assign(std::move(source));
			}
			else // Allocator isn't movable so move/copy elements from source and deallocate the source's blocks:
			{
				reset();

				#ifdef PLF_TYPE_TRAITS_SUPPORT
					if PLF_CONSTEXPR (!std::is_move_constructible<element_type>::value)
					{
						#ifdef PLF_EXCEPTIONS_SUPPORT
							if PLF_CONSTEXPR (!std::is_copy_constructible<element_type>::value)
							{
								throw std::domain_error("Cannot perform move assignment, allocators are not equal and type is not copy/move-constructible.");
							}
						#endif

						range_insert(end_iterator, source.total_size, source.begin_iterator);
					}
					else
				#endif
				{
					range_insert(end_iterator, source.total_size, plf::make_move_iterator(source.begin_iterator));
				}

				source.reset();
			}

			return *this;
		}
	#endif



	#ifdef PLF_INITIALIZER_LIST_SUPPORT
		list & operator = (const std::initializer_list<element_type> &element_list)
		{
			range_assign(element_list.begin(), static_cast<size_type>(element_list.size()));
			return *this;
		}
	#endif



	friend bool operator == (const list &lh, const list &rh) PLF_NOEXCEPT
	{
		if (lh.total_size != rh.total_size) return false;

		for (const_iterator lh_iterator = lh.begin_iterator, rh_iterator = rh.begin_iterator; lh_iterator != lh.end_iterator; ++lh_iterator, ++rh_iterator)
		{
			if (*lh_iterator != *rh_iterator) return false;
		}

		return true;
	}



	friend bool operator != (const list &lh, const list &rh) PLF_NOEXCEPT
	{
		return !(lh == rh);
	}



	#ifdef PLF_CPP20_SUPPORT
		friend constexpr std::strong_ordering operator <=> (const list &lh, const list &rh)
		{
			return std::lexicographical_compare_three_way(lh.begin(), lh.end(), rh.begin(), rh.end());
		}



		[[nodiscard]]
	#endif
	bool empty() const PLF_NOEXCEPT
	{
		return total_size == 0;
	}



	size_type size() const PLF_NOEXCEPT
	{
		return total_size;
	}



	size_type max_size() const PLF_NOEXCEPT
	{
		#ifdef PLF_ALLOCATOR_TRAITS_SUPPORT
			return std::allocator_traits<allocator_type>::max_size(*this);
		#else
			return allocator_type::max_size();
		#endif
	}



	size_type capacity() const PLF_NOEXCEPT
	{
		return groups.node_pointer_allocator_pair.capacity;
	}



	size_type memory() const PLF_NOEXCEPT
	{
		return sizeof(*this) + (groups.node_pointer_allocator_pair.capacity * sizeof(node)) + (groups.group_allocator_pair.capacity * sizeof(group));
	}



private:


	// Function-object to redirect the sort function to sort pointers by the elements they point to, not the pointer value
	template <class comparison_function>
	struct sort_dereferencer
	{
		comparison_function stored_instance;

		explicit sort_dereferencer(const comparison_function &function_instance) PLF_NOEXCEPT:
			stored_instance(function_instance)
		{}

		bool operator() (const node_pointer_type first, const node_pointer_type second)
		{
			return stored_instance(first->element, second->element);
		}
	};



public:


	template <class comparison_function>
	void sort(comparison_function compare)
	{
		if (total_size < 2) return;

		node_pointer_type * const node_pointers = PLF_ALLOCATE(node_pointer_allocator_type, groups.node_pointer_allocator_pair, total_size, NULL);
		node_pointer_type *node_pointer = node_pointers;

		// According to the C++ standard, construction of a pointer (of any type) may not trigger an exception - hence, no try-catch blocks are necessary for constructing the pointers:
		for (group_pointer_type current_group = groups.block_pointer; current_group != groups.last_endpoint_group; ++current_group)
		{
			const node_pointer_type end = current_group->beyond_end;

			if (end - current_group->nodes != current_group->number_of_elements) // If there are erased nodes present in the group
			{
				for (node_pointer_type current_node = current_group->nodes; current_node != end; ++current_node)
				{
					if (current_node->next != NULL) // is not free list node
					{
						PLF_CONSTRUCT(node_pointer_allocator_type, groups.node_pointer_allocator_pair, node_pointer++, current_node);
					}
				}
			}
			else // If no erased nodes present we can avoid the per-node testing
			{
				for (node_pointer_type current_node = current_group->nodes; current_node != end; ++current_node)
				{
					PLF_CONSTRUCT(node_pointer_allocator_type, groups.node_pointer_allocator_pair, node_pointer++, current_node);
				}
			}
		}

		if (last_endpoint - groups.last_endpoint_group->nodes != groups.last_endpoint_group->number_of_elements) // If there are erased nodes present in the group
		{
			for (node_pointer_type current_node = groups.last_endpoint_group->nodes; current_node != last_endpoint; ++current_node)
			{
				if (current_node->next != NULL)
				{
					PLF_CONSTRUCT(node_pointer_allocator_type, groups.node_pointer_allocator_pair, node_pointer++, current_node);
				}
			}
		}
		else
		{
			for (node_pointer_type current_node = groups.last_endpoint_group->nodes; current_node != last_endpoint; ++current_node)
			{
				PLF_CONSTRUCT(node_pointer_allocator_type, groups.node_pointer_allocator_pair, node_pointer++, current_node);
			}
		}


		#ifndef PLF_SORT_FUNCTION
			std::sort(node_pointers, node_pointer, sort_dereferencer<comparison_function>(compare));
		#else
			PLF_SORT_FUNCTION(node_pointers, node_pointer, sort_dereferencer<comparison_function>(compare));
		#endif


		begin_iterator.node_pointer = node_pointers[0];
		begin_iterator.node_pointer->next = node_pointers[1];
		begin_iterator.node_pointer->previous = end_iterator.node_pointer;

		end_node.next = node_pointers[0];
		end_node.previous = node_pointers[total_size - 1];
		end_node.previous->next = end_iterator.node_pointer;
		end_node.previous->previous = node_pointers[total_size - 2];

		node_pointer_type * const back = node_pointers + total_size - 1;

		for(node_pointer = node_pointers + 1; node_pointer != back; ++node_pointer)
		{
			(*node_pointer)->next = *(node_pointer + 1);
			(*node_pointer)->previous = *(node_pointer - 1);

			#ifdef PLF_TYPE_TRAITS_SUPPORT
				if PLF_CONSTEXPR (!std::is_trivially_destructible<node_pointer_type>::value)
			#endif
			{
				PLF_DESTROY(node_pointer_allocator_type, groups.node_pointer_allocator_pair, node_pointer - 1);
			}
		}

		#ifdef PLF_TYPE_TRAITS_SUPPORT
			if PLF_CONSTEXPR (!std::is_trivially_destructible<node_pointer_type>::value)
		#endif
		{
			PLF_DESTROY(node_pointer_allocator_type, groups.node_pointer_allocator_pair, back);
		}

		PLF_DEALLOCATE(node_pointer_allocator_type, groups.node_pointer_allocator_pair, node_pointers, total_size);
	}



	void sort()
	{
		sort(plf::less<element_type>());
	}



	void splice(const const_iterator position, const const_iterator first, const const_iterator last) PLF_NOEXCEPT // intra-list only splice functions - will crash if first & list are not from *this
	{
		if (position == last) return;
		if (begin_iterator == first) begin_iterator.node_pointer = last.node_pointer;

		// To avoid pointer aliasing and subsequently increase performance via simultaneous assignments:
		const node_pointer_type first_previous = first.node_pointer->previous;
		const node_pointer_type last_previous = last.node_pointer->previous;
		const node_pointer_type position_previous = position.node_pointer->previous;

		last.node_pointer->previous = first_previous;
		first.node_pointer->previous->next = last.node_pointer;

 		last_previous->next = position.node_pointer;
		first.node_pointer->previous = position_previous;

		position_previous->next = first.node_pointer;
		position.node_pointer->previous = last_previous;

		if (begin_iterator == position) begin_iterator.node_pointer = first.node_pointer;
	}



	void splice(const const_iterator position, const const_iterator location) PLF_NOEXCEPT
	{
		splice(position, location, const_iterator(location.node_pointer->next));
	}



private:

	void append_process(list &source) // used by merge and splice
	{
		node_allocator_pair.number_of_erased_nodes += source.node_allocator_pair.number_of_erased_nodes;

		if (last_endpoint != groups.last_endpoint_group->beyond_end)
		{ // Add unused nodes to group's free list
			const node_pointer_type back_node = last_endpoint - 1;
			for (node_pointer_type current_node = groups.last_endpoint_group->beyond_end - 1; current_node != back_node; --current_node)
			{
				current_node->next = NULL;
				current_node->previous = groups.last_endpoint_group->free_list_head;
				groups.last_endpoint_group->free_list_head = current_node;
			}

			node_allocator_pair.number_of_erased_nodes += static_cast<size_type>(groups.last_endpoint_group->beyond_end - last_endpoint);
		}

		groups.append(source.groups);
		last_endpoint = source.last_endpoint;
		total_size += source.total_size;
		source.blank();
	}




public:

	void splice(iterator position, list &source)
	{
		assert(&source != this);

		if (source.total_size == 0)
		{
			return;
		}
		else if (total_size == 0)
		{
			#ifdef PLF_MOVE_SEMANTICS_SUPPORT
				*this = std::move(source);
			#else
				reset();
				swap(source);
			#endif

			return;
		}

		if (position.node_pointer == begin_iterator.node_pointer) // put source groups at front rather than back
		{
			swap(source);
			position.node_pointer = end_iterator.node_pointer;
		}

		position.node_pointer->previous->next = source.begin_iterator.node_pointer;
		source.begin_iterator.node_pointer->previous = position.node_pointer->previous;
		position.node_pointer->previous = source.end_node.previous;
		source.end_node.previous->next = position.node_pointer;

		append_process(source);
	}



	#ifdef PLF_MOVE_SEMANTICS_SUPPORT
		void splice(iterator position, list &&source)
		{
			splice(position, source);
		}
	#endif



	template <class comparison_function>
	void merge(list &source, comparison_function compare)
	{
		splice((source.total_size >= total_size) ? end_iterator : begin_iterator, source);
		sort(compare);
	}



	void merge(list &source)
	{
		assert(&source != this);

		if (source.total_size == 0)
		{
			return;
		}
		else if (total_size == 0)
		{
			#ifdef PLF_MOVE_SEMANTICS_SUPPORT
				*this = std::move(source);
			#else
				reset();
				swap(source);
			#endif

			return;
		}

		node_pointer_type current1 = begin_iterator.node_pointer->next, current2 = source.begin_iterator.node_pointer->next;
		node_pointer_type previous = source.begin_iterator.node_pointer;
		const node_pointer_type source_end = source.end_iterator.node_pointer, this_end = end_iterator.node_pointer;

		begin_iterator.node_pointer->next = source.begin_iterator.node_pointer;
		source.begin_iterator.node_pointer->previous = begin_iterator.node_pointer;


		while ((current1 != this_end) & (current2 != source_end))
		{
			previous->next = current1;
			current1->previous = previous;
			previous = current1;
			current1 = current1->next;

			previous->next = current2;
			current2->previous = previous;
			previous = current2;
			current2 = current2->next;
		}

		if (current1 != this_end)
		{
			previous->next = current1;
			current1->previous = previous;
		}
		else
		{
			end_node.previous = source.end_node.previous;
			source.end_node.previous->next = end_iterator.node_pointer;
		}

		append_process(source);
	}



	#ifdef PLF_MOVE_SEMANTICS_SUPPORT
		template <class comparison_function>
		void merge(list &&source, comparison_function compare)
		{
			merge(source, compare);
		}



		void merge(list &&source)
		{
			merge(source);
		}
	#endif



	void reverse() PLF_NOEXCEPT
	{
		// Note: current_node->next has to be read during swapping in this process anyway, so including a test to figure out whether or not a given group has erased elements within it, and thus avoid per-node tests, is actually detrimental to performance according to benchmarks. This is unlike sort() where current_node->next is not used in the rest of the process and avoiding per-node tests of it's value is therefore beneficial in benchmarks.
		if (total_size > 1)
		{
			for (group_pointer_type current_group = groups.block_pointer; current_group != groups.last_endpoint_group; ++current_group)
			{
				const node_pointer_type end = current_group->beyond_end;

				for (node_pointer_type current_node = current_group->nodes; current_node != end; ++current_node)
				{
					if (current_node->next != NULL) /* ie. is not free list node */ std::swap(current_node->next, current_node->previous);
				}
			}

			for (node_pointer_type current_node = groups.last_endpoint_group->nodes; current_node != last_endpoint; ++current_node)
			{
				if (current_node->next != NULL) std::swap(current_node->next, current_node->previous);
			}

			std::swap(end_node.previous, begin_iterator.node_pointer);
			end_node.previous->next = end_iterator.node_pointer;
			begin_iterator.node_pointer->previous = end_iterator.node_pointer;
		}
	}



private:

	// Used by unique()
	struct eq
	{
		bool operator() (const element_type &a, const element_type &b) const PLF_NOEXCEPT
		{
			return a == b;
		}
	};



public:

	template <class comparison_function>
	size_type unique(comparison_function compare)
	{
  		const size_type original_number_of_elements = total_size;

		if (original_number_of_elements > 1)
		{
			element_type *previous = &(begin_iterator.node_pointer->element);

			for (iterator current = ++iterator(begin_iterator); current != end_iterator;)
			{
				if (compare(*current, *previous))
				{
					current = erase(current);
				}
				else
				{
					previous = &(current++.node_pointer->element);
				}
			}
		}

		return original_number_of_elements - total_size;
	}



	size_type unique()
	{
		return unique(eq());
	}



	template <class predicate_function>
	size_type remove_if(predicate_function predicate)
	{
  		const size_type original_number_of_elements = total_size;

		if (original_number_of_elements != 0)
		{
			for (group_pointer_type current_group = groups.block_pointer; current_group != groups.last_endpoint_group; ++current_group)
			{
				group_size_type num_elements = current_group->number_of_elements;
				const node_pointer_type end = current_group->beyond_end;

				if (end - current_group->nodes != num_elements) // If there are erased nodes present in the group
				{
					for (node_pointer_type current_node = current_group->nodes; current_node != end; ++current_node)
					{
						if (current_node->next != NULL && predicate(current_node->element)) // is not free list node and validates predicate
						{
							erase(current_node);

							if (--num_elements == 0) // ie. group will be empty (and removed) now - nothing left to iterate over
							{
								--current_group; // As current group has been removed, subsequent groups have already shifted back by one, hence, the ++ to the current group in the for loop is unnecessary, and negated here
								break;
							}
						}
					}
				}
				else // No erased nodes in group
				{
					for (node_pointer_type current_node = current_group->nodes; current_node != end; ++current_node)
					{
						if (predicate(current_node->element))
						{
							erase(current_node);

							if (--num_elements == 0)
							{
								--current_group;
								break;
							}
						}
					}
				}
			}

			group_size_type num_elements = groups.last_endpoint_group->number_of_elements;

			if (last_endpoint - groups.last_endpoint_group->nodes != num_elements) // If there are erased nodes present in the group
			{
				for (node_pointer_type current_node = groups.last_endpoint_group->nodes; current_node != last_endpoint; ++current_node)
				{
					if (current_node->next != NULL && predicate(current_node->element))
					{
						erase(current_node);
						if (--num_elements == 0) break;
					}
				}
			}
			else
			{
				for (node_pointer_type current_node = groups.last_endpoint_group->nodes; current_node != last_endpoint; ++current_node)
				{
					if (predicate(current_node->element))
					{
						erase(current_node);
						if (--num_elements == 0) break;
					}
				}
			}
		}

		return original_number_of_elements - total_size;
	}



	size_type remove(const element_type &value)
	{
		return remove_if(plf::equal_to<element_type>(value));
	}



	void resize(const size_type number_of_elements, const element_type &value = element_type())
	{
		if (total_size == number_of_elements)
		{
			return;
		}
		else if (number_of_elements == 0)
		{
			clear();
			return;
		}
		else if (total_size < number_of_elements)
		{
			insert(end_iterator, number_of_elements - total_size, value);
		}
		else // ie. total_size > number_of_elements
		{
			const_iterator current(end_node.previous);

			for (size_type number_to_remove = total_size - number_of_elements; number_to_remove != 0; --number_to_remove)
			{
				const node_pointer_type temp = current.node_pointer->previous;
				erase(current);
				current.node_pointer = temp;
			}
		}
	}



	void reserve(size_type reserve_amount)
	{
		if (reserve_amount == 0 || reserve_amount <= groups.node_pointer_allocator_pair.capacity)
		{
			return;
		}
		else if (reserve_amount < list_min_block_capacity())
		{
			reserve_amount = list_min_block_capacity();
		}
		else if (reserve_amount > max_size())
		{
			#ifdef PLF_EXCEPTIONS_SUPPORT
				throw std::length_error("Capacity requested via reserve() greater than max_size()");
			#else
				std::terminate();
			#endif
		}


		if (groups.block_pointer != NULL && total_size == 0)
		{ // edge case: has been filled with elements then clear()'d - some groups may be smaller than would be desired, should be replaced
			group_size_type end_group_size = static_cast<group_size_type>((groups.block_pointer + groups.size - 1)->beyond_end - (groups.block_pointer + groups.size - 1)->nodes);

			if (reserve_amount > end_group_size && end_group_size != list_max_block_capacity()) // if last group isn't large enough, remove all groups
			{
				reset();
			}
			else
			{
				size_type number_of_full_groups_needed = reserve_amount / list_max_block_capacity();
				group_size_type remainder = static_cast<group_size_type>(reserve_amount - (number_of_full_groups_needed * list_max_block_capacity()));

				// Remove any max_size groups which're not needed and any groups that're smaller than remainder:
				for (group_pointer_type current_group = groups.block_pointer; current_group < groups.block_pointer + groups.size;) // note: groups.size may change during procedure so we calculate this every loop
				{
					const group_size_type current_group_size = static_cast<group_size_type>(groups.block_pointer->beyond_end - groups.block_pointer->nodes);

					if (number_of_full_groups_needed != 0 && current_group_size == list_max_block_capacity())
					{
						--number_of_full_groups_needed;
						++current_group;
					}
					else if (remainder != 0 && current_group_size >= remainder)
					{
						remainder = 0;
						++current_group;
					}
					else
					{
						groups.remove(current_group);
					}
				}

				last_endpoint = groups.block_pointer->nodes;
			}
		}

		reserve_amount -= groups.node_pointer_allocator_pair.capacity;

		// To correct from possible reallocation caused by add_new:
		const difference_type last_endpoint_group_number = groups.last_endpoint_group - groups.block_pointer;

		size_type number_of_full_groups = reserve_amount / list_max_block_capacity();
		reserve_amount -= (number_of_full_groups++ * list_max_block_capacity()); // ++ to aid while loop below

		if (groups.block_pointer == NULL) // Previously uninitialized list or reset in above if statement; most common scenario
		{
			if (reserve_amount != 0)
			{
				groups.initialize(static_cast<group_size_type>(((reserve_amount < list_min_block_capacity()) ? list_min_block_capacity() : reserve_amount)));
			}
			else
			{
				groups.initialize(list_max_block_capacity());
				--number_of_full_groups;
			}
		}
		else if (reserve_amount != 0)
		{ // Create a group at least as large as the last group - may allocate more than necessary, but better solution than creating a very small group in the middle of the group vector, I think:
			const group_size_type last_endpoint_group_capacity = static_cast<group_size_type>(groups.last_endpoint_group->beyond_end - groups.last_endpoint_group->nodes);
			groups.add_new(static_cast<group_size_type>((reserve_amount < last_endpoint_group_capacity) ? last_endpoint_group_capacity : reserve_amount));
		}

		while (--number_of_full_groups != 0)
		{
			groups.add_new(list_max_block_capacity());
		}

		groups.last_endpoint_group = groups.block_pointer + last_endpoint_group_number;
	}



	void trim_capacity() PLF_NOEXCEPT
	{
		groups.trim_unused_groups();
	}



	void shrink_to_fit()
	{
		if ((groups.block_pointer == NULL) | (total_size == groups.node_pointer_allocator_pair.capacity)) // if list is uninitialized or full
		{
			return;
		}
		else if (total_size == 0) // Edge case
		{
			reset();
			return;
		}
		else if (node_allocator_pair.number_of_erased_nodes == 0 && last_endpoint == groups.last_endpoint_group->beyond_end) //edge case - currently no wasted space except for possible trailing groups
		{
			groups.trim_unused_groups();
			return;
		}

		#ifdef PLF_MOVE_SEMANTICS_SUPPORT
			list temp;

			#ifdef PLF_TYPE_TRAITS_SUPPORT
				if PLF_CONSTEXPR (std::is_move_assignable<element_type>::value && std::is_move_constructible<element_type>::value) // move elements if possible, otherwise copy them
				{
					temp.range_insert(temp.end_iterator, total_size, plf::make_move_iterator(begin_iterator));
				}
				else
			#endif
			{
				temp.range_insert(temp.end_iterator, total_size, begin_iterator);
			}

			*this = std::move(temp);
		#else
			list temp(*this);
			reset();
			swap(temp);
		#endif
	}



	// Range assign:

private:


 	template <class iterator_type>
 	void range_assign(iterator_type it, size_type range_size)
	{
		if (range_size == 0)
		{
			clear();
			return;
		}

		#ifdef PLF_TYPE_TRAITS_SUPPORT
			if PLF_CONSTEXPR ((std::is_trivially_destructible<element_type>::value && std::is_trivially_constructible<element_type>::value && std::is_trivially_copy_assignable<element_type>::value) || !std::is_copy_assignable<element_type>::value) // ie. If there is no benefit nor difference to assigning vs constructing, or if we can't assign
			{
				clear();
				range_insert(end_iterator, range_size, it);
			}
			else
		#endif
		{
			if (total_size == 0)
			{
				range_insert(end_iterator, range_size, it);
			}
			else if (range_size < total_size)
			{
				iterator current = begin_iterator;

				do
				{
					*current++ = *it++;
				} while (--range_size != 0);

				erase(current, end_iterator);
			}
			else
			{
				iterator current = begin_iterator;

				do
				{
					*current = *it++;
				} while (++current != end_iterator);

				range_insert(end_iterator, range_size - total_size, it);
			}
		}

		groups.trim_unused_groups();
	}




public:


 	template <class iterator_type>
 	void assign(typename plf::enable_if<!std::numeric_limits<iterator_type>::is_integer, iterator_type>::type first, const iterator_type last)
	{
		range_assign(first, std::distance(first, last));
	}



	#ifdef PLF_MOVE_SEMANTICS_SUPPORT
		template <class iterator_type>
		void insert (const std::move_iterator<iterator_type> first, const std::move_iterator<iterator_type> last)
		{
			range_assign(first, static_cast<size_type>(std::distance(first.base(),last.base())));
		}
	#endif



	// Fill assign:

	void assign(size_type number_of_elements, const element_type &value)
	{
		if (number_of_elements == 0)
		{
			clear();
			return;
		}

		#ifdef PLF_TYPE_TRAITS_SUPPORT
			if PLF_CONSTEXPR ((std::is_trivially_destructible<element_type>::value && std::is_trivially_constructible<element_type>::value && std::is_trivially_copy_assignable<element_type>::value) || !std::is_copy_assignable<element_type>::value)
			{
				clear();
				insert(end_iterator, number_of_elements, value);
			}
		#endif
		{
			if (total_size == 0)
			{
				insert(end_iterator, number_of_elements, value);
			}
			else if (number_of_elements < total_size)
			{
				iterator current = begin_iterator;

				do
				{
					*current++ = value;
				} while (--number_of_elements != 0);

				erase(current, end_iterator);
			}
			else
			{
				iterator current = begin_iterator;

				do
				{
					*current = value;
				} while (++current != end_iterator);

				insert(end_iterator, number_of_elements - total_size, value);
			}
		}

		groups.trim_unused_groups();
	}



	#ifdef PLF_INITIALIZER_LIST_SUPPORT
		// Initializer-list assign:

		void assign(const std::initializer_list<element_type> &element_list)
		{
			range_assign(element_list.begin(), static_cast<size_type>(element_list.size()));
		}
	#endif



	#ifdef PLF_CPP20_SUPPORT
		template<compatible_range<element_type> range_type>
		void assign_range(range_type &&the_range)
		{
			range_assign(std::ranges::begin(the_range), static_cast<size_type>(std::ranges::distance(the_range)));
		}
	#endif



	allocator_type get_allocator() const PLF_NOEXCEPT
	{
		return allocator_type();
	}



	template <class predicate_function>
	iterator unordered_find_single(predicate_function predicate) const PLF_NOEXCEPT
	{
		if (total_size != 0)
		{
			for (group_pointer_type current_group = groups.block_pointer; current_group != groups.last_endpoint_group; ++current_group)
			{
				const node_pointer_type end = current_group->beyond_end;

				if (end - current_group->nodes != current_group->number_of_elements) // If there are erased nodes present in the group
				{
					for (node_pointer_type current_node = current_group->nodes; current_node != end; ++current_node)
					{
						if (current_node->next != NULL && predicate(current_node->element)) return iterator(current_node); // is not free list node and matches element
					}
				}
				else // No erased nodes in group
				{
					for (node_pointer_type current_node = current_group->nodes; current_node != end; ++current_node)
					{
						if (predicate(current_node->element)) return iterator(current_node);
					}
				}
			}

			if (last_endpoint - groups.last_endpoint_group->nodes != groups.last_endpoint_group->number_of_elements)
			{
				for (node_pointer_type current_node = groups.last_endpoint_group->nodes; current_node != last_endpoint; ++current_node)
				{
					if (current_node->next != NULL && predicate(current_node->element)) return iterator(current_node);
				}
			}
			else
			{
				for (node_pointer_type current_node = groups.last_endpoint_group->nodes; current_node != last_endpoint; ++current_node)
				{
					if (predicate(current_node->element)) return iterator(current_node);
				}
			}
		}

		return end_iterator;
	}



	iterator unordered_find_single(const element_type &element_to_match) const PLF_NOEXCEPT
	{
		return unordered_find_single(plf::equal_to<element_type>(element_to_match));
	}



	template <class predicate_function>
	list<iterator> unordered_find_multiple(predicate_function predicate, size_type number_to_find) const
	{
		list<iterator> return_list;

		if (total_size != 0)
		{
			for (group_pointer_type current_group = groups.block_pointer; current_group != groups.last_endpoint_group; ++current_group)
			{
				const node_pointer_type end = current_group->beyond_end;

				if (end - current_group->nodes != current_group->number_of_elements)
				{
					for (node_pointer_type current_node = current_group->nodes; current_node != end; ++current_node)
					{
						if (current_node->next != NULL && predicate(current_node->element))
						{
							return_list.push_back(iterator(current_node));
							if (--number_to_find == 0) return return_list;
						}
					}
				}
				else // No erased nodes in group
				{
					for (node_pointer_type current_node = current_group->nodes; current_node != end; ++current_node)
					{
						if (predicate(current_node->element))
						{
							return_list.push_back(iterator(current_node));
							if (--number_to_find == 0) return return_list;
						}
					}
				}
			}

			if (last_endpoint - groups.last_endpoint_group->nodes != groups.last_endpoint_group->number_of_elements)
			{
				for (node_pointer_type current_node = groups.last_endpoint_group->nodes; current_node != last_endpoint; ++current_node)
				{
					if (current_node->next != NULL && predicate(current_node->element))
					{
						return_list.push_back(iterator(current_node));
						if (--number_to_find == 0) return return_list;
					}
				}
			}
			else
			{
				for (node_pointer_type current_node = groups.last_endpoint_group->nodes; current_node != last_endpoint; ++current_node)
				{
					if (predicate(current_node->element))
					{
						return_list.push_back(iterator(current_node));
						if (--number_to_find == 0) return return_list;
					}
				}
			}
		}

		return return_list;
	}



	list<iterator> unordered_find_multiple(const element_type &element_to_match, size_type number_to_find) const
	{
		return unordered_find_multiple(plf::equal_to<element_type>(element_to_match), number_to_find);
	}



	template <class predicate_function>
	list<iterator> unordered_find_all(predicate_function predicate) const
	{
		list<iterator> return_list;

		if (total_size != 0)
		{
			for (group_pointer_type current_group = groups.block_pointer; current_group != groups.last_endpoint_group; ++current_group)
			{
				const node_pointer_type end = current_group->beyond_end;

				if (end - current_group->nodes != current_group->number_of_elements)
				{
					for (node_pointer_type current_node = current_group->nodes; current_node != end; ++current_node)
					{
						if (current_node->next != NULL && predicate(current_node->element)) return_list.push_back(iterator(current_node));
					}
				}
				else // No erased nodes in group
				{
					for (node_pointer_type current_node = current_group->nodes; current_node != end; ++current_node)
					{
						if (predicate(current_node->element)) return_list.push_back(iterator(current_node));
					}
				}
			}

			if (last_endpoint - groups.last_endpoint_group->nodes != groups.last_endpoint_group->number_of_elements)
			{
				for (node_pointer_type current_node = groups.last_endpoint_group->nodes; current_node != last_endpoint; ++current_node)
				{
					if (current_node->next != NULL && predicate(current_node->element)) return_list.push_back(iterator(current_node));
				}
			}
			else
			{
				for (node_pointer_type current_node = groups.last_endpoint_group->nodes; current_node != last_endpoint; ++current_node)
				{
					if (predicate(current_node->element)) return_list.push_back(iterator(current_node));
				}
			}
		}

		return return_list;
	}



	list<iterator> unordered_find_all(const element_type &element_to_match) const
	{
		return unordered_find_all(plf::equal_to<element_type>(element_to_match));
	}



	void swap(list &source) PLF_NOEXCEPT_SWAP(allocator_type)
	{
		#if defined(PLF_TYPE_TRAITS_SUPPORT) && defined(PLF_MOVE_SEMANTICS_SUPPORT)
			if PLF_CONSTEXPR (std::is_move_assignable<group_pointer_type>::value && std::is_move_assignable<node_pointer_type>::value && std::is_move_constructible<group_pointer_type>::value && std::is_move_constructible<node_pointer_type>::value)
			{
				list temp(std::move(source));
				source = std::move(*this);
				*this = std::move(temp);
			}
			else
		#endif
		{
			groups.swap(source.groups);

			const node_pointer_type swap_end_node_previous = end_node.previous, swap_last_endpoint = last_endpoint;
			const iterator swap_begin_iterator = begin_iterator;
			const size_type swap_total_size = total_size, swap_number_of_erased_nodes = node_allocator_pair.number_of_erased_nodes;

			last_endpoint = source.last_endpoint;
			end_node.next = begin_iterator.node_pointer = (source.begin_iterator.node_pointer != source.end_iterator.node_pointer) ? source.begin_iterator.node_pointer : end_iterator.node_pointer;
			end_node.previous = (source.begin_iterator.node_pointer != source.end_iterator.node_pointer) ? source.end_node.previous : end_iterator.node_pointer;
			end_node.previous->next = begin_iterator.node_pointer->previous = end_iterator.node_pointer;
			total_size = source.total_size;
			node_allocator_pair.number_of_erased_nodes = source.node_allocator_pair.number_of_erased_nodes;

			source.last_endpoint = swap_last_endpoint;
			source.end_node.next = source.begin_iterator.node_pointer = (swap_begin_iterator.node_pointer != end_iterator.node_pointer) ? swap_begin_iterator.node_pointer : source.end_iterator.node_pointer;
			source.end_node.previous = (swap_begin_iterator.node_pointer != end_iterator.node_pointer) ? swap_end_node_previous : source.end_iterator.node_pointer;
			source.end_node.previous->next = source.begin_iterator.node_pointer->previous = source.end_iterator.node_pointer;
			source.total_size = swap_total_size;
			source.node_allocator_pair.number_of_erased_nodes = swap_number_of_erased_nodes;

			#ifdef PLF_IS_ALWAYS_EQUAL_SUPPORT
				if PLF_CONSTEXPR (std::allocator_traits<allocator_type>::propagate_on_container_swap::value && !std::allocator_traits<allocator_type>::is_always_equal::value)
			#endif
			{
				std::swap(static_cast<allocator_type &>(source), static_cast<allocator_type &>(*this));

				// Reconstruct rebinds for swapped allocators:
				static_cast<node_allocator_type &>(node_allocator_pair) = node_allocator_type(*this);
				static_cast<node_allocator_type &>(source.node_allocator_pair) = node_allocator_type(source);
			} // else: undefined behaviour, as per standard
		}
	}



	// Iterators:

	template <bool is_const> class list_iterator
	{
	private:
		typedef typename list::node_pointer_type node_pointer_type;

		#ifdef PLF_DEFAULT_SUPPORT
			node_pointer_type node_pointer {NULL};
		#else
			node_pointer_type node_pointer;
		#endif

	public:
		struct list_iterator_tag {};
		typedef std::bidirectional_iterator_tag 	iterator_category;
		typedef std::bidirectional_iterator_tag	iterator_concept;
		typedef typename list::value_type 			value_type;
		typedef typename list::difference_type 	difference_type;
		typedef list_reverse_iterator<is_const> 	reverse_type;
		typedef typename plf::conditional<is_const, typename list::const_pointer, typename list::pointer>::type		pointer;
		typedef typename plf::conditional<is_const, typename list::const_reference, typename list::reference>::type	reference;

		friend class list;



		bool operator == (const list_iterator rh) const PLF_NOEXCEPT
		{
			return (node_pointer == rh.node_pointer);
		}



		bool operator == (const list_iterator<!is_const> rh) const PLF_NOEXCEPT
		{
			return (node_pointer == rh.node_pointer);
		}



		bool operator != (const list_iterator rh) const PLF_NOEXCEPT
		{
			return (node_pointer != rh.node_pointer);
		}



		bool operator != (const list_iterator<!is_const> rh) const PLF_NOEXCEPT
		{
			return (node_pointer != rh.node_pointer);
		}



		reference operator * () const
		{
			return node_pointer->element;
		}



		pointer operator -> () const
		{
			return &(node_pointer->element);
		}



		list_iterator & operator ++ () PLF_NOEXCEPT
		{
			assert(node_pointer != NULL); // covers uninitialised list_iterator
			node_pointer = node_pointer->next;
			return *this;
		}



		list_iterator operator ++(int) PLF_NOEXCEPT
		{
			const list_iterator copy(*this);
			++*this;
			return copy;
		}



		list_iterator & operator -- () PLF_NOEXCEPT
		{
			assert(node_pointer != NULL);
			node_pointer = node_pointer->previous;
			return *this;
		}



		list_iterator operator -- (int) PLF_NOEXCEPT
		{
			const list_iterator copy(*this);
			--*this;
			return copy;
		}



		#ifdef PLF_DEFAULT_SUPPORT // Note: defaulting allows for treating iterators as trivially-copyable types, which enables optimizations in some functions and containers using list as back-end storage
			list_iterator() PLF_NOEXCEPT = default;
		#else
			list_iterator() PLF_NOEXCEPT: node_pointer(NULL) {}
		#endif



		list_iterator(const list_iterator &source) PLF_NOEXCEPT
		#ifdef PLF_DEFAULT_SUPPORT
			= default;
		#else
			: node_pointer(source.node_pointer) {}
		#endif



		#ifdef PLF_DEFAULT_SUPPORT
			template <bool is_const_it = is_const, class = typename plf::enable_if<is_const_it>::type >
			list_iterator(const list_iterator<false> &source) PLF_NOEXCEPT: node_pointer(source.node_pointer) {}
		#else
			list_iterator(const list_iterator<!is_const> &source) PLF_NOEXCEPT: node_pointer(source.node_pointer) {}
		#endif



		list_iterator & operator = (const list_iterator &rh) PLF_NOEXCEPT
		#ifdef PLF_DEFAULT_SUPPORT
			= default;
		#else
			{
				node_pointer = rh.node_pointer;
				return *this;
			}
		#endif



		#ifdef PLF_DEFAULT_SUPPORT
			template <bool is_const_it = is_const, class = typename plf::enable_if<is_const_it>::type >
			list_iterator & operator = (const list_iterator<false> &rh) PLF_NOEXCEPT
		#else
			list_iterator & operator = (const list_iterator<!is_const> &rh) PLF_NOEXCEPT
		#endif
		{
			node_pointer = rh.node_pointer;
			return *this;
		}



		#ifdef PLF_MOVE_SEMANTICS_SUPPORT
			list_iterator (list_iterator &&source) PLF_NOEXCEPT
			#ifdef PLF_DEFAULT_SUPPORT
				= default;
			#else
				: node_pointer(std::move(source.node_pointer)) {}
			#endif



			#ifdef PLF_DEFAULT_SUPPORT
				template <bool is_const_it = is_const, class = typename plf::enable_if<is_const_it>::type >
				list_iterator(list_iterator<false> &&source) PLF_NOEXCEPT: node_pointer(std::move(source.node_pointer)) {}
			#else
				list_iterator(list_iterator<!is_const> &&source) PLF_NOEXCEPT: node_pointer(std::move(source.node_pointer)) {}
			#endif



			list_iterator & operator = (list_iterator &&rh) PLF_NOEXCEPT
			#ifdef PLF_DEFAULT_SUPPORT
				= default;
			#else
				: node_pointer(std::move(rh.node_pointer)) {}
			#endif



			#ifdef PLF_DEFAULT_SUPPORT
				template <bool is_const_it = is_const, class = typename plf::enable_if<is_const_it>::type >
				list_iterator & operator = (list_iterator<false> &&rh) PLF_NOEXCEPT
			#else
				list_iterator & operator = (list_iterator<!is_const> &&rh) PLF_NOEXCEPT
			#endif
			{
				node_pointer = std::move(rh.node_pointer);
				return *this;
			}
		#endif



	private:

		list_iterator (const node_pointer_type node_p) PLF_NOEXCEPT: node_pointer(node_p) {}
	};




	template <bool is_const_r> class list_reverse_iterator
	{
	private:
		typedef typename list::node_pointer_type node_pointer_type;

		#ifdef PLF_DEFAULT_SUPPORT
			node_pointer_type node_pointer {NULL};
		#else
			node_pointer_type node_pointer;
		#endif

	public:
		typedef std::bidirectional_iterator_tag 	iterator_concept;
		typedef std::bidirectional_iterator_tag 	iterator_category;
		typedef typename list::value_type 			value_type;
		typedef typename list::difference_type 	difference_type;
		typedef typename plf::conditional<is_const_r, typename list::const_pointer, typename list::pointer>::type		pointer;
		typedef typename plf::conditional<is_const_r, typename list::const_reference, typename list::reference>::type	reference;

		friend class list;


		bool operator == (const list_reverse_iterator rh) const PLF_NOEXCEPT
		{
			return (node_pointer == rh.node_pointer);
		}



		bool operator == (const list_reverse_iterator<!is_const_r> rh) const PLF_NOEXCEPT
		{
			return (node_pointer == rh.node_pointer);
		}



		bool operator != (const list_reverse_iterator rh) const PLF_NOEXCEPT
		{
			return (node_pointer != rh.node_pointer);
		}



		bool operator != (const list_reverse_iterator<!is_const_r> rh) const PLF_NOEXCEPT
		{
			return (node_pointer != rh.node_pointer);
		}



		reference operator * () const
		{
			return node_pointer->element;
		}



		pointer operator -> () const
		{
			return &(node_pointer->element);
		}



		list_reverse_iterator & operator ++ () PLF_NOEXCEPT
		{
			assert(node_pointer != NULL);
			node_pointer = node_pointer->previous;
			return *this;
		}



		list_reverse_iterator operator ++(int) PLF_NOEXCEPT
		{
			const list_reverse_iterator copy(*this);
			++*this;
			return copy;
		}



		list_reverse_iterator & operator -- () PLF_NOEXCEPT
		{
			assert(node_pointer != NULL);
			node_pointer = node_pointer->next;
			return *this;
		}



		list_reverse_iterator operator -- (int) PLF_NOEXCEPT
		{
			const list_reverse_iterator copy(*this);
			--*this;
			return copy;
		}



		typename list::iterator base() const PLF_NOEXCEPT
		{
			return typename list::iterator(node_pointer->next);
		}



		list_reverse_iterator() PLF_NOEXCEPT
		#ifdef PLF_DEFAULT_SUPPORT
			= default;
		#else
			: node_pointer(NULL) {}
		#endif



		list_reverse_iterator(const list_reverse_iterator &source) PLF_NOEXCEPT
		#ifdef PLF_DEFAULT_SUPPORT
			= default;
		#else
			: node_pointer(source.node_pointer) {}
		#endif



		#ifdef PLF_DEFAULT_SUPPORT
			template <bool is_const_rit = is_const_r, class = typename plf::enable_if<is_const_rit>::type >
			list_reverse_iterator (const list_reverse_iterator<false> &source) PLF_NOEXCEPT:
		#else
			list_reverse_iterator (const list_reverse_iterator<!is_const_r> &source) PLF_NOEXCEPT:
		#endif
			node_pointer(source.node_pointer)
		{}



		list_reverse_iterator& operator = (const list_reverse_iterator &source) PLF_NOEXCEPT
		#ifdef PLF_DEFAULT_SUPPORT
			= default;
		#else
			{
				node_pointer = source.node_pointer;
				return *this;
			}
		#endif



		#ifdef PLF_DEFAULT_SUPPORT
			template <bool is_const_rit = is_const_r, class = typename plf::enable_if<is_const_rit>::type >
			list_reverse_iterator& operator = (const list_reverse_iterator<false> &source) PLF_NOEXCEPT
		#else
			list_reverse_iterator& operator = (const list_reverse_iterator<!is_const_r> &source) PLF_NOEXCEPT
		#endif
		{
			node_pointer = source.node_pointer;
			return *this;
		}



		list_reverse_iterator (const list_iterator<is_const_r> &source) PLF_NOEXCEPT:
			node_pointer(source.node_pointer->previous)
		{}



		list_reverse_iterator& operator = (const list_iterator<is_const_r> &source) PLF_NOEXCEPT
		{
			node_pointer = source.node_pointer->previous;
			return *this;
		}



		#ifdef PLF_DEFAULT_SUPPORT
			template <bool is_const_rit = is_const_r, class = typename plf::enable_if<is_const_rit>::type >
			list_reverse_iterator& operator = (const list_iterator<false> &source) PLF_NOEXCEPT
		#else
			list_reverse_iterator& operator = (const list_iterator<!is_const_r> &source) PLF_NOEXCEPT
		#endif
		{
			node_pointer = source.node_pointer->previous;
			return *this;
		}



		#ifdef PLF_MOVE_SEMANTICS_SUPPORT
			list_reverse_iterator (list_reverse_iterator &&source) PLF_NOEXCEPT
			#ifdef PLF_DEFAULT_SUPPORT
				= default;
			#else
				: node_pointer(std::move(source.node_pointer)) {}
			#endif




			#ifdef PLF_DEFAULT_SUPPORT
				template <bool is_const_rit = is_const_r, class = typename plf::enable_if<is_const_rit>::type >
				list_reverse_iterator (list_reverse_iterator<false> &&source) PLF_NOEXCEPT:
			#else
				list_reverse_iterator (list_reverse_iterator<!is_const_r> &&source) PLF_NOEXCEPT:
			#endif
				node_pointer(std::move(source.node_pointer))
			{}



			list_reverse_iterator& operator = (list_reverse_iterator &&source) PLF_NOEXCEPT
			#ifdef PLF_DEFAULT_SUPPORT
				= default;
			#else
				{
					assert (&source != this);
					node_pointer = std::move(source.node_pointer);
					return *this;
				}
			#endif



			#ifdef PLF_DEFAULT_SUPPORT
				template <bool is_const_rit = is_const_r, class = typename plf::enable_if<is_const_rit>::type >
				list_reverse_iterator& operator = (list_reverse_iterator<false> &&source) PLF_NOEXCEPT
			#else
				list_reverse_iterator& operator = (list_reverse_iterator<!is_const_r> &&source) PLF_NOEXCEPT
			#endif
			{
				assert (&source != this);
				node_pointer = std::move(source.node_pointer);
				return *this;
			}
		#endif



	private:

		list_reverse_iterator (const node_pointer_type node_p) PLF_NOEXCEPT: node_pointer(node_p) {}
	};



}; // end of plf::list


} // plf namespace


namespace std
{
	template <class element_type, class allocator_type>
	void swap(plf::list<element_type, allocator_type> &a, plf::list<element_type, allocator_type> &b) PLF_NOEXCEPT_SWAP(allocator_type)
	{
		a.swap(b);
	}



	template <class element_type, class allocator_type, class predicate_function>
	typename plf::list<element_type, allocator_type>::size_type erase_if(plf::list<element_type, allocator_type> &container, predicate_function predicate)
	{
		return container.remove_if(predicate);
	}



	template <class element_type, class allocator_type>
	typename plf::list<element_type, allocator_type>::size_type erase(plf::list<element_type, allocator_type> &container, const element_type &value)
	{
		return container.remove(value);
	}



	#ifdef PLF_CPP20_SUPPORT
		// std::reverse_iterator overload, to allow use of plf::list with ranges and make_reverse_iterator primarily:
		template <plf::list_iterator_concept it_type>
		class reverse_iterator<it_type> : public it_type::reverse_type
		{
		public:
			typedef typename it_type::reverse_type rit;
			using rit::rit;
		};
	#endif
}


#undef PLF_CONSTRUCT_NODE
#undef PLF_EXCEPTIONS_SUPPORT
#undef PLF_TO_ADDRESS
#undef PLF_DEFAULT_SUPPORT
#undef PLF_ALIGNMENT_SUPPORT
#undef PLF_INITIALIZER_LIST_SUPPORT
#undef PLF_TYPE_TRAITS_SUPPORT
#undef PLF_IS_ALWAYS_EQUAL_SUPPORT
#undef PLF_ALLOCATOR_TRAITS_SUPPORT
#undef PLF_VARIADICS_SUPPORT
#undef PLF_MOVE_SEMANTICS_SUPPORT
#undef PLF_NOEXCEPT
#undef PLF_NOEXCEPT_ALLOCATOR
#undef PLF_NOEXCEPT_SWAP
#undef PLF_NOEXCEPT_MOVE_ASSIGN
#undef PLF_CONSTEXPR
#undef PLF_CONSTFUNC
#undef PLF_CPP20_SUPPORT

#undef PLF_CONSTRUCT
#undef PLF_DESTROY
#undef PLF_ALLOCATE
#undef PLF_DEALLOCATE

#if defined(_MSC_VER) && !defined(__clang__) && !defined(__GNUC__)
	#pragma warning ( pop )
#endif

#endif // PLF_LIST_H
