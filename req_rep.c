#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <thread>

/**
* req-rep is a simply bidirection pipeline
* the limitation is only client can initial a request
* you can do multiple servers and one client, or one server and multiple clients
*/
#define HELLO_URL   ("inproc://*:5555")
void server(void *ctx)
{
    void *responder = zmq_socket (ctx, ZMQ_REP);
    int rc = zmq_bind (responder, HELLO_URL);
    assert (rc == 0);

    while (1) {
        char buffer [10];
        zmq_recv (responder, buffer, 10, 0);
        if(strcmp(buffer, "Close") == 0)
        {
            break;
        }
        sleep (1);          //  Do some 'work'
        zmq_send (responder, "World", 6, 0);
    }
    printf("Close server socket\n");
    zmq_close(responder);
}

void client(void *ctx)
{
    printf ("Connecting to hello world server…\n");
    void *requester = zmq_socket (ctx, ZMQ_REQ);
    zmq_connect (requester, HELLO_URL);

    int request_nbr;
    for (request_nbr = 0; request_nbr != 10; request_nbr++) {
        char buffer [10];
        printf ("Sending Hello %d…\n", request_nbr);
        zmq_send (requester, "Hello", 6, 0);
        zmq_recv (requester, buffer, 10, 0);
        printf ("Received %s %d\n", buffer, request_nbr);
    }
    zmq_send (requester, "Close", 6, 0);
    printf("Close client socket\n");
    zmq_close (requester);
}

void print_version()
{
    int major, minor, patch;
    zmq_version (&major, &minor, &patch);
    printf ("Current 0MQ version is %d.%d.%d\n", major, minor, patch);
}

int main()
{
    print_version();

    void *ctx = zmq_ctx_new ();
    if ( ctx == NULL )
    {
        printf("zmq_ctx_new failed: %s", strerror(errno));
        return 0;
    }
    std::thread s(server, ctx);
    std::thread c(client, ctx);

    c.join();
    s.join();
    printf("destroy zmq ctx\n");
    zmq_ctx_destroy(ctx);
    return 0;
}


