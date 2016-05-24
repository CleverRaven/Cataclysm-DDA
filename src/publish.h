#ifndef PUBLISH_H
#define PUBLISH_H

#ifndef ZMQ
#define report(type,msg) ;

#else
#define report(type,msg) g->remote.send(type,msg);

#include <string>

class publish
{
    public:
        publish() = default;
        ~publish();

        publish( publish && );
        publish &operator=( publish && );

        publish( const publish & ) = delete;
        publish &operator= ( const publish & ) = delete;

        explicit publish( const std::string &remote );

        void send( const std::string &type, const std::string &msg );

    private:
        void *ctx = nullptr; /** ZeroMQ context */
        void *sock = nullptr; /** ZeroMQ socket */
};

#endif

#endif
