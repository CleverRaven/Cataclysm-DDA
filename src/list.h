// Copyright (c) 2019, Matthew Bentley (mattreecebentley@gmail.com) www.plflib.org

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
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgement in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.


#ifndef PLF_LIST_H
#define PLF_LIST_H


#define PLF_LIST_BLOCK_MIN static_cast<group_size_type>((sizeof(node) * 8 > (sizeof(*this) + sizeof(group)) * 2) ? 8 : (((sizeof(*this) + sizeof(group)) * 2) / sizeof(node)) + 1)
#define PLF_LIST_BLOCK_MAX 2048



// Compiler-specific defines used by list:

#if defined(_MSC_VER)
    #define PLF_LIST_FORCE_INLINE __forceinline

    #if _MSC_VER < 1600
        #define PLF_LIST_NOEXCEPT throw()
        #define PLF_LIST_NOEXCEPT_SWAP(the_allocator)
        #define PLF_LIST_NOEXCEPT_MOVE_ASSIGNMENT(the_allocator) throw()
    #elif _MSC_VER == 1600
        #define PLF_LIST_MOVE_SEMANTICS_SUPPORT
        #define PLF_LIST_NOEXCEPT throw()
        #define PLF_LIST_NOEXCEPT_SWAP(the_allocator)
        #define PLF_LIST_NOEXCEPT_MOVE_ASSIGNMENT(the_allocator) throw()
    #elif _MSC_VER == 1700
        #define PLF_LIST_TYPE_TRAITS_SUPPORT
        #define PLF_LIST_ALLOCATOR_TRAITS_SUPPORT
        #define PLF_LIST_MOVE_SEMANTICS_SUPPORT
        #define PLF_LIST_NOEXCEPT throw()
        #define PLF_LIST_NOEXCEPT_SWAP(the_allocator)
        #define PLF_LIST_NOEXCEPT_MOVE_ASSIGNMENT(the_allocator) throw()
    #elif _MSC_VER == 1800
        #define PLF_LIST_TYPE_TRAITS_SUPPORT
        #define PLF_LIST_ALLOCATOR_TRAITS_SUPPORT
        #define PLF_LIST_VARIADICS_SUPPORT // Variadics, in this context, means both variadic templates and variadic macros are supported
        #define PLF_LIST_MOVE_SEMANTICS_SUPPORT
        #define PLF_LIST_NOEXCEPT throw()
        #define PLF_LIST_NOEXCEPT_SWAP(the_allocator)
        #define PLF_LIST_NOEXCEPT_MOVE_ASSIGNMENT(the_allocator) throw()
        #define PLF_LIST_INITIALIZER_LIST_SUPPORT
    #elif _MSC_VER >= 1900
        #define PLF_LIST_ALIGNMENT_SUPPORT
        #define PLF_LIST_TYPE_TRAITS_SUPPORT
        #define PLF_LIST_ALLOCATOR_TRAITS_SUPPORT
        #define PLF_LIST_VARIADICS_SUPPORT
        #define PLF_LIST_MOVE_SEMANTICS_SUPPORT
        #define PLF_LIST_NOEXCEPT noexcept
        #define PLF_LIST_NOEXCEPT_SWAP(the_allocator) noexcept(std::allocator_traits<the_allocator>::propagate_on_container_swap::value)
        #define PLF_LIST_NOEXCEPT_MOVE_ASSIGNMENT(the_allocator) noexcept(std::allocator_traits<the_allocator>::is_always_equal::value)
        #define PLF_LIST_INITIALIZER_LIST_SUPPORT
    #endif

    #if defined(_MSVC_LANG) && (_MSVC_LANG >= 201703L)
        #define PLF_LIST_CONSTEXPR constexpr
    #else
        #define PLF_LIST_CONSTEXPR
    #endif

#elif defined(__cplusplus) && __cplusplus >= 201103L // C++11 support, at least
    #define PLF_LIST_FORCE_INLINE // note: GCC creates faster code without forcing inline

    #if defined(__GNUC__) && defined(__GNUC_MINOR__) && !defined(__clang__) // If compiler is GCC/G++
        #if (__GNUC__ == 4 && __GNUC_MINOR__ >= 3) || __GNUC__ > 4 // 4.2 and below do not support variadic templates
            #define PLF_LIST_VARIADICS_SUPPORT
        #endif
        #if (__GNUC__ == 4 && __GNUC_MINOR__ >= 4) || __GNUC__ > 4 // 4.3 and below do not support initializer lists
            #define PLF_LIST_INITIALIZER_LIST_SUPPORT
        #endif
        #if (__GNUC__ == 4 && __GNUC_MINOR__ < 6) || __GNUC__ < 4
            #define PLF_LIST_NOEXCEPT throw()
            #define PLF_LIST_NOEXCEPT_MOVE_ASSIGNMENT(the_allocator)
            #define PLF_LIST_NOEXCEPT_SWAP(the_allocator)
        #elif __GNUC__ < 6
            #define PLF_LIST_NOEXCEPT noexcept
            #define PLF_LIST_NOEXCEPT_MOVE_ASSIGNMENT(the_allocator) noexcept
            #define PLF_LIST_NOEXCEPT_SWAP(the_allocator) noexcept
        #else // C++17 support
            #define PLF_LIST_NOEXCEPT noexcept
            #define PLF_LIST_NOEXCEPT_MOVE_ASSIGNMENT(the_allocator) noexcept(std::allocator_traits<the_allocator>::is_always_equal::value)
            #define PLF_LIST_NOEXCEPT_SWAP(the_allocator) noexcept(std::allocator_traits<the_allocator>::propagate_on_container_swap::value)
        #endif
        #if (__GNUC__ == 4 && __GNUC_MINOR__ >= 7) || __GNUC__ > 4
            #define PLF_LIST_ALLOCATOR_TRAITS_SUPPORT
        #endif
        #if (__GNUC__ == 4 && __GNUC_MINOR__ >= 8) || __GNUC__ > 4
            #define PLF_LIST_ALIGNMENT_SUPPORT
        #endif
        #if __GNUC__ >= 5 // GCC v4.9 and below do not support std::is_trivially_copyable
            #define PLF_LIST_TYPE_TRAITS_SUPPORT
        #endif
    #elif defined(__GLIBCXX__) // Using another compiler type with libstdc++ - we are assuming full c++11 compliance for compiler - which may not be true
        #if __GLIBCXX__ >= 20080606     // libstdc++ 4.2 and below do not support variadic templates
            #define PLF_LIST_VARIADICS_SUPPORT
        #endif
        #if __GLIBCXX__ >= 20090421     // libstdc++ 4.3 and below do not support initializer lists
            #define PLF_LIST_INITIALIZER_LIST_SUPPORT
        #endif
        #if __GLIBCXX__ >= 20160111
            #define PLF_LIST_ALLOCATOR_TRAITS_SUPPORT
            #define PLF_LIST_NOEXCEPT noexcept
            #define PLF_LIST_NOEXCEPT_MOVE_ASSIGNMENT(the_allocator) noexcept(std::allocator_traits<the_allocator>::is_always_equal::value)
            #define PLF_LIST_NOEXCEPT_SWAP(the_allocator) noexcept(std::allocator_traits<the_allocator>::propagate_on_container_swap::value)
        #elif __GLIBCXX__ >= 20120322
            #define PLF_LIST_ALLOCATOR_TRAITS_SUPPORT
            #define PLF_LIST_NOEXCEPT noexcept
            #define PLF_LIST_NOEXCEPT_MOVE_ASSIGNMENT(the_allocator) noexcept
            #define PLF_LIST_NOEXCEPT_SWAP(the_allocator) noexcept
        #else
            #define PLF_LIST_NOEXCEPT throw()
            #define PLF_LIST_NOEXCEPT_MOVE_ASSIGNMENT(the_allocator)
            #define PLF_LIST_NOEXCEPT_SWAP(the_allocator)
        #endif
        #if __GLIBCXX__ >= 20130322
            #define PLF_LIST_ALIGNMENT_SUPPORT
        #endif
        #if __GLIBCXX__ >= 20150422 // libstdc++ v4.9 and below do not support std::is_trivially_copyable
            #define PLF_LIST_TYPE_TRAITS_SUPPORT
        #endif
    #elif defined(_LIBCPP_VERSION)
        #define PLF_LIST_ALLOCATOR_TRAITS_SUPPORT
        #define PLF_LIST_VARIADICS_SUPPORT
        #define PLF_LIST_INITIALIZER_LIST_SUPPORT
        #define PLF_LIST_ALIGNMENT_SUPPORT
        #define PLF_LIST_NOEXCEPT noexcept
        #define PLF_LIST_NOEXCEPT_MOVE_ASSIGNMENT(the_allocator) noexcept(std::allocator_traits<the_allocator>::is_always_equal::value)
        #define PLF_LIST_NOEXCEPT_SWAP(the_allocator) noexcept

        #if !(defined(_LIBCPP_CXX03_LANG) || defined(_LIBCPP_HAS_NO_RVALUE_REFERENCES))
            #define PLF_LIST_TYPE_TRAITS_SUPPORT
        #endif
    #else // Assume type traits and initializer support for other compilers and standard libraries
        #define PLF_LIST_ALLOCATOR_TRAITS_SUPPORT
        #define PLF_LIST_ALIGNMENT_SUPPORT
        #define PLF_LIST_VARIADICS_SUPPORT
        #define PLF_LIST_INITIALIZER_LIST_SUPPORT
        #define PLF_LIST_TYPE_TRAITS_SUPPORT
        #define PLF_LIST_NOEXCEPT noexcept
        #define PLF_LIST_NOEXCEPT_MOVE_ASSIGNMENT(the_allocator) noexcept(std::allocator_traits<the_allocator>::is_always_equal::value)
        #define PLF_LIST_NOEXCEPT_SWAP(the_allocator) noexcept
    #endif

    #if __cplusplus >= 201703L
        #if defined(__clang__) && ((__clang_major__ == 3 && __clang_minor__ == 9) || __clang_major__ > 3)
            #define PLF_LIST_CONSTEXPR constexpr
        #elif defined(__GNUC__) && __GNUC__ >= 7
            #define PLF_LIST_CONSTEXPR constexpr
        #elif !defined(__clang__) && !defined(__GNUC__)
            #define PLF_LIST_CONSTEXPR constexpr // assume correct C++17 implementation for other compilers
        #else
            #define PLF_LIST_CONSTEXPR
        #endif
    #else
        #define PLF_LIST_CONSTEXPR
    #endif

    #define PLF_LIST_MOVE_SEMANTICS_SUPPORT
#else
    #define PLF_LIST_FORCE_INLINE
    #define PLF_LIST_NOEXCEPT throw()
    #define PLF_LIST_NOEXCEPT_SWAP(the_allocator)
    #define PLF_LIST_NOEXCEPT_MOVE_ASSIGNMENT(the_allocator)
    #define PLF_LIST_CONSTEXPR
#endif



#ifdef PLF_LIST_ALLOCATOR_TRAITS_SUPPORT
    #ifdef PLF_LIST_VARIADICS_SUPPORT
        #define PLF_LIST_CONSTRUCT(the_allocator, allocator_instance, location, ...)    std::allocator_traits<the_allocator>::construct(allocator_instance, location, __VA_ARGS__)
    #else
        #define PLF_LIST_CONSTRUCT(the_allocator, allocator_instance, location, data)   std::allocator_traits<the_allocator>::construct(allocator_instance, location, data)
    #endif

    #define PLF_LIST_DESTROY(the_allocator, allocator_instance, location)               std::allocator_traits<the_allocator>::destroy(allocator_instance, location)
    #define PLF_LIST_ALLOCATE(the_allocator, allocator_instance, size, hint)            std::allocator_traits<the_allocator>::allocate(allocator_instance, size, hint)
    #define PLF_LIST_ALLOCATE_INITIALIZATION(the_allocator, size, hint)                 std::allocator_traits<the_allocator>::allocate(*this, size, hint)
    #define PLF_LIST_DEALLOCATE(the_allocator, allocator_instance, location, size)      std::allocator_traits<the_allocator>::deallocate(allocator_instance, location, size)
#else
    #ifdef PLF_LIST_VARIADICS_SUPPORT
        #define PLF_LIST_CONSTRUCT(the_allocator, allocator_instance, location, ...)    allocator_instance.construct(location, __VA_ARGS__)
    #else
        #define PLF_LIST_CONSTRUCT(the_allocator, allocator_instance, location, data)   allocator_instance.construct(location, data)
    #endif

    #define PLF_LIST_DESTROY(the_allocator, allocator_instance, location)               allocator_instance.destroy(location)
    #define PLF_LIST_ALLOCATE(the_allocator, allocator_instance, size, hint)            allocator_instance.allocate(size, hint)
    #define PLF_LIST_ALLOCATE_INITIALIZATION(the_allocator, size, hint)                 the_allocator::allocate(size, hint)
    #define PLF_LIST_DEALLOCATE(the_allocator, allocator_instance, location, size)      allocator_instance.deallocate(location, size)
#endif




#include <cstring>  // memmove, memcpy
#include <cassert>  // assert
#include <limits>   // std::numeric_limits
#include <memory>   // std::uninitialized_copy, std::allocator
#include <iterator>     // std::bidirectional_iterator_tag


#ifndef GFX_TIMSORT_HPP
    #include <algorithm> // std::sort
#endif

#ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
    #include <type_traits> // std::is_trivially_destructible, etc
#endif

#ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
    #include <utility> // std::move
#endif

#ifdef PLF_LIST_INITIALIZER_LIST_SUPPORT
    #include <initializer_list>
#endif




