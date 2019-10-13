// This software is a modified version of the original.
// The original license is as follows:

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

#pragma once
#ifndef COLONY_H
#define COLONY_H

// Compiler-specific defines used by colony:
#define COLONY_CONSTEXPR
#define COLONY_NOEXCEPT_MOVE_ASSIGNMENT(the_allocator) noexcept
#define COLONY_NOEXCEPT_SWAP(the_allocator) noexcept

// TODO: Switch to these when we move to C++17
// #define COLONY_CONSTEXPR constexpr
// #define COLONY_NOEXCEPT_MOVE_ASSIGNMENT(the_allocator) noexcept(std::allocator_traits<the_allocator>::is_always_equal::value)
// #define COLONY_NOEXCEPT_SWAP(the_allocator) noexcept(std::allocator_traits<the_allocator>::propagate_on_container_swap::value)

// Note: GCC creates faster code without forcing inline
#if defined(_MSC_VER)
#define COLONY_FORCE_INLINE __forceinline
#else
#define COLONY_FORCE_INLINE
#endif

// TODO: get rid of these defines
#define COLONY_CONSTRUCT(the_allocator, allocator_instance, location, ...)      std::allocator_traits<the_allocator>::construct(allocator_instance, location, __VA_ARGS__)
#define COLONY_DESTROY(the_allocator, allocator_instance, location)             std::allocator_traits<the_allocator>::destroy(allocator_instance, location)
#define COLONY_ALLOCATE(the_allocator, allocator_instance, size, hint)          std::allocator_traits<the_allocator>::allocate(allocator_instance, size, hint)
#define COLONY_ALLOCATE_INITIALIZATION(the_allocator, size, hint)               std::allocator_traits<the_allocator>::allocate(*this, size, hint)
#define COLONY_DEALLOCATE(the_allocator, allocator_instance, location, size)    std::allocator_traits<the_allocator>::deallocate(allocator_instance, location, size)

#include <algorithm> // std::sort and std::fill_n
#include <cassert> // assert
#include <cstddef> // offsetof, used in blank()
#include <cstring> // memset, memcpy
#include <initializer_list>
#include <iterator> // std::bidirectional_iterator_tag
#include <limits> // std::numeric_limits
#include <memory> // std::allocator
#include <type_traits> // std::is_trivially_destructible, etc
#include <utility> // std::move

