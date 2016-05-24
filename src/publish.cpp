#include "publish.h"

#ifdef ZMQ

#include <zmq.h>

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

    // the first frame is the header containing the message type
    std::string header = std::string( PUBLISH_VERSION ) + type;
    int rc = zmq_send( sock, header.c_str(), header.size(), ZMQ_SNDMORE );

    // the second frame is the payload
    if( rc != -1 ) {
        rc = zmq_send( sock, msg.c_str(), msg.size(), 0 );
    }

    if( rc == -1 ) {
        // warn on the first failure only and make no further attempts
        debugmsg( "send() failed: %s (%i)", zmq_strerror( errno ), errno );
        zmq_close( sock );
        zmq_ctx_destroy( ctx );
        sock = nullptr;
        ctx = nullptr;
    }
}

#endif