namespace plf
{



template <class element_type, class element_allocator_type = std::allocator<element_type> > class list : private element_allocator_type
{
public:
    // Standard container typedefs:
    typedef element_type                                                            value_type;
    typedef element_allocator_type                                                  allocator_type;
    typedef unsigned short                                                          group_size_type;

    #ifdef PLF_LIST_ALLOCATOR_TRAITS_SUPPORT // C++11
        typedef typename std::allocator_traits<element_allocator_type>::size_type           size_type;
        typedef typename std::allocator_traits<element_allocator_type>::difference_type     difference_type;
        typedef element_type &                                                              reference;
        typedef const element_type &                                                        const_reference;
        typedef typename std::allocator_traits<element_allocator_type>::pointer             pointer;
        typedef typename std::allocator_traits<element_allocator_type>::const_pointer       const_pointer;
    #else
        typedef typename element_allocator_type::size_type          size_type;
        typedef typename element_allocator_type::difference_type    difference_type;
        typedef typename element_allocator_type::reference          reference;
        typedef typename element_allocator_type::const_reference    const_reference;
        typedef typename element_allocator_type::pointer            pointer;
        typedef typename element_allocator_type::const_pointer      const_pointer;
    #endif


    // Iterator declarations:
    template <bool is_const> class list_iterator;
    typedef list_iterator<false>        iterator;
    typedef list_iterator<true>         const_iterator;
    friend class list_iterator<false>; // Using 'iterator' typedef name here is illegal under C++03
    friend class list_iterator<true>;

    template <bool is_const> class list_reverse_iterator;
    typedef list_reverse_iterator<false>        reverse_iterator;
    typedef list_reverse_iterator<true>         const_reverse_iterator;
    friend class list_reverse_iterator<false>;
    friend class list_reverse_iterator<true>;


private:
    struct group; // forward declarations for typedefs below
    struct node;

    #ifdef PLF_LIST_ALLOCATOR_TRAITS_SUPPORT // C++11
        typedef typename std::allocator_traits<element_allocator_type>::template rebind_alloc<group>                group_allocator_type;
        typedef typename std::allocator_traits<element_allocator_type>::template rebind_alloc<node>                 node_allocator_type;
        typedef typename std::allocator_traits<group_allocator_type>::pointer               group_pointer_type;
        typedef typename std::allocator_traits<node_allocator_type>::pointer                node_pointer_type;
        typedef typename std::allocator_traits<element_allocator_type>::template rebind_alloc<node_pointer_type>    node_pointer_allocator_type;
    #else
        typedef typename element_allocator_type::template rebind<group>::other              group_allocator_type;
        typedef typename element_allocator_type::template rebind<node>::other               node_allocator_type;
        typedef typename group_allocator_type::pointer              group_pointer_type;
        typedef typename node_allocator_type::pointer               node_pointer_type;
        typedef typename element_allocator_type::template rebind<node_pointer_type>::other  node_pointer_allocator_type;
    #endif



    struct node_base
    {
        node_pointer_type next, previous;

        node_base()
        {}

        node_base(const node_pointer_type &n, const node_pointer_type &p):
            next(n),
            previous(p)
        {}


        #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
            node_base(node_pointer_type &&n, node_pointer_type &&p) PLF_LIST_NOEXCEPT:
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


        #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
            node(node_pointer_type &&next, node_pointer_type &&previous, element_type &&source) PLF_LIST_NOEXCEPT:
                node_base(std::move(next), std::move(previous)),
                element(std::move(source))
            {}
        #endif


        #ifdef PLF_LIST_VARIADICS_SUPPORT
            template<typename... arguments>
            node(node_pointer_type const next, node_pointer_type const previous, arguments&&... parameters):
                node_base(next, previous),
                element(std::forward<arguments>(parameters) ...)
            {}
        #endif
    };



    struct group : public node_allocator_type
    {
        node_pointer_type nodes;
        node_pointer_type free_list_head;
        node_pointer_type beyond_end;
        group_size_type number_of_elements;


        group() PLF_LIST_NOEXCEPT:
            nodes(NULL),
            free_list_head(NULL),
            beyond_end(NULL),
            number_of_elements(0)
        {}


        #if defined(PLF_LIST_VARIADICS_SUPPORT) || defined(PLF_LIST_MOVE_SEMANTICS_SUPPORT)
            group(const group_size_type group_size, node_pointer_type const previous = NULL):
                nodes(PLF_LIST_ALLOCATE_INITIALIZATION(node_allocator_type, group_size, previous)),
                free_list_head(NULL),
                beyond_end(nodes + group_size),
                number_of_elements(0)
            {}
        #else
            // This is a hack around the fact that allocator_type::construct only supports copy construction in C++03 and copy elision does not occur on the vast majority of compilers in this circumstance. And to avoid running out of memory (and performance loss) from allocating the same block twice, we're allocating in this constructor and moving data in the copy constructor.
            group(const group_size_type group_size, node_pointer_type const previous = NULL) PLF_LIST_NOEXCEPT:
                nodes(NULL),
                free_list_head(previous),
                beyond_end(NULL),
                number_of_elements(group_size)
            {}

            // Not a real copy constructor ie. actually a move constructor. Only used for allocator.construct in C++03 for reasons stated above:
            group(const group &source):
                node_allocator_type(source),
                nodes(PLF_LIST_ALLOCATE_INITIALIZATION(node_allocator_type, source.number_of_elements, source.free_list_head)),
                free_list_head(NULL),
                beyond_end(nodes + source.number_of_elements),
                number_of_elements(0)
            {}
        #endif


        group & operator = (const group &source) PLF_LIST_NOEXCEPT // Actually a move operator, used by c++03 in group_vector's remove, expand_capacity and append
        {
            nodes = source.nodes;
            free_list_head = source.free_list_head;
            beyond_end = source.beyond_end;
            number_of_elements = source.number_of_elements;
            return *this;
        }


        #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
            group(group &&source) PLF_LIST_NOEXCEPT:
                node_allocator_type(source),
                nodes(std::move(source.nodes)),
                free_list_head(std::move(source.free_list_head)),
                beyond_end(std::move(source.beyond_end)),
                number_of_elements(source.number_of_elements)
            {
                source.nodes = NULL;
                source.beyond_end = NULL;
            }


            group & operator = (group &&source) PLF_LIST_NOEXCEPT
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


        ~group() PLF_LIST_NOEXCEPT
        {
            PLF_LIST_DEALLOCATE(node_allocator_type, (*this), nodes, static_cast<size_type>(beyond_end - nodes));
        }
    };




    class group_vector : private node_pointer_allocator_type
    {
    public:
        group_pointer_type last_endpoint_group, block_pointer, last_searched_group; // last_endpoint_group is the last -active- group in the block. Other -inactive- (previously used, now empty of elements) groups may be stored after this group for future usage (to reduce deallocation/reallocation of nodes). block_pointer + size - 1 == the last group in the block, regardless of whether or not the group is active.
        size_type size;


        struct ebco_pair2 : allocator_type // empty-base-class optimisation
        {
            size_type capacity; // Total element capacity of all initialized groups
            explicit ebco_pair2(const size_type number_of_elements) PLF_LIST_NOEXCEPT: capacity(number_of_elements) {};
        }       element_allocator_pair;

        struct ebco_pair : group_allocator_type
        {
            size_type capacity; // Total group capacity
            explicit ebco_pair(const size_type number_of_groups) PLF_LIST_NOEXCEPT: capacity(number_of_groups) {};
        }       group_allocator_pair;



        group_vector() PLF_LIST_NOEXCEPT:
            node_pointer_allocator_type(node_pointer_allocator_type()),
            last_endpoint_group(NULL),
            block_pointer(NULL),
            last_searched_group(NULL),
            size(0),
            element_allocator_pair(0),
            group_allocator_pair(0)
        {}



        inline PLF_LIST_FORCE_INLINE void blank() PLF_LIST_NOEXCEPT
        {
            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                if PLF_LIST_CONSTEXPR (std::is_trivial<group_pointer_type>::value)
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
                element_allocator_pair.capacity = 0;
                group_allocator_pair.capacity = 0;
            }
        }



        #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
            group_vector(group_vector &&source) PLF_LIST_NOEXCEPT:
                last_endpoint_group(std::move(source.last_endpoint_group)),
                block_pointer(std::move(source.block_pointer)),
                last_searched_group(std::move(source.last_searched_group)),
                size(source.size),
                element_allocator_pair(source.element_allocator_pair.capacity),
                group_allocator_pair(source.group_allocator_pair.capacity)
            {
                source.blank();
            }


            group_vector & operator = (group_vector &&source) PLF_LIST_NOEXCEPT
            {
                #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                    if PLF_LIST_CONSTEXPR (std::is_trivial<group_pointer_type>::value)
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
                    element_allocator_pair.capacity = source.element_allocator_pair.capacity;
                    group_allocator_pair.capacity = source.group_allocator_pair.capacity;
                }

                source.blank();
                return *this;
            }
        #endif



        ~group_vector() PLF_LIST_NOEXCEPT
        {}



        void destroy_all_data(const node_pointer_type last_endpoint_node) PLF_LIST_NOEXCEPT
        {
            if (block_pointer == NULL)
            {
                return;
            }

            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                if PLF_LIST_CONSTEXPR (!std::is_trivially_destructible<element_type>::value || !std::is_trivially_destructible<node_pointer_type>::value)
            #endif
            {
                clear(last_endpoint_node); // If clear has already been called, last_endpoint_node will already be == block_pointer->nodes, so no work will occur
            }

            const group_pointer_type end_group = block_pointer + size;
            for (group_pointer_type current_group = block_pointer; current_group != end_group; ++current_group)
            {
                PLF_LIST_DESTROY(group_allocator_type, group_allocator_pair, current_group);
            }

            PLF_LIST_DEALLOCATE(group_allocator_type, group_allocator_pair, block_pointer, group_allocator_pair.capacity);
            blank();
        }



        void clear(const node_pointer_type last_endpoint_node) PLF_LIST_NOEXCEPT
        {
            for (group_pointer_type current_group = block_pointer; current_group != last_endpoint_group; ++current_group)
            {
                #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                    if PLF_LIST_CONSTEXPR (!std::is_trivially_destructible<element_type>::value || !std::is_trivially_destructible<node_pointer_type>::value)
                #endif
                {
                    const node_pointer_type end = current_group->beyond_end;

                    if ((end - current_group->nodes) != current_group->number_of_elements) // If there are erased nodes present in the group
                    {
                        for (node_pointer_type current_node = current_group->nodes; current_node != end; ++current_node)
                        {
                            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                                if PLF_LIST_CONSTEXPR (!std::is_trivially_destructible<element_type>::value)
                            #endif
                            {
                                if (current_node->next != NULL) // ie. is not part of free list
                                {
                                    PLF_LIST_DESTROY(element_allocator_type, element_allocator_pair, &(current_node->element));
                                }
                            }

                            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                                if PLF_LIST_CONSTEXPR (!std::is_trivially_destructible<node_pointer_type>::value)
                            #endif
                            {
                                PLF_LIST_DESTROY(node_pointer_allocator_type, (*this), &(current_node->next));
                                PLF_LIST_DESTROY(node_pointer_allocator_type, (*this), &(current_node->previous));
                            }
                        }
                    }
                    else
                    {
                        for (node_pointer_type current_node = current_group->nodes; current_node != end; ++current_node)
                        {
                            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                                if PLF_LIST_CONSTEXPR (!std::is_trivially_destructible<element_type>::value)
                            #endif
                            {
                                PLF_LIST_DESTROY(element_allocator_type, element_allocator_pair, &(current_node->element));
                            }

                            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                                if PLF_LIST_CONSTEXPR (!std::is_trivially_destructible<node_pointer_type>::value)
                            #endif
                            {
                                PLF_LIST_DESTROY(node_pointer_allocator_type, (*this), &(current_node->next));
                                PLF_LIST_DESTROY(node_pointer_allocator_type, (*this), &(current_node->previous));
                            }
                        }
                    }
                }

                current_group->free_list_head = NULL;
                current_group->number_of_elements = 0;
            }

            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                if PLF_LIST_CONSTEXPR (!std::is_trivially_destructible<element_type>::value || !std::is_trivially_destructible<node_pointer_type>::value)
            #endif
            {
                if ((last_endpoint_node - last_endpoint_group->nodes) != last_endpoint_group->number_of_elements) // If there are erased nodes present in the group
                {
                    for (node_pointer_type current_node = last_endpoint_group->nodes; current_node != last_endpoint_node; ++current_node)
                    {
                        #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                            if PLF_LIST_CONSTEXPR (!std::is_trivially_destructible<element_type>::value)
                        #endif
                        {
                            if (current_node->next != NULL) // is not part of free list ie. element has not already had it's destructor called
                            {
                                PLF_LIST_DESTROY(element_allocator_type, element_allocator_pair, &(current_node->element));
                            }
                        }

                        #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                            if PLF_LIST_CONSTEXPR (!std::is_trivially_destructible<node_pointer_type>::value)
                        #endif
                        {
                            PLF_LIST_DESTROY(node_pointer_allocator_type, (*this), &(current_node->next));
                            PLF_LIST_DESTROY(node_pointer_allocator_type, (*this), &(current_node->previous));
                        }
                    }
                }
                else
                {
                    for (node_pointer_type current_node = last_endpoint_group->nodes; current_node != last_endpoint_node; ++current_node)
                    {
                        #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                            if PLF_LIST_CONSTEXPR (!std::is_trivially_destructible<element_type>::value)
                        #endif
                        {
                            PLF_LIST_DESTROY(element_allocator_type, element_allocator_pair, &(current_node->element));
                        }

                        #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                            if PLF_LIST_CONSTEXPR (!std::is_trivially_destructible<node_pointer_type>::value)
                        #endif
                        {
                            PLF_LIST_DESTROY(node_pointer_allocator_type, (*this), &(current_node->next));
                            PLF_LIST_DESTROY(node_pointer_allocator_type, (*this), &(current_node->previous));
                        }
                    }
                }
            }

            last_endpoint_group->free_list_head = NULL;
            last_endpoint_group->number_of_elements = 0;
            last_searched_group = last_endpoint_group = block_pointer;
        }



