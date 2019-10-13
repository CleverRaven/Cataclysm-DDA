#include "safe_reference.h"

safe_reference_anchor::safe_reference_anchor() : impl( std::make_shared<empty>() )
{}

safe_reference_anchor::safe_reference_anchor( const safe_reference_anchor & ) :
    safe_reference_anchor()
{}

safe_reference_anchor &safe_reference_anchor::operator=( const safe_reference_anchor & )
{
    impl = std::make_shared<empty>();
    return *this;
}
