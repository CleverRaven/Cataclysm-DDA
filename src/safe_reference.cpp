#include "safe_reference.h"

static_assert( std::is_nothrow_move_constructible<safe_reference_anchor>::value, "" );
static_assert( std::is_nothrow_move_assignable<safe_reference_anchor>::value, "" );

safe_reference_anchor::safe_reference_anchor()
{
    impl = std::make_shared<empty>();
}

safe_reference_anchor::safe_reference_anchor( const safe_reference_anchor & ) :
    safe_reference_anchor()
{}

// Technically the move constructor and move assignment can throw bad_alloc,
// but we have no way to recover from that so mark it noexcept anyway; will
// cause terminate on out-of-memory.
safe_reference_anchor::safe_reference_anchor( safe_reference_anchor && ) noexcept :
    safe_reference_anchor()
{}

safe_reference_anchor &safe_reference_anchor::operator=( const safe_reference_anchor & )
{
    impl = std::make_shared<empty>();
    return *this;
}

safe_reference_anchor &safe_reference_anchor::operator=( safe_reference_anchor && ) noexcept
{
    impl = std::make_shared<empty>();
    return *this;
}