        void expand_capacity(const size_type new_capacity) // used by add_new and append
        {
            group_pointer_type const old_block = block_pointer;
            block_pointer = PLF_LIST_ALLOCATE(group_allocator_type, group_allocator_pair, new_capacity, 0);

            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                if PLF_LIST_CONSTEXPR (std::is_trivially_copyable<node_pointer_type>::value && std::is_trivially_destructible<node_pointer_type>::value)
                { // Dereferencing here in order to deal with smart pointer situations ie. obtaining the raw pointer from the smart pointer
                    std::memcpy(static_cast<void *>(&*block_pointer), static_cast<void *>(&*old_block), sizeof(group) * size); // reinterpret_cast necessary to deal with GCC 8 warnings
                }
                #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
                    else if PLF_LIST_CONSTEXPR (std::is_move_constructible<node_pointer_type>::value)
                    {
                        std::uninitialized_copy(std::make_move_iterator(old_block), std::make_move_iterator(old_block + size), block_pointer);
                    }
                #endif
                else
            #endif
            {
                // If allocator supplies non-trivial pointers it becomes necessary to destroy the group. uninitialized_copy will not work in this context as the copy constructor for "group" is overriden in C++03/98. The = operator for "group" has been overriden to make the following work:
                const group_pointer_type beyond_end = old_block + size;
                group_pointer_type current_new_group = block_pointer;

                for (group_pointer_type current_group = old_block; current_group != beyond_end; ++current_group)
                {
                    *(current_new_group++) = *(current_group);

                    current_group->nodes = NULL;
                    current_group->beyond_end = NULL;
                    PLF_LIST_DESTROY(group_allocator_type, group_allocator_pair, current_group);
                }
            }

            last_searched_group = block_pointer + (last_searched_group - old_block); // correct pointer post-reallocation
            PLF_LIST_DEALLOCATE(group_allocator_type, group_allocator_pair, old_block, group_allocator_pair.capacity);
            group_allocator_pair.capacity = new_capacity;
        }



        void add_new(const group_size_type group_size)
        {
            if (group_allocator_pair.capacity == size)
            {
                expand_capacity(group_allocator_pair.capacity * 2);
            }

            last_endpoint_group = block_pointer + size - 1;

            #ifdef PLF_LIST_VARIADICS_SUPPORT
                PLF_LIST_CONSTRUCT(group_allocator_type, group_allocator_pair, last_endpoint_group + 1, group_size, last_endpoint_group->nodes);
            #else
                PLF_LIST_CONSTRUCT(group_allocator_type, group_allocator_pair, last_endpoint_group + 1, group(group_size, last_endpoint_group->nodes));
            #endif

            ++last_endpoint_group; // Doing this here instead of pre-construct to avoid need for a try-catch block
            element_allocator_pair.capacity += group_size;
            ++size;
        }



        void initialize(const group_size_type group_size) // For adding first group *only* when group vector is completely empty and block_pointer is NULL
        {
            last_endpoint_group = block_pointer = last_searched_group = PLF_LIST_ALLOCATE(group_allocator_type, group_allocator_pair, 1, 0);
            group_allocator_pair.capacity = 1;

            #ifdef PLF_LIST_VARIADICS_SUPPORT
                PLF_LIST_CONSTRUCT(group_allocator_type, group_allocator_pair, last_endpoint_group, group_size);
            #else
                PLF_LIST_CONSTRUCT(group_allocator_type, group_allocator_pair, last_endpoint_group, group(group_size));
            #endif

            size = 1; // Doing these here instead of pre-construct to avoid need for a try-catch block
            element_allocator_pair.capacity = group_size;
        }



        void remove(group_pointer_type const group_to_erase) PLF_LIST_NOEXCEPT
        {
            if (last_searched_group >= group_to_erase && last_searched_group != block_pointer)
            {
                --last_searched_group;
            }

            element_allocator_pair.capacity -= static_cast<size_type>(group_to_erase->beyond_end - group_to_erase->nodes);

            PLF_LIST_DESTROY(group_allocator_type, group_allocator_pair, group_to_erase);

            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                if PLF_LIST_CONSTEXPR (std::is_trivially_copyable<node_pointer_type>::value && std::is_trivially_destructible<node_pointer_type>::value)
                { // Dereferencing here in order to deal with smart pointer situations ie. obtaining the raw pointer from the smart pointer
                    std::memmove(static_cast<void *>(&*group_to_erase), static_cast<void *>(&*group_to_erase + 1), sizeof(group) * (--size - static_cast<size_type>(&*group_to_erase - &*block_pointer)));
                }
                #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
                    else if PLF_LIST_CONSTEXPR (std::is_move_constructible<node_pointer_type>::value)
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
                PLF_LIST_DESTROY(group_allocator_type, group_allocator_pair, back);
            }
        }



        void move_to_back(group_pointer_type const group_to_erase)
        {
            if (last_searched_group >= group_to_erase && last_searched_group != block_pointer)
            {
                --last_searched_group;
            }

            group *temp_group = PLF_LIST_ALLOCATE(group_allocator_type, group_allocator_pair, 1, NULL);

            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                if PLF_LIST_CONSTEXPR (std::is_trivially_copyable<node_pointer_type>::value && std::is_trivially_destructible<node_pointer_type>::value)
                {
                    std::memcpy(static_cast<void *>(&*temp_group), static_cast<void *>(&*group_to_erase), sizeof(group));
                    std::memmove(static_cast<void *>(&*group_to_erase), static_cast<void *>(&*group_to_erase + 1), sizeof(group) * ((size - 1) - static_cast<size_type>(&*group_to_erase - &*block_pointer)));
                    std::memcpy(static_cast<void *>(&*(block_pointer + size - 1)), static_cast<void *>(&*temp_group), sizeof(group));
                }
                #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
                    else if PLF_LIST_CONSTEXPR (std::is_move_constructible<node_pointer_type>::value)
                    {
                        PLF_LIST_CONSTRUCT(group_allocator_type, group_allocator_pair, temp_group, std::move(*group_to_erase));
                        std::move(group_to_erase + 1, block_pointer + size, group_to_erase);
                        *(block_pointer + size - 1) = std::move(*temp_group);

                        if PLF_LIST_CONSTEXPR (!std::is_trivially_destructible<node_pointer_type>::value)
                        {
                            PLF_LIST_DESTROY(group_allocator_type, group_allocator_pair, temp_group);
                        }
                    }
                #endif
                else
            #endif
            {
                PLF_LIST_CONSTRUCT(group_allocator_type, group_allocator_pair, temp_group, group());

                *temp_group = *group_to_erase;
                std::copy(group_to_erase + 1, block_pointer + size, group_to_erase);
                *(block_pointer + --size) = *temp_group;

                temp_group->nodes = NULL;
                PLF_LIST_DESTROY(group_allocator_type, group_allocator_pair, temp_group);
            }

            PLF_LIST_DEALLOCATE(group_allocator_type, group_allocator_pair, temp_group, 1);
        }



        group_pointer_type get_nearest_freelist_group(const node_pointer_type location_node) PLF_LIST_NOEXCEPT // In working implementation this cannot throw
        {
            const group_pointer_type beyond_end_group = last_endpoint_group + 1;
            group_pointer_type left = last_searched_group - 1, right = last_searched_group + 1, freelist_group = NULL;
            bool right_not_beyond_back = (right < beyond_end_group);
            bool left_not_beyond_front = (left >= block_pointer);


            if (location_node >= last_searched_group->nodes && location_node < last_searched_group->beyond_end) // ie. location is within last_search_group
            {
                if (last_searched_group->free_list_head != NULL) // if last_searched_group has previously-erased nodes
                {
                    return last_searched_group;
                }
            }
            else // search for the node group which location_node is located within, using last_searched_group as a starting point and searching left and right. Try and find the closest node group with reusable erased-element locations along the way:
            {
                group_pointer_type closest_freelist_left = (last_searched_group->free_list_head == NULL) ? NULL : last_searched_group, closest_freelist_right = (last_searched_group->free_list_head == NULL) ? NULL : last_searched_group;

                while (true)
                {
                    if (right_not_beyond_back)
                    {
                        if ((location_node < right->beyond_end) && (location_node >= right->nodes)) // location_node's group is found
                        {
                            if (right->free_list_head != NULL) // group has erased nodes, reuse them:
                            {
                                last_searched_group = right;
                                return right;
                            }

                            difference_type left_distance;

                            if (closest_freelist_right != NULL)
                            {
                                last_searched_group = right;
                                left_distance = right - closest_freelist_right;

                                if (left_distance <= 2) // ie. this group is close enough to location_node's group
                                {
                                    return closest_freelist_right;
                                }

                                freelist_group = closest_freelist_right;
                            }
                            else
                            {
                                last_searched_group = right;
                                left_distance = right - left;
                            }


                            // Otherwise find closest group with freelist - check an equal distance on the right to the distance we've checked on the left:
                            const group_pointer_type end_group = (((right + left_distance) > beyond_end_group) ? beyond_end_group : (right + left_distance - 1));

                            while (++right != end_group)
                            {
                                if (right->free_list_head != NULL)
                                {
                                    return right;
                                }
                            }

                            if (freelist_group != NULL)
                            {
                                return freelist_group;
                            }

                            right_not_beyond_back = (right < beyond_end_group);
                            break; // group with reusable erased nodes not found yet, continue searching in loop below
                        }

                        if (right->free_list_head != NULL) // location_node's group not found, but a reusable location found
                        {
                            if ((closest_freelist_right == NULL) & (closest_freelist_left == NULL))
                            {
                                closest_freelist_left = right;
                            }

                            closest_freelist_right = right;
                        }

                        right_not_beyond_back = (++right < beyond_end_group);
                    }


                    if (left_not_beyond_front)
                    {
                        if ((location_node >= left->nodes) && (location_node < left->beyond_end))
                        {
                            if (left->free_list_head != NULL)
                            {
                                last_searched_group = left;
                                return left;
                            }

                            difference_type right_distance;

                            if (closest_freelist_left != NULL)
                            {
                                last_searched_group = left;
                                right_distance = closest_freelist_left - left;

                                if (right_distance <= 2)
                                {
                                    return closest_freelist_left;
                                }

                                freelist_group = closest_freelist_left;
                            }
                            else
                            {
                                last_searched_group = left;
                                right_distance = right - left;
                            }

                            // Otherwise find closest group with freelist:
                            const group_pointer_type end_group = (((left - right_distance) < block_pointer) ? block_pointer - 1 : (left - right_distance) + 1);

                            while (--left != end_group)
                            {
                                if (left->free_list_head != NULL)
                                {
                                    return left;
                                }
                            }

                            if (freelist_group != NULL)
                            {
                                return freelist_group;
                            }

                            left_not_beyond_front = (left >= block_pointer);
                            break;
                        }

                        if (left->free_list_head != NULL)
                        {
                            if ((closest_freelist_left == NULL) & (closest_freelist_right == NULL))
                            {
                                closest_freelist_right = left;
                            }

                            closest_freelist_left = left;
                        }

                        left_not_beyond_front = (--left >= block_pointer);
                    }
                }
            }


            // The node group which location_node is located within, is known at this point. Continue searching outwards from this group until a group is found with a reusable location:
            while (true)
            {
                if (right_not_beyond_back)
                {
                    if (right->free_list_head != NULL)
                    {
                        return right;
                    }

                    right_not_beyond_back = (++right < beyond_end_group);
                }

                if (left_not_beyond_front)
                {
                    if (left->free_list_head != NULL)
                    {
                        return left;
                    }

                    left_not_beyond_front = (--left >= block_pointer);
                }
            }

            // Will never reach here on functioning implementations
        }



        void swap(group_vector &source) PLF_LIST_NOEXCEPT_SWAP(group_allocator_type)
        {
            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                if PLF_LIST_CONSTEXPR (std::is_trivial<group_pointer_type>::value) // if all pointer types are trivial we can just copy using memcpy - faster - avoids constructors/destructors etc
                {
                    char temp[sizeof(group_vector)];
                    std::memcpy(static_cast<void *>(&temp), static_cast<void *>(this), sizeof(group_vector));
                    std::memcpy(static_cast<void *>(this), static_cast<void *>(&source), sizeof(group_vector));
                    std::memcpy(static_cast<void *>(&source), static_cast<void *>(&temp), sizeof(group_vector));
                }
                else
            #endif
            {
                const group_pointer_type swap_last_endpoint_group = last_endpoint_group, swap_block_pointer = block_pointer, swap_last_searched_group = last_searched_group;
                const size_type swap_size = size, swap_element_capacity = element_allocator_pair.capacity, swap_capacity = group_allocator_pair.capacity;

                last_endpoint_group = source.last_endpoint_group;
                block_pointer = source.block_pointer;
                last_searched_group = source.last_searched_group;
                size = source.size;
                element_allocator_pair.capacity = source.element_allocator_pair.capacity;
                group_allocator_pair.capacity = source.group_allocator_pair.capacity;

                source.last_endpoint_group = swap_last_endpoint_group;
                source.block_pointer = swap_block_pointer;
                source.last_searched_group = swap_last_searched_group;
                source.size = swap_size;
                source.element_allocator_pair.capacity = swap_element_capacity;
                source.group_allocator_pair.capacity = swap_capacity;
            }
        }



        void trim_trailing_groups() PLF_LIST_NOEXCEPT
        {
            const group_pointer_type beyond_last = block_pointer + size;

            for (group_pointer_type current_group = last_endpoint_group + 1; current_group != beyond_last; ++current_group)
            {
                element_allocator_pair.capacity -= static_cast<size_type>(current_group->beyond_end - current_group->nodes);
                PLF_LIST_DESTROY(group_allocator_type, group_allocator_pair, current_group);
            }

            size -= static_cast<size_type>(beyond_last - (last_endpoint_group + 1));
        }



        void append(group_vector &source)
        {
            source.trim_trailing_groups();
            trim_trailing_groups();

            if (size + source.size > group_allocator_pair.capacity)
            {
                expand_capacity(size + source.size);
            }

            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                if PLF_LIST_CONSTEXPR (std::is_trivially_copyable<node_pointer_type>::value && std::is_trivially_destructible<node_pointer_type>::value)
                { // &* in order to deal with smart pointer situations ie. obtaining the raw pointer from the smart pointer
                    std::memcpy(static_cast<void *>(&*block_pointer + size), static_cast<void *>(&*source.block_pointer), sizeof(group) * source.size);
                }
                #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
                    else if PLF_LIST_CONSTEXPR (std::is_move_constructible<node_pointer_type>::value)
                    {
                        std::uninitialized_copy(std::make_move_iterator(source.block_pointer), std::make_move_iterator(source.block_pointer + source.size), block_pointer + size);
                    }
                #endif
                else
            #endif
            {
                group_pointer_type current_new_group = block_pointer + size;
                const group_pointer_type beyond_end_source = source.block_pointer + source.size;

                for (group_pointer_type current_group = source.block_pointer; current_group != beyond_end_source; ++current_group)
                {
                    *(current_new_group++) = *(current_group);

                    current_group->nodes = NULL;
                    current_group->beyond_end = NULL;
                    PLF_LIST_DESTROY(group_allocator_type, source.group_allocator_pair, current_group);
                }
            }

            PLF_LIST_DEALLOCATE(group_allocator_type, source.group_allocator_pair, source.block_pointer, source.group_allocator_pair.capacity);
            size += source.size;
            last_endpoint_group = block_pointer + size - 1;
            element_allocator_pair.capacity += source.element_allocator_pair.capacity;
            source.blank();
        }
    };



    // Implement const/non-const iterator switching pattern:
    template <bool flag, class IsTrue, class IsFalse> struct choose;

    template <class IsTrue, class IsFalse> struct choose<true, IsTrue, IsFalse>
    {
        typedef IsTrue type;
    };

    template <class IsTrue, class IsFalse> struct choose<false, IsTrue, IsFalse>
    {
        typedef IsFalse type;
    };


