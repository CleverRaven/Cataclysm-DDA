#pragma once
#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include "event.h"

class event_bus;

class event_subscriber
{
    public:
        event_subscriber() = default;
        event_subscriber( const event_subscriber & ) = delete;
        event_subscriber &operator=( const event_subscriber & ) = delete;
        virtual ~event_subscriber();
        virtual void notify( const cata::event & ) = 0;
    private:
        friend class event_bus;
        void on_subscribe( event_bus * );
        void on_unsubscribe( event_bus * );
        event_bus *subscribed_to = nullptr;
};

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
        template<event_type Type, typename... Args>
        void send( Args &&... args ) const {
            send( cata::event::make<Type>( std::forward<Args>( args )... ) );
        }
    private:
        std::vector<event_subscriber *> subscribers;
};

#endif // EVENT_BUS_H
