#include "publish.h"

#ifdef ZMQ

#include <zmq.h>

#include "game.h"
#include "debug.h"

#define PUBLISH_VERSION "$C1-"

publish::publish( publish &&rhs )
{
    ctx = rhs.ctx;
    sock = rhs.sock;

    rhs.ctx = nullptr;
    rhs.sock = nullptr;
}

publish &publish::operator=( publish &&rhs )
{
    ctx = rhs.ctx;
    sock = rhs.sock;

    rhs.ctx = nullptr;
    rhs.sock = nullptr;
    return *this;
}

publish::publish( const std::string &remote )
{
    ctx = zmq_ctx_new();
    if( !ctx ) {
        return;
    }

    sock = zmq_socket( ctx, ZMQ_PUSH );
    if( !sock ) {
        zmq_ctx_destroy( ctx );
        ctx = nullptr;
        return;
    }

    int rc = zmq_connect( sock, remote.c_str() );
    if( rc != 0 ) {
        zmq_close( sock );
        zmq_ctx_destroy( ctx );
        sock = nullptr;
        ctx = nullptr;
    }
}

publish::~publish()
{
    if( sock ) {
        zmq_close( sock );
    }
    if( ctx ) {
        zmq_ctx_destroy( ctx );
    }
}

void publish::send( const std::string &type, const std::string &msg )
{
    if( !( ctx && sock ) ) {
        return;
    }

    bool fail = false;

    // 1. header containing the version and message type
    std::string header = std::string( PUBLISH_VERSION ) + type;
    fail |= zmq_send( sock, header.c_str(), header.size(), ZMQ_SNDMORE ) == -1;

    // 2. UUID
    fail |= zmq_send( sock, g->id.c_str(), g->id.size(), ZMQ_SNDMORE ) == -1;

    // 3. game turn
    char buf[22]; // max length of 64-bit signed integer printed in decimal including '\0'
    sprintf( buf, "%i", int( calendar::turn ) );
    fail |= zmq_send( sock, buf, strnlen(buf, sizeof(buf) - 1) + 1, ZMQ_SNDMORE ) == -1;

    // 4. payload
    fail |= zmq_send( sock, msg.c_str(), msg.size(), 0 ) == -1;

    if( fail ) {
        // warn on the first failure only and make no further attempts
        debugmsg( "send() failed: %s (%i)", zmq_strerror( errno ), errno );
        zmq_close( sock );
        zmq_ctx_destroy( ctx );
        sock = nullptr;
        ctx = nullptr;
    }
}

#endif
