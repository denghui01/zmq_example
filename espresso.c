#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <thread>
#include <zmq.h>

#define ESPRESSO_FRONT    ("inproc://frontend")
#define ESPRESSO_BACK     ("inproc://backend")
#define ESPRESSO_CAPTURE  ("inproc://capture")

void server(void *ctx)
{
    char buffer [20];
    void *pub = zmq_socket (ctx, ZMQ_PUB);
    int rc = zmq_connect (pub, ESPRESSO_FRONT);
    assert (rc == 0);

    srand((unsigned) time (NULL));
    while (1) {

        int zipcode, temperature, humidity;
        zipcode = rand() % 10 + 10000;
        temperature = rand() % 215 - 80;
        humidity = rand() % 50 + 10;

        sprintf(buffer,  "%05d %d %d", zipcode, temperature, humidity);
        rc = zmq_send (pub, buffer, strlen(buffer) + 1, 0);
        if( rc == -1 )
        {
            printf("zmq_send failed: %s\n", strerror(errno));
        }
        usleep(100*1000);
    }
    zmq_close(pub);
}

void client(void *ctx)
{
    void *sub = zmq_socket (ctx, ZMQ_SUB);
    int rc = zmq_connect (sub, ESPRESSO_BACK);
    assert (rc == 0);

    const char *filter = "10001 ";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert(rc == 0);

    int count = 0;
    for (count = 0; count < 100; ++count)
    {
        char buffer[20];
        rc = zmq_recv(sub, buffer, 20, 0);
        if (rc != -1)
        {
            printf("Recieved weather: %s\n", buffer);
        }
        else
        {
            printf("zmq_recv failed: %s\n", strerror(errno));
        }
    }
    zmq_close(sub);
}

void listener(void *ctx)
{
    void *lis = zmq_socket (ctx, ZMQ_SUB);
    int rc = zmq_connect (lis, ESPRESSO_CAPTURE);
    assert (rc == 0);

    rc = zmq_setsockopt( lis, ZMQ_SUBSCRIBE, "", 0);
    assert(rc == 0);

    while (1)
    {
        char buffer[20];
        rc = zmq_recv(lis, buffer, 20, 0);
        if (rc != -1)
        {
            printf("Captured message: %s\n", buffer);
        }
        else
        {
            printf("zmq_recv failed: %s\n", strerror(errno));
        }
    }
    zmq_close(lis);
}

void proxy(void *ctx)
{
    void *front = zmq_socket (ctx, ZMQ_XSUB);
    int rc = zmq_bind (front, ESPRESSO_FRONT);
    assert (rc == 0);

    void *back = zmq_socket (ctx, ZMQ_XPUB);
    rc = zmq_bind (back, ESPRESSO_BACK);
    assert (rc == 0);

    void *cap = zmq_socket (ctx, ZMQ_PUB);
    rc = zmq_bind (back, ESPRESSO_CAPTURE);
    assert (rc == 0);

    zmq_proxy(front, back, cap);
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
    std::thread s(server, ctx);
    std::thread c(client, ctx);
    std::thread l(listener, ctx);
    std::thread p(proxy, ctx);


    c.join();
    s.join();
    zmq_ctx_destroy(ctx);
    return 0;
}

