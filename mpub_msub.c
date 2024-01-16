#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <thread>

/**
* This demo shows pub/sub pattern support many-to-many
* There is no support for many-to-many natively, because zmq_bind can only
* be called once. but you could use multiple one-to-many to achieve it,
* that needs multiple endpoints
*/
#define WEATHER1_URL  ("inproc://weather1")
#define WEATHER2_URL  ("inproc://weather2")

void server1(void *ctx)
{
    char buffer [20];
    int count1 = 0;
    int count2 = 0;
    void *pub = zmq_socket (ctx, ZMQ_PUB);
    int rc = zmq_bind(pub, WEATHER1_URL);
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
        if(zipcode == 10001)
        {
            ++count1;
        }
        if(zipcode == 10002)
        {
            ++count2;
        }
        if(count1 >=10 && count2 >= 10)
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
    int count1 = 0;
    int count2 = 0;
    void *pub = zmq_socket (ctx, ZMQ_PUB);
    int rc = zmq_bind(pub, WEATHER2_URL);
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
        if(zipcode == 20001)
        {
            ++count1;
        }
        if(zipcode == 20002)
        {
            ++count2;
        }
        if(count1 >=10 && count2 >= 10)
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
    int rc;
    const char *filter;

    rc = zmq_connect(sub, WEATHER1_URL);
    assert (rc == 0);
    filter = "10001";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert(rc == 0);

    rc = zmq_connect(sub, WEATHER2_URL);
    assert (rc == 0);
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

void client2(void *ctx)
{
    void *sub = zmq_socket (ctx, ZMQ_SUB);
    int rc;
    const char *filter;

    rc = zmq_connect(sub, WEATHER1_URL);
    assert (rc == 0);
    filter = "10002";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert(rc == 0);

    rc = zmq_connect(sub, WEATHER2_URL);
    assert (rc == 0);
    filter = "20002";
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
    printf("Close client2 socket\n");
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
    std::thread c2(client2, ctx);

    s1.join();
    s2.join();
    c1.join();
    c2.join();

    printf("Destroy zmq ctx\n");
    zmq_ctx_destroy(ctx);
    return 0;
}