namespace cata
{

template <class element_type, class element_allocator_type = std::allocator<element_type>, typename element_skipfield_type = unsigned short >
// Empty base class optimization - inheriting allocator functions
class colony : private element_allocator_type
// Note: unsigned short is equivalent to uint_least16_t ie. Using 16-bit unsigned integer in best-case scenario, greater-than-16-bit unsigned integer where platform doesn't support 16-bit types
{
    public:
        // Standard container typedefs:
        using value_type = element_type;
        using allocator_type = element_allocator_type;
        using skipfield_type = element_skipfield_type;

        using aligned_element_type = typename std::aligned_storage < sizeof( element_type ),
              ( alignof( element_type ) > ( sizeof( element_skipfield_type ) * 2 ) ) ? alignof( element_type ) :
              ( sizeof( element_skipfield_type ) * 2 ) >::type;

        using size_type = typename std::allocator_traits<element_allocator_type>::size_type;
        using difference_type = typename std::allocator_traits<element_allocator_type>::difference_type;
        using reference = element_type&;
        using const_reference = const element_type&;
        using pointer = typename std::allocator_traits<element_allocator_type>::pointer;
        using const_pointer = typename std::allocator_traits<element_allocator_type>::const_pointer;

        // Iterator declarations:
        template <bool is_const> class      colony_iterator;
        using iterator = colony_iterator<false>;
        using const_iterator = colony_iterator<true>;
        friend class colony_iterator<false>; // Using above typedef name here is illegal under C++03
        friend class colony_iterator<true>;

        template <bool r_is_const> class        colony_reverse_iterator;
        using reverse_iterator = colony_reverse_iterator<false>;
        using const_reverse_iterator = colony_reverse_iterator<true>;
        friend class colony_reverse_iterator<false>;
        friend class colony_reverse_iterator<true>;

    private:

        struct group; // forward declaration for typedefs below

        using aligned_element_allocator_type = typename
                                               std::allocator_traits<element_allocator_type>::template rebind_alloc<aligned_element_type>;
        using group_allocator_type = typename std::allocator_traits<element_allocator_type>::template
                                     rebind_alloc<group>;
        using skipfield_allocator_type = typename std::allocator_traits<element_allocator_type>::template
                                         rebind_alloc<skipfield_type>;

        // Using uchar as the generic allocator type, as sizeof is always guaranteed to be 1 byte regardless of the number of bits in a byte on given computer, whereas for example, uint8_t would fail on machines where there are more than 8 bits in a byte eg. Texas Instruments C54x DSPs.
        using uchar_allocator_type = typename std::allocator_traits<element_allocator_type>::template
                                     rebind_alloc<unsigned char>;
        // Different typedef to 'pointer' - this is a pointer to the over aligned element type, not the original element type
        using aligned_pointer_type = typename
                                     std::allocator_traits<aligned_element_allocator_type>::pointer;
        using group_pointer_type = typename std::allocator_traits<group_allocator_type>::pointer;
        using skipfield_pointer_type = typename std::allocator_traits<skipfield_allocator_type>::pointer;
        using uchar_pointer_type = typename std::allocator_traits<uchar_allocator_type>::pointer;

        using pointer_allocator_type = typename std::allocator_traits<element_allocator_type>::template
                                       rebind_alloc<pointer>;

        // Colony groups:

        // Empty base class optimization (EBCO) - inheriting allocator functions
        struct group : private uchar_allocator_type {
            aligned_pointer_type
            last_endpoint;              // The address that is one past the highest cell number that's been used so far in this group - does not change with erase command but may change with insert (if no previously-erased locations are available) - is necessary because an iterator cannot access the colony's end_iterator. Most-used variable in colony use (operator ++, --) so first in struct
            group_pointer_type
            next_group;                 // Next group in the intrusive list of all groups. NULL if no next group
            const aligned_pointer_type elements; // Element storage
            const skipfield_pointer_type
            skipfield;                  // Skipfield storage. The element and skipfield arrays are allocated contiguously, hence the skipfield pointer also functions as a 'one-past-end' pointer for the elements array. There will always be one additional skipfield node allocated compared to the number of elements. This is to ensure a faster ++ iterator operation (fewer checks are required when this is present). The extra node is unused and always zero, but checked, and not having it will result in out-of-bounds memory errors.
            group_pointer_type
            previous_group;             // previous group in the intrusive list of all groups. NULL if no preceding group
            skipfield_type
            free_list_head;             // The index of the last erased element in the group. The last erased element will, in turn, contain the number of the index of the next erased element, and so on. If this is == maximum skipfield_type value then free_list is empty ie. no erasures have occurred in the group (or if they have, the erased locations have then been reused via insert()).
            const skipfield_type
            capacity;                   // The element capacity of this particular group
            skipfield_type
            number_of_elements;         // indicates total number of active elements in group - changes with insert and erase commands - used to check for empty group in erase function, as an indication to remove the group
            group_pointer_type
            erasures_list_next_group;   // The next group in the intrusive singly-linked list of groups with erasures ie. with active erased-element free lists
            size_type
            group_number;               // Used for comparison (> < >= <=) iterator operators (used by distance function and user)

            group( const skipfield_type elements_per_group, group_pointer_type const previous = nullptr ):
                last_endpoint( reinterpret_cast<aligned_pointer_type>( COLONY_ALLOCATE_INITIALIZATION(
                                   uchar_allocator_type, ( ( elements_per_group * ( sizeof( aligned_element_type ) ) ) + ( (
                                               elements_per_group + 1u ) * sizeof( skipfield_type ) ) ),
                                   ( previous == nullptr ) ? nullptr :
                                   previous->elements ) ) ), /* allocating to here purely because it is first in the struct sequence - actual pointer is elements, last_endpoint is only initialised to element's base value initially, then incremented by one below */
                next_group( nullptr ),
                elements( last_endpoint++ ),
                skipfield( reinterpret_cast<skipfield_pointer_type>( elements + elements_per_group ) ),
                previous_group( previous ),
                free_list_head( std::numeric_limits<skipfield_type>::max() ),
                capacity( elements_per_group ),
                number_of_elements( 1 ),
                erasures_list_next_group( nullptr ),
                group_number( ( previous == nullptr ) ? 0 : previous->group_number + 1u ) {
                // Static casts to unsigned int from short not necessary as C++ automatically promotes lesser types for arithmetic purposes.
                std::memset( &*skipfield, 0, sizeof( skipfield_type ) * ( elements_per_group +
                             1u ) ); // &* to avoid problems with non-trivial pointers
            }

            ~group() noexcept {
                // Null check not necessary (for copied group as above) as delete will also perform a null check.
                COLONY_DEALLOCATE( uchar_allocator_type, ( *this ),
                                   reinterpret_cast<uchar_pointer_type>( elements ),
                                   ( capacity * sizeof( aligned_element_type ) ) + ( ( capacity + 1u ) * sizeof( skipfield_type ) ) );
            }
        };

        // Implement const/non-const iterator switching pattern:
        template <bool flag, class is_true, class is_false> struct choose;

        template <class is_true, class is_false> struct choose<true, is_true, is_false> {
            using type = is_true;
        };

        template <class is_true, class is_false> struct choose<false, is_true, is_false> {
            using type = is_false;
        };

    public:

        // Iterators:
        template <bool is_const> class colony_iterator
        {
            private:
                group_pointer_type      group_pointer;
                aligned_pointer_type    element_pointer;
                skipfield_pointer_type  skipfield_pointer;

            public:
                using iterator_category = std::bidirectional_iterator_tag;
                using value_type = typename colony::value_type;
                using difference_type = typename colony::difference_type;
                using pointer = typename
                                choose<is_const, typename colony::const_pointer, typename colony::pointer>::type;
                using reference = typename
                                  choose<is_const, typename colony::const_reference, typename colony::reference>::type;

                friend class colony;
                friend class colony_reverse_iterator<false>;
                friend class colony_reverse_iterator<true>;

                inline colony_iterator &operator=( const colony_iterator &source ) noexcept {
                    group_pointer = source.group_pointer;
                    element_pointer = source.element_pointer;
                    skipfield_pointer = source.skipfield_pointer;
                    return *this;
                }

                inline colony_iterator &operator=( const colony_iterator < !is_const > &source ) noexcept {
                    group_pointer = source.group_pointer;
                    element_pointer = source.element_pointer;
                    skipfield_pointer = source.skipfield_pointer;
                    return *this;
                }

                // Move assignment - only really necessary if the allocator uses non-standard ie. smart pointers
                inline colony_iterator &operator=( colony_iterator &&source )
                noexcept { // Move is a copy in this scenario
                    assert( &source != this );
                    group_pointer = std::move( source.group_pointer );
                    element_pointer = std::move( source.element_pointer );
                    skipfield_pointer = std::move( source.skipfield_pointer );
                    return *this;
                }

                inline colony_iterator &operator=( colony_iterator < !is_const > &&source ) noexcept {
                    assert( &source != this );
                    group_pointer = std::move( source.group_pointer );
                    element_pointer = std::move( source.element_pointer );
                    skipfield_pointer = std::move( source.skipfield_pointer );
                    return *this;
                }

                inline COLONY_FORCE_INLINE bool operator==( const colony_iterator &rh ) const noexcept {
                    return ( element_pointer == rh.element_pointer );
                }

                inline COLONY_FORCE_INLINE bool operator==( const colony_iterator < !is_const > &rh ) const
                noexcept {
                    return ( element_pointer == rh.element_pointer );
                }

                inline COLONY_FORCE_INLINE bool operator!=( const colony_iterator &rh ) const noexcept {
                    return ( element_pointer != rh.element_pointer );
                }

                inline COLONY_FORCE_INLINE bool operator!=( const colony_iterator < !is_const > &rh ) const
                noexcept {
                    return ( element_pointer != rh.element_pointer );
                }

                // may cause exception with uninitialized iterator
                inline COLONY_FORCE_INLINE reference operator*() const {
                    return *( reinterpret_cast<pointer>( element_pointer ) );
                }

                inline COLONY_FORCE_INLINE pointer operator->() const noexcept {
                    return reinterpret_cast<pointer>( element_pointer );
                }

                colony_iterator &operator++() {
                    // covers uninitialised colony_iterator
                    assert( group_pointer != nullptr );
                    // Assert that iterator is not already at end()
                    assert( !( element_pointer == group_pointer->last_endpoint &&
                               group_pointer->next_group != nullptr ) );

                    skipfield_type skip = *( ++skipfield_pointer );

                    // ie. beyond end of available data
                    if( ( element_pointer += skip + 1 ) == group_pointer->last_endpoint &&
                        group_pointer->next_group != nullptr ) {
                        group_pointer = group_pointer->next_group;
                        skip = *( group_pointer->skipfield );
                        element_pointer = group_pointer->elements + skip;
                        skipfield_pointer = group_pointer->skipfield;
                    }

                    skipfield_pointer += skip;
                    return *this;
                }

                inline colony_iterator operator++( int ) {
                    const colony_iterator copy( *this );
                    ++*this;
                    return copy;
                }

            private:
                inline COLONY_FORCE_INLINE void check_for_end_of_group_and_progress() { // used by erase
                    if( element_pointer == group_pointer->last_endpoint && group_pointer->next_group != nullptr ) {
                        group_pointer = group_pointer->next_group;
                        skipfield_pointer = group_pointer->skipfield;
                        element_pointer = group_pointer->elements + *skipfield_pointer;
                        skipfield_pointer += *skipfield_pointer;
                    }
                }

            public:

                colony_iterator &operator--() {
                    assert( group_pointer != nullptr );
                    assert( !( element_pointer == group_pointer->elements &&
                               group_pointer->previous_group ==
                               nullptr ) ); // Assert that we are not already at begin() - this is not required to be tested in the code below as we don't need a special condition to progress to begin(), like we do with end() in operator ++

                    if( element_pointer != group_pointer->elements ) { // ie. not already at beginning of group
                        const skipfield_type skip = *( --skipfield_pointer );
                        skipfield_pointer -= skip;

                        if( ( element_pointer -= skip + 1 ) != group_pointer->elements -
                            1 ) { // ie. iterator was not already at beginning of colony (with some previous consecutive deleted elements), and skipfield does not takes us into the previous group)
                            return *this;
                        }
                    }

                    group_pointer = group_pointer->previous_group;
                    skipfield_pointer = group_pointer->skipfield + group_pointer->capacity - 1;
                    element_pointer = ( reinterpret_cast<colony::aligned_pointer_type>( group_pointer->skipfield ) - 1 )
                                      - *skipfield_pointer;
                    skipfield_pointer -= *skipfield_pointer;

                    return *this;
                }

                inline colony_iterator operator--( int ) {
                    const colony_iterator copy( *this );
                    --*this;
                    return copy;
                }

                inline bool operator>( const colony_iterator &rh ) const noexcept {
                    return ( ( group_pointer == rh.group_pointer ) & ( element_pointer > rh.element_pointer ) ) ||
                           ( group_pointer != rh.group_pointer &&
                             group_pointer->group_number > rh.group_pointer->group_number );
                }

                inline bool operator<( const colony_iterator &rh ) const noexcept {
                    return rh > *this;
                }

                inline bool operator>=( const colony_iterator &rh ) const noexcept {
                    return !( rh > *this );
                }

                inline bool operator<=( const colony_iterator &rh ) const noexcept {
                    return !( *this > rh );
                }

                inline bool operator>( const colony_iterator < !is_const > &rh ) const noexcept {
                    return ( ( group_pointer == rh.group_pointer ) & ( element_pointer > rh.element_pointer ) ) ||
                           ( group_pointer != rh.group_pointer &&
                             group_pointer->group_number > rh.group_pointer->group_number );
                }

                inline bool operator<( const colony_iterator < !is_const > &rh ) const noexcept {
                    return rh > *this;
                }

                inline bool operator>=( const colony_iterator < !is_const > &rh ) const noexcept {
                    return !( rh > *this );
                }

                inline bool operator<=( const colony_iterator < !is_const > &rh ) const noexcept {
                    return !( *this > rh );
                }

                colony_iterator() noexcept: group_pointer( nullptr ), element_pointer( nullptr ),
                    skipfield_pointer( nullptr )  {}

            private:
                // Used by cend(), erase() etc:
                colony_iterator( const group_pointer_type group_p, const aligned_pointer_type element_p,
                                 const skipfield_pointer_type skipfield_p ) noexcept: group_pointer( group_p ),
                    element_pointer( element_p ), skipfield_pointer( skipfield_p ) {}

            public:

                inline colony_iterator( const colony_iterator &source ) noexcept:
                    group_pointer( source.group_pointer ),
                    element_pointer( source.element_pointer ),
                    skipfield_pointer( source.skipfield_pointer ) {}

                inline colony_iterator( const colony_iterator < !is_const > &source ) noexcept:
                    group_pointer( source.group_pointer ),
                    element_pointer( source.element_pointer ),
                    skipfield_pointer( source.skipfield_pointer ) {}

                // move constructor
                inline colony_iterator( colony_iterator &&source ) noexcept:
                    group_pointer( std::move( source.group_pointer ) ),
                    element_pointer( std::move( source.element_pointer ) ),
                    skipfield_pointer( std::move( source.skipfield_pointer ) ) {}

                inline colony_iterator( colony_iterator < !is_const > &&source ) noexcept:
                    group_pointer( std::move( source.group_pointer ) ),
                    element_pointer( std::move( source.element_pointer ) ),
                    skipfield_pointer( std::move( source.skipfield_pointer ) ) {}
        }; // colony_iterator

        // Reverse iterators:
        template <bool r_is_const> class colony_reverse_iterator
        {
            private:
                iterator it;

            public:
                using iterator_category = std::bidirectional_iterator_tag;
                using value_type = typename colony::value_type;
                using difference_type = typename colony::difference_type;
                using pointer = typename
                                choose<r_is_const, typename colony::const_pointer, typename colony::pointer>::type;
                using reference = typename
                                  choose<r_is_const, typename colony::const_reference, typename colony::reference>::type;

                friend class colony;

                inline colony_reverse_iterator &operator=( const colony_reverse_iterator &source ) noexcept {
                    it = source.it;
                    return *this;
                }

                // move assignment
                inline colony_reverse_iterator &operator=( colony_reverse_iterator &&source ) noexcept {
                    it = std::move( source.it );
                    return *this;
                }

                inline COLONY_FORCE_INLINE bool operator==( const colony_reverse_iterator &rh ) const noexcept {
                    return ( it == rh.it );
                }

                inline COLONY_FORCE_INLINE bool operator!=( const colony_reverse_iterator &rh ) const noexcept {
                    return ( it != rh.it );
                }

                inline COLONY_FORCE_INLINE reference operator*() const noexcept {
                    return *( reinterpret_cast<pointer>( it.element_pointer ) );
                }

                inline COLONY_FORCE_INLINE pointer *operator->() const noexcept {
                    return reinterpret_cast<pointer>( it.element_pointer );
                }

                // In this case we have to redefine the algorithm, rather than using the internal iterator's -- operator, in order for the reverse_iterator to be allowed to reach rend() ie. begin_iterator - 1
                colony_reverse_iterator &operator++() {
                    colony::group_pointer_type &group_pointer = it.group_pointer;
                    colony::aligned_pointer_type &element_pointer = it.element_pointer;
                    colony::skipfield_pointer_type &skipfield_pointer = it.skipfield_pointer;

                    assert( group_pointer != nullptr );
                    assert( !( element_pointer == group_pointer->elements - 1 &&
                               group_pointer->previous_group == nullptr ) ); // Assert that we are not already at rend()

                    if( element_pointer != group_pointer->elements ) { // ie. not already at beginning of group
                        element_pointer -= *( --skipfield_pointer ) + 1;
                        skipfield_pointer -= *skipfield_pointer;

                        if( !( element_pointer == group_pointer->elements - 1 &&
                               group_pointer->previous_group == nullptr ) ) { // ie. iterator is not == rend()
                            return *this;
                        }
                    }

                    if( group_pointer->previous_group != nullptr ) { // ie. not first group in colony
                        group_pointer = group_pointer->previous_group;
                        skipfield_pointer = group_pointer->skipfield + group_pointer->capacity - 1;
                        element_pointer = ( reinterpret_cast<colony::aligned_pointer_type>( group_pointer->skipfield ) - 1 )
                                          - *skipfield_pointer;
                        skipfield_pointer -= *skipfield_pointer;
                    } else { // necessary so that reverse_iterator can end up == rend(), if we were already at first element in colony
                        --element_pointer;
                        --skipfield_pointer;
                    }

                    return *this;
                }

                inline colony_reverse_iterator operator++( int ) {
                    const colony_reverse_iterator copy( *this );
                    ++*this;
                    return copy;
                }

                inline COLONY_FORCE_INLINE colony_reverse_iterator &operator--() {
                    // ie. Check that we are not already at rbegin()
                    assert( !( it.element_pointer == it.group_pointer->last_endpoint - 1 &&
                               it.group_pointer->next_group == nullptr ) );
                    ++it;
                    return *this;
                }

                inline colony_reverse_iterator operator--( int ) {
                    const colony_reverse_iterator copy( *this );
                    --*this;
                    return copy;
                }

                inline typename colony::iterator base() const {
                    return ++( typename colony::iterator( it ) );
                }

                inline bool operator>( const colony_reverse_iterator &rh ) const noexcept {
                    return ( rh.it > it );
                }

                inline bool operator<( const colony_reverse_iterator &rh ) const noexcept {
                    return ( it > rh.it );
                }

                inline bool operator>=( const colony_reverse_iterator &rh ) const noexcept {
                    return !( it > rh.it );
                }

                inline bool operator<=( const colony_reverse_iterator &rh ) const noexcept {
                    return !( rh.it > it );
                }

                inline COLONY_FORCE_INLINE bool operator==( const colony_reverse_iterator < !r_is_const > &rh )
                const noexcept {
                    return ( it == rh.it );
                }

                inline COLONY_FORCE_INLINE bool operator!=( const colony_reverse_iterator < !r_is_const > &rh )
                const noexcept {
                    return ( it != rh.it );
                }

                inline bool operator>( const colony_reverse_iterator < !r_is_const > &rh ) const noexcept {
                    return ( rh.it > it );
                }

                inline bool operator<( const colony_reverse_iterator < !r_is_const > &rh ) const noexcept {
                    return ( it > rh.it );
                }

                inline bool operator>=( const colony_reverse_iterator < !r_is_const > &rh ) const noexcept {
                    return !( it > rh.it );
                }

                inline bool operator<=( const colony_reverse_iterator < !r_is_const > &rh ) const noexcept {
                    return !( rh.it > it );
                }

                colony_reverse_iterator() noexcept = default;

                colony_reverse_iterator( const colony_reverse_iterator &source ) noexcept:
                    it( source.it ) {}

                colony_reverse_iterator( const typename colony::iterator &source ) noexcept:
                    it( source ) {}

            private:
                // Used by rend(), etc:
                colony_reverse_iterator( const group_pointer_type group_p, const aligned_pointer_type element_p,
                                         const skipfield_pointer_type skipfield_p ) noexcept:
                    it( group_p, element_p, skipfield_p ) {}

            public:

                // move constructors
                colony_reverse_iterator( colony_reverse_iterator &&source ) noexcept:
                    it( std::move( source.it ) ) {}

                colony_reverse_iterator( typename colony::iterator &&source ) noexcept:
                    it( std::move( source ) ) {}

        }; // colony_reverse_iterator

    private:

        // Used to prevent fill-insert/constructor calls being mistakenly resolved to range-insert/constructor calls
        template <bool condition, class T = void>
        struct enable_if_c {
            using type = T;
        };

        template <class T>
        struct enable_if_c<false, T> {
        };

        iterator end_iterator, begin_iterator;

        // Head of a singly-linked intrusive list of groups which have erased-element memory locations available for reuse
        group_pointer_type groups_with_erasures_list_head;
        size_type total_number_of_elements, total_capacity;

        // Packaging the element pointer allocator with a lesser-used member variable, for empty-base-class optimisation
        struct ebco_pair2 : pointer_allocator_type {
            skipfield_type min_elements_per_group;
            explicit ebco_pair2( const skipfield_type min_elements ) noexcept:
                min_elements_per_group( min_elements ) {}
        } pointer_allocator_pair;

        struct ebco_pair : group_allocator_type {
            skipfield_type max_elements_per_group;
            explicit ebco_pair( const skipfield_type max_elements ) noexcept:
                max_elements_per_group( max_elements ) {}
        } group_allocator_pair;

    public:

        /**
         * Default constructor:
         * default minimum group size is 8, default maximum group size is
         * std::numeric_limits<Skipfield_type>::max() (typically 65535). You cannot set the group
         * sizes from the constructor in this scenario, but you can call the change_group_sizes()
         * member function after construction has occurred.
         */
        colony() noexcept:
            element_allocator_type( element_allocator_type() ),
            groups_with_erasures_list_head( nullptr ),
            total_number_of_elements( 0 ),
            total_capacity( 0 ),
            pointer_allocator_pair( ( sizeof( aligned_element_type ) * 8 > ( sizeof( *this ) + sizeof(
                                          group ) ) * 2 ) ? 8 : ( ( ( sizeof( *this ) + sizeof( group ) ) * 2 ) / sizeof(
                                                  aligned_element_type ) ) ),
            group_allocator_pair( std::numeric_limits<skipfield_type>::max() ) {
            // skipfield type must be of unsigned integer type (uchar, ushort, uint etc)
            assert( std::numeric_limits<skipfield_type>::is_integer &
                    !std::numeric_limits<skipfield_type>::is_signed );
        }

        /**
         * Default constructor, but using a custom memory allocator eg. something other than
         * std::allocator.
         */
        explicit colony( const element_allocator_type &alloc ):
            element_allocator_type( alloc ),
            groups_with_erasures_list_head( NULL ),
            total_number_of_elements( 0 ),
            total_capacity( 0 ),
            pointer_allocator_pair( ( sizeof( aligned_element_type ) * 8 > ( sizeof( *this ) + sizeof(
                                          group ) ) * 2 ) ? 8 : ( ( ( sizeof( *this ) + sizeof( group ) ) * 2 ) / sizeof(
                                                  aligned_element_type ) ) ),
            group_allocator_pair( std::numeric_limits<skipfield_type>::max() ) {
            assert( std::numeric_limits<skipfield_type>::is_integer &
                    !std::numeric_limits<skipfield_type>::is_signed );
        }

        /**
         * Copy constructor:
         * Copy all contents from source colony, removes any empty (erased) element locations in the
         * process. Size of groups created is either the total size of the source colony, or the
         * maximum group size of the source colony, whichever is the smaller.
         */
        colony( const colony &source ):
            element_allocator_type( source ),
            groups_with_erasures_list_head( nullptr ),
            total_number_of_elements( 0 ),
            total_capacity( 0 ),
            // Make the first colony group capacity the greater of min_elements_per_group or total_number_of_elements, so long as total_number_of_elements isn't larger than max_elements_per_group
            pointer_allocator_pair( static_cast<skipfield_type>( (
                                        source.pointer_allocator_pair.min_elements_per_group > source.total_number_of_elements ) ?
                                    source.pointer_allocator_pair.min_elements_per_group : ( ( source.total_number_of_elements >
                                            source.group_allocator_pair.max_elements_per_group ) ?
                                            source.group_allocator_pair.max_elements_per_group :
                                            source.total_number_of_elements ) ) ),
            group_allocator_pair( source.group_allocator_pair.max_elements_per_group ) {
            insert( source.begin_iterator, source.end_iterator );
            // reset to correct value for future clear() or erasures
            pointer_allocator_pair.min_elements_per_group =
                source.pointer_allocator_pair.min_elements_per_group;
        }

        // Copy constructor (allocator-extended):
        colony( const colony &source, const allocator_type &alloc ):
            element_allocator_type( alloc ),
            groups_with_erasures_list_head( nullptr ),
            total_number_of_elements( 0 ),
            total_capacity( 0 ),
            pointer_allocator_pair( static_cast<skipfield_type>( (
                                        source.pointer_allocator_pair.min_elements_per_group > source.total_number_of_elements ) ?
                                    source.pointer_allocator_pair.min_elements_per_group : ( ( source.total_number_of_elements >
                                            source.group_allocator_pair.max_elements_per_group ) ?
                                            source.group_allocator_pair.max_elements_per_group : source.total_number_of_elements ) ) ),
            group_allocator_pair( source.group_allocator_pair.max_elements_per_group ) {
            insert( source.begin_iterator, source.end_iterator );
            pointer_allocator_pair.min_elements_per_group =
                source.pointer_allocator_pair.min_elements_per_group;
        }

    private:

        inline void blank() noexcept {
            // if all pointer types are trivial, we can just nuke it from orbit with memset (NULL is always 0 in C++):
            if COLONY_CONSTEXPR( std::is_trivial<group_pointer_type>::value &&
                                 std::is_trivial<aligned_pointer_type>::value &&
                                 std::is_trivial<skipfield_pointer_type>::value ) {
                std::memset( static_cast<void *>( this ), 0, offsetof( colony, pointer_allocator_pair ) );
            } else {
                end_iterator.group_pointer = nullptr;
                end_iterator.element_pointer = nullptr;
                end_iterator.skipfield_pointer = nullptr;
                begin_iterator.group_pointer = nullptr;
                begin_iterator.element_pointer = nullptr;
                begin_iterator.skipfield_pointer = nullptr;
                groups_with_erasures_list_head = nullptr;
                total_number_of_elements = 0;
                total_capacity = 0;
            }
        }

    public:

        /**
         * Move constructor:
         * Move all contents from source colony, does not remove any erased element locations or
         * alter any of the source group sizes. Source colony is now empty and can be safely
         * destructed or otherwise used.
         */
        colony( colony &&source ) noexcept:
            element_allocator_type( source ),
            end_iterator( std::move( source.end_iterator ) ),
            begin_iterator( std::move( source.begin_iterator ) ),
            groups_with_erasures_list_head( std::move( source.groups_with_erasures_list_head ) ),
            total_number_of_elements( source.total_number_of_elements ),
            total_capacity( source.total_capacity ),
            pointer_allocator_pair( source.pointer_allocator_pair.min_elements_per_group ),
            group_allocator_pair( source.group_allocator_pair.max_elements_per_group ) {
            source.blank();
        }

        // Move constructor (allocator-extended):
        colony( colony &&source, const allocator_type &alloc ):
            element_allocator_type( alloc ),
            end_iterator( std::move( source.end_iterator ) ),
            begin_iterator( std::move( source.begin_iterator ) ),
            groups_with_erasures_list_head( std::move( source.groups_with_erasures_list_head ) ),
            total_number_of_elements( source.total_number_of_elements ),
            total_capacity( source.total_capacity ),
            pointer_allocator_pair( source.pointer_allocator_pair.min_elements_per_group ),
            group_allocator_pair( source.group_allocator_pair.max_elements_per_group ) {
            source.blank();
        }

        /**
         * Fill constructor with value_type unspecified, so the value_type's default constructor is
         * used. n specifies the number of elements to create upon construction. If n is larger than
         * min_group_size, the size of the groups created will either be n and max_group_size,
         * depending on which is smaller. min_group_size (ie. the smallest possible number of
         * elements which can be stored in a colony group) can be defined, as can the max_group_size.
         * Setting the group sizes can be a performance advantage if you know in advance roughly how
         * many objects are likely to be stored in your colony long-term - or at least the rough
         * scale of storage. If that case, using this can stop many small initial groups being
         * allocated (reserve() will achieve a similar result, but structurally at the moment is
         * limited to allocating one group).
         */
        colony( const size_type fill_number, const element_type &element,
                const skipfield_type min_allocation_amount = 0,
                const skipfield_type max_allocation_amount = std::numeric_limits<skipfield_type>::max(),
                const element_allocator_type &alloc = element_allocator_type() ):
            element_allocator_type( alloc ),
            groups_with_erasures_list_head( nullptr ),
            total_number_of_elements( 0 ),
            total_capacity( 0 ),
            pointer_allocator_pair( ( min_allocation_amount != 0 ) ? min_allocation_amount :
                                    ( fill_number > max_allocation_amount ) ? max_allocation_amount :
                                    ( fill_number > 8 ) ? static_cast<skipfield_type>( fill_number ) : 8 ),
            group_allocator_pair( max_allocation_amount ) {
            assert( std::numeric_limits<skipfield_type>::is_integer &
                    !std::numeric_limits<skipfield_type>::is_signed );
            assert( ( pointer_allocator_pair.min_elements_per_group > 2 ) &
                    ( pointer_allocator_pair.min_elements_per_group <= group_allocator_pair.max_elements_per_group ) );

            insert( fill_number, element );
        }

        // Range constructor:
        template<typename iterator_type>
        colony( const typename enable_if_c < !std::numeric_limits<iterator_type>::is_integer,
                iterator_type >::type &first, const iterator_type &last,
                const skipfield_type min_allocation_amount = 8,
                const skipfield_type max_allocation_amount = std::numeric_limits<skipfield_type>::max(),
                const element_allocator_type &alloc = element_allocator_type() ):
            element_allocator_type( alloc ),
            groups_with_erasures_list_head( nullptr ),
            total_number_of_elements( 0 ),
            total_capacity( 0 ),
            pointer_allocator_pair( min_allocation_amount ),
            group_allocator_pair( max_allocation_amount ) {
            assert( std::numeric_limits<skipfield_type>::is_integer &
                    !std::numeric_limits<skipfield_type>::is_signed );
            assert( ( pointer_allocator_pair.min_elements_per_group > 2 ) &
                    ( pointer_allocator_pair.min_elements_per_group <= group_allocator_pair.max_elements_per_group ) );

            insert<iterator_type>( first, last );
        }

        // Initializer-list constructor:
        colony( const std::initializer_list<element_type> &element_list,
                const skipfield_type min_allocation_amount = 0,
                const skipfield_type max_allocation_amount = std::numeric_limits<skipfield_type>::max(),
                const element_allocator_type &alloc = element_allocator_type() ):
            element_allocator_type( alloc ),
            groups_with_erasures_list_head( nullptr ),
            total_number_of_elements( 0 ),
            total_capacity( 0 ),
            pointer_allocator_pair( ( min_allocation_amount != 0 ) ? min_allocation_amount :
                                    ( element_list.size() > max_allocation_amount ) ? max_allocation_amount :
                                    ( element_list.size() > 8 ) ? static_cast<skipfield_type>( element_list.size() ) : 8 ),
            group_allocator_pair( max_allocation_amount ) {
            assert( std::numeric_limits<skipfield_type>::is_integer &
                    !std::numeric_limits<skipfield_type>::is_signed );
            assert( ( pointer_allocator_pair.min_elements_per_group > 2 ) &
                    ( pointer_allocator_pair.min_elements_per_group <= group_allocator_pair.max_elements_per_group ) );

            insert( element_list );
        }

        inline COLONY_FORCE_INLINE iterator begin() noexcept {
            return begin_iterator;
        }

        inline COLONY_FORCE_INLINE const iterator &begin() const
        noexcept { // To allow for functions which only take const colony & as a source eg. copy constructor
            return begin_iterator;
        }

        inline COLONY_FORCE_INLINE iterator end() noexcept {
            return end_iterator;
        }

        inline COLONY_FORCE_INLINE const iterator &end() const noexcept {
            return end_iterator;
        }

        inline const_iterator cbegin() const noexcept {
            return const_iterator( begin_iterator.group_pointer, begin_iterator.element_pointer,
                                   begin_iterator.skipfield_pointer );
        }

        inline const_iterator cend() const noexcept {
            return const_iterator( end_iterator.group_pointer, end_iterator.element_pointer,
                                   end_iterator.skipfield_pointer );
        }

        inline reverse_iterator rbegin()
        const { // May throw exception if colony is empty so end_iterator is uninitialized
            return ++reverse_iterator( end_iterator );
        }

        inline reverse_iterator rend() const noexcept {
            return reverse_iterator( begin_iterator.group_pointer, begin_iterator.element_pointer - 1,
                                     begin_iterator.skipfield_pointer - 1 );
        }

        inline const_reverse_iterator crbegin() const {
            return ++const_reverse_iterator( end_iterator );
        }

        inline const_reverse_iterator crend() const noexcept {
            return const_reverse_iterator( begin_iterator.group_pointer, begin_iterator.element_pointer - 1,
                                           begin_iterator.skipfield_pointer - 1 );
        }

        ~colony() noexcept {
            destroy_all_data();
        }

    private:

        void destroy_all_data() noexcept {
            // Amusingly enough, these changes from && to logical & actually do make a significant difference in debug mode
            if( ( total_number_of_elements != 0 ) & !std::is_trivially_destructible<element_type>::value ) {
                total_number_of_elements = 0; // to avoid double-destruction

                while( true ) {
                    const aligned_pointer_type end_pointer = begin_iterator.group_pointer->last_endpoint;

                    do {
                        COLONY_DESTROY( element_allocator_type, ( *this ),
                                        reinterpret_cast<pointer>( begin_iterator.element_pointer ) );
                        ++begin_iterator.skipfield_pointer;
                        begin_iterator.element_pointer += *begin_iterator.skipfield_pointer + 1;
                        begin_iterator.skipfield_pointer += *begin_iterator.skipfield_pointer;
                    } while( begin_iterator.element_pointer != end_pointer ); // ie. beyond end of available data

                    const group_pointer_type next_group = begin_iterator.group_pointer->next_group;
                    COLONY_DESTROY( group_allocator_type, group_allocator_pair, begin_iterator.group_pointer );
                    COLONY_DEALLOCATE( group_allocator_type, group_allocator_pair, begin_iterator.group_pointer, 1 );
                    begin_iterator.group_pointer =
                        next_group; // required to be before if statement in order for first_group to be NULL and avoid potential double-destruction in future

                    if( next_group == nullptr ) {
                        return;
                    }

                    begin_iterator.element_pointer = next_group->elements + *( next_group->skipfield );
                    begin_iterator.skipfield_pointer = next_group->skipfield + *( next_group->skipfield );
                }
            } else { // Avoid iteration for both empty groups and trivially-destructible types eg. POD, structs, classes with empty destructors
                // Technically under a type-traits-supporting compiler total_number_of_elements could be non-zero at this point, but since begin_iterator.group_pointer would already be NULL in the case of double-destruction, it's unnecessary to zero total_number_of_elements
                while( begin_iterator.group_pointer != nullptr ) {
                    const group_pointer_type next_group = begin_iterator.group_pointer->next_group;
                    COLONY_DESTROY( group_allocator_type, group_allocator_pair, begin_iterator.group_pointer );
                    COLONY_DEALLOCATE( group_allocator_type, group_allocator_pair, begin_iterator.group_pointer, 1 );
                    begin_iterator.group_pointer = next_group;
                }
            }
        }

        void initialize( const skipfield_type first_group_size ) {
            begin_iterator.group_pointer = COLONY_ALLOCATE( group_allocator_type, group_allocator_pair, 1,
                                           nullptr );

            try {
                COLONY_CONSTRUCT( group_allocator_type, group_allocator_pair, begin_iterator.group_pointer,
                                  first_group_size );
            } catch( ... ) {
                COLONY_DEALLOCATE( group_allocator_type, group_allocator_pair, begin_iterator.group_pointer, 1 );
                begin_iterator.group_pointer = nullptr;
                throw;
            }

            end_iterator.group_pointer = begin_iterator.group_pointer;
            end_iterator.element_pointer = begin_iterator.element_pointer =
                                               begin_iterator.group_pointer->elements;
            end_iterator.skipfield_pointer = begin_iterator.skipfield_pointer =
                                                 begin_iterator.group_pointer->skipfield;
            total_capacity = first_group_size;
        }

    public:

        /**
         * Inserts the element supplied to the colony, using the object's copy-constructor. Will
         * insert the element into a previously erased element slot if one exists, otherwise will
         * insert to back of colony. Returns iterator to location of inserted element.
         */
        iterator insert( const element_type &element ) {
            if( end_iterator.element_pointer != nullptr ) {
                switch( ( ( groups_with_erasures_list_head != nullptr ) << 1 ) | ( end_iterator.element_pointer ==
                        reinterpret_cast<aligned_pointer_type>( end_iterator.group_pointer->skipfield ) ) ) {
                    case 0: { // ie. there are no erased elements and end_iterator is not at end of current final group
                        // Make copy for return before modifying end_iterator
                        const iterator return_iterator = end_iterator;

                        if COLONY_CONSTEXPR( std::is_nothrow_copy_constructible<element_type>::value ) {
                            // For no good reason this compiles to faster code under GCC:
                            COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                              reinterpret_cast<pointer>( end_iterator.element_pointer++ ), element );
                            end_iterator.group_pointer->last_endpoint = end_iterator.element_pointer;
                        } else {
                            COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                              reinterpret_cast<pointer>( end_iterator.element_pointer ), element );
                            // Shift the addition to the second operation, avoiding problems if an exception is thrown during construction
                            end_iterator.group_pointer->last_endpoint = ++end_iterator.element_pointer;
                        }

                        ++( end_iterator.group_pointer->number_of_elements );
                        ++end_iterator.skipfield_pointer;
                        ++total_number_of_elements;

                        return return_iterator; // return value before incrementation
                    }
                    case 1: { // ie. there are no erased elements and end_iterator is at end of current final group - ie. colony is full - create new group
                        end_iterator.group_pointer->next_group = COLONY_ALLOCATE( group_allocator_type,
                                group_allocator_pair, 1, end_iterator.group_pointer );
                        group &next_group = *( end_iterator.group_pointer->next_group );
                        const skipfield_type new_group_size = ( total_number_of_elements < static_cast<size_type>
                                                                ( group_allocator_pair.max_elements_per_group ) ) ? static_cast<skipfield_type>
                                                              ( total_number_of_elements ) : group_allocator_pair.max_elements_per_group;

                        try {
                            COLONY_CONSTRUCT( group_allocator_type, group_allocator_pair, &next_group, new_group_size,
                                              end_iterator.group_pointer );
                        } catch( ... ) {
                            COLONY_DEALLOCATE( group_allocator_type, group_allocator_pair, &next_group, 1 );
                            end_iterator.group_pointer->next_group = nullptr;
                            throw;
                        }

                        if COLONY_CONSTEXPR( std::is_nothrow_copy_constructible<element_type>::value ) {
                            COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                              reinterpret_cast<pointer>( next_group.elements ), element );
                        } else {
                            try {
                                COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                                  reinterpret_cast<pointer>( next_group.elements ), element );
                            } catch( ... ) {
                                COLONY_DESTROY( group_allocator_type, group_allocator_pair, &next_group );
                                COLONY_DEALLOCATE( group_allocator_type, group_allocator_pair, &next_group, 1 );
                                end_iterator.group_pointer->next_group = nullptr;
                                throw;
                            }
                        }

                        end_iterator.group_pointer = &next_group;
                        end_iterator.element_pointer = next_group.last_endpoint;
                        end_iterator.skipfield_pointer = next_group.skipfield + 1;
                        ++total_number_of_elements;
                        total_capacity += new_group_size;

                        // returns value before incrementation
                        return iterator( end_iterator.group_pointer, next_group.elements, next_group.skipfield );
                    }
                    default: { // ie. there are erased elements, reuse previous-erased element locations
                        iterator new_location;
                        new_location.group_pointer = groups_with_erasures_list_head;
                        new_location.element_pointer = groups_with_erasures_list_head->elements +
                                                       groups_with_erasures_list_head->free_list_head;
                        new_location.skipfield_pointer = groups_with_erasures_list_head->skipfield +
                                                         groups_with_erasures_list_head->free_list_head;

                        // always at start of skipblock, update skipblock:
                        const skipfield_type prev_free_list_index = *( reinterpret_cast<skipfield_pointer_type>
                                ( new_location.element_pointer ) );

                        COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                          reinterpret_cast<pointer>( new_location.element_pointer ), element );

