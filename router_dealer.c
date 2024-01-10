#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <thread>
#include <string>
#include <map>
#include <zmq.h>

using std::string;

/**
* router-dealer pattern is an extension of req-rep pattern
* In the proxy, you should use zmq_msg APIs instead of raw data APIs because the
* message has the router ID to indicate which requester should be replied. The
* router ID is transparent to both client and worker.
* Hence the client and worker can still use raw data APIs.
*
* Similar to push-pull pattern, client cannot specify which worker will do the
* job, every worker has the same priority. There is a fair queue in the proxy.
*/
#define ZMQ_ROUTER_ENDPOINT     ("inproc://router")
#define ZMQ_DEALER_ENDPOINT     ("inproc://dealer")
#define ZMQ_SYNC_ENDPOINT       ("inproc://sync")

void client(void *ctx)
{
    printf("Start client...\n");
    assert(ctx);
    void *req = zmq_socket (ctx, ZMQ_REQ);
    int rc = zmq_connect (req, ZMQ_ROUTER_ENDPOINT);
    assert(rc == 0);

    void *sync = zmq_socket (ctx, ZMQ_PUSH);
    rc = zmq_connect (sync, ZMQ_SYNC_ENDPOINT);
    assert(rc == 0);

    char string[20];
    int request_nbr;
    for (request_nbr = 0; request_nbr != 5; request_nbr++)
    {
        printf ("Client send request %d [Hello]\n", request_nbr);
        zmq_send (req, "Hello", 6, 0);
        zmq_recv (req, string, 20, 0);
        printf ("Client received reply %d [%s]\n", request_nbr, string);
    }
    zmq_send(req, "done", 5, 0);
    zmq_recv(req, string, 20, 0);
    zmq_send (sync, "done", 5, 0);
    printf("client close req and sync socket\n");
    zmq_close (req);
    zmq_close (sync);
}

void worker(void *ctx)
{
    printf("Start worker...\n");
    void *rep = zmq_socket (ctx, ZMQ_REP);
    int rc = zmq_connect (rep, ZMQ_DEALER_ENDPOINT);
    assert(rc == 0);

    char string[20];
    while (1)
    {
        rc = zmq_recv (rep, string, 20, 0);
        if( strcmp(string, "done") == 0 )
        {
            zmq_send (rep, "done", 5, 0);
            break;
        }
        sleep (1);
        zmq_send (rep, "World", 6, 0);
    }
    printf("worker close rep socket\n");
    zmq_close (rep);
}

void proxy(void *ctx)
{
    printf("Start proxy...\n");
    void *frontend = zmq_socket (ctx, ZMQ_ROUTER);
    void *backend = zmq_socket (ctx, ZMQ_DEALER);
    zmq_bind (frontend, ZMQ_ROUTER_ENDPOINT);
    zmq_bind (backend, ZMQ_DEALER_ENDPOINT);

    void *sync = zmq_socket (ctx, ZMQ_PULL);
    zmq_bind(sync, ZMQ_SYNC_ENDPOINT);

    zmq_pollitem_t items [] = {
        { frontend, 0, ZMQ_POLLIN, 0 },
        { backend, 0, ZMQ_POLLIN, 0 },
        { sync, 0, ZMQ_POLLIN, 0 }
    };

    while (1) {
        zmq_msg_t message;
        int more;
        zmq_poll (items, 3, -1);
        if (items [0].revents & ZMQ_POLLIN) {
            while (1) {
                zmq_msg_init (&message);
                zmq_msg_recv (&message, frontend, 0);
                size_t more_size = sizeof (more);
                zmq_getsockopt (frontend, ZMQ_RCVMORE, &more, &more_size);
                zmq_msg_send (&message, backend, more? ZMQ_SNDMORE: 0);
                zmq_msg_close (&message);
                if (!more)
                {
                    break;
                }
            }
        }
        if (items [1].revents & ZMQ_POLLIN) {
            while (1) {
                zmq_msg_init (&message);
                zmq_msg_recv (&message, backend, 0);
                size_t more_size = sizeof (more);
                zmq_getsockopt (backend, ZMQ_RCVMORE, &more, &more_size);
                zmq_msg_send (&message, frontend, more? ZMQ_SNDMORE: 0);
                zmq_msg_close (&message);
                if (!more)
                {
                    break;
                }
            }
        }
        if (items [2].revents & ZMQ_POLLIN) {
            char str[20];
            zmq_recv(sync, str, 20, 0);
            if(strcmp(str, "done") == 0)
            {
                break;
            }
        }
    }
    printf("proxy close frontend backend and sync socket\n");
    zmq_close (frontend);
    zmq_close (backend);
    zmq_close (sync);
}

void proxy2(void *ctx)
{
    printf("Start proxy2...\n");
    void *frontend = zmq_socket (ctx, ZMQ_ROUTER);
    void *backend = zmq_socket (ctx, ZMQ_DEALER);
    zmq_bind (frontend, ZMQ_ROUTER_ENDPOINT);
    zmq_bind (backend, ZMQ_DEALER_ENDPOINT);

    zmq_proxy(frontend, backend, NULL);

    printf("proxy close frontend backend and sync socket\n");
    zmq_close (frontend);
    zmq_close (backend);
}

int main()
{
    int major, minor, patch;
    zmq_version(&major, &minor, &patch);
    printf("Zmq version: %d.%d.%d\n", major, minor, patch);

    void *ctx = zmq_ctx_new ();
    if ( ctx == NULL )
    {
        printf("zmq_ctx_new failed: %s", strerror(errno));
        return 0;
    }
    std::thread c(client, ctx);
    std::thread w(worker, ctx);
    std::thread p(proxy, ctx);

    c.join();
    w.join();
    p.join();

    printf("Destroy zmq context\n");
    zmq_ctx_destroy(ctx);
    return 0;
}