public:

    template <bool is_const> class list_iterator
    {
    private:
        node_pointer_type node_pointer;

    public:
        typedef std::bidirectional_iterator_tag     iterator_category;
        typedef typename list::value_type           value_type;
        typedef typename list::difference_type      difference_type;
        typedef typename choose<is_const, typename list::const_pointer, typename list::pointer>::type       pointer;
        typedef typename choose<is_const, typename list::const_reference, typename list::reference>::type   reference;

        friend class list;



        inline PLF_LIST_FORCE_INLINE bool operator == (const list_iterator rh) const PLF_LIST_NOEXCEPT
        {
            return (node_pointer == rh.node_pointer);
        }



        inline PLF_LIST_FORCE_INLINE bool operator == (const list_iterator<!is_const> rh) const PLF_LIST_NOEXCEPT
        {
            return (node_pointer == rh.node_pointer);
        }



        inline PLF_LIST_FORCE_INLINE bool operator != (const list_iterator rh) const PLF_LIST_NOEXCEPT
        {
            return (node_pointer != rh.node_pointer);
        }



        inline PLF_LIST_FORCE_INLINE bool operator != (const list_iterator<!is_const> rh) const PLF_LIST_NOEXCEPT
        {
            return (node_pointer != rh.node_pointer);
        }



        inline PLF_LIST_FORCE_INLINE reference operator * () const
        {
            return node_pointer->element;
        }



        inline PLF_LIST_FORCE_INLINE pointer operator -> () const
        {
            return &(node_pointer->element);
        }



        inline PLF_LIST_FORCE_INLINE list_iterator & operator ++ () PLF_LIST_NOEXCEPT
        {
            assert(node_pointer != NULL); // covers uninitialised list_iterator
            node_pointer = node_pointer->next;
            return *this;
        }



        inline list_iterator operator ++(int) PLF_LIST_NOEXCEPT
        {
            const list_iterator copy(*this);
            ++*this;
            return copy;
        }



        inline PLF_LIST_FORCE_INLINE list_iterator & operator -- () PLF_LIST_NOEXCEPT
        {
            assert(node_pointer != NULL); // covers uninitialised list_iterator
            node_pointer = node_pointer->previous;
            return *this;
        }



        inline list_iterator operator -- (int) PLF_LIST_NOEXCEPT
        {
            const list_iterator copy(*this);
            --*this;
            return copy;
        }



        inline list_iterator & operator = (const list_iterator &rh) PLF_LIST_NOEXCEPT
        {
            node_pointer = rh.node_pointer;
            return *this;
        }



        inline list_iterator & operator = (const list_iterator<!is_const> &rh) PLF_LIST_NOEXCEPT
        {
            node_pointer = rh.node_pointer;
            return *this;
        }



        #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
            inline list_iterator & operator = (const list_iterator &&rh) PLF_LIST_NOEXCEPT
            {
                node_pointer = std::move(rh.node_pointer);
                return *this;
            }


            inline list_iterator & operator = (const list_iterator<!is_const> &&rh) PLF_LIST_NOEXCEPT
            {
                node_pointer = std::move(rh.node_pointer);
                return *this;
            }
        #endif



        list_iterator() PLF_LIST_NOEXCEPT: node_pointer(NULL) {}

        list_iterator(const list_iterator &source) PLF_LIST_NOEXCEPT: node_pointer(source.node_pointer) {}

        list_iterator(const list_iterator<!is_const> &source) PLF_LIST_NOEXCEPT: node_pointer(source.node_pointer) {}

        #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
            list_iterator (const list_iterator &&source) PLF_LIST_NOEXCEPT: node_pointer(std::move(source.node_pointer)) {}

            list_iterator(const list_iterator<!is_const> &&source) PLF_LIST_NOEXCEPT: node_pointer(std::move(source.node_pointer)) {}
        #endif

    private:

        list_iterator (const node_pointer_type node_p) PLF_LIST_NOEXCEPT: node_pointer(node_p) {}
    };




    template <bool is_const> class list_reverse_iterator
    {
    private:
        node_pointer_type node_pointer;

    public:
        typedef std::bidirectional_iterator_tag     iterator_category;
        typedef typename list::value_type       value_type;
        typedef typename list::difference_type      difference_type;
        typedef typename choose<is_const, typename list::const_pointer, typename list::pointer>::type       pointer;
        typedef typename choose<is_const, typename list::const_reference, typename list::reference>::type   reference;

        friend class list;


        inline PLF_LIST_FORCE_INLINE bool operator == (const list_reverse_iterator rh) const PLF_LIST_NOEXCEPT
        {
            return (node_pointer == rh.node_pointer);
        }



        inline PLF_LIST_FORCE_INLINE bool operator == (const list_reverse_iterator<!is_const> rh) const PLF_LIST_NOEXCEPT
        {
            return (node_pointer == rh.node_pointer);
        }



        inline PLF_LIST_FORCE_INLINE bool operator != (const list_reverse_iterator rh) const PLF_LIST_NOEXCEPT
        {
            return (node_pointer != rh.node_pointer);
        }



        inline PLF_LIST_FORCE_INLINE bool operator != (const list_reverse_iterator<!is_const> rh) const PLF_LIST_NOEXCEPT
        {
            return (node_pointer != rh.node_pointer);
        }



        inline PLF_LIST_FORCE_INLINE reference operator * () const
        {
            return node_pointer->element;
        }



        inline PLF_LIST_FORCE_INLINE pointer operator -> () const
        {
            return &(node_pointer->element);
        }



        inline PLF_LIST_FORCE_INLINE list_reverse_iterator & operator ++ () PLF_LIST_NOEXCEPT
        {
            assert(node_pointer != NULL); // covers uninitialised list_reverse_iterator
            node_pointer = node_pointer->previous;
            return *this;
        }



        inline list_reverse_iterator operator ++(int) PLF_LIST_NOEXCEPT
        {
            const list_reverse_iterator copy(*this);
            ++*this;
            return copy;
        }



        inline PLF_LIST_FORCE_INLINE list_reverse_iterator & operator -- () PLF_LIST_NOEXCEPT
        {
            assert(node_pointer != NULL);
            node_pointer = node_pointer->next;
            return *this;
        }



        inline list_reverse_iterator operator -- (int) PLF_LIST_NOEXCEPT
        {
            const list_reverse_iterator copy(*this);
            --*this;
            return copy;
        }



        inline list_reverse_iterator & operator = (const list_reverse_iterator &rh) PLF_LIST_NOEXCEPT
        {
            node_pointer = rh.node_pointer;
            return *this;
        }



        inline list_reverse_iterator & operator = (const list_reverse_iterator<!is_const> &rh) PLF_LIST_NOEXCEPT
        {
            node_pointer = rh.node_pointer;
            return *this;
        }



        #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
            inline list_reverse_iterator & operator = (const list_reverse_iterator &&rh) PLF_LIST_NOEXCEPT
            {
                node_pointer = std::move(rh.node_pointer);
                return *this;
            }


            inline list_reverse_iterator & operator = (const list_reverse_iterator<!is_const> &&rh) PLF_LIST_NOEXCEPT
            {
                node_pointer = std::move(rh.node_pointer);
                return *this;
            }
        #endif



        inline typename list::iterator base() const PLF_LIST_NOEXCEPT
        {
            return typename list::iterator(node_pointer->next);
        }



        list_reverse_iterator() PLF_LIST_NOEXCEPT: node_pointer(NULL) {}

        list_reverse_iterator(const list_reverse_iterator &source) PLF_LIST_NOEXCEPT: node_pointer(source.node_pointer) {}

        #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
            list_reverse_iterator (const list_reverse_iterator &&source) PLF_LIST_NOEXCEPT: node_pointer(std::move(source.node_pointer)) {}
        #endif

    private:

        list_reverse_iterator (const node_pointer_type node_p) PLF_LIST_NOEXCEPT: node_pointer(node_p) {}
    };



private:

    // Used by range-insert and range-constructor to prevent fill-insert and fill-constructor function calls mistakenly resolving to the range insert/constructor
    template <bool condition, class T = void>
    struct plf_enable_if_c
    {
        typedef T type;
    };

    template <class T>
    struct plf_enable_if_c<false, T>
    {};



    group_vector groups;
    node_base end_node;
    node_pointer_type last_endpoint; // last_endpoint being NULL means no elements have been constructed, but there may still be groups available due to clear() or reservee()
    iterator end_iterator, begin_iterator; // end_iterator is always the last entry point in last group in list (or one past the end of group)

    struct ebco_pair1 : node_pointer_allocator_type // Packaging the group allocator with least-used member variables, for empty-base-class optimisation
    {
        size_type total_number_of_elements;
        explicit ebco_pair1(const size_type total_num_elements) PLF_LIST_NOEXCEPT: total_number_of_elements(total_num_elements) {}
    }       node_pointer_allocator_pair;

    struct ebco_pair2 : node_allocator_type
    {
        size_type number_of_erased_nodes;
        explicit ebco_pair2(const size_type num_erased_nodes) PLF_LIST_NOEXCEPT: number_of_erased_nodes(num_erased_nodes) {}
    }       node_allocator_pair;