                        // Update skipblock:
                        const skipfield_type new_value = *( new_location.skipfield_pointer ) - 1;

                        // ie. skipfield was not 1, ie. a single-node skipblock, with no additional nodes to update
                        if( new_value != 0 ) {
                            // set (new) start and (original) end of skipblock to new value:
                            *( new_location.skipfield_pointer + new_value ) = *( new_location.skipfield_pointer + 1 ) =
                                        new_value;

                            // transfer free list node to new start node:
                            ++( groups_with_erasures_list_head->free_list_head );

                            // ie. not the tail free list node
                            if( prev_free_list_index != std::numeric_limits<skipfield_type>::max() ) {
                                *( reinterpret_cast<skipfield_pointer_type>( new_location.group_pointer->elements +
                                        prev_free_list_index ) + 1 ) = groups_with_erasures_list_head->free_list_head;
                            }

                            *( reinterpret_cast<skipfield_pointer_type>( new_location.element_pointer + 1 ) ) =
                                prev_free_list_index;
                            *( reinterpret_cast<skipfield_pointer_type>( new_location.element_pointer + 1 ) + 1 ) =
                                std::numeric_limits<skipfield_type>::max();
                        } else {
                            groups_with_erasures_list_head->free_list_head = prev_free_list_index;

                            // ie. not the last free list node
                            if( prev_free_list_index != std::numeric_limits<skipfield_type>::max() ) {
                                *( reinterpret_cast<skipfield_pointer_type>( new_location.group_pointer->elements +
                                        prev_free_list_index ) + 1 ) = std::numeric_limits<skipfield_type>::max();
                            } else {
                                groups_with_erasures_list_head = groups_with_erasures_list_head->erasures_list_next_group;
                            }
                        }

                        *( new_location.skipfield_pointer ) = 0;
                        ++( new_location.group_pointer->number_of_elements );

                        if( new_location.group_pointer == begin_iterator.group_pointer &&
                            new_location.element_pointer < begin_iterator.element_pointer ) {
                            // ie. begin_iterator was moved forwards as the result of an erasure at some point, this erased element is before the current begin, hence, set current begin iterator to this element
                            begin_iterator = new_location;
                        }

