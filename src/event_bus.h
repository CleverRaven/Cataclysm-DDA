#pragma once
#ifndef CATA_SRC_EVENT_BUS_H
#define CATA_SRC_EVENT_BUS_H

#include <type_traits>
#include <vector>

#include "event.h"

class Creature;
class event_subscriber;
class item_location;
class talker;

class event_bus
{
    public:
        event_bus() = default;
        event_bus( const event_bus & ) = delete;
        event_bus &operator=( const event_bus & ) = delete;
        ~event_bus();
        void subscribe( event_subscriber * );
        void unsubscribe( event_subscriber * );

        void send( const cata::event & ) const;
        void send_with_talker( Creature *, Creature *, const cata::event & ) const;
        void send_with_talker( Creature *, item_location *, const cata::event & ) const;
        template<event_type Type, typename... Args>
        void send( Args &&... args ) const {
            send( cata::event::make<Type>( std::forward<Args>( args )... ) );
        }
    private:
        std::vector<event_subscriber *> subscribers;
};

event_bus &get_event_bus();

#endif // CATA_SRC_EVENT_BUS_H
