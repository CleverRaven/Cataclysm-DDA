#pragma once
#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include "event.h"

class event_bus;

class event_subscriber
{
    public:
        virtual ~event_subscriber();
        virtual void notify( const event & ) = 0;
    private:
        friend class event_bus;
        void on_subscribe( event_bus * );
        void on_unsubscribe( event_bus * );
        event_bus *subscribed_to = nullptr;
};

class event_bus
{
    public:
        ~event_bus();
        void subscribe( event_subscriber * );
        void unsubscribe( event_subscriber * );

        void send( const event & ) const;
    private:
        std::vector<event_subscriber *> subscribers;
};

#endif // EVENT_BUS_H
