#include "event_bus.h"

#include <algorithm>

event_subscriber::~event_subscriber()
{
    if( subscribed_to ) {
        subscribed_to->unsubscribe( this );
    }
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

void event_bus::send( const event &e ) const
{
    for( event_subscriber *s : subscribers ) {
        s->notify( e );
    }
}
