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
#define WEATHER1_URL  ("inproc://weather1")

void server1(void *ctx)
{
    char buffer [20];
    int count1 = 0;
    void *pub = zmq_socket (ctx, ZMQ_PUB);
    int rc = zmq_connect (pub, WEATHER1_URL);
    assert (rc == 0);

    srand((unsigned) time (NULL));
    while (1) {

        int zipcode, temperature, humidity;
        zipcode = rand() % 1000 + 1000;
        temperature = rand() % 215 - 80;
        humidity = rand() % 50 + 10;

        sprintf(buffer,  "%04d %d %d", zipcode, temperature, humidity);
        rc = zmq_send(pub, buffer, strlen(buffer) + 1, 0);
        if( rc == -1 )
        {
            printf("zmq_send failed: %s\n", strerror(errno));
        }
        if(zipcode == 1001)
        {
            printf("server1 send %s\n", buffer);
            if(++count1 >= 10)
            {
                break;
            }
        }
        usleep(1);
    }
    printf("Close publisher2 socket\n");
    zmq_close(pub);
}

void server2(void *ctx)
{
    char buffer [20];
    int count2 = 0;
    void *pub = zmq_socket (ctx, ZMQ_PUB);
    int rc = zmq_connect (pub, WEATHER1_URL);
    assert (rc == 0);

    srand((unsigned) time (NULL));
    while (1) {

        int zipcode, temperature, humidity;
        zipcode = rand() % 1000 + 2000;
        temperature = rand() % 215 - 80;
        humidity = rand() % 50 + 10;

        sprintf(buffer,  "%04d %d %d", zipcode, temperature, humidity);
        rc = zmq_send (pub, buffer, strlen(buffer) + 1, 0);
        if( rc == -1 )
        {
            printf("zmq_send failed: %s\n", strerror(errno));
        }
        if(zipcode == 2001)
        {
            printf("server2 send %s\n", buffer);
            if(++count2 >= 10)
            {
                break;
            }
        }
        usleep(1);
    }
    printf("Close publisher2 socket\n");
    zmq_close(pub);
}

void client1(void *ctx)
{
    void *sub = zmq_socket (ctx, ZMQ_SUB);
    int rc = zmq_bind (sub, WEATHER1_URL);
    assert (rc == 0);

    const char *filter = "1001 ";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));

    filter = "2001 ";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));

    assert(rc == 0);

    int count = 0;
    for (count = 0; count < 20; ++count)
    {
        char buffer[20];
        rc = zmq_recv(sub, buffer, 20, 0);
        if (rc != -1)
        {
            printf("Client 1 recieved weather: %s\n", buffer);
        }
        else
        {
            printf("zmq_recv failed: %s\n", strerror(errno));
        }
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

