#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <thread>

/**
 * When usepub/sub pattern, the topic and data should always be seperated
 * to make sure the subscription filter only matches topic
*/
#define ZMQ_PUB_ENDPOINT  ("inproc://weather")
#define CITY1_ZIP   10001
#define CITY2_ZIP   10002

void server(void *ctx)
{
    int count1 = 0;
    int count2 = 0;
    char buffer [20];
    void *pub = zmq_socket (ctx, ZMQ_PUB);
    int rc = zmq_bind (pub, ZMQ_PUB_ENDPOINT);
    assert (rc == 0);

    srand((unsigned) time (NULL));
    while (1) {

        int zipcode, temperature, humidity;
        zipcode = rand() % 100000;
        temperature = rand() % 215 - 80;
        humidity = rand() % 50 + 10;

        sprintf(buffer,  "%05d ", zipcode);
        rc = zmq_send(pub, buffer, strlen(buffer), ZMQ_SNDMORE);
        sprintf(buffer,  "%d %d", temperature, humidity);
        zmq_send(pub, buffer, strlen(buffer) + 1, 0);
        if(zipcode == CITY1_ZIP)
        {
            ++count1;
        }
        if(zipcode == CITY2_ZIP)
        {
            ++count2;
        }
        if(count1 >=10 && count2 >= 10)
        {
            break;
        }

        //usleep(1);
    }
    printf("Close publisher socket\n");
    zmq_close(pub);
}

void client1(void *ctx)
{
    void *sub = zmq_socket (ctx, ZMQ_SUB);
    int rc = zmq_connect (sub, ZMQ_PUB_ENDPOINT);
    assert (rc == 0);

    const char *filter = "10001";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert(rc == 0);

    int count = 0;
    for (count = 0; count < 10; ++count)
    {
        char buffer[32];
        rc = zmq_recv(sub, buffer, 32, 0);
        assert(rc);
        rc = zmq_recv(sub, buffer + rc, 32 - rc , 0);
        assert(rc);
        printf("Client1 rcvd: %s (%d)\n", buffer, count);
    }
    printf("Close subscriber socket\n");
    zmq_close(sub);
}

void client2(void *ctx)
{
    void *sub = zmq_socket (ctx, ZMQ_SUB);
    int rc = zmq_connect (sub, ZMQ_PUB_ENDPOINT);
    assert (rc == 0);

    const char *filter = "10002";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert(rc == 0);

    int count = 0;
    for (count = 0; count < 10; ++count)
    {
        char buffer[32];
        rc = zmq_recv(sub, buffer, 32, 0);
        assert(rc);
        rc = zmq_recv(sub, buffer + rc, 32 - rc , 0);
        assert(rc);
        printf("Client2 rcvd: %s (%d)\n", buffer, count);
    }
    printf("Close subscriber socket\n");
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
    std::thread s1(server, ctx);
    std::thread c1(client1, ctx);
    std::thread c2(client2, ctx);

    c1.join();
    c2.join();
    s1.join();
    printf("Destroy zmq ctx\n");
    zmq_ctx_destroy(ctx);
    return 0;
}