public:

    // Default constructor:

    list() PLF_LIST_NOEXCEPT:
        element_allocator_type(element_allocator_type()),
        end_node(reinterpret_cast<node_pointer_type>(&end_node), reinterpret_cast<node_pointer_type>(&end_node)),
        last_endpoint(NULL),
        end_iterator(reinterpret_cast<node_pointer_type>(&end_node)),
        begin_iterator(reinterpret_cast<node_pointer_type>(&end_node)),
        node_pointer_allocator_pair(0),
        node_allocator_pair(0)
    {}



    // Allocator-extended constructor:

    explicit list(const element_allocator_type &alloc):
        element_allocator_type(alloc),
        end_node(reinterpret_cast<node_pointer_type>(&end_node), reinterpret_cast<node_pointer_type>(&end_node)),
        last_endpoint(NULL),
        end_iterator(reinterpret_cast<node_pointer_type>(&end_node)),
        begin_iterator(reinterpret_cast<node_pointer_type>(&end_node)),
        node_pointer_allocator_pair(0),
        node_allocator_pair(0)
    {}



    // Copy constructor:

    list(const list &source):
        element_allocator_type(source),
        end_node(reinterpret_cast<node_pointer_type>(&end_node), reinterpret_cast<node_pointer_type>(&end_node)),
        last_endpoint(NULL),
        end_iterator(reinterpret_cast<node_pointer_type>(&end_node)),
        begin_iterator(reinterpret_cast<node_pointer_type>(&end_node)),
        node_pointer_allocator_pair(0),
        node_allocator_pair(0)
    {
        reserve(source.node_pointer_allocator_pair.total_number_of_elements);
        insert(end_iterator, source.begin_iterator, source.end_iterator);
    }



    // Allocator-extended copy constructor:

    list(const list &source, const allocator_type &alloc):
        element_allocator_type(alloc),
        end_node(reinterpret_cast<node_pointer_type>(&end_node), reinterpret_cast<node_pointer_type>(&end_node)),
        last_endpoint(NULL),
        end_iterator(reinterpret_cast<node_pointer_type>(&end_node)),
        begin_iterator(reinterpret_cast<node_pointer_type>(&end_node)),
        node_pointer_allocator_pair(0),
        node_allocator_pair(0)
    {
        reserve(source.node_pointer_allocator_pair.total_number_of_elements);
        insert(end_iterator, source.begin_iterator, source.end_iterator);
    }



    #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
        // Move constructor:

        list(list &&source) PLF_LIST_NOEXCEPT:
            element_allocator_type(source),
            groups(std::move(source.groups)),
            end_node(std::move(source.end_node)),
            last_endpoint(std::move(source.last_endpoint)),
            end_iterator(reinterpret_cast<node_pointer_type>(&end_node)),
            begin_iterator((source.begin_iterator.node_pointer == source.end_iterator.node_pointer) ? reinterpret_cast<node_pointer_type>(&end_node) : std::move(source.begin_iterator)),
            node_pointer_allocator_pair(source.node_pointer_allocator_pair.total_number_of_elements),
            node_allocator_pair(source.node_allocator_pair.number_of_erased_nodes)
        {
            end_node.previous->next = begin_iterator.node_pointer->previous = end_iterator.node_pointer;
            source.groups.blank();
            source.reset();
        }



        // Allocator-extended move constructor:

        list(list &&source, const allocator_type &alloc):
            element_allocator_type(alloc),
            groups(std::move(source.groups)),
            end_node(std::move(source.end_node)),
            last_endpoint(std::move(source.last_endpoint)),
            end_iterator(reinterpret_cast<node_pointer_type>(&end_node)),
            begin_iterator((source.begin_iterator.node_pointer == source.end_iterator.node_pointer) ? reinterpret_cast<node_pointer_type>(&end_node) : std::move(source.begin_iterator)),
            node_pointer_allocator_pair(source.node_pointer_allocator_pair.total_number_of_elements),
            node_allocator_pair(source.node_allocator_pair.number_of_erased_nodes)
        {
            end_node.previous->next = begin_iterator.node_pointer->previous = end_iterator.node_pointer;
            source.groups.blank();
            source.reset();
        }
    #endif



    // Fill constructor:

    list(const size_type fill_number, const element_type &element, const element_allocator_type &alloc = element_allocator_type()):
        element_allocator_type(alloc),
        end_node(reinterpret_cast<node_pointer_type>(&end_node), reinterpret_cast<node_pointer_type>(&end_node)),
        last_endpoint(NULL),
        end_iterator(reinterpret_cast<node_pointer_type>(&end_node)),
        begin_iterator(reinterpret_cast<node_pointer_type>(&end_node)),
        node_pointer_allocator_pair(0),
        node_allocator_pair(0)
    {
        reserve(fill_number);
        insert(end_iterator, fill_number, element);
    }



    // Range constructor:

    template<typename iterator_type>
    list(const typename plf_enable_if_c<!std::numeric_limits<iterator_type>::is_integer, iterator_type>::type &first, const iterator_type &last, const element_allocator_type &alloc = element_allocator_type()):
        element_allocator_type(alloc),
        end_node(reinterpret_cast<node_pointer_type>(&end_node), reinterpret_cast<node_pointer_type>(&end_node)),
        last_endpoint(NULL),
        end_iterator(reinterpret_cast<node_pointer_type>(&end_node)),
        begin_iterator(reinterpret_cast<node_pointer_type>(&end_node)),
        node_pointer_allocator_pair(0),
        node_allocator_pair(0)
    {
        insert<iterator_type>(end_iterator, first, last);
    }



    // Initializer-list constructor:

    #ifdef PLF_LIST_INITIALIZER_LIST_SUPPORT
        list(const std::initializer_list<element_type> &element_list, const element_allocator_type &alloc = element_allocator_type()):
            element_allocator_type(alloc),
            end_node(reinterpret_cast<node_pointer_type>(&end_node), reinterpret_cast<node_pointer_type>(&end_node)),
            last_endpoint(NULL),
            end_iterator(reinterpret_cast<node_pointer_type>(&end_node)),
            begin_iterator(reinterpret_cast<node_pointer_type>(&end_node)),
            node_pointer_allocator_pair(0),
            node_allocator_pair(0)
        {
            reserve(element_list.size());
            insert(end_iterator, element_list);
        }

    #endif



    ~list() PLF_LIST_NOEXCEPT
    {
        groups.destroy_all_data(last_endpoint);
    }



    inline iterator begin() PLF_LIST_NOEXCEPT
    {
        return begin_iterator;
    }



    inline const_iterator begin() const PLF_LIST_NOEXCEPT
    {
        return begin_iterator;
    }



    inline iterator end() PLF_LIST_NOEXCEPT
    {
        return end_iterator;
    }



    inline const_iterator end() const PLF_LIST_NOEXCEPT
    {
        return end_iterator;
    }



    inline const_iterator cbegin() const PLF_LIST_NOEXCEPT
    {
        return const_iterator(begin_iterator.node_pointer);
    }



    inline const_iterator cend() const PLF_LIST_NOEXCEPT
    {
        return const_iterator(end_iterator.node_pointer);
    }



    inline reverse_iterator rbegin() const PLF_LIST_NOEXCEPT
    {
        return reverse_iterator(end_node.previous);
    }



    inline reverse_iterator rend() const PLF_LIST_NOEXCEPT
    {
        return reverse_iterator(end_iterator.node_pointer);
    }



    inline const_reverse_iterator crbegin() const PLF_LIST_NOEXCEPT
    {
        return const_reverse_iterator(end_node.previous);
    }



    inline const_reverse_iterator crend() const PLF_LIST_NOEXCEPT
    {
        return const_reverse_iterator(end_iterator.node_pointer);
    }



    inline reference front()
    {
        assert(begin_iterator.node_pointer != &end_node);
        return begin_iterator.node_pointer->element;
    }



    inline const_reference front() const
    {
        assert(begin_iterator.node_pointer != &end_node);
        return begin_iterator.node_pointer->element;
    }



    inline reference back()
    {
        assert(end_node.previous != &end_node);
        return end_node.previous->element;
    }



    inline const_reference back() const
    {
        assert(end_node.previous != &end_node);
        return end_node.previous->element;
    }



    void clear() PLF_LIST_NOEXCEPT
    {
        if (last_endpoint == NULL)
        {
            return;
        }

        if (node_pointer_allocator_pair.total_number_of_elements != 0)
        {
            groups.clear(last_endpoint);
        }

        end_node.next = reinterpret_cast<node_pointer_type>(&end_node);
        end_node.previous = reinterpret_cast<node_pointer_type>(&end_node);
        last_endpoint = groups.block_pointer->nodes;
        begin_iterator.node_pointer = end_iterator.node_pointer;
        node_pointer_allocator_pair.total_number_of_elements = 0;
        node_allocator_pair.number_of_erased_nodes = 0;
    }



private:


    void reset() PLF_LIST_NOEXCEPT
    {
        groups.destroy_all_data(last_endpoint);
        last_endpoint = NULL;
        end_node.next = reinterpret_cast<node_pointer_type>(&end_node);
        end_node.previous = reinterpret_cast<node_pointer_type>(&end_node);
        begin_iterator.node_pointer = end_iterator.node_pointer;
        node_pointer_allocator_pair.total_number_of_elements = 0;
        node_allocator_pair.number_of_erased_nodes = 0;
    }




public:


    iterator insert(const iterator it, const element_type &element)
    {
        if (last_endpoint != NULL) // ie. list is not empty
        {
            if (node_allocator_pair.number_of_erased_nodes == 0) // No erased nodes available for reuse
            {
                if (last_endpoint == groups.last_endpoint_group->beyond_end) // last_endpoint is beyond the end of a group
                {
                    if (static_cast<size_type>(groups.last_endpoint_group - groups.block_pointer) == groups.size - 1) // ie. there are no reusable groups available at the back of group vector
                    {
                        groups.add_new((node_pointer_allocator_pair.total_number_of_elements < PLF_LIST_BLOCK_MAX) ? static_cast<group_size_type>(node_pointer_allocator_pair.total_number_of_elements) : PLF_LIST_BLOCK_MAX);
                    }
                    else
                    {
                        ++groups.last_endpoint_group;
                    }

                    last_endpoint = groups.last_endpoint_group->nodes;
                }

                #ifdef PLF_LIST_VARIADICS_SUPPORT
                    PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint, it.node_pointer, it.node_pointer->previous, element);
                #else
                    PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint, node(it.node_pointer, it.node_pointer->previous, element));
                #endif

                ++(groups.last_endpoint_group->number_of_elements);
                ++node_pointer_allocator_pair.total_number_of_elements;

                if (it.node_pointer == begin_iterator.node_pointer)
                {
                    begin_iterator.node_pointer = last_endpoint;
                }

                it.node_pointer->previous->next = last_endpoint;
                it.node_pointer->previous = last_endpoint;

                return iterator(last_endpoint++);
            }
            else
            {
                group_pointer_type const node_group = groups.get_nearest_freelist_group((it.node_pointer != end_iterator.node_pointer) ? it.node_pointer : end_node.previous);
                node_pointer_type const selected_node = node_group->free_list_head;
                const node_pointer_type previous = node_group->free_list_head->previous;

                #ifdef PLF_LIST_VARIADICS_SUPPORT
                    PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, selected_node, it.node_pointer, it.node_pointer->previous, element);
                #else
                    PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, selected_node, node(it.node_pointer, it.node_pointer->previous, element));
                #endif

                node_group->free_list_head = previous;
                ++(node_group->number_of_elements);
                ++node_pointer_allocator_pair.total_number_of_elements;
                --node_allocator_pair.number_of_erased_nodes;

                it.node_pointer->previous->next = selected_node;
                it.node_pointer->previous = selected_node;

                if (it.node_pointer == begin_iterator.node_pointer)
                {
                    begin_iterator.node_pointer = selected_node;
                }

                return iterator(selected_node);
            }
        }
        else // list is empty
        {
            if (groups.block_pointer == NULL) // In case of prior reserve/clear call as opposed to being uninitialized
            {
                groups.initialize(PLF_LIST_BLOCK_MIN);
            }

            groups.last_endpoint_group->number_of_elements = 1;
            end_node.next = end_node.previous = last_endpoint = begin_iterator.node_pointer = groups.last_endpoint_group->nodes;
            node_pointer_allocator_pair.total_number_of_elements = 1;

            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                if PLF_LIST_CONSTEXPR (std::is_nothrow_copy_constructible<node>::value) // Avoid try-catch code generation
                {
                    #ifdef PLF_LIST_VARIADICS_SUPPORT
                        PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint++, end_iterator.node_pointer, end_iterator.node_pointer, element);
                    #else
                        PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint++, node(end_iterator.node_pointer, end_iterator.node_pointer, element));
                    #endif
                }
                else
            #endif
            {
                try
                {
                    #ifdef PLF_LIST_VARIADICS_SUPPORT
                        PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint++, end_iterator.node_pointer, end_iterator.node_pointer, element);
                    #else
                        PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint++, node(end_iterator.node_pointer, end_iterator.node_pointer, element));
                    #endif
                }
                catch (...)
                {
                    reset();
                    throw;
                }
            }

            return begin_iterator;
        }
    }



    inline PLF_LIST_FORCE_INLINE void push_back(const element_type &element)
    {
        insert(end_iterator, element);
    }



    inline PLF_LIST_FORCE_INLINE void push_front(const element_type &element)
    {
        insert(begin_iterator, element);
    }



    #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
        iterator insert(const iterator it, element_type &&element) // This is almost identical to the insert implementation above with the only change being std::move of the element
        {
            if (last_endpoint != NULL)
            {
                if (node_allocator_pair.number_of_erased_nodes == 0)
                {
                    if (last_endpoint == groups.last_endpoint_group->beyond_end)
                    {
                        if (static_cast<size_type>(groups.last_endpoint_group - groups.block_pointer) == groups.size - 1)
                        {
                            groups.add_new((node_pointer_allocator_pair.total_number_of_elements < PLF_LIST_BLOCK_MAX) ? static_cast<group_size_type>(node_pointer_allocator_pair.total_number_of_elements) : PLF_LIST_BLOCK_MAX);
                        }
                        else
                        {
                            ++groups.last_endpoint_group;
                        }

                        last_endpoint = groups.last_endpoint_group->nodes;
                    }

                    #ifdef PLF_LIST_VARIADICS_SUPPORT
                        PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint, it.node_pointer, it.node_pointer->previous, std::move(element));
                    #else
                        PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint, node(it.node_pointer, it.node_pointer->previous, std::move(element)));
                    #endif

                    ++(groups.last_endpoint_group->number_of_elements);
                    ++node_pointer_allocator_pair.total_number_of_elements;

                    if (it.node_pointer == begin_iterator.node_pointer)
                    {
                        begin_iterator.node_pointer = last_endpoint;
                    }

                    it.node_pointer->previous->next = last_endpoint;
                    it.node_pointer->previous = last_endpoint;

                    return iterator(last_endpoint++);
                }
                else
                {
                    group_pointer_type const node_group = groups.get_nearest_freelist_group((it.node_pointer != end_iterator.node_pointer) ? it.node_pointer : end_node.previous);
                    node_pointer_type const selected_node = node_group->free_list_head;
                    const node_pointer_type previous = node_group->free_list_head->previous;

                    #ifdef PLF_LIST_VARIADICS_SUPPORT
                        PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, selected_node, it.node_pointer, it.node_pointer->previous, std::move(element));
                    #else
                        PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, selected_node, node(it.node_pointer, it.node_pointer->previous, std::move(element)));
                    #endif

                    node_group->free_list_head = previous;
                    ++(node_group->number_of_elements);
                    ++node_pointer_allocator_pair.total_number_of_elements;
                    --node_allocator_pair.number_of_erased_nodes;

                    it.node_pointer->previous->next = selected_node;
                    it.node_pointer->previous = selected_node;

                    if (it.node_pointer == begin_iterator.node_pointer)
                    {
                        begin_iterator.node_pointer = selected_node;
                    }

                    return iterator(selected_node);
                }
            }
            else
            {
                if (groups.block_pointer == NULL)
                {
                    groups.initialize(PLF_LIST_BLOCK_MIN);
                }

                groups.last_endpoint_group->number_of_elements = 1;
                end_node.next = end_node.previous = last_endpoint = begin_iterator.node_pointer = groups.last_endpoint_group->nodes;
                node_pointer_allocator_pair.total_number_of_elements = 1;

                #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                    if PLF_LIST_CONSTEXPR (std::is_nothrow_move_constructible<node>::value)
                    {
                        #ifdef PLF_LIST_VARIADICS_SUPPORT
                            PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint++, end_iterator.node_pointer, end_iterator.node_pointer, std::move(element));
                        #else
                            PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint++, node(end_iterator.node_pointer, end_iterator.node_pointer, std::move(element)));
                        #endif
                    }
                    else
                #endif
                {
                    try
                    {
                        #ifdef PLF_LIST_VARIADICS_SUPPORT
                            PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint++, end_iterator.node_pointer, end_iterator.node_pointer, std::move(element));
                        #else
                            PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint++, node(end_iterator.node_pointer, end_iterator.node_pointer, std::move(element)));
                        #endif
                    }
                    catch (...)
                    {
                        reset();
                        throw;
                    }
                }

                return begin_iterator;
            }
        }



        inline PLF_LIST_FORCE_INLINE void push_back(element_type &&element)
        {
            insert(end_iterator, std::move(element));
        }



        inline PLF_LIST_FORCE_INLINE void push_front(element_type &&element)
        {
            insert(begin_iterator, std::move(element));
        }
    #endif




    #ifdef PLF_LIST_VARIADICS_SUPPORT
        template<typename... arguments>
        iterator emplace(const iterator it, arguments &&... parameters) // This is almost identical to the insert implementations above with the only changes being std::forward of element parameters and removal of VARIADICS support checking
        {
            if (last_endpoint != NULL)
            {
                if (node_allocator_pair.number_of_erased_nodes == 0)
                {
                    if (last_endpoint == groups.last_endpoint_group->beyond_end)
                    {
                        if (static_cast<size_type>(groups.last_endpoint_group - groups.block_pointer) == groups.size - 1)
                        {
                            groups.add_new((node_pointer_allocator_pair.total_number_of_elements < PLF_LIST_BLOCK_MAX) ? static_cast<group_size_type>(node_pointer_allocator_pair.total_number_of_elements) : PLF_LIST_BLOCK_MAX);
                        }
                        else
                        {
                            ++groups.last_endpoint_group;
                        }

                        last_endpoint = groups.last_endpoint_group->nodes;
                    }

                    PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint, it.node_pointer, it.node_pointer->previous, std::forward<arguments>(parameters)...);

                    ++(groups.last_endpoint_group->number_of_elements);
                    ++node_pointer_allocator_pair.total_number_of_elements;

                    if (it.node_pointer == begin_iterator.node_pointer)
                    {
                        begin_iterator.node_pointer = last_endpoint;
                    }

                    it.node_pointer->previous->next = last_endpoint;
                    it.node_pointer->previous = last_endpoint;

                    return iterator(last_endpoint++);
                }
                else
                {
                    group_pointer_type const node_group = groups.get_nearest_freelist_group((it.node_pointer != end_iterator.node_pointer) ? it.node_pointer : end_node.previous);
                    node_pointer_type const selected_node = node_group->free_list_head;
                    const node_pointer_type previous = node_group->free_list_head->previous;

                    PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, selected_node, it.node_pointer, it.node_pointer->previous, std::forward<arguments>(parameters)...);

                    node_group->free_list_head = previous;
                    ++(node_group->number_of_elements);
                    ++node_pointer_allocator_pair.total_number_of_elements;
                    --node_allocator_pair.number_of_erased_nodes;

                    it.node_pointer->previous->next = selected_node;
                    it.node_pointer->previous = selected_node;

                    if (it.node_pointer == begin_iterator.node_pointer)
                    {
                        begin_iterator.node_pointer = selected_node;
                    }

                    return iterator(selected_node);
                }
            }
            else
            {
                if (groups.block_pointer == NULL)
                {
                    groups.initialize(PLF_LIST_BLOCK_MIN);
                }

                groups.last_endpoint_group->number_of_elements = 1;
                end_node.next = end_node.previous = last_endpoint = begin_iterator.node_pointer = groups.last_endpoint_group->nodes;
                node_pointer_allocator_pair.total_number_of_elements = 1;

                #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                    if PLF_LIST_CONSTEXPR (std::is_nothrow_constructible<element_type, arguments ...>::value)
                    {
                        PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint++, end_iterator.node_pointer, end_iterator.node_pointer, std::forward<arguments>(parameters)...);
                    }
                    else
                #endif
                {
                    try
                    {
                        PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint++, end_iterator.node_pointer, end_iterator.node_pointer, std::forward<arguments>(parameters)...);
                    }
                    catch (...)
                    {
                        reset();
                        throw;
                    }
                }

                return begin_iterator;
            }
        }



        template<typename... arguments>
        inline PLF_LIST_FORCE_INLINE reference emplace_back(arguments &&... parameters)
        {
            return (emplace(end_iterator, std::forward<arguments>(parameters)...)).node_pointer->element;
        }



        template<typename... arguments>
        inline PLF_LIST_FORCE_INLINE reference emplace_front(arguments &&... parameters)
        {
            return (emplace(begin_iterator, std::forward<arguments>(parameters)...)).node_pointer->element;
        }


    #endif



