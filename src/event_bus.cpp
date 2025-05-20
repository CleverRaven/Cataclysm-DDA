#include "event_bus.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <type_traits>

#include "calendar.h"
#include "cata_variant.h"
#include "creature.h"
#include "debug.h"
#include "event_subscriber.h"
#include "item_location.h"
#include "talker.h"  // IWYU pragma: keep
#include "vehicle.h"

namespace cata::event_detail
{
template <event_type Type> struct event_spec;
}  // namespace cata::event_detail

event_subscriber::~event_subscriber()
{
    if( subscribed_to ) {
        subscribed_to->unsubscribe( this );
    }
}

void event_subscriber::notify( const cata::event &e, std::unique_ptr<talker>,
                               std::unique_ptr<talker> )
{
    notify( e );
}

void event_subscriber::on_subscribe( event_bus *b )
{
    if( subscribed_to ) {
        debugmsg( "Subscribing a single event_subscriber multiple times is not supported" );
    }
    subscribed_to = b;
}

void event_subscriber::on_unsubscribe( event_bus *b )
{
    if( subscribed_to != b ) {
        debugmsg( "Unexpected notification of unsubscription from wrong bus" );
    } else {
        subscribed_to = nullptr;
    }
}

event_bus::~event_bus()
{
    while( !subscribers.empty() ) {
        unsubscribe( subscribers.front() );
    }
}

void event_bus::subscribe( event_subscriber *s )
{
    subscribers.push_back( s );
    s->on_subscribe( this );
}

void event_bus::unsubscribe( event_subscriber *s )
{
    auto it = std::find( subscribers.begin(), subscribers.end(), s );
    if( it == subscribers.end() ) {
        debugmsg( "Trying to remove subscriber that isn't there" );
    } else {
        ( *it )->on_unsubscribe( this );
        subscribers.erase( it );
    }
}

void event_bus::send( const cata::event &e ) const
{
    // don't accept malformed events (ex: wrong number of arguments)
    // from event::make_dyn()
    if( e.type() == event_type::num_event_types ) {
        debugmsg( "Null event sent to bus.  REJECTED!" );
        return;
    }
    for( event_subscriber *s : subscribers ) {
        s->notify( e );
    }
}

void event_bus::send_with_talker( Creature *alpha, Creature *beta,
                                  const cata::event &e ) const
{
    for( event_subscriber *s : subscribers ) {
        s->notify( e, get_talker_for( alpha ), get_talker_for( beta ) );
    }
}

void event_bus::send_with_talker( Creature *alpha, item_location *beta,
                                  const cata::event &e ) const
{
    for( event_subscriber *s : subscribers ) {
        s->notify( e, get_talker_for( alpha ), get_talker_for( beta ) );
    }
}

void event_bus::send_with_talker( vehicle *alpha,  Creature *beta, const cata::event &e ) const
{
    for( event_subscriber *s : subscribers ) {
        s->notify( e, get_talker_for( alpha ), get_talker_for( beta ) );
    }
}
namespace
{
template<event_type Type, typename IndexSequence>
struct make_dyn_helper;

template<event_type Type, size_t... I>
struct make_dyn_helper<Type, std::index_sequence<I...>> {
    using Spec = cata::event_detail::event_spec<Type>;

    cata::event operator()( time_point time, std::vector<std::string> &args ) {
        return cata::event(
                   Type,
                   time,
        std::map<std::string, cata_variant> { {
                Spec::fields[I].first,
                cata_variant::from_string( Spec::fields[I].second, std::move( args[I] ) )
            } ...
        } );
    }
};

template<event_type Type>
cata::event _make_dyn( std::vector<std::string> &args )
{
    using Spec = cata::event_detail::event_spec<Type>;
    static_assert( std::is_empty_v<Spec>,
                   "spec for this event type must be defined and empty" );

    if( Spec::fields.size() != args.size() ) {
        debugmsg(
            R"(Wrong number of arguments passed to event type "%s".  Expected %d, got %d)",
            io::enum_to_string( Type ), Spec::fields.size(), args.size() );
        return {}; // event_bus::send() can reject a null event
    }

    return make_dyn_helper <
           Type, std::make_index_sequence<Spec::fields.size()>
           > ()( calendar::turn, std::forward<std::vector<std::string> &>( args ) );
}

template<size_t I>
cata::event _make_dyn_impl( std::vector<std::string> &s )
{
    constexpr event_type T = static_cast<event_type>( I );
    return _make_dyn<T>( s );
}

using make_dyn_ptr_t = cata::event( * )( std::vector<std::string> & );

template<size_t... I>
constexpr make_dyn_ptr_t _make_dyn_impl_2( event_type type,
        std::index_sequence<I...>  /* IS */ )
{
    constexpr size_t num_types = static_cast<size_t>( event_type::num_event_types );
    constexpr std::array<make_dyn_ptr_t, num_types> _make_dyn_ptrs = {{
            _make_dyn_impl<I>...
        }
    };
    return _make_dyn_ptrs[static_cast<size_t>( type )];
}
} // namespace

cata::event cata::event::make_dyn( event_type type, std::vector<std::string> &args )
{
    constexpr size_t num_types = static_cast<size_t>( event_type::num_event_types );
    return _make_dyn_impl_2( type, std::make_index_sequence<num_types> {} )( args );
}

