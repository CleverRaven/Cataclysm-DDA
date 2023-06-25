#pragma once
#ifndef CATA_SRC_EVENT_SUBSCRIBER_H
#define CATA_SRC_EVENT_SUBSCRIBER_H

namespace cata
{
class event;
}  // namespace cata
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

#endif // CATA_SRC_EVENT_SUBSCRIBER_H
