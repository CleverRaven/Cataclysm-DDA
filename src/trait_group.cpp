#include "debug.h"
#include "rng.h"
#include "trait_group.h"

using namespace trait_group;

Trait_list Trait_creation_data::create() const
{
    RecursionList rec;
    auto result = create( rec );
    return result;
}

Trait_group::Trait_group( int probability )
    : Trait_creation_data( probability )
{
}

Trait_group::~Trait_group()
{
    for( auto &creator : creators ) {
        delete creator;
    }
    creators.clear();
}

Single_trait_creator::Single_trait_creator( const Trait_id &id, int probability )
    : Trait_creation_data( probability ), id( id )
{
}

Trait_list Single_trait_creator::create( RecursionList & /* rec */ ) const
{
    Trait_list result;
    if( rng( 0, 99 ) < probability ) {
        result.push_back( id );
    }
    return result;
}

void Single_trait_creator::check_consistency() const
{
    if( !id.is_valid() ) {
        debugmsg( "trait id %s is unknown", id.c_str() );
    }
}

bool Single_trait_creator::remove_trait( const Trait_id & /* tid */ )
{
    // Cannot remove self... I assume my parent has removed me!
    return true;
}

bool Single_trait_creator::has_trait( const Trait_id &tid ) const
{
    return tid == id;
}

Trait_group_creator::Trait_group_creator( const Trait_group_tag &id, int probability )
    : Trait_creation_data( probability ), id( id )
{
}

Trait_list Trait_group_creator::create( RecursionList &rec ) const
{
    Trait_list result;
    if( std::find( rec.begin(), rec.end(), id ) != rec.end() ) {
        debugmsg( "recursion in trait creation list %s", id.c_str() );
        return result;
    }
    rec.push_back( id );

    // TODO(sm)
    Trait_creation_data *tcd = nullptr; // trait_controller->get_group(id);
    if( !tcd ) {
        debugmsg( "unknown trait creation list %s", id.c_str() );
        return result;
    }

    Trait_list tmplist = tcd->create( rec );
    rec.pop_back(); // TODO(sm): equivalent to erase(end() - 1), right?
    result.insert( result.end(), tmplist.begin(), tmplist.end() );

    return result;
}

void Trait_group_creator::check_consistency() const
{
    // TODO(sm): needs trait factory
    if( false ) {
        debugmsg( "trait group id %s is unknown", id.c_str() );
    }
}

bool Trait_group_creator::remove_trait( const Trait_id &tid )
{
    // TODO(sm): needs trait factory
    Trait_creation_data *tcd = nullptr; /* trait_controller->get_group(tid); */

    if( tcd ) {
        tcd->remove_trait( tid );
    }
    return false;
}

bool Trait_group_creator::has_trait( const Trait_id &tid ) const
{
    // TODO(sm): needs trait factory
    Trait_creation_data *tcd = nullptr; /* trait_controller->get_group(tid); */

    return tcd && tcd->has_trait( tid );
}

void Trait_group::add_trait_entry( const Trait_id &tid, int probability )
{
    std::unique_ptr<Trait_creation_data> ptr( new Single_trait_creator( tid, probability ) );
    add_entry( ptr );
}

void Trait_group::add_group_entry( const Trait_group_tag &gid, int probability )
{
    std::unique_ptr<Trait_creation_data> ptr( new Trait_group_creator( gid, probability ) );
    add_entry( ptr );
}

void Trait_group::check_consistency() const
{
    for( const auto &creator : creators ) {
        creator->check_consistency();
    }
}

bool Trait_group::remove_trait( const Trait_id &tid )
{
    for( auto it = creators.begin(); it != creators.end(); ) {
        if( ( *it )->remove_trait( tid ) ) {
            sum_prob -= ( *it )->probability;
            delete *it;
            it = creators.erase( it );
        } else {
            ++it;
        }
    }
    return creators.empty();
}

bool Trait_group::has_trait( const Trait_id &tid ) const
{
    for( const auto &creator : creators ) {
        if( creator->has_trait( tid ) ) {
            return true;
        }
    }
    return false;
}

Trait_group_collection::Trait_group_collection( int probability )
    : Trait_group( probability )
{
    if( probability <= 0 || probability > 100 ) {
        debugmsg( "Probability %d out of range", probability );
    }
};

Trait_list Trait_group_collection::create( RecursionList &rec ) const
{
    Trait_list result;
    for( const auto &creator : creators ) {
        if( rng( 0, 99 ) >= ( creator )->probability ) {
            continue;
        }
        Trait_list tmp = ( creator )->create( rec );
        result.insert( result.end(), tmp.begin(), tmp.end() );
    }
    return result;
}

void Trait_group_collection::add_entry( std::unique_ptr<Trait_creation_data> &ptr )
{
    assert( ptr.get() != nullptr );
    if( ptr->probability <= 0 ) {
        return;
    }

    ptr->probability = std::min( 100, ptr->probability );
    sum_prob += ptr->probability;

    creators.push_back( ptr.get() );
    ptr.release();
}

void Trait_group_distribution::add_entry( std::unique_ptr<Trait_creation_data> &ptr )
{
    assert( ptr.get() != nullptr );
    if( ptr->probability <= 0 ) {
        return;
    }

    sum_prob += ptr->probability;

    creators.push_back( ptr.get() );
    ptr.release();
}

Trait_list Trait_group_distribution::create( RecursionList &rec ) const
{
    Trait_list result;
    int p = rng( 0, sum_prob - 1 );
    for( const auto &creator : creators ) {
        p -= ( creator )->probability;
        if( p >= 0 ) {
            continue;
        }
        Trait_list tmp = ( creator )->create( rec );
        result.insert( result.end(), tmp.begin(), tmp.end() );
        break;
    }
    return result;
}
