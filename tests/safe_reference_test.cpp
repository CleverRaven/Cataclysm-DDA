#include <memory>

#include "catch/catch.hpp"
#include "safe_reference.h"

struct example {
    safe_reference_anchor anchor;

    safe_reference<example> get_ref() {
        return anchor.reference_to( this );
    }
};

TEST_CASE( "safe_reference_returns_correct_object", "[safe_reference]" )
{
    example e;
    safe_reference<example> ref = e.get_ref();
    CHECK( ref );
    CHECK( ref.get() == &e );
}

TEST_CASE( "safe_reference_invalidated_by_destructor", "[safe_reference]" )
{
    std::unique_ptr<example> e = std::make_unique<example>();
    safe_reference<example> ref = e->get_ref();
    e.reset();
    CHECK( !ref );
    CHECK( ref.get() == nullptr );
}

TEST_CASE( "safe_reference_invalidated_by_assignment", "[safe_reference]" )
{
    example e;
    safe_reference<example> ref = e.get_ref();
    e = example();
    CHECK( !ref );
    CHECK( ref.get() == nullptr );
}

TEST_CASE( "safe_reference_not_shared_by_copy", "[safe_reference]" )
{
    std::unique_ptr<example> e0 = std::make_unique<example>();
    safe_reference<example> ref0 = e0->get_ref();
    std::unique_ptr<example> e1 = std::make_unique<example>( *e0 );
    safe_reference<example> ref1 = e1->get_ref();
    CHECK( ref0 );
    CHECK( ref1 );
    CHECK( ref0.get() != ref1.get() );
    e0.reset();
    CHECK( !ref0 );
    CHECK( ref1 );
}