private:

    void group_fill_position(const element_type &element, group_size_type number_of_elements, node_pointer_type const position)
    {
        position->previous->next = last_endpoint;
        groups.last_endpoint_group->number_of_elements += number_of_elements;
        node_pointer_type previous = position->previous;

        do
        {
            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                if PLF_LIST_CONSTEXPR (std::is_nothrow_copy_constructible<element_type>::value)
                {
                    #ifdef PLF_LIST_VARIADICS_SUPPORT
                        PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint, last_endpoint + 1, previous, element);
                    #else
                        PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint, node(last_endpoint + 1, previous, element));
                    #endif
                }
                else
            #endif
            {
                try
                {
                    #ifdef PLF_LIST_VARIADICS_SUPPORT
                        PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint, last_endpoint + 1, previous, element);
                    #else
                        PLF_LIST_CONSTRUCT(node_allocator_type, node_allocator_pair, last_endpoint, node(last_endpoint + 1, previous, element));
                    #endif
                }
                catch (...)
                {
                    previous->next = position;
                    position->previous = --previous;
                    groups.last_endpoint_group->number_of_elements -= static_cast<group_size_type>(number_of_elements - (last_endpoint - position));
                    throw;
                }
            }

            previous = last_endpoint++;
        } while (--number_of_elements != 0);

        previous->next = position;
        position->previous = previous;
    }



public:

    // Fill insert

    iterator insert(iterator position, const size_type number_of_elements, const element_type &element)
    {
        if (number_of_elements == 0)
        {
            return end_iterator;
        }
        else if (number_of_elements == 1)
        {
            return insert(position, element);
        }


        if (node_pointer_allocator_pair.total_number_of_elements == 0 && last_endpoint != NULL && (static_cast<size_type>(groups.block_pointer->beyond_end - groups.block_pointer->nodes) < number_of_elements) && (static_cast<size_type>(groups.block_pointer->beyond_end - groups.block_pointer->nodes) < PLF_LIST_BLOCK_MAX))
        {
            reset();
        }


        if (groups.block_pointer == NULL) // ie. Uninitialized list
        {
            if (number_of_elements > PLF_LIST_BLOCK_MAX)
            {
                size_type multiples = number_of_elements / PLF_LIST_BLOCK_MAX;
                const group_size_type remainder = static_cast<group_size_type>(number_of_elements - (multiples++ * PLF_LIST_BLOCK_MAX)); // ++ to aid while loop below

                // Create and fill first group:
                if (remainder != 0) // make sure smallest block is first
                {
                    if (remainder >= PLF_LIST_BLOCK_MIN)
                    {
                        groups.initialize(remainder);
                        end_node.next = end_node.previous = last_endpoint = begin_iterator.node_pointer = groups.last_endpoint_group->nodes;
                        group_fill_position(element, remainder, end_iterator.node_pointer);
                    }
                    else
                    { // Create first group as BLOCK_MIN size then subtract difference between BLOCK_MIN and remainder from next group:
                        groups.initialize(PLF_LIST_BLOCK_MIN);
                        end_node.next = end_node.previous = last_endpoint = begin_iterator.node_pointer = groups.last_endpoint_group->nodes;
                        group_fill_position(element, PLF_LIST_BLOCK_MIN, end_iterator.node_pointer);

                        groups.add_new(PLF_LIST_BLOCK_MAX - (PLF_LIST_BLOCK_MIN - remainder));
                        end_node.previous = last_endpoint = groups.last_endpoint_group->nodes;
                        group_fill_position(element, PLF_LIST_BLOCK_MAX - (PLF_LIST_BLOCK_MIN - remainder), end_iterator.node_pointer);
                        --multiples;
                    }
                }
                else
                {
                    groups.initialize(PLF_LIST_BLOCK_MAX);
                    end_node.next = end_node.previous = last_endpoint = begin_iterator.node_pointer = groups.last_endpoint_group->nodes;
                    group_fill_position(element, PLF_LIST_BLOCK_MAX, end_iterator.node_pointer);
                    --multiples;
                }

                while (--multiples != 0)
                {
                    groups.add_new(PLF_LIST_BLOCK_MAX);
                    end_node.previous = last_endpoint = groups.last_endpoint_group->nodes;
                    group_fill_position(element, PLF_LIST_BLOCK_MAX, end_iterator.node_pointer);
                }

            }
            else
            {
                groups.initialize((number_of_elements < PLF_LIST_BLOCK_MIN) ? PLF_LIST_BLOCK_MIN : static_cast<group_size_type>(number_of_elements)); // Construct first group
                end_node.next = end_node.previous = last_endpoint = begin_iterator.node_pointer = groups.last_endpoint_group->nodes;
                group_fill_position(element, static_cast<group_size_type>(number_of_elements), end_iterator.node_pointer);
            }

            node_pointer_allocator_pair.total_number_of_elements = number_of_elements;
            return begin_iterator;
        }
        else
        {
            // Insert first element, then use up any erased nodes:
            size_type remainder = number_of_elements - 1;
            const iterator return_iterator = insert(position, element);

            while (node_allocator_pair.number_of_erased_nodes != 0)
            {
                insert(position, element);
                --node_allocator_pair.number_of_erased_nodes;

                if (--remainder == 0)
                {
                    return return_iterator;
                }
            }

            node_pointer_allocator_pair.total_number_of_elements += remainder;

            // then use up remainder of last_endpoint_group:
            const group_size_type remaining_nodes_in_group = static_cast<group_size_type>(groups.last_endpoint_group->beyond_end - last_endpoint);

            if (remaining_nodes_in_group != 0)
            {
                if (remaining_nodes_in_group < remainder)
                {
                    group_fill_position(element, remaining_nodes_in_group, position.node_pointer);
                    remainder -= remaining_nodes_in_group;
                }
                else
                {
                    group_fill_position(element, static_cast<group_size_type>(remainder), position.node_pointer);
                    return return_iterator;
                }
            }


            // use up trailing groups:
            while ((groups.last_endpoint_group != (groups.block_pointer + groups.size - 1)) & (remainder != 0))
            {
                last_endpoint = (++groups.last_endpoint_group)->nodes;
                const group_size_type group_size = static_cast<group_size_type>(groups.last_endpoint_group->beyond_end - groups.last_endpoint_group->nodes);

                if (group_size < remainder)
                {
                    group_fill_position(element, group_size, position.node_pointer);
                    remainder -= group_size;
                }
                else
                {
                    group_fill_position(element, static_cast<group_size_type>(remainder), position.node_pointer);
                    return return_iterator;
                }
            }

            size_type multiples = remainder / static_cast<size_type>(PLF_LIST_BLOCK_MAX);
            remainder -= multiples * PLF_LIST_BLOCK_MAX;

            while (multiples-- != 0)
            {
                groups.add_new(PLF_LIST_BLOCK_MAX);
                last_endpoint = groups.last_endpoint_group->nodes;
                group_fill_position(element, PLF_LIST_BLOCK_MAX, position.node_pointer);
            }

            if (remainder != 0) // Bit annoying to create a large block to house a lower number of elements, but beats the alternatives
            {
                groups.add_new(PLF_LIST_BLOCK_MAX);
                last_endpoint = groups.last_endpoint_group->nodes;
                group_fill_position(element, static_cast<group_size_type>(remainder), position.node_pointer);
            }

            return return_iterator;
        }
    }



    // Range insert

    template <class iterator_type>
    iterator insert(const iterator it, typename plf_enable_if_c<!std::numeric_limits<iterator_type>::is_integer, iterator_type>::type first, const iterator_type last)
    {
        if (first == last)
        {
            return end_iterator;
        }

        const iterator return_iterator = insert(it, *first);

        while(++first != last)
        {
            insert(it, *first);
        }

        return return_iterator;
    }



    // Initializer-list insert

    #ifdef PLF_LIST_INITIALIZER_LIST_SUPPORT
        inline iterator insert(const iterator it, const std::initializer_list<element_type> &element_list)
        { // use range insert:
            return insert(it, element_list.begin(), element_list.end());
        }
    #endif