                        ++total_number_of_elements;
                        return new_location;
                    }
                }
            } else { // ie. newly-constructed colony, no insertions yet and no groups
                initialize( pointer_allocator_pair.min_elements_per_group );

                if COLONY_CONSTEXPR( std::is_nothrow_copy_constructible<element_type>::value ) {
                    COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                      reinterpret_cast<pointer>( end_iterator.element_pointer++ ), element );
                } else {
                    try {
                        COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                          reinterpret_cast<pointer>( end_iterator.element_pointer++ ), element );
                    } catch( ... ) {
                        clear();
                        throw;
                    }
                }

                ++end_iterator.skipfield_pointer;
                total_number_of_elements = 1;
                return begin_iterator;
            }
        }

        /**
         * Moves the element supplied to the colony, using the object's move-constructor. Will
         * insert the element in a previously erased element slot if one exists, otherwise will
         * insert to back of colony. Returns iterator to location of inserted element.
         *
         * The move-insert function is near-identical to the regular insert function, with the
         * exception of the element construction method and is_nothrow tests.
         */
        iterator insert( element_type &&element ) {
            if( end_iterator.element_pointer != nullptr ) {
                switch( ( ( groups_with_erasures_list_head != nullptr ) << 1 ) | ( end_iterator.element_pointer ==
                        reinterpret_cast<aligned_pointer_type>( end_iterator.group_pointer->skipfield ) ) ) {
                    case 0: {
                        const iterator return_iterator = end_iterator;

                        if COLONY_CONSTEXPR( std::is_nothrow_move_constructible<element_type>::value ) {
                            COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                              reinterpret_cast<pointer>( end_iterator.element_pointer++ ), std::move( element ) );
                            end_iterator.group_pointer->last_endpoint = end_iterator.element_pointer;
                        } else {
                            COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                              reinterpret_cast<pointer>( end_iterator.element_pointer ), std::move( element ) );
                            end_iterator.group_pointer->last_endpoint = ++end_iterator.element_pointer;
                        }

                        ++( end_iterator.group_pointer->number_of_elements );
                        ++end_iterator.skipfield_pointer;
                        ++total_number_of_elements;

                        return return_iterator;
                    }
                    case 1: {
                        end_iterator.group_pointer->next_group = COLONY_ALLOCATE( group_allocator_type,
                                group_allocator_pair, 1, end_iterator.group_pointer );
                        group &next_group = *( end_iterator.group_pointer->next_group );
                        const skipfield_type new_group_size = ( total_number_of_elements < static_cast<size_type>
                                                                ( group_allocator_pair.max_elements_per_group ) ) ? static_cast<skipfield_type>
                                                              ( total_number_of_elements ) : group_allocator_pair.max_elements_per_group;

                        try {
                            COLONY_CONSTRUCT( group_allocator_type, group_allocator_pair, &next_group, new_group_size,
                                              end_iterator.group_pointer );
                        } catch( ... ) {
                            COLONY_DEALLOCATE( group_allocator_type, group_allocator_pair, &next_group, 1 );
                            end_iterator.group_pointer->next_group = nullptr;
                            throw;
                        }

                        if COLONY_CONSTEXPR( std::is_nothrow_move_constructible<element_type>::value ) {
                            COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                              reinterpret_cast<pointer>( next_group.elements ), std::move( element ) );
                        } else {
                            try {
                                COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                                  reinterpret_cast<pointer>( next_group.elements ), std::move( element ) );
                            } catch( ... ) {
                                COLONY_DESTROY( group_allocator_type, group_allocator_pair, &next_group );
                                COLONY_DEALLOCATE( group_allocator_type, group_allocator_pair, &next_group, 1 );
                                end_iterator.group_pointer->next_group = nullptr;
                                throw;
                            }
                        }

                        end_iterator.group_pointer = &next_group;
                        end_iterator.element_pointer = next_group.last_endpoint;
                        end_iterator.skipfield_pointer = next_group.skipfield + 1;
                        ++total_number_of_elements;
                        total_capacity += new_group_size;

                        return iterator( end_iterator.group_pointer, next_group.elements, next_group.skipfield );
                    }
                    default: {
                        iterator new_location;
                        new_location.group_pointer = groups_with_erasures_list_head;
                        new_location.element_pointer = groups_with_erasures_list_head->elements +
                                                       groups_with_erasures_list_head->free_list_head;
                        new_location.skipfield_pointer = groups_with_erasures_list_head->skipfield +
                                                         groups_with_erasures_list_head->free_list_head;

                        const skipfield_type prev_free_list_index = *( reinterpret_cast<skipfield_pointer_type>
                                ( new_location.element_pointer ) );
                        COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                          reinterpret_cast<pointer>( new_location.element_pointer ), std::move( element ) );

                        const skipfield_type new_value = *( new_location.skipfield_pointer ) - 1;

                        if( new_value != 0 ) {
                            *( new_location.skipfield_pointer + new_value ) = *( new_location.skipfield_pointer + 1 ) =
                                        new_value;
                            ++( groups_with_erasures_list_head->free_list_head );

                            if( prev_free_list_index != std::numeric_limits<skipfield_type>::max() ) {
                                *( reinterpret_cast<skipfield_pointer_type>( new_location.group_pointer->elements +
                                        prev_free_list_index ) + 1 ) = groups_with_erasures_list_head->free_list_head;
                            }

                            *( reinterpret_cast<skipfield_pointer_type>( new_location.element_pointer + 1 ) ) =
                                prev_free_list_index;
                            *( reinterpret_cast<skipfield_pointer_type>( new_location.element_pointer + 1 ) + 1 ) =
                                std::numeric_limits<skipfield_type>::max();
                        } else {
                            groups_with_erasures_list_head->free_list_head = prev_free_list_index;

                            if( prev_free_list_index != std::numeric_limits<skipfield_type>::max() ) {
                                *( reinterpret_cast<skipfield_pointer_type>( new_location.group_pointer->elements +
                                        prev_free_list_index ) + 1 ) = std::numeric_limits<skipfield_type>::max();
                            } else {
                                groups_with_erasures_list_head = groups_with_erasures_list_head->erasures_list_next_group;
                            }
                        }

                        *( new_location.skipfield_pointer ) = 0;
                        ++( new_location.group_pointer->number_of_elements );

                        if( new_location.group_pointer == begin_iterator.group_pointer &&
                            new_location.element_pointer < begin_iterator.element_pointer ) {
                            begin_iterator = new_location;
                        }

                        ++total_number_of_elements;

                        return new_location;
                    }
                }
            } else {
                initialize( pointer_allocator_pair.min_elements_per_group );

                if COLONY_CONSTEXPR( std::is_nothrow_move_constructible<element_type>::value ) {
                    COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                      reinterpret_cast<pointer>( end_iterator.element_pointer++ ), std::move( element ) );
                } else {
                    try {
                        COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                          reinterpret_cast<pointer>( end_iterator.element_pointer++ ), std::move( element ) );
                    } catch( ... ) {
                        clear();
                        throw;
                    }
                }

                ++end_iterator.skipfield_pointer;
                total_number_of_elements = 1;
                return begin_iterator;
            }
        }

        /**
         * Constructs new element directly within colony. Will insert the element in a previously
         * erased element slot if one exists, otherwise will insert to back of colony. Returns
         * iterator to location of inserted element. "...parameters" are whatever parameters are
         * required by the object's constructor.
         *
         * The emplace function is near-identical to the regular insert function, with the exception
         * of the element construction method and change to is_nothrow tests.
         */
        template<typename... arguments>
        iterator emplace( arguments &&... parameters ) {
            if( end_iterator.element_pointer != nullptr ) {
                switch( ( ( groups_with_erasures_list_head != nullptr ) << 1 ) | ( end_iterator.element_pointer ==
                        reinterpret_cast<aligned_pointer_type>( end_iterator.group_pointer->skipfield ) ) ) {
                    case 0: {
                        const iterator return_iterator = end_iterator;

                        if COLONY_CONSTEXPR( std::is_nothrow_constructible<element_type, arguments ...>::value ) {
                            COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                              reinterpret_cast<pointer>( end_iterator.element_pointer++ ),
                                              std::forward<arguments>( parameters )... );
                            end_iterator.group_pointer->last_endpoint = end_iterator.element_pointer;
                        } else {
                            COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                              reinterpret_cast<pointer>( end_iterator.element_pointer ),
                                              std::forward<arguments>( parameters )... );
                            end_iterator.group_pointer->last_endpoint = ++end_iterator.element_pointer;
                        }

                        ++( end_iterator.group_pointer->number_of_elements );
                        ++end_iterator.skipfield_pointer;
                        ++total_number_of_elements;

                        return return_iterator;
                    }
                    case 1: {
                        end_iterator.group_pointer->next_group = COLONY_ALLOCATE( group_allocator_type,
                                group_allocator_pair, 1, end_iterator.group_pointer );
                        group &next_group = *( end_iterator.group_pointer->next_group );
                        const skipfield_type new_group_size = ( total_number_of_elements < static_cast<size_type>
                                                                ( group_allocator_pair.max_elements_per_group ) ) ? static_cast<skipfield_type>
                                                              ( total_number_of_elements ) : group_allocator_pair.max_elements_per_group;

                        try {
                            COLONY_CONSTRUCT( group_allocator_type, group_allocator_pair, &next_group, new_group_size,
                                              end_iterator.group_pointer );
                        } catch( ... ) {
                            COLONY_DEALLOCATE( group_allocator_type, group_allocator_pair, &next_group, 1 );
                            end_iterator.group_pointer->next_group = nullptr;
                            throw;
                        }

                        if COLONY_CONSTEXPR( std::is_nothrow_constructible<element_type, arguments ...>::value ) {
                            COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                              reinterpret_cast<pointer>( next_group.elements ), std::forward<arguments>( parameters )... );
                        } else {
                            try {
                                COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                                  reinterpret_cast<pointer>( next_group.elements ), std::forward<arguments>( parameters )... );
                            } catch( ... ) {
                                COLONY_DESTROY( group_allocator_type, group_allocator_pair, &next_group );
                                COLONY_DEALLOCATE( group_allocator_type, group_allocator_pair, &next_group, 1 );
                                end_iterator.group_pointer->next_group = nullptr;
                                throw;
                            }
                        }

                        end_iterator.group_pointer = &next_group;
                        end_iterator.element_pointer = next_group.last_endpoint;
                        end_iterator.skipfield_pointer = next_group.skipfield + 1;
                        total_capacity += new_group_size;
                        ++total_number_of_elements;

                        return iterator( end_iterator.group_pointer, next_group.elements, next_group.skipfield );
                    }
                    default: {
                        iterator new_location;
                        new_location.group_pointer = groups_with_erasures_list_head;
                        new_location.element_pointer = groups_with_erasures_list_head->elements +
                                                       groups_with_erasures_list_head->free_list_head;
                        new_location.skipfield_pointer = groups_with_erasures_list_head->skipfield +
                                                         groups_with_erasures_list_head->free_list_head;

                        const skipfield_type prev_free_list_index = *( reinterpret_cast<skipfield_pointer_type>
                                ( new_location.element_pointer ) );
                        COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                          reinterpret_cast<pointer>( new_location.element_pointer ),
                                          std::forward<arguments>( parameters ) ... );
                        const skipfield_type new_value = *( new_location.skipfield_pointer ) - 1;

                        if( new_value != 0 ) {
                            *( new_location.skipfield_pointer + new_value ) = *( new_location.skipfield_pointer + 1 ) =
                                        new_value;
                            ++( groups_with_erasures_list_head->free_list_head );

                            if( prev_free_list_index != std::numeric_limits<skipfield_type>::max() ) {
                                *( reinterpret_cast<skipfield_pointer_type>( new_location.group_pointer->elements +
                                        prev_free_list_index ) + 1 ) = groups_with_erasures_list_head->free_list_head;
                            }

                            *( reinterpret_cast<skipfield_pointer_type>( new_location.element_pointer + 1 ) ) =
                                prev_free_list_index;
                            *( reinterpret_cast<skipfield_pointer_type>( new_location.element_pointer + 1 ) + 1 ) =
                                std::numeric_limits<skipfield_type>::max();
                        } else {
                            groups_with_erasures_list_head->free_list_head = prev_free_list_index;

                            if( prev_free_list_index != std::numeric_limits<skipfield_type>::max() ) {
                                *( reinterpret_cast<skipfield_pointer_type>( new_location.group_pointer->elements +
                                        prev_free_list_index ) + 1 ) = std::numeric_limits<skipfield_type>::max();
                            } else {
                                groups_with_erasures_list_head = groups_with_erasures_list_head->erasures_list_next_group;
                            }
                        }

                        *( new_location.skipfield_pointer ) = 0;
                        ++( new_location.group_pointer->number_of_elements );

                        if( new_location.group_pointer == begin_iterator.group_pointer &&
                            new_location.element_pointer < begin_iterator.element_pointer ) {
                            begin_iterator = new_location;
                        }

                        ++total_number_of_elements;

                        return new_location;
                    }
                }
            } else {
                initialize( pointer_allocator_pair.min_elements_per_group );

                if COLONY_CONSTEXPR( std::is_nothrow_constructible<element_type, arguments ...>::value ) {
                    COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                      reinterpret_cast<pointer>( end_iterator.element_pointer++ ),
                                      std::forward<arguments>( parameters ) ... );
                } else {
                    try {
                        COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                          reinterpret_cast<pointer>( end_iterator.element_pointer++ ),
                                          std::forward<arguments>( parameters ) ... );
                    } catch( ... ) {
                        clear();
                        throw;
                    }
                }

                ++end_iterator.skipfield_pointer;
                total_number_of_elements = 1;
                return begin_iterator;
            }
        }

    private:

        // Internal functions for fill insert:
        void group_create( const skipfield_type number_of_elements ) {
            const group_pointer_type next_group = end_iterator.group_pointer->next_group = COLONY_ALLOCATE(
                    group_allocator_type, group_allocator_pair, 1, end_iterator.group_pointer );

            try {
                COLONY_CONSTRUCT( group_allocator_type, group_allocator_pair, next_group, number_of_elements,
                                  end_iterator.group_pointer );
            } catch( ... ) {
                COLONY_DESTROY( group_allocator_type, group_allocator_pair, next_group );
                COLONY_DEALLOCATE( group_allocator_type, group_allocator_pair, next_group, 1 );
                end_iterator.group_pointer->next_group = nullptr;
                throw;
            }

            end_iterator.group_pointer = next_group;
            end_iterator.element_pointer = next_group->elements;
            // group constructor sets this to 1 by default to allow for faster insertion during insertion/emplace in other cases
            next_group->number_of_elements = 0;
            total_capacity += number_of_elements;
        }

        void group_fill( const element_type &element, const skipfield_type number_of_elements ) {
            // ie. we can get away with using the cheaper fill_n here if there is no chance of an exception being thrown:
            if COLONY_CONSTEXPR( std::is_trivially_copyable<element_type>::value &&
                                 std::is_trivially_copy_constructible<element_type>::value &&
                                 std::is_nothrow_copy_constructible<element_type>::value ) {
                if COLONY_CONSTEXPR( sizeof( aligned_element_type ) == sizeof( element_type ) ) {
                    std::fill_n( reinterpret_cast<pointer>( end_iterator.element_pointer ), number_of_elements,
                                 element );
                } else {
                    // to avoid potentially violating memory boundaries in line below, create an initial copy object of same (but aligned) type
                    alignas( sizeof( aligned_element_type ) ) element_type aligned_copy = element;
                    std::fill_n( end_iterator.element_pointer, number_of_elements,
                                 *( reinterpret_cast<aligned_pointer_type>( &aligned_copy ) ) );
                }

                end_iterator.element_pointer += number_of_elements;
            } else {
                const aligned_pointer_type fill_end = end_iterator.element_pointer + number_of_elements;

                do {
                    try {
                        COLONY_CONSTRUCT( element_allocator_type, ( *this ),
                                          reinterpret_cast<pointer>( end_iterator.element_pointer++ ), element );
                    } catch( ... ) {
                        end_iterator.group_pointer->last_endpoint = --end_iterator.element_pointer;
                        const skipfield_type elements_constructed_before_exception = static_cast<skipfield_type>
                                ( end_iterator.element_pointer - end_iterator.group_pointer->elements );
                        end_iterator.group_pointer->number_of_elements = elements_constructed_before_exception;
                        end_iterator.skipfield_pointer = end_iterator.group_pointer->skipfield +
                                                         elements_constructed_before_exception;
                        throw;
                    }
                } while( end_iterator.element_pointer != fill_end );
            }

            end_iterator.group_pointer->last_endpoint = end_iterator.element_pointer;
            end_iterator.group_pointer->number_of_elements += number_of_elements;
        }

        void fill_skipblock( const element_type &element, aligned_pointer_type const location,
                             skipfield_pointer_type const skipfield_pointer, const skipfield_type number_of_elements ) {
            // ie. we can get away with using the cheaper fill_n here if there is no chance of an exception being thrown:
            if COLONY_CONSTEXPR( std::is_trivially_copyable<element_type>::value &&
                                 std::is_trivially_copy_constructible<element_type>::value &&
                                 std::is_nothrow_copy_constructible<element_type>::value ) {
                if COLONY_CONSTEXPR( sizeof( aligned_element_type ) == sizeof( element_type ) ) {
                    std::fill_n( reinterpret_cast<pointer>( location ), number_of_elements, element );
                } else {
                    // to avoid potentially violating memory boundaries in line below, create an initial copy object of same (but aligned) type
                    alignas( sizeof( aligned_element_type ) ) element_type aligned_copy = element;
                    std::fill_n( location, number_of_elements,
                                 *( reinterpret_cast<aligned_pointer_type>( &aligned_copy ) ) );
                }
            } else {
                // in case of exception, grabbing indexes before free_list node is reused
                const skipfield_type prev_free_list_node = *( reinterpret_cast<skipfield_pointer_type>
                        ( location ) );
                const aligned_pointer_type fill_end = location + number_of_elements;

                for( aligned_pointer_type current_location = location; current_location != fill_end;
                     ++current_location ) {
                    try {
                        COLONY_CONSTRUCT( element_allocator_type, ( *this ), reinterpret_cast<pointer>( current_location ),
                                          element );
                    } catch( ... ) {
                        // Reconstruct existing skipblock and free-list indexes to reflect partially-reused skipblock:
                        const skipfield_type elements_constructed_before_exception = static_cast<skipfield_type>( (
                                    current_location - 1 ) - location );
                        groups_with_erasures_list_head->number_of_elements += elements_constructed_before_exception;
                        total_number_of_elements += elements_constructed_before_exception;

                        std::memset( skipfield_pointer, 0,
                                     elements_constructed_before_exception * sizeof( skipfield_type ) );

                        *( reinterpret_cast<skipfield_pointer_type>( location + elements_constructed_before_exception ) ) =
                            prev_free_list_node;
                        *( reinterpret_cast<skipfield_pointer_type>( location + elements_constructed_before_exception ) +
                           1 ) = std::numeric_limits<skipfield_type>::max();

                        const skipfield_type new_skipblock_head_index = static_cast<skipfield_type>
                                ( location - groups_with_erasures_list_head->elements ) + elements_constructed_before_exception;
                        groups_with_erasures_list_head->free_list_head = new_skipblock_head_index;

                        if( prev_free_list_node != std::numeric_limits<skipfield_type>::max() ) {
                            *( reinterpret_cast<skipfield_pointer_type>( groups_with_erasures_list_head->elements +
                                    prev_free_list_node ) + 1 ) = new_skipblock_head_index;
                        }

                        throw;
                    }
                }
            }

            // reset skipfield nodes within skipblock to 0
            std::memset( skipfield_pointer, 0, number_of_elements * sizeof( skipfield_type ) );
            groups_with_erasures_list_head->number_of_elements += number_of_elements;
            total_number_of_elements += number_of_elements;
        }

    public:

        /**
         * Fill insert:
         * Inserts n copies of val into the colony. Will insert the element into a previously erased
         * element slot if one exists, otherwise will insert to back of colony.
         */
        void insert( size_type number_of_elements, const element_type &element ) {
            if( number_of_elements == 0 ) {
                return;
            } else if( number_of_elements == 1 ) {
                insert( element );
                return;
            }

            if( begin_iterator.group_pointer == nullptr ) { // Empty colony, no groups created yet
                initialize( ( number_of_elements > group_allocator_pair.max_elements_per_group ) ?
                            group_allocator_pair.max_elements_per_group : ( number_of_elements <
                                    pointer_allocator_pair.min_elements_per_group ) ? pointer_allocator_pair.min_elements_per_group :
                            static_cast<skipfield_type>( number_of_elements ) ); // Construct first group
                begin_iterator.group_pointer->number_of_elements = 0;
            }

            // ie. not an uninitialized colony or a situation where reserve has been called
            if( total_number_of_elements != 0 ) {
                // Use up erased locations if available:
                if( groups_with_erasures_list_head != nullptr ) {
                    do { // skipblock loop: breaks when group is exhausted of reusable skipblocks, or returns if number_of_elements == 0
                        aligned_pointer_type const element_pointer = groups_with_erasures_list_head->elements +
                                groups_with_erasures_list_head->free_list_head;
                        skipfield_pointer_type const skipfield_pointer = groups_with_erasures_list_head->skipfield +
                                groups_with_erasures_list_head->free_list_head;
                        const skipfield_type skipblock_size = *skipfield_pointer;

                        if( groups_with_erasures_list_head == begin_iterator.group_pointer &&
                            element_pointer < begin_iterator.element_pointer ) {
                            begin_iterator.element_pointer = element_pointer;
                            begin_iterator.skipfield_pointer = skipfield_pointer;
                        }

                        if( skipblock_size <= number_of_elements ) {
                            // set free list head to previous free list node
                            groups_with_erasures_list_head->free_list_head = *( reinterpret_cast<skipfield_pointer_type>
                                    ( element_pointer ) );
                            fill_skipblock( element, element_pointer, skipfield_pointer, skipblock_size );
                            number_of_elements -= skipblock_size;

                            if( groups_with_erasures_list_head->free_list_head != std::numeric_limits<skipfield_type>::max() ) {
                                // set 'next' index of new free list head to 'end' (numeric max)
                                *( reinterpret_cast<skipfield_pointer_type>( groups_with_erasures_list_head->elements +
                                        groups_with_erasures_list_head->free_list_head ) + 1 ) = std::numeric_limits<skipfield_type>::max();
                            } else {
                                // change groups
                                groups_with_erasures_list_head = groups_with_erasures_list_head->erasures_list_next_group;

                                if( groups_with_erasures_list_head == nullptr ) {
                                    break;
                                }
                            }
                        } else { // skipblock is larger than remaining number of elements
                            // save before element location is overwritten
                            const skipfield_type prev_index = *( reinterpret_cast<skipfield_pointer_type>( element_pointer ) );
                            fill_skipblock( element, element_pointer, skipfield_pointer,
                                            static_cast<skipfield_type>( number_of_elements ) );
                            const skipfield_type new_skipblock_size = static_cast<skipfield_type>( skipblock_size -
                                    number_of_elements );

                            // Update skipfield (earlier nodes already memset'd in fill_skipblock function):
                            *( skipfield_pointer + number_of_elements ) = new_skipblock_size;
                            *( skipfield_pointer + skipblock_size - 1 ) = new_skipblock_size;
                            // set free list head to new start node
                            groups_with_erasures_list_head->free_list_head += static_cast<skipfield_type>( number_of_elements );

                            // Update free list with new head:
                            *( reinterpret_cast<skipfield_pointer_type>( element_pointer + number_of_elements ) ) = prev_index;
                            *( reinterpret_cast<skipfield_pointer_type>( element_pointer + number_of_elements ) + 1 ) =
                                std::numeric_limits<skipfield_type>::max();

                            if( prev_index != std::numeric_limits<skipfield_type>::max() ) {
                                // set 'next' index of previous skipblock to new start of skipblock
                                *( reinterpret_cast<skipfield_pointer_type>( groups_with_erasures_list_head->elements + prev_index )
                                   + 1 ) = groups_with_erasures_list_head->free_list_head;
                            }

                            return;
                        }
                    } while( number_of_elements != 0 );
                }

                // Use up remaining available element locations in end group:
                const skipfield_type group_remainder = ( static_cast<skipfield_type>
                                                       ( reinterpret_cast<aligned_pointer_type>( end_iterator.group_pointer->skipfield ) -
                                                               end_iterator.element_pointer ) > number_of_elements ) ? static_cast<skipfield_type>
                                                       ( number_of_elements ) : static_cast<skipfield_type>( reinterpret_cast<aligned_pointer_type>
                                                               ( end_iterator.group_pointer->skipfield ) - end_iterator.element_pointer );

                if( group_remainder != 0 ) {
                    group_fill( element, group_remainder );
                    total_number_of_elements += group_remainder;
                    number_of_elements -= group_remainder;
                }
            } else if( end_iterator.group_pointer->capacity >= number_of_elements ) {
                group_fill( element, static_cast<skipfield_type>( number_of_elements ) );
                end_iterator.skipfield_pointer = end_iterator.group_pointer->skipfield + number_of_elements;
                total_number_of_elements = number_of_elements;
                return;
            } else {
                group_fill( element, end_iterator.group_pointer->capacity );
                total_number_of_elements = end_iterator.group_pointer->capacity;
                number_of_elements -= end_iterator.group_pointer->capacity;
            }

            // If there's some elements left that need to be created, create new groups and fill:
            if( number_of_elements > group_allocator_pair.max_elements_per_group ) {
                size_type multiples = ( number_of_elements / static_cast<size_type>
                                        ( group_allocator_pair.max_elements_per_group ) );
                const skipfield_type element_remainder = static_cast<skipfield_type>( number_of_elements -
                        ( multiples * static_cast<size_type>( group_allocator_pair.max_elements_per_group ) ) );

                while( multiples-- != 0 ) {
                    group_create( group_allocator_pair.max_elements_per_group );
                    group_fill( element, group_allocator_pair.max_elements_per_group );
                }

                if( element_remainder != 0 ) {
                    group_create( group_allocator_pair.max_elements_per_group );
                    group_fill( element, element_remainder );
                }
            } else if( number_of_elements != 0 ) {
                group_create( static_cast<skipfield_type>( ( number_of_elements > total_number_of_elements ) ?
                              number_of_elements : total_number_of_elements ) );
                group_fill( element, static_cast<skipfield_type>( number_of_elements ) );
            }

            // Adds the remainder from the last if-block - the insert functions in the first if/else block will already have incremented total_number_of_elements
            total_number_of_elements += number_of_elements;
            end_iterator.skipfield_pointer = end_iterator.group_pointer->skipfield +
                                             ( end_iterator.element_pointer - end_iterator.group_pointer->elements );
        }

        /**
         * Range insert:
         * Inserts a series of value_type elements from an external source into a colony holding the
         * same value_type (eg. int, float, a particular class, etcetera). Stops inserting once it
         * reaches last.
         */
        template <class iterator_type>
        inline void insert( typename enable_if_c < !std::numeric_limits<iterator_type>::is_integer,
                            iterator_type >::type first, const iterator_type last ) {
            while( first != last ) {
                insert( *first++ );
            }
        }

        /**
         * Initializer-list insert:
         * Copies elements from an initializer list into the colony. Will insert the element in a
         * previously erased element slot if one exists, otherwise will insert to back of colony.
         */
        inline void insert( const std::initializer_list<element_type> &element_list ) {
            // use range insert:
            insert( element_list.begin(), element_list.end() );
        }

    private:

        inline COLONY_FORCE_INLINE void update_subsequent_group_numbers( group_pointer_type current_group )
        noexcept {
            do {
                --( current_group->group_number );
                current_group = current_group->next_group;
            } while( current_group != nullptr );
        }

        // get all elements contiguous in memory and shrink to fit, remove erasures and erasure free lists
        inline COLONY_FORCE_INLINE void consolidate() {
            colony temp;
            // Make first allocated group as large total number of elements, where possible
            temp.change_group_sizes( static_cast<skipfield_type>( (
                                         pointer_allocator_pair.min_elements_per_group > total_number_of_elements ) ?
                                     pointer_allocator_pair.min_elements_per_group : ( ( total_number_of_elements >
                                             group_allocator_pair.max_elements_per_group ) ? group_allocator_pair.max_elements_per_group :
                                             total_number_of_elements ) ),
                                     group_allocator_pair.max_elements_per_group );

            if COLONY_CONSTEXPR( std::is_move_assignable<element_type>::value &&
                                 std::is_move_constructible<element_type>::value ) {
                temp.insert( std::make_move_iterator( begin_iterator ), std::make_move_iterator( end_iterator ) );
            } else {
                temp.insert( begin_iterator, end_iterator );
            }
            // reset to correct value for future clear() or erasures
            temp.pointer_allocator_pair.min_elements_per_group = pointer_allocator_pair.min_elements_per_group;
            // Avoid generating 2nd temporary
            *this = std::move( temp );
        }

        void remove_from_groups_with_erasures_list( const group_pointer_type group_to_remove ) noexcept {
            if( group_to_remove == groups_with_erasures_list_head ) {
                groups_with_erasures_list_head = groups_with_erasures_list_head->erasures_list_next_group;
                return;
            }

            group_pointer_type previous_group = groups_with_erasures_list_head;
            group_pointer_type current_group = groups_with_erasures_list_head->erasures_list_next_group;

            while( group_to_remove != current_group ) {
                previous_group = current_group;
                current_group = current_group->erasures_list_next_group;
            }

            previous_group->erasures_list_next_group = current_group->erasures_list_next_group;
        }

    public:

        /**
         * Removes the element pointed to by the supplied iterator, from the colony. Returns an
         * iterator pointing to the next non-erased element in the colony (or to end() if no more
         * elements are available). This must return an iterator because if a colony group becomes
         * entirely empty, it will be removed from the colony, invalidating the existing iterator.
         * Attempting to erase a previously-erased element results in undefined behaviour (this is
         * checked for via an assert in debug mode).
         *
         * must return iterator to subsequent non-erased element (or end()), in case the group
         * containing the element which the iterator points to becomes empty after the erasure, and
         * is thereafter removed from the colony chain, making the current iterator invalid and
         * unusable in a ++ operation:
         *
         * if uninitialized/invalid iterator supplied, function could generate an exception
         */
        iterator erase( const const_iterator &it ) {
            assert( !empty() );
            const group_pointer_type group_pointer = it.group_pointer;
            // not uninitialized iterator
            assert( group_pointer != nullptr );
            // != end()
            assert( it.element_pointer != group_pointer->last_endpoint );
            // element pointed to by iterator has not been erased previously
            assert( *( it.skipfield_pointer ) == 0 );

            // This if-statement should be removed by the compiler on resolution of element_type. For some optimizing compilers this step won't be necessary (for MSVC 2013 it makes a difference)
            if COLONY_CONSTEXPR( !( std::is_trivially_destructible<element_type>::value ) ) {
                COLONY_DESTROY( element_allocator_type, ( *this ),
                                reinterpret_cast<pointer>( it.element_pointer ) ); // Destruct element
            }

            --total_number_of_elements;

            // ie. non-empty group at this point in time, don't consolidate - optimization note: GCC optimizes postfix + 1 comparison better than prefix + 1 comparison in many cases.
            if( group_pointer->number_of_elements-- != 1 ) {
                // Code logic for following section:
                // ---------------------------------
                // If current skipfield node has no skipped node on either side, continue as usual
                // If node only has skipped node on left, set current node and start node of the skipblock to left node value + 1.
                // If node only has skipped node on right, make this node the start node of the skipblock and update end node
                // If node has skipped nodes on left and right, set start node of left skipblock and end node of right skipblock to the values of the left + right nodes + 1

                // Optimization explanation:
                // The contextual logic below is the same as that in the insert() functions but in this case the value of the current skipfield node will always be
                // zero (since it is not yet erased), meaning no additional manipulations are necessary for the previous skipfield node comparison - we only have to check against zero
                const char prev_skipfield = *( it.skipfield_pointer - ( it.skipfield_pointer !=
                                               group_pointer->skipfield ) ) != 0;
                // NOTE: boundary test (checking against end-of-elements) is able to be skipped due to the extra skipfield node (compared to element field) - which is present to enable faster iterator operator ++ operations
                const char after_skipfield = *( it.skipfield_pointer + 1 ) != 0;
                skipfield_type update_value = 1;

                switch( ( after_skipfield << 1 ) | prev_skipfield ) {
                    case 0: { // no consecutive erased elements
                        *it.skipfield_pointer = 1; // solo skipped node
                        const skipfield_type index = static_cast<skipfield_type>( it.element_pointer -
                                                     group_pointer->elements );

                        // ie. if this group already has some erased elements
                        if( group_pointer->free_list_head != std::numeric_limits<skipfield_type>::max() ) {
                            // set prev free list head's 'next index' number to the index of the current element
                            *( reinterpret_cast<skipfield_pointer_type>( group_pointer->elements +
                                    group_pointer->free_list_head ) + 1 ) = index;
                        } else {
                            // add it to the groups-with-erasures free list
                            group_pointer->erasures_list_next_group = groups_with_erasures_list_head;
                            groups_with_erasures_list_head = group_pointer;
                        }

                        *( reinterpret_cast<skipfield_pointer_type>( it.element_pointer ) ) = group_pointer->free_list_head;
                        *( reinterpret_cast<skipfield_pointer_type>( it.element_pointer ) + 1 ) =
                            std::numeric_limits<skipfield_type>::max();
                        group_pointer->free_list_head = index;
                        break;
                    }
                    case 1: { // previous erased consecutive elements, none following
                        *( it.skipfield_pointer - * ( it.skipfield_pointer - 1 ) ) = *it.skipfield_pointer = *
                                ( it.skipfield_pointer - 1 ) + 1;
                        break;
                    }
                    case 2: { // following erased consecutive elements, none preceding
                        const skipfield_type following_value = *( it.skipfield_pointer + 1 ) + 1;
                        *( it.skipfield_pointer + following_value - 1 ) = *( it.skipfield_pointer ) = following_value;

                        const skipfield_type following_previous = *( reinterpret_cast<skipfield_pointer_type>
                                ( it.element_pointer + 1 ) );
                        const skipfield_type following_next = *( reinterpret_cast<skipfield_pointer_type>
                                                              ( it.element_pointer + 1 ) + 1 );
                        *( reinterpret_cast<skipfield_pointer_type>( it.element_pointer ) ) = following_previous;
                        *( reinterpret_cast<skipfield_pointer_type>( it.element_pointer ) + 1 ) = following_next;

                        const skipfield_type index = static_cast<skipfield_type>( it.element_pointer -
                                                     group_pointer->elements );

                        if( following_previous != std::numeric_limits<skipfield_type>::max() ) {
                            // Set next index of previous free list node to this node's 'next' index
                            *( reinterpret_cast<skipfield_pointer_type>( group_pointer->elements + following_previous ) + 1 ) =
                                index;
                        }

                        if( following_next != std::numeric_limits<skipfield_type>::max() ) {
                            // Set previous index of next free list node to this node's 'previous' index
                            *( reinterpret_cast<skipfield_pointer_type>( group_pointer->elements + following_next ) ) = index;
                        } else {
                            group_pointer->free_list_head = index;
                        }

                        update_value = following_value;
                        break;
                    }
                    case 3: { // both preceding and following consecutive erased elements
                        const skipfield_type preceding_value = *( it.skipfield_pointer - 1 );
                        const skipfield_type following_value = *( it.skipfield_pointer + 1 ) + 1;

                        // Join the skipblocks
                        *( it.skipfield_pointer - preceding_value ) = *( it.skipfield_pointer + following_value - 1 ) =
                                    preceding_value + following_value;

                        // Remove the following skipblock's entry from the free list
                        const skipfield_type following_previous = *( reinterpret_cast<skipfield_pointer_type>
                                ( it.element_pointer + 1 ) );
                        const skipfield_type following_next = *( reinterpret_cast<skipfield_pointer_type>
                                                              ( it.element_pointer + 1 ) + 1 );

                        if( following_previous != std::numeric_limits<skipfield_type>::max() ) {
                            // Set next index of previous free list node to this node's 'next' index
                            *( reinterpret_cast<skipfield_pointer_type>( group_pointer->elements + following_previous ) + 1 ) =
                                following_next;
                        }

                        if( following_next != std::numeric_limits<skipfield_type>::max() ) {
                            // Set previous index of next free list node to this node's 'previous' index
                            *( reinterpret_cast<skipfield_pointer_type>( group_pointer->elements + following_next ) ) =
                                following_previous;
                        } else {
                            group_pointer->free_list_head = following_previous;
                        }

                        update_value = following_value;
                        break;
                    }
                }

                iterator return_iterator( it.group_pointer, it.element_pointer + update_value,
                                          it.skipfield_pointer + update_value );
                return_iterator.check_for_end_of_group_and_progress();

                // If original iterator was first element in colony, update it's value with the next non-erased element:
                if( it.element_pointer == begin_iterator.element_pointer ) {
                    begin_iterator = return_iterator;
                }

                return return_iterator;
            }

            // else: group is empty, consolidate groups
            switch( ( group_pointer->next_group != nullptr ) | ( ( group_pointer !=
                    begin_iterator.group_pointer )
                    << 1 ) ) {
                case 0: { // ie. group_pointer == begin_iterator.group_pointer && group_pointer->next_group == NULL; only group in colony
                    // Reset skipfield and free list rather than clearing - leads to fewer allocations/deallocations:
                    // &* to avoid problems with non-trivial pointers. Although there is one more skipfield than group_pointer->capacity, capacity + 1 is not necessary here as the end skipfield is never written to after initialization
                    std::memset( &*( group_pointer->skipfield ), 0,
                                 sizeof( skipfield_type ) * group_pointer->capacity );
                    group_pointer->free_list_head = std::numeric_limits<skipfield_type>::max();
                    groups_with_erasures_list_head = nullptr;

                    // Reset begin and end iterators:
                    end_iterator.element_pointer = begin_iterator.element_pointer = group_pointer->last_endpoint =
                                                       group_pointer->elements;
                    end_iterator.skipfield_pointer = begin_iterator.skipfield_pointer = group_pointer->skipfield;

                    return end_iterator;
                }
                case 1: { // ie. group_pointer == begin_iterator.group_pointer && group_pointer->next_group != NULL. Remove first group, change first group to next group
                    group_pointer->next_group->previous_group = nullptr; // Cut off this group from the chain
                    begin_iterator.group_pointer = group_pointer->next_group; // Make the next group the first group

                    update_subsequent_group_numbers( begin_iterator.group_pointer );

                    // Erasures present within the group, ie. was part of the intrusive list of groups with erasures.
                    if( group_pointer->free_list_head != std::numeric_limits<skipfield_type>::max() ) {
                        remove_from_groups_with_erasures_list( group_pointer );
                    }

                    total_capacity -= group_pointer->capacity;
                    COLONY_DESTROY( group_allocator_type, group_allocator_pair, group_pointer );
                    COLONY_DEALLOCATE( group_allocator_type, group_allocator_pair, group_pointer, 1 );

                    // note: end iterator only needs to be changed if the deleted group was the final group in the chain ie. not in this case
                    // If the beginning index has been erased (ie. skipfield != 0), skip to next non-erased element
                    begin_iterator.element_pointer = begin_iterator.group_pointer->elements + *
                                                     ( begin_iterator.group_pointer->skipfield );
                    begin_iterator.skipfield_pointer = begin_iterator.group_pointer->skipfield + *
                                                       ( begin_iterator.group_pointer->skipfield );

                    return begin_iterator;
                }
                case 3: { // this is a non-first group but not final group in chain: delete the group, then link previous group to the next group in the chain:
                    group_pointer->next_group->previous_group = group_pointer->previous_group;
                    // close the chain, removing this group from it
                    const group_pointer_type return_group = group_pointer->previous_group->next_group =
                            group_pointer->next_group;

                    update_subsequent_group_numbers( return_group );

                    if( group_pointer->free_list_head != std::numeric_limits<skipfield_type>::max() ) {
                        remove_from_groups_with_erasures_list( group_pointer );
                    }

                    total_capacity -= group_pointer->capacity;
                    COLONY_DESTROY( group_allocator_type, group_allocator_pair, group_pointer );
                    COLONY_DEALLOCATE( group_allocator_type, group_allocator_pair, group_pointer, 1 );

                    // Return next group's first non-erased element:
                    return iterator( return_group, return_group->elements + * ( return_group->skipfield ),
                                     return_group->skipfield + * ( return_group->skipfield ) );
                }
                default: { // this is a non-first group and the final group in the chain
                    if( group_pointer->free_list_head != std::numeric_limits<skipfield_type>::max() ) {
                        remove_from_groups_with_erasures_list( group_pointer );
                    }

                    group_pointer->previous_group->next_group = nullptr;
                    // end iterator needs to be changed as element supplied was the back element of the colony
                    end_iterator.group_pointer = group_pointer->previous_group;
                    end_iterator.element_pointer = reinterpret_cast<aligned_pointer_type>
                                                   ( end_iterator.group_pointer->skipfield );
                    end_iterator.skipfield_pointer = end_iterator.group_pointer->skipfield +
                                                     end_iterator.group_pointer->capacity;

                    total_capacity -= group_pointer->capacity;
                    COLONY_DESTROY( group_allocator_type, group_allocator_pair, group_pointer );
                    COLONY_DEALLOCATE( group_allocator_type, group_allocator_pair, group_pointer, 1 );

                    return end_iterator;
                }
            }
        }

        /**
         * Range erase:
         * Erases all elements of a given colony from first to the element before the last iterator.
         * This function is optimized for multiple consecutive erasures and will always be faster
         * than sequential single-element erase calls in that scenario.
         */
        void erase( const const_iterator &iterator1, const const_iterator &iterator2 ) {
            // if uninitialized/invalid iterators supplied, function could generate an exception. If iterator1 > iterator2, behaviour is undefined.
            assert( iterator1 <= iterator2 );

            iterator current = iterator1;

            if( current.group_pointer != iterator2.group_pointer ) {
                if( current.element_pointer != current.group_pointer->elements + *
                    ( current.group_pointer->skipfield ) ) {
                    // if iterator1 is not the first non-erased element in it's group - most common case
                    size_type number_of_group_erasures = 0;

                    // Now update skipfield:
                    const aligned_pointer_type end = iterator1.group_pointer->last_endpoint;

                    // Schema: first erase all non-erased elements until end of group & remove all skipblocks post-iterator1 from the free_list. Then, either update preceding skipblock or create new one:

                    // if trivially-destructible, and no erasures in group, skip while loop below and just jump straight to the location
                    if( ( std::is_trivially_destructible<element_type>::value ) &
                        ( current.group_pointer->free_list_head == std::numeric_limits<skipfield_type>::max() ) ) {
                        number_of_group_erasures += static_cast<size_type>( end - current.element_pointer );
                    } else {
                        while( current.element_pointer != end ) {
                            if( *current.skipfield_pointer == 0 ) {
                                if COLONY_CONSTEXPR( !( std::is_trivially_destructible<element_type>::value ) ) {
                                    COLONY_DESTROY( element_allocator_type, ( *this ),
                                                    reinterpret_cast<pointer>( current.element_pointer ) ); // Destruct element
                                }

                                ++number_of_group_erasures;
                                ++current.element_pointer;
                                ++current.skipfield_pointer;
                            } else { // remove skipblock from group:
                                const skipfield_type prev_free_list_index = *( reinterpret_cast<skipfield_pointer_type>
                                        ( current.element_pointer ) );
                                const skipfield_type next_free_list_index = *( reinterpret_cast<skipfield_pointer_type>
                                        ( current.element_pointer ) + 1 );

                                current.element_pointer += *( current.skipfield_pointer );
                                current.skipfield_pointer += *( current.skipfield_pointer );

                                // if this is the last skipblock in the free list
                                if( next_free_list_index == std::numeric_limits<skipfield_type>::max() &&
                                    prev_free_list_index == std::numeric_limits<skipfield_type>::max() ) {
                                    // remove group from list of free-list groups - will be added back in down below, but not worth optimizing for
                                    remove_from_groups_with_erasures_list( iterator1.group_pointer );
                                    iterator1.group_pointer->free_list_head = std::numeric_limits<skipfield_type>::max();
                                    number_of_group_erasures += static_cast<size_type>( end - current.element_pointer );

                                    if COLONY_CONSTEXPR( !( std::is_trivially_destructible<element_type>::value ) ) {
                                        // miniloop - avoid checking skipfield for rest of elements in group, as there are no more skipped elements now
                                        while( current.element_pointer != end ) {
                                            COLONY_DESTROY( element_allocator_type, ( *this ),
                                                            reinterpret_cast<pointer>( current.element_pointer++ ) ); // Destruct element
                                        }
                                    }

                                    break; // end overall while loop
                                } else if( next_free_list_index == std::numeric_limits<skipfield_type>::max() ) {
                                    // if this is the head of the free list
                                    // make free list head equal to next free list node
                                    current.group_pointer->free_list_head = prev_free_list_index;
                                    *( reinterpret_cast<skipfield_pointer_type>( current.group_pointer->elements +
                                            prev_free_list_index ) + 1 ) = std::numeric_limits<skipfield_type>::max();
                                } else { // either a tail or middle free list node
                                    *( reinterpret_cast<skipfield_pointer_type>( current.group_pointer->elements +
                                            next_free_list_index ) ) = prev_free_list_index;

                                    if( prev_free_list_index !=
                                        std::numeric_limits<skipfield_type>::max() ) { // ie. not the tail free list node
                                        *( reinterpret_cast<skipfield_pointer_type>( current.group_pointer->elements +
                                                prev_free_list_index ) + 1 ) = next_free_list_index;
                                    }
                                }
                            }
                        }
                    }

                    const skipfield_type previous_node_value = *( iterator1.skipfield_pointer - 1 );
                    const skipfield_type distance_to_end = static_cast<skipfield_type>( end -
                                                           iterator1.element_pointer );

                    if( previous_node_value == 0 ) { // no previous skipblock
                        *iterator1.skipfield_pointer = distance_to_end;
                        *( iterator1.skipfield_pointer + distance_to_end - 1 ) = distance_to_end;

                        const skipfield_type index = static_cast<skipfield_type>( iterator1.element_pointer -
                                                     iterator1.group_pointer->elements );

                        if( iterator1.group_pointer->free_list_head != std::numeric_limits<skipfield_type>::max() ) {
                            // if this group already has some erased elements
                            // set prev free list head's 'next index' number to the index of the iterator1 element
                            *( reinterpret_cast<skipfield_pointer_type>( iterator1.group_pointer->elements +
                                    iterator1.group_pointer->free_list_head ) + 1 ) = index;
                        } else {
                            // add it to the groups-with-erasures free list
                            iterator1.group_pointer->erasures_list_next_group = groups_with_erasures_list_head;
                            groups_with_erasures_list_head = iterator1.group_pointer;
                        }

                        *( reinterpret_cast<skipfield_pointer_type>( iterator1.element_pointer ) ) =
                            iterator1.group_pointer->free_list_head;
                        *( reinterpret_cast<skipfield_pointer_type>( iterator1.element_pointer ) + 1 ) =
                            std::numeric_limits<skipfield_type>::max();
                        iterator1.group_pointer->free_list_head = index;
                    } else {
                        // update previous skipblock, no need to update free list:
                        *( iterator1.skipfield_pointer - previous_node_value ) = *( iterator1.skipfield_pointer +
                                distance_to_end - 1 ) = previous_node_value + distance_to_end;
                    }

                    iterator1.group_pointer->number_of_elements -= static_cast<skipfield_type>
                            ( number_of_group_erasures );
                    total_number_of_elements -= number_of_group_erasures;

                    current.group_pointer = current.group_pointer->next_group;
                }

                // Intermediate groups:
                const group_pointer_type previous_group = current.group_pointer->previous_group;

                while( current.group_pointer != iterator2.group_pointer ) {
                    if COLONY_CONSTEXPR( !( std::is_trivially_destructible<element_type>::value ) ) {
                        current.element_pointer = current.group_pointer->elements + *( current.group_pointer->skipfield );
                        current.skipfield_pointer = current.group_pointer->skipfield + *
                                                    ( current.group_pointer->skipfield );
                        const aligned_pointer_type end = current.group_pointer->last_endpoint;

                        do {
                            COLONY_DESTROY( element_allocator_type, ( *this ),
                                            reinterpret_cast<pointer>( current.element_pointer ) ); // Destruct element
                            ++current.skipfield_pointer;
                            current.element_pointer += *current.skipfield_pointer + 1;
                            current.skipfield_pointer += *current.skipfield_pointer;
                        } while( current.element_pointer != end );
                    }

                    if( current.group_pointer->free_list_head != std::numeric_limits<skipfield_type>::max() ) {
                        remove_from_groups_with_erasures_list( current.group_pointer );
                    }

                    total_number_of_elements -= current.group_pointer->number_of_elements;
                    const group_pointer_type current_group = current.group_pointer;
                    current.group_pointer = current.group_pointer->next_group;

                    total_capacity -= current_group->capacity;
                    COLONY_DESTROY( group_allocator_type, group_allocator_pair, current_group );
                    COLONY_DEALLOCATE( group_allocator_type, group_allocator_pair, current_group, 1 );
                }

                current.element_pointer = current.group_pointer->elements + *( current.group_pointer->skipfield );
                current.skipfield_pointer = current.group_pointer->skipfield + *
                                            ( current.group_pointer->skipfield );
                current.group_pointer->previous_group = previous_group;

                if( previous_group != nullptr ) {
                    previous_group->next_group = current.group_pointer;
                } else {
                    // This line is included here primarily to avoid a secondary if statement within the if block below - it will not be needed in any other situation
                    begin_iterator = iterator2;
                }
            }

            // in case iterator2 was at beginning of it's group - also covers empty range case (first == last)
            if( current.element_pointer == iterator2.element_pointer ) {
                return;
            }

            // Final group:
            // Code explanation:
            // If not erasing entire final group, 1. Destruct elements (if non-trivial destructor) and add locations to group free list. 2. process skipfield.
            // If erasing entire group, 1. Destruct elements (if non-trivial destructor), 2. if no elements left in colony, clear() 3. otherwise reset end_iterator and remove group from groups-with-erasures list (if free list of erasures present)
            if( iterator2.element_pointer != end_iterator.element_pointer ||
                current.element_pointer != current.group_pointer->elements + *
                ( current.group_pointer->skipfield ) ) { // ie. not erasing entire group
                size_type number_of_group_erasures = 0;
                // Schema: first erased all non-erased elements until end of group & remove all skipblocks post-iterator2 from the free_list. Then, either update preceding skipblock or create new one:

                const iterator current_saved = current;

                // if trivially-destructible, and no erasures in group, skip while loop below and just jump straight to the location
                if( ( std::is_trivially_destructible<element_type>::value ) &
                    ( current.group_pointer->free_list_head == std::numeric_limits<skipfield_type>::max() ) ) {
                    number_of_group_erasures += static_cast<size_type>( iterator2.element_pointer -
                                                current.element_pointer );
                } else {
                    while( current.element_pointer != iterator2.element_pointer ) {
                        if( *current.skipfield_pointer == 0 ) {
                            if COLONY_CONSTEXPR( !( std::is_trivially_destructible<element_type>::value ) ) {
                                COLONY_DESTROY( element_allocator_type, ( *this ),
                                                reinterpret_cast<pointer>( current.element_pointer ) ); // Destruct element
                            }

                            ++number_of_group_erasures;
                            ++current.element_pointer;
                            ++current.skipfield_pointer;
                        } else { // remove skipblock from group:
                            const skipfield_type prev_free_list_index = *( reinterpret_cast<skipfield_pointer_type>
                                    ( current.element_pointer ) );
                            const skipfield_type next_free_list_index = *( reinterpret_cast<skipfield_pointer_type>
                                    ( current.element_pointer ) + 1 );

                            current.element_pointer += *( current.skipfield_pointer );
                            current.skipfield_pointer += *( current.skipfield_pointer );

                            if( next_free_list_index == std::numeric_limits<skipfield_type>::max() &&
                                prev_free_list_index == std::numeric_limits<skipfield_type>::max() ) {
                                // if this is the last skipblock in the free list
                                // remove group from list of free-list groups - will be added back in down below, but not worth optimizing for
                                remove_from_groups_with_erasures_list( iterator2.group_pointer );
                                iterator2.group_pointer->free_list_head = std::numeric_limits<skipfield_type>::max();
                                number_of_group_erasures += static_cast<size_type>( iterator2.element_pointer -
                                                            current.element_pointer );

                                if COLONY_CONSTEXPR( !( std::is_trivially_destructible<element_type>::value ) ) {
                                    while( current.element_pointer != iterator2.element_pointer ) {
                                        COLONY_DESTROY( element_allocator_type, ( *this ),
                                                        reinterpret_cast<pointer>( current.element_pointer++ ) ); // Destruct element
                                    }
                                }

                                break; // end overall while loop
                            } else if( next_free_list_index ==
                                       std::numeric_limits<skipfield_type>::max() ) { // if this is the head of the free list
                                current.group_pointer->free_list_head = prev_free_list_index;
                                *( reinterpret_cast<skipfield_pointer_type>( current.group_pointer->elements +
                                        prev_free_list_index ) + 1 ) = std::numeric_limits<skipfield_type>::max();
                            } else {
                                *( reinterpret_cast<skipfield_pointer_type>( current.group_pointer->elements +
                                        next_free_list_index ) ) = prev_free_list_index;

                                if( prev_free_list_index !=
                                    std::numeric_limits<skipfield_type>::max() ) { // ie. not the tail free list node
                                    *( reinterpret_cast<skipfield_pointer_type>( current.group_pointer->elements +
                                            prev_free_list_index ) + 1 ) = next_free_list_index;
                                }
                            }
                        }
                    }
                }

                const skipfield_type distance_to_iterator2 = static_cast<skipfield_type>
                        ( iterator2.element_pointer - current_saved.element_pointer );
                const skipfield_type index = static_cast<skipfield_type>( current_saved.element_pointer -
                                             iterator2.group_pointer->elements );

                if( index == 0 || *( current_saved.skipfield_pointer - 1 ) == 0 ) {
                    // element is either at start of group or previous skipfield node is 0
                    *( current_saved.skipfield_pointer ) = distance_to_iterator2;
                    *( iterator2.skipfield_pointer - 1 ) = distance_to_iterator2;

                    if( iterator2.group_pointer->free_list_head != std::numeric_limits<skipfield_type>::max() ) {
                        // if this group already has some erased elements
                        *( reinterpret_cast<skipfield_pointer_type>( iterator2.group_pointer->elements +
                                iterator2.group_pointer->free_list_head ) + 1 ) = index;
                    } else {
                        // add it to the groups-with-erasures free list
                        iterator2.group_pointer->erasures_list_next_group = groups_with_erasures_list_head;
                        groups_with_erasures_list_head = iterator2.group_pointer;
                    }

                    *( reinterpret_cast<skipfield_pointer_type>( current_saved.element_pointer ) ) =
                        iterator2.group_pointer->free_list_head;
                    *( reinterpret_cast<skipfield_pointer_type>( current_saved.element_pointer ) + 1 ) =
                        std::numeric_limits<skipfield_type>::max();
                    iterator2.group_pointer->free_list_head = index;
                } else { // If iterator 1 & 2 are in same group, but iterator 1 was not at start of group, and previous skipfield node is an end node in a skipblock:
                    // Just update existing skipblock, no need to create new free list node:
                    const skipfield_type prev_node_value = *( current_saved.skipfield_pointer - 1 );
                    *( current_saved.skipfield_pointer - prev_node_value ) = prev_node_value + distance_to_iterator2;
                    *( iterator2.skipfield_pointer - 1 ) = prev_node_value + distance_to_iterator2;
                }

                if( iterator1.element_pointer == begin_iterator.element_pointer ) {
                    begin_iterator = iterator2;
                }

                iterator2.group_pointer->number_of_elements -= static_cast<skipfield_type>
                        ( number_of_group_erasures );
                total_number_of_elements -= number_of_group_erasures;
            } else { // ie. full group erasure
                if COLONY_CONSTEXPR( !( std::is_trivially_destructible<element_type>::value ) ) {
                    while( current.element_pointer != iterator2.element_pointer ) {
                        COLONY_DESTROY( element_allocator_type, ( *this ),
                                        reinterpret_cast<pointer>( current.element_pointer ) );
                        ++current.skipfield_pointer;
                        current.element_pointer += 1 + *current.skipfield_pointer;
                        current.skipfield_pointer += *current.skipfield_pointer;
                    }
                }

                if( ( total_number_of_elements -= current.group_pointer->number_of_elements ) != 0 ) {
                    // ie. previous_group != NULL
                    current.group_pointer->previous_group->next_group = current.group_pointer->next_group;
                    end_iterator.group_pointer = current.group_pointer->previous_group;
                    end_iterator.element_pointer = current.group_pointer->previous_group->last_endpoint;
                    end_iterator.skipfield_pointer = current.group_pointer->previous_group->skipfield +
                                                     current.group_pointer->previous_group->capacity;
                    total_capacity -= current.group_pointer->capacity;

                    if( current.group_pointer->free_list_head != std::numeric_limits<skipfield_type>::max() ) {
                        remove_from_groups_with_erasures_list( current.group_pointer );
                    }
                } else { // ie. colony is now empty
                    blank();
                }

                COLONY_DESTROY( group_allocator_type, group_allocator_pair, current.group_pointer );
                COLONY_DEALLOCATE( group_allocator_type, group_allocator_pair, current.group_pointer, 1 );
            }
        }

        /** Returns a boolean indicating whether the colony is currently empty of elements. */
        inline COLONY_FORCE_INLINE bool empty() const noexcept {
            return total_number_of_elements == 0;
        }

        /** Returns total number of elements currently stored in container. */
        inline size_type size() const noexcept {
            return total_number_of_elements;
        }

        /** Returns the maximum number of elements that the allocator can store in the container. */
        inline size_type max_size() const noexcept {
            return std::allocator_traits<element_allocator_type>::max_size( *this );
        }

        /** Returns total number of elements currently able to be stored in container without expansion. */
        inline size_type capacity() const noexcept {
            return total_capacity;
        }

        /**
         * Returns the approximate memory use of the container plus it's elements. Will be
         * inaccurate if the elements themselves dynamically-allocate data. Utility function
         * primarily for benchmarking.
         */
        inline size_type approximate_memory_use() const noexcept {
            return
                sizeof( *this ) + // sizeof colony basic structure
                ( total_capacity * ( sizeof( aligned_element_type ) + sizeof( skipfield_type ) ) )
                + // sizeof current colony data capacity + skipfields
                ( ( end_iterator.group_pointer == NULL ) ? 0 : ( ( end_iterator.group_pointer->group_number + 1 )
                        * ( sizeof( group ) + sizeof(
                                skipfield_type ) ) ) ); // if colony not empty, add the memory usage of the group structures themselves, adding the extra skipfield node
        }

        /**
         * Changes the minimum and maximum internal group sizes, in terms of number of elements
         * stored per group. If the colony is not empty and either min_group_size is larger than the
         * smallest group in the colony, or max_group_size is smaller than the largest group in the
         * colony, the colony will be internally copy-constructed into a new colony which uses the
         * new group sizes, invalidating all pointers/iterators/references. If trying to change
         * group sizes with a colony storing a non-copyable/movable type, please use the
         * reinitialize function instead.
         */
        void change_group_sizes( const skipfield_type min_allocation_amount,
                                 const skipfield_type max_allocation_amount ) {
            assert( ( min_allocation_amount > 2 ) & ( min_allocation_amount <= max_allocation_amount ) );

            pointer_allocator_pair.min_elements_per_group = min_allocation_amount;
            group_allocator_pair.max_elements_per_group = max_allocation_amount;

            if( begin_iterator.group_pointer != nullptr &&
                ( begin_iterator.group_pointer->capacity < min_allocation_amount ||
                  end_iterator.group_pointer->capacity > max_allocation_amount ) ) {
                consolidate();
            }
        }

        /**
         * Changes the minimum internal group size only, in terms of minimum number of elements
         * stored per group. If the colony is not empty and min_group_size is larger than the
         * smallest group in the colony, the colony will be internally move-constructed (if possible)
         * or copy-constructed into a new colony which uses the new minimum group size, invalidating
         * all pointers/iterators/references. If trying to change group sizes with a colony storing
         * a non-copyable/movable type, please use the reinitialize function instead.
         */
        inline void change_minimum_group_size( const skipfield_type min_allocation_amount ) {
            change_group_sizes( min_allocation_amount, group_allocator_pair.max_elements_per_group );
        }

        /**
         * Changes the maximum internal group size only, in terms of maximum number of elements
         * stored per group. If the colony is not empty and either max_group_size is smaller than
         * the largest group in the colony, the colony will be internally move-constructed (if
         * possible) or copy-constructed into a new colony which uses the new maximum group size,
         * invalidating all pointers/iterators/references. If trying to change group sizes with a
         * colony storing a non-copyable/movable type, please use the reinitialize function instead.
         */
        inline void change_maximum_group_size( const skipfield_type max_allocation_amount ) {
            change_group_sizes( pointer_allocator_pair.min_elements_per_group, max_allocation_amount );
        }

        /**
         * Sets @param minimum_group_size and @param maximum_group_size to the minimum and maximum
         * group size respectively
         */
        inline void get_group_sizes( skipfield_type &minimum_group_size,
                                     skipfield_type &maximum_group_size ) const noexcept {
            minimum_group_size = pointer_allocator_pair.min_elements_per_group;
            maximum_group_size = group_allocator_pair.max_elements_per_group;
        }

        /**
         * Semantics of this function are the same as
         *     clear();
         *     change_group_sizes(min_group_size, max_group_size);
         * but without the move/copy-construction code of the change_group_sizes() function - this
         * means it can be used with element types which are non-copy-constructible and
         * non-move-constructible, unlike change_group_sizes().
         */
        inline void reinitialize( const skipfield_type min_allocation_amount,
                                  const skipfield_type max_allocation_amount ) noexcept {
            assert( ( min_allocation_amount > 2 ) & ( min_allocation_amount <= max_allocation_amount ) );
            clear();
            pointer_allocator_pair.min_elements_per_group = min_allocation_amount;
            group_allocator_pair.max_elements_per_group = max_allocation_amount;
        }

        /**
         * Empties the colony and removes all elements and groups.
         */
        inline COLONY_FORCE_INLINE void clear() noexcept {
            destroy_all_data();
            blank();
        }

        /**
         * Copy assignment:
         * Copy the elements from another colony to this colony, clearing this colony of existing
         * elements first.
         */
        inline colony &operator=( const colony &source ) {
            assert( &source != this );

            destroy_all_data();
            colony temp( source );
            // Avoid generating 2nd temporary
            *this = std::move( temp );

            return *this;
        }

        /**
         * Move assignment:
         * Move the elements from another colony to this colony, clearing this colony of existing
         * elements first. Source colony is now empty and in a valid state (same as a new colony
         * without any insertions), can be safely destructed or used in any regular way without
         * problems.
         */
        colony &operator=( colony &&source ) COLONY_NOEXCEPT_MOVE_ASSIGNMENT( allocator_type ) {
            assert( &source != this );
            destroy_all_data();

            if COLONY_CONSTEXPR( std::is_trivial<group_pointer_type>::value &&
                                 std::is_trivial<aligned_pointer_type>::value && std::is_trivial<skipfield_pointer_type>::value ) {
                // NOLINTNEXTLINE(bugprone-undefined-memory-manipulation)
                std::memcpy( static_cast<void *>( this ), &source, sizeof( colony ) );
            } else {
                end_iterator = std::move( source.end_iterator );
                begin_iterator = std::move( source.begin_iterator );
                groups_with_erasures_list_head = source.groups_with_erasures_list_head;
                total_number_of_elements = source.total_number_of_elements;
                total_capacity = source.total_capacity;
                pointer_allocator_pair.min_elements_per_group =
                    source.pointer_allocator_pair.min_elements_per_group;
                group_allocator_pair.max_elements_per_group = source.group_allocator_pair.max_elements_per_group;
            }

            source.blank();
            return *this;
        }

        /** Compare contents of another colony to this colony. Returns true if they are equal. */
        bool operator==( const colony &rh ) const noexcept {
            assert( this != &rh );

            if( total_number_of_elements != rh.total_number_of_elements ) {
                return false;
            }

            for( iterator lh_iterator = begin_iterator, rh_iterator = rh.begin_iterator;
                 lh_iterator != end_iterator; ) {
                if( *rh_iterator++ != *lh_iterator++ ) {
                    return false;
                }
            }

            return true;
        }

        /** Compare contents of another colony to this colony. Returns true if they are unequal. */
        inline bool operator!=( const colony &rh ) const noexcept {
            return !( *this == rh );
        }

        /**
         * Reduces container capacity to the amount necessary to store all currently stored
         * elements, consolidates elements and removes any erased locations. If the total number of
         * elements is larger than the maximum group size, the resultant capacity will be equal to
         * ((total_elements / max_group_size) + 1) * max_group_size (rounding down at division).
         * Invalidates all pointers, iterators and references to elements within the container.
         */
        void shrink_to_fit() {
            if( total_number_of_elements == total_capacity ) {
                return;
            } else if( total_number_of_elements == 0 ) { // Edge case
                clear();
                return;
            }

            consolidate();
        }

        /**
         * Preallocates memory space sufficient to store the number of elements indicated by
         * reserve_amount. The maximum size for this number is currently limited to the maximum
         * group size of the colony and will be rounded down if necessary. The default maximum group
         * size is 65535 on the majority of platforms. The default minimum reserve amount is the
         * same as the current minimum group size, and will be rounded up silently if necessary.
         * This function is useful from a performance perspective when the user is inserting
         * elements singly, but the overall number of insertions is known in advance. By reserving,
         * colony can forgo creating many smaller memory block allocations (due to colony's growth
         * factor) and reserve a single memory block instead. Alternatively one could simply change
         * the default group sizes.
         */
        void reserve( const size_type original_reserve_amount ) {
            if( original_reserve_amount == 0 || original_reserve_amount <= total_capacity ) {
                // Already have enough space allocated
                return;
            }

            skipfield_type reserve_amount;

            if( original_reserve_amount > static_cast<size_type>
                ( group_allocator_pair.max_elements_per_group ) ) {
                reserve_amount = group_allocator_pair.max_elements_per_group;
            } else if( original_reserve_amount < static_cast<size_type>
                       ( pointer_allocator_pair.min_elements_per_group ) ) {
                reserve_amount = pointer_allocator_pair.min_elements_per_group;
            } else if( original_reserve_amount > max_size() ) {
                reserve_amount = static_cast<skipfield_type>( max_size() );
            } else {
                reserve_amount = static_cast<skipfield_type>( original_reserve_amount );
            }

            if( total_number_of_elements == 0 ) { // Most common scenario - empty colony
                if( begin_iterator.group_pointer != nullptr ) {
                    // Edge case - empty colony but first group is initialized ie. had some insertions but all elements got subsequently erased
                    COLONY_DESTROY( group_allocator_type, group_allocator_pair, begin_iterator.group_pointer );
                    COLONY_DEALLOCATE( group_allocator_type, group_allocator_pair, begin_iterator.group_pointer, 1 );
                } // else: Empty colony, no insertions yet, time to allocate

                initialize( reserve_amount );
                // last_endpoint initially == elements + 1 via default constructor
                begin_iterator.group_pointer->last_endpoint = begin_iterator.group_pointer->elements;
                begin_iterator.group_pointer->number_of_elements = 0; // 1 by default
            } else { // Non-empty colony, don't have enough space allocated
                const skipfield_type original_min_elements = pointer_allocator_pair.min_elements_per_group;
                // Make sure all groups are at maximum appropriate capacity (this amount already rounded down to a skipfield type earlier in function)
                pointer_allocator_pair.min_elements_per_group = static_cast<skipfield_type>( reserve_amount );
                consolidate();
                pointer_allocator_pair.min_elements_per_group = original_min_elements;
            }
        }

        /**
         * Increments/decrements the iterator supplied by the positive or negative amount indicated
         * by distance. Speed of incrementation will almost always be faster than using the ++
         * operator on the iterator for increments greater than 1. In some cases it may approximate
         * O(1). The iterator_type can be an iterator, const_iterator, reverse_iterator or
         * const_reverse_iterator.
         */
        // Advance implementation for iterator and const_iterator:
        // Cannot be noexcept due to the possibility of an uninitialized iterator
        template <bool is_const>
        void advance( colony_iterator<is_const> &it, difference_type distance ) const {
            // For code simplicity - should hopefully be optimized out by compiler:
            group_pointer_type &group_pointer = it.group_pointer;
            aligned_pointer_type &element_pointer = it.element_pointer;
            skipfield_pointer_type &skipfield_pointer = it.skipfield_pointer;

            // covers uninitialized colony_iterator && empty group
            assert( group_pointer != nullptr );

            // Now, run code based on the nature of the distance type - negative, positive or zero:
            if( distance > 0 ) { // ie. +=
                // Code explanation:
                // For the initial state of the iterator, we don't know how what elements have been erased before that element in that group.
                // So for the first group, we follow the following logic:
                // 1. If no elements have been erased in the group, we do simple addition to progress either to within the group (if the distance is small enough) or the end of the group and subtract from distance accordingly.
                // 2. If any of the first group elements have been erased, we manually iterate, as we don't know whether the erased elements occur before or after the initial iterator position, and we subtract 1 from the distance amount each time. Iteration continues until either distance becomes zero, or we reach the end of the group.

                // For all subsequent groups, we follow this logic:
                // 1. If distance is larger than the total number of non-erased elements in a group, we skip that group and subtract the number of elements in that group from distance
                // 2. If distance is smaller than the total number of non-erased elements in a group, then:
                //    a. if there're no erased elements in the group we simply add distance to group->elements to find the new location for the iterator
                //    b. if there are erased elements in the group, we manually iterate and subtract 1 from distance on each iteration, until the new iterator location is found ie. distance = 0

                // Note: incrementing element_pointer is avoided until necessary to avoid needless calculations

                // Check that we're not already at end()
                assert( !( element_pointer == group_pointer->last_endpoint &&
                           group_pointer->next_group == nullptr ) );

                // Special case for initial element pointer and initial group (we don't know how far into the group the element pointer is)
                if( element_pointer != group_pointer->elements + * ( group_pointer->skipfield ) ) {
                    // ie. != first non-erased element in group
                    const difference_type distance_from_end = static_cast<difference_type>
                            ( group_pointer->last_endpoint - element_pointer );

                    if( group_pointer->number_of_elements == static_cast<skipfield_type>( distance_from_end ) ) {
                        // ie. if there are no erasures in the group (using endpoint - elements_start to determine number of elements in group just in case this is the last group of the colony, in which case group->last_endpoint != group->elements + group->capacity)
                        if( distance < distance_from_end ) {
                            element_pointer += distance;
                            skipfield_pointer += distance;
                            return;
                        } else if( group_pointer->next_group == nullptr ) {
                            // either we've reached end() or gone beyond it, so bound to end()
                            element_pointer = group_pointer->last_endpoint;
                            skipfield_pointer += distance_from_end;
                            return;
                        } else {
                            distance -= distance_from_end;
                        }
                    } else {
                        const skipfield_pointer_type endpoint = skipfield_pointer + distance_from_end;

                        while( true ) {
                            ++skipfield_pointer;
                            skipfield_pointer += *skipfield_pointer;
                            --distance;

                            if( skipfield_pointer == endpoint ) {
                                break;
                            } else if( distance == 0 ) {
                                element_pointer = group_pointer->elements + ( skipfield_pointer - group_pointer->skipfield );
                                return;
                            }
                        }

                        if( group_pointer->next_group == nullptr ) {
                            // either we've reached end() or gone beyond it, so bound to end()
                            element_pointer = group_pointer->last_endpoint;
                            return;
                        }
                    }

                    group_pointer = group_pointer->next_group;

                    if( distance == 0 ) {
                        element_pointer = group_pointer->elements + *( group_pointer->skipfield );
                        skipfield_pointer = group_pointer->skipfield + *( group_pointer->skipfield );
                        return;
                    }
                }

                // Intermediary groups - at the start of this code block and the subsequent block, the position of the iterator is assumed to be the first non-erased element in the current group:
                while( static_cast<difference_type>( group_pointer->number_of_elements ) <= distance ) {
                    if( group_pointer->next_group == nullptr ) {
                        // either we've reached end() or gone beyond it, so bound to end()
                        element_pointer = group_pointer->last_endpoint;
                        skipfield_pointer = group_pointer->skipfield + ( group_pointer->last_endpoint -
                                            group_pointer->elements );
                        return;
                    } else if( ( distance -= group_pointer->number_of_elements ) == 0 ) {
                        group_pointer = group_pointer->next_group;
                        element_pointer = group_pointer->elements + *( group_pointer->skipfield );
                        skipfield_pointer = group_pointer->skipfield + *( group_pointer->skipfield );
                        return;
                    } else {
                        group_pointer = group_pointer->next_group;
                    }
                }

                // Final group (if not already reached):
                if( group_pointer->free_list_head == std::numeric_limits<skipfield_type>::max() ) {
                    // No erasures in this group, use straight pointer addition
                    element_pointer = group_pointer->elements + distance;
                    skipfield_pointer = group_pointer->skipfield + distance;
                    return;
                } else { // ie. number_of_elements > distance - safe to ignore endpoint check condition while incrementing:
                    skipfield_pointer = group_pointer->skipfield + *( group_pointer->skipfield );

                    do {
                        ++skipfield_pointer;
                        skipfield_pointer += *skipfield_pointer;
                    } while( --distance != 0 );

                    element_pointer = group_pointer->elements + ( skipfield_pointer - group_pointer->skipfield );
                    return;
                }

                return;
            } else if( distance < 0 ) { // for negative change
                // Code logic is very similar to += above

                // check that we're not already at begin()
                assert( !( ( element_pointer == group_pointer->elements + * ( group_pointer->skipfield ) ) &&
                           group_pointer->previous_group == nullptr ) );
                distance = -distance;

                // Special case for initial element pointer and initial group (we don't know how far into the group the element pointer is)
                if( element_pointer != group_pointer->last_endpoint ) { // ie. != end()
                    if( group_pointer->free_list_head == std::numeric_limits<skipfield_type>::max() ) {
                        // ie. no prior erasures have occurred in this group
                        const difference_type distance_from_beginning = static_cast<difference_type>
                                ( element_pointer - group_pointer->elements );

                        if( distance <= distance_from_beginning ) {
                            element_pointer -= distance;
                            skipfield_pointer -= distance;
                            return;
                        } else if( group_pointer->previous_group == nullptr ) {
                            // ie. we've gone before begin(), so bound to begin()
                            element_pointer = group_pointer->elements;
                            skipfield_pointer = group_pointer->skipfield;
                            return;
                        } else {
                            distance -= distance_from_beginning;
                        }
                    } else {
                        const skipfield_pointer_type beginning_point = group_pointer->skipfield + *
                                ( group_pointer->skipfield );

                        while( skipfield_pointer != beginning_point ) {
                            --skipfield_pointer;
                            skipfield_pointer -= *skipfield_pointer;

                            if( --distance == 0 ) {
                                element_pointer = group_pointer->elements + ( skipfield_pointer - group_pointer->skipfield );
                                return;
                            }
                        }

                        if( group_pointer->previous_group == nullptr ) {
                            // This is first group, so bound to begin() (just in case final decrement took us before begin())
                            element_pointer = group_pointer->elements + *( group_pointer->skipfield );
                            skipfield_pointer = group_pointer->skipfield + *( group_pointer->skipfield );
                            return;
                        }
                    }

                    group_pointer = group_pointer->previous_group;
                }

                // Intermediary groups - at the start of this code block and the subsequent block, the position of the iterator is assumed to be either the first non-erased element in the next group over, or end():
                while( static_cast<difference_type>( group_pointer->number_of_elements ) < distance ) {
                    if( group_pointer->previous_group == nullptr ) {
                        // we've gone beyond begin(), so bound to it
                        element_pointer = group_pointer->elements + *( group_pointer->skipfield );
                        skipfield_pointer = group_pointer->skipfield + *( group_pointer->skipfield );
                        return;
                    }

                    distance -= group_pointer->number_of_elements;
                    group_pointer = group_pointer->previous_group;
                }

                // Final group (if not already reached):
                if( static_cast<difference_type>( group_pointer->number_of_elements ) == distance ) {
                    element_pointer = group_pointer->elements + *( group_pointer->skipfield );
                    skipfield_pointer = group_pointer->skipfield + *( group_pointer->skipfield );
                    return;
                } else if( group_pointer->free_list_head == std::numeric_limits<skipfield_type>::max() ) {
                    // ie. no erased elements in this group
                    element_pointer = reinterpret_cast<aligned_pointer_type>( group_pointer->skipfield ) - distance;
                    skipfield_pointer = ( group_pointer->skipfield + group_pointer->capacity ) - distance;
                    return;
                } else { // ie. no more groups to traverse but there are erased elements in this group
                    skipfield_pointer = group_pointer->skipfield + group_pointer->capacity;

                    do {
                        --skipfield_pointer;
                        skipfield_pointer -= *skipfield_pointer;
                    } while( --distance != 0 );

                    element_pointer = group_pointer->elements + ( skipfield_pointer - group_pointer->skipfield );
                    return;
                }
            }
            // Only distance == 0 reaches here
        }

        // Advance for reverse_iterator and const_reverse_iterator - this needs to be implemented slightly differently to forward-iterator's advance, as it needs to be able to reach rend() (ie. begin() - 1) and to be bounded by rbegin():
        template <bool is_const>
        void advance( colony_reverse_iterator<is_const> &reverse_it,
                      difference_type distance ) const { // could cause exception if iterator is uninitialized
            group_pointer_type &group_pointer = reverse_it.it.group_pointer;
            aligned_pointer_type &element_pointer = reverse_it.it.element_pointer;
            skipfield_pointer_type &skipfield_pointer = reverse_it.it.skipfield_pointer;

            assert( element_pointer != nullptr );

            if( distance > 0 ) {
                // Check that we're not already at rend()
                assert( !( element_pointer == group_pointer->elements - 1 &&
                           group_pointer->previous_group == nullptr ) );
                // Special case for initial element pointer and initial group (we don't know how far into the group the element pointer is)
                // Since a reverse_iterator cannot == last_endpoint (ie. before rbegin()) we don't need to check for that like with iterator
                if( group_pointer->free_list_head == std::numeric_limits<skipfield_type>::max() ) {
                    difference_type distance_from_beginning = static_cast<difference_type>
                            ( element_pointer - group_pointer->elements );

                    if( distance <= distance_from_beginning ) {
                        element_pointer -= distance;
                        skipfield_pointer -= distance;
                        return;
                    } else if( group_pointer->previous_group == nullptr ) {
                        // Either we've reached rend() or gone beyond it, so bound to rend()
                        element_pointer = group_pointer->elements - 1;
                        skipfield_pointer = group_pointer->skipfield - 1;
                        return;
                    } else {
                        distance -= distance_from_beginning;
                    }
                } else {
                    const skipfield_pointer_type beginning_point = group_pointer->skipfield + *
                            ( group_pointer->skipfield );

                    while( skipfield_pointer != beginning_point ) {
                        --skipfield_pointer;
                        skipfield_pointer -= *skipfield_pointer;

                        if( --distance == 0 ) {
                            element_pointer = group_pointer->elements + ( skipfield_pointer - group_pointer->skipfield );
                            return;
                        }
                    }

                    if( group_pointer->previous_group == nullptr ) {
                        // If we've reached rend(), bound to that
                        element_pointer = group_pointer->elements - 1;
                        skipfield_pointer = group_pointer->skipfield - 1;
                        return;
                    }
                }

                group_pointer = group_pointer->previous_group;

                // Intermediary groups - at the start of this code block and the subsequent block, the position of the iterator is assumed to be the first non-erased element in the next group:
                while( static_cast<difference_type>( group_pointer->number_of_elements ) < distance ) {
                    if( group_pointer->previous_group == nullptr ) { // bound to rend()
                        element_pointer = group_pointer->elements - 1;
                        skipfield_pointer = group_pointer->skipfield - 1;
                        return;
                    }

                    distance -= static_cast<difference_type>( group_pointer->number_of_elements );
                    group_pointer = group_pointer->previous_group;
                }

                // Final group (if not already reached)
                if( static_cast<difference_type>( group_pointer->number_of_elements ) == distance ) {
                    element_pointer = group_pointer->elements + *( group_pointer->skipfield );
                    skipfield_pointer = group_pointer->skipfield + *( group_pointer->skipfield );
                    return;
                } else if( group_pointer->free_list_head == std::numeric_limits<skipfield_type>::max() ) {
                    element_pointer = reinterpret_cast<aligned_pointer_type>( group_pointer->skipfield ) - distance;
                    skipfield_pointer = ( group_pointer->skipfield + group_pointer->capacity ) - distance;
                    return;
                } else {
                    skipfield_pointer = group_pointer->skipfield + group_pointer->capacity;

                    do {
                        --skipfield_pointer;
                        skipfield_pointer -= *skipfield_pointer;
                    } while( --distance != 0 );

                    element_pointer = group_pointer->elements + ( skipfield_pointer - group_pointer->skipfield );
                    return;
                }
            } else if( distance < 0 ) {
                // Check that we're not already at rbegin()
                assert( !( ( element_pointer == ( group_pointer->last_endpoint - 1 ) - *
                             ( group_pointer->skipfield + ( group_pointer->last_endpoint - group_pointer->elements ) - 1 ) ) &&
                           group_pointer->next_group == nullptr ) );

                if( element_pointer != group_pointer->elements + * ( group_pointer->skipfield ) ) {
                    // ie. != first non-erased element in group
                    if( group_pointer->free_list_head == std::numeric_limits<skipfield_type>::max() ) {
                        // ie. if there are no erasures in the group
                        const difference_type distance_from_end = static_cast<difference_type>
                                ( group_pointer->last_endpoint - element_pointer );

                        if( distance < distance_from_end ) {
                            element_pointer += distance;
                            skipfield_pointer += distance;
                            return;
                        } else if( group_pointer->next_group == nullptr ) { // bound to rbegin()
                            // no erasures so we don't have to subtract skipfield value as we do below
                            element_pointer = group_pointer->last_endpoint - 1;
                            skipfield_pointer += distance_from_end - 1;
                            return;
                        } else {
                            distance -= distance_from_end;
                        }
                    } else {
                        const skipfield_pointer_type endpoint = skipfield_pointer + ( group_pointer->last_endpoint -
                                                                element_pointer );

                        while( true ) {
                            ++skipfield_pointer;
                            skipfield_pointer += *skipfield_pointer;
                            --distance;

                            if( skipfield_pointer == endpoint ) {
                                break;
                            } else if( distance == 0 ) {
                                element_pointer = group_pointer->elements + ( skipfield_pointer - group_pointer->skipfield );
                                return;
                            }
                        }

                        if( group_pointer->next_group == nullptr ) { // bound to rbegin()
                            --skipfield_pointer;
                            element_pointer = ( group_pointer->last_endpoint - 1 ) - *skipfield_pointer;
                            skipfield_pointer -= *skipfield_pointer;
                            return;
                        }
                    }

                    group_pointer = group_pointer->next_group;

                    if( distance == 0 ) {
                        element_pointer = group_pointer->elements + *( group_pointer->skipfield );
                        skipfield_pointer = group_pointer->skipfield + *( group_pointer->skipfield );
                        return;
                    }
                }

                // Intermediary groups - at the start of this code block and the subsequent block, the position of the iterator is assumed to be the first non-erased element in the current group, as a result of the previous code blocks:
                while( static_cast<difference_type>( group_pointer->number_of_elements ) <= distance ) {
                    if( group_pointer->next_group == nullptr ) { // bound to rbegin()
                        skipfield_pointer = group_pointer->skipfield + ( group_pointer->last_endpoint -
                                            group_pointer->elements ) - 1;
                        --skipfield_pointer;
                        element_pointer = ( group_pointer->last_endpoint - 1 ) - *skipfield_pointer;
                        skipfield_pointer -= *skipfield_pointer;
                        return;
                    } else if( ( distance -= group_pointer->number_of_elements ) == 0 ) {
                        group_pointer = group_pointer->next_group;
                        element_pointer = group_pointer->elements + *( group_pointer->skipfield );
                        skipfield_pointer = group_pointer->skipfield + *( group_pointer->skipfield );
                        return;
                    } else {
                        group_pointer = group_pointer->next_group;
                    }
                }

                // Final group (if not already reached):
                if( group_pointer->free_list_head == std::numeric_limits<skipfield_type>::max() ) {
                    // No erasures in this group, use straight pointer addition
                    element_pointer = group_pointer->elements + distance;
                    skipfield_pointer = group_pointer->skipfield + distance;
                    return;
                } else { // ie. number_of_elements > distance - safe to ignore endpoint check condition while incrementing:
                    skipfield_pointer = group_pointer->skipfield + *( group_pointer->skipfield );

                    do {
                        ++skipfield_pointer;
                        skipfield_pointer += *skipfield_pointer;
                    } while( --distance != 0 );

                    element_pointer = group_pointer->elements + ( skipfield_pointer - group_pointer->skipfield );
                    return;
                }

                return;
            }
        }

        /**
         * Creates a copy of the iterator supplied, then increments/decrements this iterator by
         * the positive or negative amount indicated by distance.
         */
        template <bool is_const>
        inline colony_iterator<is_const> next( const colony_iterator<is_const> &it,
                                               const typename colony_iterator<is_const>::difference_type distance = 1 ) const {
            colony_iterator<is_const> return_iterator( it );
            advance( return_iterator, distance );
            return return_iterator;
        }

        /**
         * Creates a copy of the iterator supplied, then increments/decrements this iterator by
         * the positive or negative amount indicated by distance.
         */
        template <bool is_const>
        inline colony_reverse_iterator<is_const> next( const colony_reverse_iterator<is_const> &it,
                const typename colony_reverse_iterator<is_const>::difference_type distance = 1 ) const {
            colony_reverse_iterator<is_const> return_iterator( it );
            advance( return_iterator, distance );
            return return_iterator;
        }

        /**
         * Creates a copy of the iterator supplied, then decrements/increments this iterator by the
         * positive or negative amount indicated by distance.
         */
        template <bool is_const>
        inline colony_iterator<is_const> prev( const colony_iterator<is_const> &it,
                                               const typename colony_iterator<is_const>::difference_type distance = 1 ) const {
            colony_iterator<is_const> return_iterator( it );
            advance( return_iterator, -distance );
            return return_iterator;
        }

        /**
         * Creates a copy of the iterator supplied, then decrements/increments this iterator by the
         * positive or negative amount indicated by distance.
         */
        template <bool is_const>
        inline colony_reverse_iterator<is_const> prev( const colony_reverse_iterator<is_const> &it,
                const typename colony_reverse_iterator<is_const>::difference_type distance = 1 ) const {
            colony_reverse_iterator<is_const> return_iterator( it );
            advance( return_iterator, -distance );
            return return_iterator;
        }

        /**
         * Measures the distance between two iterators, returning the result, which will be negative
         * if the second iterator supplied is before the first iterator supplied in terms of its
         * location in the colony.
         */
        template <bool is_const>
        typename colony_iterator<is_const>::difference_type distance( const colony_iterator<is_const>
                &first, const colony_iterator<is_const> &last ) const {
            // Code logic:
            // If iterators are the same, return 0
            // Otherwise, find which iterator is later in colony, copy that to iterator2. Copy the lower to iterator1.
            // If they are not pointing to elements in the same group, process the intermediate groups and add distances,
            // skipping manual incrementation in all but the initial and final groups.
            // In the initial and final groups, manual incrementation must be used to calculate distance, if there have been no prior erasures in those groups.
            // If there are no prior erasures in either of those groups, we can use pointer arithmetic to calculate the distances for those groups.

            assert( !( first.group_pointer == nullptr ) &&
                    !( last.group_pointer == nullptr ) ); // Check that they are initialized

            if( last.element_pointer == first.element_pointer ) {
                return 0;
            }

            using iterator_type = colony_iterator<is_const>;
            using diff_type = typename iterator_type::difference_type;
            diff_type distance = 0;

            iterator_type iterator1 = first, iterator2 = last;
            const bool swap = first > last;

            if( swap ) { // Less common case
                iterator1 = last;
                iterator2 = first;
            }

            if( iterator1.group_pointer != iterator2.group_pointer ) {
                // if not in same group, process intermediate groups

                // Process initial group:
                if( iterator1.group_pointer->free_list_head == std::numeric_limits<skipfield_type>::max() ) {
                    // If no prior erasures have occured in this group we can do simple addition
                    distance += static_cast<diff_type>( iterator1.group_pointer->last_endpoint -
                                                        iterator1.element_pointer );
                } else if( iterator1.element_pointer == iterator1.group_pointer->elements ) {
                    // ie. element is at start of group - rare case
                    distance += static_cast<diff_type>( iterator1.group_pointer->number_of_elements );
                } else {
                    const skipfield_pointer_type endpoint = iterator1.skipfield_pointer +
                                                            ( iterator1.group_pointer->last_endpoint - iterator1.element_pointer );

                    while( iterator1.skipfield_pointer != endpoint ) {
                        iterator1.skipfield_pointer += *( ++iterator1.skipfield_pointer );
                        ++distance;
                    }
                }

                // Process all other intermediate groups:
                iterator1.group_pointer = iterator1.group_pointer->next_group;

                while( iterator1.group_pointer != iterator2.group_pointer ) {
                    distance += static_cast<diff_type>( iterator1.group_pointer->number_of_elements );
                    iterator1.group_pointer = iterator1.group_pointer->next_group;
                }

                iterator1.skipfield_pointer = iterator1.group_pointer->skipfield;
            }

            if( iterator1.group_pointer->free_list_head == std::numeric_limits<skipfield_type>::max() ) {
                // ie. no erasures in this group, direct subtraction is possible
                distance += static_cast<diff_type>( iterator2.skipfield_pointer - iterator1.skipfield_pointer );
            } else if( iterator1.group_pointer->last_endpoint - 1 >= iterator2.element_pointer ) {
                // ie. if iterator2 is .end() or 1 before
                distance += static_cast<diff_type>( iterator1.group_pointer->number_of_elements -
                                                    ( iterator1.group_pointer->last_endpoint - iterator2.element_pointer ) );
            } else {
                while( iterator1.skipfield_pointer != iterator2.skipfield_pointer ) {
                    iterator1.skipfield_pointer += *( ++iterator1.skipfield_pointer );
                    ++distance;
                }
            }

            if( swap ) {
                distance = -distance;
            }

            return distance;
        }

        template <bool is_const>
        inline typename colony_reverse_iterator<is_const>::difference_type distance(
            const colony_reverse_iterator<is_const> &first,
            const colony_reverse_iterator<is_const> &last ) const {
            return distance( last.it, first.it );
        }

        // Type-changing functions:

        /**
         * Getting a pointer from an iterator is simple - simply dereference it then grab the
         * address ie. "&(*the_iterator);". Getting an iterator from a pointer is typically not so
         * simple. This function enables the user to do exactly that. This is expected to be useful
         * in the use-case where external containers are storing pointers to colony elements instead
         * of iterators (as iterators for colonies have 3 times the size of an element pointer) and
         * the program wants to erase the element being pointed to or possibly change the element
         * being pointed to. Converting a pointer to an iterator using this method and then erasing,
         * is about 20% slower on average than erasing when you already have the iterator. This is
         * less dramatic than it sounds, as it is still faster than all std:: container erasure
         * times which it is roughly equal to. However this is generally a slower, lookup-based
         * operation. If the lookup doesn't find a non-erased element based on that pointer, it
         * returns end(). Otherwise it returns an iterator pointing to the element in question.
         *
         * Cannot be noexcept as colony could be empty or pointer invalid
         */
        iterator get_iterator_from_pointer( const pointer element_pointer ) const {
            assert( !empty() );
            assert( element_pointer != nullptr );

            // Start with last group first, as will be the largest group
            group_pointer_type current_group = end_iterator.group_pointer;

            while( current_group != nullptr ) {
                if( reinterpret_cast<aligned_pointer_type>( element_pointer ) >= current_group->elements &&
                    reinterpret_cast<aligned_pointer_type>( element_pointer ) < reinterpret_cast<aligned_pointer_type>
                    ( current_group->skipfield ) ) {
                    const skipfield_pointer_type skipfield_pointer = current_group->skipfield +
                            ( reinterpret_cast<aligned_pointer_type>( element_pointer ) - current_group->elements );
                    return ( *skipfield_pointer == 0 ) ? iterator( current_group,
                            reinterpret_cast<aligned_pointer_type>( element_pointer ),
                            skipfield_pointer ) : end_iterator; // If element has been erased, return end()
                }

                current_group = current_group->previous_group;
            }

            return end_iterator;
        }

        /**
         * While colony is a container with unordered insertion (and is therefore unordered), it
         * still has a (transitory) order which changes upon any erasure or insertion. Temporary
         * index numbers are therefore obtainable. These can be useful, for example, when creating
         * a save file in a computer game, where certain elements in a container may need to be
         * re-linked to other elements in other container upon reloading the save file.
         *
         * may throw exception if iterator is invalid/uninitialized
         */
        template <bool is_const>
        size_type get_index_from_iterator( const colony_iterator<is_const> &it ) const {
            assert( !empty() );
            assert( it.group_pointer != nullptr );

            // This is essentially a simplified version of distance() optimized for counting from begin()
            size_type index = 0;
            group_pointer_type group_pointer = begin_iterator.group_pointer;

            // For all prior groups, add group sizes
            while( group_pointer != it.group_pointer ) {
                index += static_cast<size_type>( group_pointer->number_of_elements );
                group_pointer = group_pointer->next_group;
            }

            if( group_pointer->free_list_head == std::numeric_limits<skipfield_type>::max() ) {
                // If no erased elements in group exist, do straight pointer arithmetic to get distance to start for first element
                index += static_cast<size_type>( it.element_pointer - group_pointer->elements );
            } else {
                // Otherwise do manual ++ loop - count from beginning of group until location is reached
                skipfield_pointer_type skipfield_pointer = group_pointer->skipfield + *( group_pointer->skipfield );

                while( skipfield_pointer != it.skipfield_pointer ) {
                    ++skipfield_pointer;
                    skipfield_pointer += *skipfield_pointer;
                    ++index;
                }
            }

            return index;
        }

        /**
         * The same as get_index_from_iterator, but for reverse_iterators and const_reverse_iterators.
         * Index is from front of colony (same as iterator), not back of colony.
         */
        template <bool is_const>
        inline size_type get_index_from_reverse_iterator( const colony_reverse_iterator<is_const>
                &rev_iterator ) const {
            return get_index_from_iterator( rev_iterator.it );
        }

        /**
         * As described above, there may be situations where obtaining iterators to specific
         * elements based on an index can be useful, for example, when reloading save files. This
         * function is basically a shorthand to avoid typing
         *     iterator it = colony.begin();
         *     colony.advance(it, index);
         *
         *  Cannot be noexcept as colony could be empty
         */
        template <typename index_type>
        iterator get_iterator_from_index( const index_type index ) const {
            assert( !empty() );
            assert( std::numeric_limits<index_type>::is_integer );

            iterator it( begin_iterator );
            advance( it, static_cast<difference_type>( index ) );
            return it;
        }

        /** Returns a copy of the allocator used by the colony instance. */
        inline allocator_type get_allocator() const noexcept {
            return element_allocator_type();
        }

    private:

        struct less {
            bool operator()( const element_type &a, const element_type &b ) const noexcept {
                return a < b;
            }
        };

        // Function-object, used to redirect the sort function to compare element pointers by the elements they point to, and sort the element pointers instead of the elements:
        template <class comparison_function>
        struct sort_dereferencer {
            comparison_function stored_instance;

            explicit sort_dereferencer( const comparison_function &function_instance ):
                stored_instance( function_instance )
            {}

            sort_dereferencer() noexcept = default;

            bool operator()( const pointer first, const pointer second ) {
                return stored_instance( *first, *second );
            }
        };

    public:

        /**
         * Sort the content of the colony. By default this compares the colony content using a
         * less-than operator, unless the user supplies a comparison function (ie. same conditions
         * as std::list's sort function).
         *
         * Uses std::sort internally.
         */
        template <class comparison_function>
        void sort( comparison_function compare ) {
            if( total_number_of_elements < 2 ) {
                return;
            }

            pointer *const element_pointers = COLONY_ALLOCATE( pointer_allocator_type, pointer_allocator_pair,
                                              total_number_of_elements, nullptr );
            pointer *element_pointer = element_pointers;

            // Construct pointers to all elements in the colony in sequence:
            for( iterator current_element = begin_iterator; current_element != end_iterator;
                 ++current_element ) {
                COLONY_CONSTRUCT( pointer_allocator_type, pointer_allocator_pair, element_pointer++,
                                  &*current_element );
            }

            // Now, sort the pointers by the values they point to:
            std::sort( element_pointers, element_pointers + total_number_of_elements,
                       sort_dereferencer<comparison_function>( compare ) );

            // Create a new colony and copy the elements from the old one to the new one in the order of the sorted pointers:
            colony new_location;
            new_location.change_group_sizes( pointer_allocator_pair.min_elements_per_group,
                                             group_allocator_pair.max_elements_per_group );

            try {
                new_location.reserve( static_cast<skipfield_type>( ( total_number_of_elements >
                                      std::numeric_limits<skipfield_type>::max() ) ? std::numeric_limits<skipfield_type>::max() :
                                      total_number_of_elements ) );

                if COLONY_CONSTEXPR( std::is_move_constructible<element_type>::value ) {
                    for( pointer *current_element_pointer = element_pointers;
                         current_element_pointer != element_pointer; ++current_element_pointer ) {
                        new_location.insert( std::move( *reinterpret_cast<pointer>( *current_element_pointer ) ) );
                    }
                } else {
                    for( pointer *current_element_pointer = element_pointers;
                         current_element_pointer != element_pointer; ++current_element_pointer ) {
                        new_location.insert( *reinterpret_cast<pointer>( *current_element_pointer ) );
                    }
                }
            } catch( ... ) {
                if COLONY_CONSTEXPR( !std::is_trivially_destructible<pointer>::value ) {
                    for( pointer *current_element_pointer = element_pointers;
                         current_element_pointer != element_pointer; ++current_element_pointer ) {
                        COLONY_DESTROY( pointer_allocator_type, pointer_allocator_pair, current_element_pointer );
                    }
                }

                COLONY_DEALLOCATE( pointer_allocator_type, pointer_allocator_pair, element_pointers,
                                   total_number_of_elements );
                throw;
            }

            // Make the old colony the new one, destroy the old one's data/sequence:
            *this = std::move( new_location ); // avoid generating temporary

            if COLONY_CONSTEXPR( !std::is_trivially_destructible<pointer>::value ) {
                for( pointer *current_element_pointer = element_pointers;
                     current_element_pointer != element_pointer; ++current_element_pointer ) {
                    COLONY_DESTROY( pointer_allocator_type, pointer_allocator_pair, current_element_pointer );
                }
            }

            COLONY_DEALLOCATE( pointer_allocator_type, pointer_allocator_pair, element_pointers,
                               total_number_of_elements );
        }

        inline void sort() {
            sort( less() );
        }

        /**
         * Transfer all elements from source colony into destination colony without invalidating
         * pointers/iterators to either colony's elements (in other words the destination takes
         * ownership of the source's memory blocks). After the splice, the source colony is empty.
         * Splicing is much faster than range-moving or copying all elements from one colony to
         * another. Colony does not guarantee a particular order of elements after splicing, for
         * performance reasons; the insertion location of source elements in the destination colony
         * is chosen based on the most positive performance outcome for subsequent iterations/
         * insertions. For example if the destination colony is {1, 2, 3, 4} and the source colony
         * is {5, 6, 7, 8} the destination colony post-splice could be {1, 2, 3, 4, 5, 6, 7, 8} or
         * {5, 6, 7, 8, 1, 2, 3, 4}, depending on internal state of both colonies and prior
         * insertions/erasures.
         */
        void splice( colony &source ) COLONY_NOEXCEPT_SWAP( allocator_type ) {
            // Process: if there are unused memory spaces at the end of the current back group of the chain, convert them
            // to skipped elements and add the locations to the group's free list.
            // Then link the destination's groups to the source's groups and nullify the source.
            // If the source has more unused memory spaces in the back group than the destination, swap them before processing to reduce the number of locations added to a free list and also subsequent jumps during iteration.

            assert( &source != this );

            if( source.total_number_of_elements == 0 ) {
                return;
            } else if( total_number_of_elements == 0 ) {
                *this = std::move( source );
                return;
            }

            // If there's more unused element locations at end of destination than source, swap with source to reduce number of skipped elements and size of free-list:
            if( ( reinterpret_cast<aligned_pointer_type>( end_iterator.group_pointer->skipfield ) -
                  end_iterator.element_pointer ) > ( reinterpret_cast<aligned_pointer_type>
                          ( source.end_iterator.group_pointer->skipfield ) - source.end_iterator.element_pointer ) ) {
                swap( source );
            }

            // Correct group sizes if necessary:
            if( source.pointer_allocator_pair.min_elements_per_group <
                pointer_allocator_pair.min_elements_per_group ) {
                pointer_allocator_pair.min_elements_per_group =
                    source.pointer_allocator_pair.min_elements_per_group;
            }

            if( source.group_allocator_pair.max_elements_per_group >
                group_allocator_pair.max_elements_per_group ) {
                group_allocator_pair.max_elements_per_group = source.group_allocator_pair.max_elements_per_group;
            }

            // Add source list of groups-with-erasures to destination list of groups-with-erasures:
            if( source.groups_with_erasures_list_head != nullptr ) {
                if( groups_with_erasures_list_head != nullptr ) {
                    group_pointer_type tail_group = groups_with_erasures_list_head;

                    while( tail_group->erasures_list_next_group != nullptr ) {
                        tail_group = tail_group->erasures_list_next_group;
                    }

                    tail_group->erasures_list_next_group = source.groups_with_erasures_list_head;
                } else {
                    groups_with_erasures_list_head = source.groups_with_erasures_list_head;
                }
            }

            const skipfield_type distance_to_end = static_cast<skipfield_type>
                                                   ( reinterpret_cast<aligned_pointer_type>( end_iterator.group_pointer->skipfield ) -
                                                           end_iterator.element_pointer );

            if( distance_to_end != 0 ) { // 0 == edge case
                // Mark unused element memory locations from back group as skipped/erased:

                // Update skipfield:
                const skipfield_type previous_node_value = *( end_iterator.skipfield_pointer - 1 );
                end_iterator.group_pointer->last_endpoint = reinterpret_cast<aligned_pointer_type>
                        ( end_iterator.group_pointer->skipfield );

                if( previous_node_value == 0 ) { // no previous skipblock
                    *end_iterator.skipfield_pointer = distance_to_end;
                    *( end_iterator.skipfield_pointer + distance_to_end - 1 ) = distance_to_end;

                    const skipfield_type index = static_cast<skipfield_type>( end_iterator.element_pointer -
                                                 end_iterator.group_pointer->elements );

                    if( end_iterator.group_pointer->free_list_head != std::numeric_limits<skipfield_type>::max() ) {
                        // ie. if this group already has some erased elements
                        // set prev free list head's 'next index' number to the index of the current element
                        *( reinterpret_cast<skipfield_pointer_type>( end_iterator.group_pointer->elements +
                                end_iterator.group_pointer->free_list_head ) + 1 ) = index;
                    } else {
                        // add it to the groups-with-erasures free list
                        end_iterator.group_pointer->erasures_list_next_group = groups_with_erasures_list_head;
                        groups_with_erasures_list_head = end_iterator.group_pointer;
                    }

                    *( reinterpret_cast<skipfield_pointer_type>( end_iterator.element_pointer ) ) =
                        end_iterator.group_pointer->free_list_head;
                    *( reinterpret_cast<skipfield_pointer_type>( end_iterator.element_pointer ) + 1 ) =
                        std::numeric_limits<skipfield_type>::max();
                    end_iterator.group_pointer->free_list_head = index;
                } else {
                    // update previous skipblock, no need to update free list:
                    *( end_iterator.skipfield_pointer - previous_node_value ) = *( end_iterator.skipfield_pointer +
                            distance_to_end - 1 ) = previous_node_value + distance_to_end;
                }
            }

            // Update subsequent group numbers:
            group_pointer_type current_group = source.begin_iterator.group_pointer;
            size_type current_group_number = end_iterator.group_pointer->group_number;

            do {
                current_group->group_number = ++current_group_number;
                current_group = current_group->next_group;
            } while( current_group != nullptr );

            // Join the destination and source group chains:
            end_iterator.group_pointer->next_group = source.begin_iterator.group_pointer;
            source.begin_iterator.group_pointer->previous_group = end_iterator.group_pointer;
            end_iterator = source.end_iterator;
            total_number_of_elements += source.total_number_of_elements;
            source.blank();
        }

        /** Swaps the colony's contents with that of source. */
        void swap( colony &source ) COLONY_NOEXCEPT_SWAP( allocator_type ) {
            assert( &source != this );

            // if all pointer types are trivial we can just copy using memcpy - avoids constructors/destructors etc and is faster
            if COLONY_CONSTEXPR( std::is_trivial<group_pointer_type>::value &&
                                 std::is_trivial<aligned_pointer_type>::value &&
                                 std::is_trivial<skipfield_pointer_type>::value ) {
                char temp[sizeof( colony )];
                std::memcpy( &temp, static_cast<void *>( this ), sizeof( colony ) );
                std::memcpy( static_cast<void *>( this ), static_cast<void *>( &source ), sizeof( colony ) );
                std::memcpy( static_cast<void *>( &source ), &temp, sizeof( colony ) );

                // If pointer types are not trivial, moving them is probably going to be more efficient than copying them below
            } else if COLONY_CONSTEXPR( std::is_move_assignable<group_pointer_type>::value &&
                                        std::is_move_assignable<aligned_pointer_type>::value &&
                                        std::is_move_assignable<skipfield_pointer_type>::value &&
                                        std::is_move_constructible<group_pointer_type>::value &&
                                        std::is_move_constructible<aligned_pointer_type>::value &&
                                        std::is_move_constructible<skipfield_pointer_type>::value ) {
                colony temp( std::move( source ) );
                source = std::move( *this );
                *this = std::move( temp );
            } else {
                const iterator swap_end_iterator = end_iterator;
                const iterator swap_begin_iterator = begin_iterator;
                const group_pointer_type swap_groups_with_erasures_list_head = groups_with_erasures_list_head;
                const size_type swap_total_number_of_elements = total_number_of_elements;
                const size_type swap_total_capacity = total_capacity;
                const skipfield_type swap_min_elements_per_group = pointer_allocator_pair.min_elements_per_group;
                const skipfield_type swap_max_elements_per_group = group_allocator_pair.max_elements_per_group;

                end_iterator = source.end_iterator;
                begin_iterator = source.begin_iterator;
                groups_with_erasures_list_head = source.groups_with_erasures_list_head;
                total_number_of_elements = source.total_number_of_elements;
                total_capacity = source.total_capacity;
                pointer_allocator_pair.min_elements_per_group =
                    source.pointer_allocator_pair.min_elements_per_group;
                group_allocator_pair.max_elements_per_group = source.group_allocator_pair.max_elements_per_group;

                source.end_iterator = swap_end_iterator;
                source.begin_iterator = swap_begin_iterator;
                source.groups_with_erasures_list_head = swap_groups_with_erasures_list_head;
                source.total_number_of_elements = swap_total_number_of_elements;
                source.total_capacity = swap_total_capacity;
                source.pointer_allocator_pair.min_elements_per_group = swap_min_elements_per_group;
                source.group_allocator_pair.max_elements_per_group = swap_max_elements_per_group;
            }
        }

};  // colony

/**
 * Swaps colony A's contents with that of colony B.
 * Assumes both colonies have same element type, allocator type, etc.
 */
template <class element_type, class element_allocator_type, typename element_skipfield_type>
inline void swap( colony<element_type, element_allocator_type, element_skipfield_type> &a,
                  colony<element_type, element_allocator_type, element_skipfield_type> &b ) COLONY_NOEXCEPT_SWAP(
                      element_allocator_type )
{
    a.swap( b );
}

} // namespace cata

#undef COLONY_FORCE_INLINE

#undef COLONY_NOEXCEPT_SWAP
#undef COLONY_NOEXCEPT_MOVE_ASSIGNMENT
#undef COLONY_CONSTEXPR

#undef COLONY_CONSTRUCT
#undef COLONY_DESTROY
#undef COLONY_ALLOCATE
#undef COLONY_ALLOCATE_INITIALIZATION
#undef COLONY_DEALLOCATE

#endif // COLONY_H
