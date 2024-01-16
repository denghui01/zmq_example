#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <thread>

/**
* This demo shows pub/sub pattern support many-to-one
* In zmq, who binds or connects doesn't matter, usually server should bind
* the socket to the endpoint. The limitation is an endpoint can only bind once
* In this scenario there are multiple server, so we have to connect server to
* the endpoint and client will bind to it.
*/
#define ZMQ_PUB_ENDPOINT  ("inproc://weather")

void server1(void *ctx)
{
    char buffer [20];
    int count = 0;
    void *pub = zmq_socket (ctx, ZMQ_PUB);
    int rc = zmq_connect (pub, ZMQ_PUB_ENDPOINT);
    assert (rc == 0);

    srand((unsigned) time (NULL));
    while (1) {

        int zipcode, temperature, humidity;
        zipcode = rand() % 10000 + 10000;
        temperature = rand() % 215 - 80;
        humidity = rand() % 50 + 10;

        sprintf(buffer,  "%05d ", zipcode);
        rc = zmq_send(pub, buffer, strlen(buffer), ZMQ_SNDMORE);
        sprintf(buffer,  "%d %d", temperature, humidity);
        zmq_send(pub, buffer, strlen(buffer) + 1, 0);
        if(zipcode == 10001 && ++count >= 10)
        {
            break;
        }
    }
    printf("Close publisher1 socket\n");
    zmq_close(pub);
}

void server2(void *ctx)
{
    char buffer [20];
    int count = 0;
    void *pub = zmq_socket (ctx, ZMQ_PUB);
    int rc = zmq_connect (pub, ZMQ_PUB_ENDPOINT);
    assert (rc == 0);

    srand((unsigned) time (NULL));
    while (1) {

        int zipcode, temperature, humidity;
        zipcode = rand() % 10000 + 20000;
        temperature = rand() % 215 - 80;
        humidity = rand() % 50 + 10;

        sprintf(buffer,  "%05d ", zipcode);
        rc = zmq_send(pub, buffer, strlen(buffer), ZMQ_SNDMORE);
        sprintf(buffer,  "%d %d", temperature, humidity);
        zmq_send(pub, buffer, strlen(buffer) + 1, 0);
        if(zipcode == 20001 && ++count >= 10)
        {
            break;
        }
    }
    printf("Close publisher2 socket\n");
    zmq_close(pub);
}

void client1(void *ctx)
{
    void *sub = zmq_socket (ctx, ZMQ_SUB);
    int rc = zmq_bind (sub, ZMQ_PUB_ENDPOINT);
    assert (rc == 0);

    const char *filter = "10001";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert(rc == 0);

    filter = "20001";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert(rc == 0);

    int count = 0;
    for (count = 0; count < 20; ++count)
    {
        char buffer[32];
        rc = zmq_recv(sub, buffer, 32, 0);
        assert(rc);
        rc = zmq_recv(sub, buffer + rc, 32 - rc , 0);
        assert(rc);
        printf("Client rcvd: %s (%d)\n", buffer, count);
    }
    printf("Close client1 socket\n");
    zmq_close(sub);
}

int main()
{
    void *ctx = zmq_ctx_new ();
    if ( ctx == NULL )
    {
        printf("zmq_ctx_new failed: %s", strerror(errno));
        return 0;
    }
    std::thread s1(server1, ctx);
    std::thread s2(server2, ctx);
    std::thread c1(client1, ctx);

    s1.join();
    s2.join();
    c1.join();

    printf("Destroy zmq ctx\n");
    zmq_ctx_destroy(ctx);
    return 0;
}