private:

    inline PLF_LIST_FORCE_INLINE void destroy_all_node_pointers(group_pointer_type const group_to_process, const node_pointer_type beyond_end_node) PLF_LIST_NOEXCEPT
    {
        for (node_pointer_type current_node = group_to_process->nodes; current_node != beyond_end_node; ++current_node)
        {
            PLF_LIST_DESTROY(node_pointer_allocator_type, node_pointer_allocator_pair, &(current_node->next)); // Destruct element
            PLF_LIST_DESTROY(node_pointer_allocator_type, node_pointer_allocator_pair, &(current_node->previous)); // Destruct element
        }
    }



public:


    // Single erase:

    iterator erase(const const_iterator it) // if uninitialized/invalid iterator supplied, function could generate an exception, hence no noexcept
    {
        assert(node_pointer_allocator_pair.total_number_of_elements != 0);
        assert(it.node_pointer != NULL);
        assert(it.node_pointer != end_iterator.node_pointer);

        #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
            if PLF_LIST_CONSTEXPR (!(std::is_trivially_destructible<element_type>::value))
        #endif
        {
            PLF_LIST_DESTROY(element_allocator_type, (*this), &(it.node_pointer->element)); // Destruct element
        }

        --node_pointer_allocator_pair.total_number_of_elements;
        ++node_allocator_pair.number_of_erased_nodes;


        group_pointer_type node_group = groups.last_searched_group;

        // find nearest group with reusable (erased element) memory location:
        if ((it.node_pointer < node_group->nodes) || (it.node_pointer >= node_group->beyond_end))
        {
            // Search left and right:
            const group_pointer_type beyond_end_group = groups.last_endpoint_group + 1;
            group_pointer_type left = node_group - 1;
            bool right_not_beyond_back = (++node_group < beyond_end_group);
            bool left_not_beyond_front = (left >= groups.block_pointer);

            while (true)
            {
                if (right_not_beyond_back)
                {
                    if ((it.node_pointer < node_group->beyond_end) && (it.node_pointer >= node_group->nodes)) // usable location found
                    {
                        break;
                    }

                    right_not_beyond_back = (++node_group < beyond_end_group);
                }

                if (left_not_beyond_front)
                {
                    if ((it.node_pointer >= left->nodes) && (it.node_pointer < left->beyond_end)) // usable location found
                    {
                        node_group = left;
                        break;
                    }

                    left_not_beyond_front = (--left >= groups.block_pointer);
                }
            }

            groups.last_searched_group = node_group;
        }

        it.node_pointer->next->previous = it.node_pointer->previous;
        it.node_pointer->previous->next = it.node_pointer->next;

        if (it.node_pointer == begin_iterator.node_pointer)
        {
            begin_iterator.node_pointer = it.node_pointer->next;
        }


        const iterator return_iterator(it.node_pointer->next);

        if (--(node_group->number_of_elements) != 0) // ie. group is not empty yet, add node to free list
        {
            it.node_pointer->next = NULL; // next == NULL so that destructor can detect the free list item as opposed to non-free-list item
            it.node_pointer->previous = node_group->free_list_head;
            node_group->free_list_head = it.node_pointer;
            return return_iterator;
        }
        else if (node_group != groups.last_endpoint_group--) // remove group (and decrement active back group)
        {
            const group_size_type group_size = static_cast<group_size_type>(node_group->beyond_end - node_group->nodes);
            node_allocator_pair.number_of_erased_nodes -= group_size;

            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                if PLF_LIST_CONSTEXPR (!(std::is_trivially_destructible<node_pointer_type>::value))
            #endif
            {
                destroy_all_node_pointers(node_group, node_group->beyond_end);
            }

            node_group->free_list_head = NULL;

            if ((group_size == PLF_LIST_BLOCK_MAX) | (node_group >= groups.last_endpoint_group - 1)) // Preserve only last (active) group or second/third-to-last group - seems to be best for performance under high-modification benchmarks
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
            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                if PLF_LIST_CONSTEXPR (!(std::is_trivially_destructible<node_pointer_type>::value))
            #endif
            {
                destroy_all_node_pointers(node_group, last_endpoint);
            }

            node_group->free_list_head = NULL;

            if (node_pointer_allocator_pair.total_number_of_elements != 0)
            {
                node_allocator_pair.number_of_erased_nodes -= static_cast<group_size_type>(last_endpoint - node_group->nodes);
                last_endpoint = groups.last_endpoint_group->beyond_end;
            }
            else
            {
                groups.last_endpoint_group = groups.block_pointer; // If number of elements is zero, it indicates that this was the first group in the vector. In which case the last_endpoint_group would be invalid at this point due to the decrement in the above else-if statement. So it needs to be reset, as it will not be reset in the function call below.
                clear();
            }


            return return_iterator;
        }
    }



    // Range-erase:

    inline void erase(const const_iterator iterator1, const const_iterator iterator2)  // if uninitialized/invalid iterator supplied, function could generate an exception
    {
        for (const_iterator current = iterator1; current != iterator2;)
        {
            current = erase(current);
        }
    }



    inline void pop_back() // Exception will occur on empty list
    {
        erase(iterator(end_node.previous));
    }



    inline void pop_front() // Exception will occur on empty list
    {
        erase(begin_iterator);
    }



    inline list & operator = (const list &source)
    {
        assert (&source != this);

        clear();
        reserve(source.node_pointer_allocator_pair.total_number_of_elements);
        insert(end_iterator, source.begin_iterator, source.end_iterator);

        return *this;
    }



    #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
        // Move assignment
        list & operator = (list &&source) PLF_LIST_NOEXCEPT_MOVE_ASSIGNMENT(allocator_type)
        {
            assert (&source != this);

            // Move source values across:
            groups.destroy_all_data(last_endpoint);

            groups = std::move(source.groups);
            end_node = std::move(source.end_node);
            last_endpoint = std::move(source.last_endpoint);
            begin_iterator.node_pointer = (source.begin_iterator.node_pointer == source.end_iterator.node_pointer) ? end_iterator.node_pointer : std::move(source.begin_iterator.node_pointer);
            node_pointer_allocator_pair.total_number_of_elements = source.node_pointer_allocator_pair.total_number_of_elements;
            node_allocator_pair.number_of_erased_nodes = source.node_allocator_pair.number_of_erased_nodes;

            end_node.previous->next = begin_iterator.node_pointer->previous = end_iterator.node_pointer;

            source.groups.blank();
            source.reset();
            return *this;
        }
    #endif



    bool operator == (const list &rh) const PLF_LIST_NOEXCEPT
    {
        assert (this != &rh);

        if (node_pointer_allocator_pair.total_number_of_elements != rh.node_pointer_allocator_pair.total_number_of_elements)
        {
            return false;
        }

        iterator rh_iterator = rh.begin_iterator;

        for (iterator lh_iterator = begin_iterator; lh_iterator != end_iterator;)
        {
            if (*rh_iterator++ != *lh_iterator++)
            {
                return false;
            }
        }

        return true;
    }



    inline bool operator != (const list &rh) const PLF_LIST_NOEXCEPT
    {
        return !(*this == rh);
    }



    inline bool empty() const PLF_LIST_NOEXCEPT
    {
        return node_pointer_allocator_pair.total_number_of_elements == 0;
    }



    inline size_type size() const PLF_LIST_NOEXCEPT
    {
        return node_pointer_allocator_pair.total_number_of_elements;
    }



    inline size_type max_size() const PLF_LIST_NOEXCEPT
    {
        #ifdef PLF_LIST_ALLOCATOR_TRAITS_SUPPORT
        return std::allocator_traits<element_allocator_type>::max_size(*this);
        #else
        return element_allocator_type::max_size();
      #endif
    }



    inline size_type capacity() const PLF_LIST_NOEXCEPT
    {
        return groups.element_allocator_pair.capacity;
    }



    inline size_type approximate_memory_use() const PLF_LIST_NOEXCEPT
    {
        return static_cast<size_type>(sizeof(*this) + (groups.element_allocator_pair.capacity * sizeof(node)) + (sizeof(group) * groups.group_allocator_pair.capacity));
    }



private:


    struct less
    {
        inline bool operator() (const element_type &a, const element_type &b) const PLF_LIST_NOEXCEPT
        {
            return a < b;
        }
    };



    // Function-object to redirect the sort function to sort pointers by the elements they point to, not the pointer value
    template <class comparison_function>
    struct sort_dereferencer
    {
        comparison_function stored_instance;

        explicit sort_dereferencer(const comparison_function &function_instance):
            stored_instance(function_instance)
        {}

        sort_dereferencer() PLF_LIST_NOEXCEPT
        {}

        inline bool operator() (const node_pointer_type first, const node_pointer_type second)
        {
            return stored_instance(first->element, second->element);
        }
    };



