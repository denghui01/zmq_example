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
            //printf("server1 sent: %s\n", buffer);
            ++count1;
        }
        if(zipcode == 1002)
        {
            //printf("server1 sent: %s\n", buffer);
            ++count2;
        }
        if(count1 >=10 && count2 >= 10)
        {
            break;
        }
        usleep(1);
    }
    printf("Close publisher2 socket\n");
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
            //printf("server2 sent: %s\n", buffer);
            ++count1;
        }
        if(zipcode == 2002)
        {
            //printf("server2 sent: %s\n", buffer);
            ++count2;
        }
        if(count1 >=10 && count2 >= 10)
        {
            break;
        }
        usleep(1);
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
    filter = "1001 ";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert(rc == 0);

    rc = zmq_connect(sub, WEATHER2_URL);
    assert (rc == 0);
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
            printf("Client1 recieved weather: %s\n", buffer);
        }
        else
        {
            printf("zmq_recv failed: %s\n", strerror(errno));
        }
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
    filter = "1002 ";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert(rc == 0);

    rc = zmq_connect(sub, WEATHER2_URL);
    assert (rc == 0);
    filter = "2002 ";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert(rc == 0);

    int count = 0;
    for (count = 0; count < 20; ++count)
    {
        char buffer[20];
        rc = zmq_recv(sub, buffer, 20, 0);
        if (rc != -1)
        {
            printf("Client2 recieved weather: %s\n", buffer);
        }
        else
        {
            printf("zmq_recv failed: %s\n", strerror(errno));
        }
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