public:


    template <class comparison_function>
    void sort(comparison_function compare)
    {
        if (node_pointer_allocator_pair.total_number_of_elements < 2)
        {
            return;
        }

        node_pointer_type * const node_pointers = PLF_LIST_ALLOCATE(node_pointer_allocator_type, node_pointer_allocator_pair, node_pointer_allocator_pair.total_number_of_elements, NULL);
        node_pointer_type *node_pointer = node_pointers;


        // According to the C++ standard, construction of a pointer (of any type) may not trigger an exception - hence, no try-catch blocks are necessary for constructing the pointers:
        for (group_pointer_type current_group = groups.block_pointer; current_group != groups.last_endpoint_group; ++current_group)
        {
            const node_pointer_type end = current_group->beyond_end;

            if ((end - current_group->nodes) != current_group->number_of_elements) // If there are erased nodes present in the group
            {
                for (node_pointer_type current_node = current_group->nodes; current_node != end; ++current_node)
                {
                    if (current_node->next != NULL) // is not free list node
                    {
                        PLF_LIST_CONSTRUCT(node_pointer_allocator_type, node_pointer_allocator_pair, node_pointer++, current_node);
                    }
                }
            }
            else
            {
                for (node_pointer_type current_node = current_group->nodes; current_node != end; ++current_node)
                {
                    PLF_LIST_CONSTRUCT(node_pointer_allocator_type, node_pointer_allocator_pair, node_pointer++, current_node);
                }
            }
        }

        if ((last_endpoint - groups.last_endpoint_group->nodes) != groups.last_endpoint_group->number_of_elements) // If there are erased nodes present in the group
        {
            for (node_pointer_type current_node = groups.last_endpoint_group->nodes; current_node != last_endpoint; ++current_node)
            {
                if (current_node->next != NULL)
                {
                    PLF_LIST_CONSTRUCT(node_pointer_allocator_type, node_pointer_allocator_pair, node_pointer++, current_node);
                }
            }
        }
        else
        {
            for (node_pointer_type current_node = groups.last_endpoint_group->nodes; current_node != last_endpoint; ++current_node)
            {
                PLF_LIST_CONSTRUCT(node_pointer_allocator_type, node_pointer_allocator_pair, node_pointer++, current_node);
            }
        }


        #ifdef GFX_TIMSORT_HPP
            gfx::timsort(node_pointers, node_pointers + node_pointer_allocator_pair.total_number_of_elements, sort_dereferencer<comparison_function>(compare));
        #else
            std::sort(node_pointers, node_pointers + node_pointer_allocator_pair.total_number_of_elements, sort_dereferencer<comparison_function>(compare));
        #endif

        begin_iterator.node_pointer = node_pointers[0];
        begin_iterator.node_pointer->next = node_pointers[1];
        begin_iterator.node_pointer->previous = end_iterator.node_pointer;

        end_node.next = node_pointers[0];
        end_node.previous = node_pointers[node_pointer_allocator_pair.total_number_of_elements - 1];
        end_node.previous->next = end_iterator.node_pointer;
        end_node.previous->previous = node_pointers[node_pointer_allocator_pair.total_number_of_elements - 2];

        node_pointer_type * const back = node_pointers + node_pointer_allocator_pair.total_number_of_elements - 1;

        for(node_pointer = node_pointers + 1; node_pointer != back; ++node_pointer)
        {
            (*node_pointer)->next = *(node_pointer + 1);
            (*node_pointer)->previous = *(node_pointer - 1);

            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                if PLF_LIST_CONSTEXPR (!std::is_trivially_destructible<node_pointer_type>::value)
            #endif
            {
                PLF_LIST_DESTROY(node_pointer_allocator_type, node_pointer_allocator_pair, node_pointer - 1);
            }
        }

        #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
            if PLF_LIST_CONSTEXPR (!std::is_trivially_destructible<node_pointer_type>::value)
        #endif
        {
            PLF_LIST_DESTROY(node_pointer_allocator_type, node_pointer_allocator_pair, back);
        }

        PLF_LIST_DEALLOCATE(node_pointer_allocator_type, node_pointer_allocator_pair, node_pointers, node_pointer_allocator_pair.total_number_of_elements);
    }



    inline void sort()
    {
        sort(less());
    }



    void reorder(const iterator position, const iterator first, const iterator last) PLF_LIST_NOEXCEPT
    {
        last.node_pointer->next->previous = first.node_pointer->previous;
        first.node_pointer->previous->next = last.node_pointer->next;

        last.node_pointer->next = position.node_pointer;
        first.node_pointer->previous = position.node_pointer->previous;

        position.node_pointer->previous->next = first.node_pointer;
        position.node_pointer->previous = last.node_pointer;

        if (begin_iterator == position)
        {
            begin_iterator = first;
        }
    }



    inline void reorder(const iterator position, const iterator location) PLF_LIST_NOEXCEPT
    {
        reorder(position, location, location);
    }



    void reserve(size_type reserve_amount)
    {
        if (reserve_amount == 0 || reserve_amount <= groups.element_allocator_pair.capacity)
        {
            return;
        }
        else if (reserve_amount < PLF_LIST_BLOCK_MIN)
        {
            reserve_amount = PLF_LIST_BLOCK_MIN;
        }
        else if (reserve_amount > max_size())
        {
            reserve_amount = max_size();
        }


        if (groups.block_pointer != NULL && node_pointer_allocator_pair.total_number_of_elements == 0)
        { // edge case: has been filled with elements then clear()'d - some groups may be smaller than would be desired, should be replaced
            group_size_type end_group_size = static_cast<group_size_type>((groups.block_pointer + groups.size - 1)->beyond_end - (groups.block_pointer + groups.size - 1)->nodes);

            if (reserve_amount > end_group_size && end_group_size != PLF_LIST_BLOCK_MAX) // if last group isn't large enough, remove all groups
            {
                reset();
            }
            else
            {
                size_type number_of_full_groups_needed = reserve_amount / PLF_LIST_BLOCK_MAX;
                group_size_type remainder = static_cast<group_size_type>(reserve_amount - (number_of_full_groups_needed * PLF_LIST_BLOCK_MAX));

                // Remove any max_size groups which're not needed and any groups that're smaller than remainder:
                for (group_pointer_type current_group = groups.block_pointer; current_group < groups.block_pointer + groups.size;)
                {
                    const group_size_type current_group_size = static_cast<group_size_type>(groups.block_pointer->beyond_end - groups.block_pointer->nodes);

                    if (number_of_full_groups_needed != 0 && current_group_size == PLF_LIST_BLOCK_MAX)
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

        reserve_amount -= groups.element_allocator_pair.capacity;

        // To correct from possible reallocation caused by add_new:
        const difference_type last_endpoint_group_number = groups.last_endpoint_group - groups.block_pointer;

        size_type number_of_full_groups = (reserve_amount / PLF_LIST_BLOCK_MAX);
        reserve_amount -= (number_of_full_groups++ * PLF_LIST_BLOCK_MAX); // ++ to aid while loop below

        if (groups.block_pointer == NULL) // Previously uninitialized list or reset in above if statement; most common scenario
        {
            if (reserve_amount != 0)
            {
                groups.initialize(static_cast<group_size_type>(((reserve_amount < PLF_LIST_BLOCK_MIN) ? PLF_LIST_BLOCK_MIN : reserve_amount)));
            }
            else
            {
                groups.initialize(PLF_LIST_BLOCK_MAX);
                --number_of_full_groups;
            }
        }
        else if (reserve_amount != 0)
        { // Create a group at least as large as the last group - may allocate more than necessary, but better solution than creating a veyr small group in the middle of the group vector, I think:
            const group_size_type last_endpoint_group_capacity = static_cast<group_size_type>(groups.last_endpoint_group->beyond_end - groups.last_endpoint_group->nodes);
            groups.add_new(static_cast<group_size_type>((reserve_amount < last_endpoint_group_capacity) ? last_endpoint_group_capacity : reserve_amount));
        }

        while (--number_of_full_groups != 0)
        {
            groups.add_new(PLF_LIST_BLOCK_MAX);
        }

        groups.last_endpoint_group = groups.block_pointer + last_endpoint_group_number;
    }



    inline PLF_LIST_FORCE_INLINE void free_unused_memory() PLF_LIST_NOEXCEPT
    {
        groups.trim_trailing_groups();
    }



    void shrink_to_fit()
    {
        if ((last_endpoint == NULL) | (node_pointer_allocator_pair.total_number_of_elements == groups.element_allocator_pair.capacity)) // uninitialized list or full
        {
            return;
        }
        else if (node_pointer_allocator_pair.total_number_of_elements == 0) // Edge case
        {
            reset();
            return;
        }
        else if (node_allocator_pair.number_of_erased_nodes == 0 && last_endpoint == groups.last_endpoint_group->beyond_end) //edge case - currently no waste except for possible trailing groups
        {
            groups.trim_trailing_groups();
            return;
        }

        #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
            list temp;
            temp.reserve(node_pointer_allocator_pair.total_number_of_elements);

            #ifdef PLF_LIST_TYPE_TRAITS_SUPPORT
                if PLF_LIST_CONSTEXPR (std::is_move_assignable<element_type>::value && std::is_move_constructible<element_type>::value) // move elements if possible, otherwise copy them
                {
                    temp.insert(temp.end_iterator, std::make_move_iterator(begin_iterator), std::make_move_iterator(end_iterator));
                }
                else
            #endif
            {
                temp.insert(temp.end_iterator, begin_iterator, end_iterator);
            }

            *this = std::move(temp);
        #else
            list temp(*this);
            reset();
            swap(temp);
        #endif
    }



private:

    void append_process(list &source) // used by merge and splice
    {
        if (last_endpoint != groups.last_endpoint_group->beyond_end)
        {   // Add unused nodes to group's free list
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
        node_pointer_allocator_pair.total_number_of_elements += source.node_pointer_allocator_pair.total_number_of_elements;
        source.reset();
    }




public:

    void splice(iterator position, list &source)
    {
        assert(&source != this);

        if (source.node_pointer_allocator_pair.total_number_of_elements == 0)
        {
            return;
        }
        else if (node_pointer_allocator_pair.total_number_of_elements == 0)
        {
            #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
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



    template <class comparison_function>
    void merge(list &source, comparison_function compare)
    {
        assert(&source != this);
        splice((source.node_pointer_allocator_pair.total_number_of_elements >= node_pointer_allocator_pair.total_number_of_elements) ? end_iterator : begin_iterator, source);
        sort(compare);
    }



    void merge(list &source)
    {
        assert(&source != this);

        if (source.node_pointer_allocator_pair.total_number_of_elements == 0)
        {
            return;
        }
        else if (node_pointer_allocator_pair.total_number_of_elements == 0)
        {
            #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
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



    void reverse() PLF_LIST_NOEXCEPT
    {
        if (node_pointer_allocator_pair.total_number_of_elements > 1)
        {
            for (group_pointer_type current_group = groups.block_pointer; current_group != groups.last_endpoint_group; ++current_group)
            {
                const node_pointer_type end = current_group->beyond_end;

                for (node_pointer_type current_node = current_group->nodes; current_node != end; ++current_node)
                {
                    if (current_node->next != NULL) // is not free list node
                    { // swap the pointers:
                        const node_pointer_type temp = current_node->next;
                        current_node->next = current_node->previous;
                        current_node->previous = temp;
                    }
                }
            }

            for (node_pointer_type current_node = groups.last_endpoint_group->nodes; current_node != last_endpoint; ++current_node)
            {
                if (current_node->next != NULL)
                {
                    const node_pointer_type temp = current_node->next;
                    current_node->next = current_node->previous;
                    current_node->previous = temp;
                }
            }

            const node_pointer_type temp = end_node.previous;
            end_node.previous = begin_iterator.node_pointer;
            begin_iterator.node_pointer = temp;

            end_node.previous->next = end_iterator.node_pointer;
            begin_iterator.node_pointer->previous = end_iterator.node_pointer;
        }
    }



private:

    // Used by unique()
    struct eq
    {
        inline bool operator() (const element_type &a, const element_type &b) const PLF_LIST_NOEXCEPT
        {
            return a == b;
        }
    };



    // Used by remove()
    struct eq_to
    {
        const element_type value;

        explicit eq_to(const element_type store_value):
            value(store_value)
        {}

        eq_to() PLF_LIST_NOEXCEPT
        {}

        inline bool operator() (const element_type compare_value) const PLF_LIST_NOEXCEPT
        {
            return value == compare_value;
        }
    };



public:

    template <class comparison_function>
    size_type unique(comparison_function compare)
    {
        const size_type original_number_of_elements = node_pointer_allocator_pair.total_number_of_elements;

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

        return original_number_of_elements - node_pointer_allocator_pair.total_number_of_elements;
    }



    inline size_type unique()
    {
        return unique(eq());
    }



    template <class predicate_function>
    size_type remove_if(predicate_function predicate)
    {
        const size_type original_number_of_elements = node_pointer_allocator_pair.total_number_of_elements;

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

                        if (--num_elements == 0)
                        {
                            break;
                        }
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

                        if (--num_elements == 0)
                        {
                            break;
                        }
                    }
                }
            }
        }

        return original_number_of_elements - node_pointer_allocator_pair.total_number_of_elements;
    }



    inline size_type remove(const element_type &value)
    {
        return remove_if(eq_to(value));
    }



    void resize(const size_type number_of_elements, const element_type &value = element_type())
    {
        if (node_pointer_allocator_pair.total_number_of_elements == number_of_elements)
        {
            return;
        }
        else if (number_of_elements == 0)
        {
            clear();
            return;
        }
        else if (node_pointer_allocator_pair.total_number_of_elements < number_of_elements)
        {
            insert(end_iterator, number_of_elements - node_pointer_allocator_pair.total_number_of_elements, value);
        }
        else // ie. node_pointer_allocator_pair.total_number_of_elements > number_of_elements
        {
            iterator current(end_node.previous);

            for (size_type number_to_remove = node_pointer_allocator_pair.total_number_of_elements - number_of_elements; number_to_remove != 0; --number_to_remove)
            {
                const node_pointer_type temp = current.node_pointer->previous;
                erase(current);
                current.node_pointer = temp;
            }
        }
    }



    // Range assign:
    template <class iterator_type>
    inline void assign(const typename plf_enable_if_c<!std::numeric_limits<iterator_type>::is_integer, iterator_type>::type first, const iterator_type last)
    {
        clear();
        insert(end_iterator, first, last);
        groups.trim_trailing_groups();
    }



    // Fill assign:
    inline void assign(const size_type number_of_elements, const element_type &value)
    {
        clear();
        reserve(number_of_elements); // Will return anyway if capacity already > number_of_elements
        insert(end_iterator, number_of_elements, value);
    }



    #ifdef PLF_LIST_INITIALIZER_LIST_SUPPORT
        // Initializer-list assign:
        inline void assign(const std::initializer_list<element_type> &element_list)
        {
            clear();
            reserve(element_list.size());
            insert(end_iterator, element_list);
        }
    #endif



    inline allocator_type get_allocator() const PLF_LIST_NOEXCEPT
    {
        return element_allocator_type();
    }



    void swap(list &source) PLF_LIST_NOEXCEPT_SWAP(allocator_type)
    {
        #ifdef PLF_LIST_MOVE_SEMANTICS_SUPPORT
            list temp(std::move(source));
            source = std::move(*this);
            *this = std::move(temp);
        #else
            groups.swap(source.groups);

            const node_pointer_type swap_end_node_previous = end_node.previous, swap_last_endpoint = last_endpoint;
            const iterator swap_begin_iterator = begin_iterator;
            const size_type swap_total_number_of_elements = node_pointer_allocator_pair.total_number_of_elements, swap_number_of_erased_nodes = node_allocator_pair.number_of_erased_nodes;

            last_endpoint = source.last_endpoint;
            end_node.next = begin_iterator.node_pointer = (source.begin_iterator.node_pointer != source.end_iterator.node_pointer) ? source.begin_iterator.node_pointer : end_iterator.node_pointer;
            end_node.previous = (source.begin_iterator.node_pointer != source.end_iterator.node_pointer) ? source.end_node.previous : end_iterator.node_pointer;
            end_node.previous->next = begin_iterator.node_pointer->previous = end_iterator.node_pointer;
            node_pointer_allocator_pair.total_number_of_elements = source.node_pointer_allocator_pair.total_number_of_elements;
            node_allocator_pair.number_of_erased_nodes = source.node_allocator_pair.number_of_erased_nodes;

            source.last_endpoint = swap_last_endpoint;
            source.end_node.next = source.begin_iterator.node_pointer = (swap_begin_iterator.node_pointer != end_iterator.node_pointer) ? swap_begin_iterator.node_pointer : source.end_iterator.node_pointer;
            source.end_node.previous = (swap_begin_iterator.node_pointer != end_iterator.node_pointer) ? swap_end_node_previous : source.end_iterator.node_pointer;
            source.end_node.previous->next = source.begin_iterator.node_pointer->previous = source.end_iterator.node_pointer;
            source.node_pointer_allocator_pair.total_number_of_elements = swap_total_number_of_elements;
            source.node_allocator_pair.number_of_erased_nodes = swap_number_of_erased_nodes;
        #endif
    }

};



template <class swap_element_type, class swap_element_allocator_type>
inline void swap(list<swap_element_type, swap_element_allocator_type> &a, list<swap_element_type, swap_element_allocator_type> &b) PLF_LIST_NOEXCEPT_SWAP(swap_element_allocator_type)
{
    a.swap(b);
}



} // plf namespace

#undef PLF_LIST_BLOCK_MAX
#undef PLF_LIST_BLOCK_MIN

#undef PLF_LIST_FORCE_INLINE

#undef PLF_LIST_INITIALIZER_LIST_SUPPORT
#undef PLF_LIST_TYPE_TRAITS_SUPPORT
#undef PLF_LIST_ALLOCATOR_TRAITS_SUPPORT
#undef PLF_LIST_VARIADICS_SUPPORT
#undef PLF_LIST_MOVE_SEMANTICS_SUPPORT
#undef PLF_LIST_NOEXCEPT
#undef PLF_LIST_NOEXCEPT_SWAP
#undef PLF_LIST_NOEXCEPT_MOVE_ASSIGNMENT
#undef PLF_LIST_CONSTEXPR

#undef PLF_LIST_CONSTRUCT
#undef PLF_LIST_DESTROY
#undef PLF_LIST_ALLOCATE
#undef PLF_LIST_ALLOCATE_INITIALIZATION
#undef PLF_LIST_DEALLOCATE


#endif // PLF_LIST_H
