#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <thread>

#define ZMQ_PUB_ENDPOINT  ("inproc://weather")
#define CITY1_ZIP   10001
#define CITY2_ZIP   10002

void server(void *ctx)
{
    char buffer [20];
    void *pub = zmq_socket (ctx, ZMQ_PUB);
    int count1 = 0;
    int count2 = 0;
    int rc = zmq_bind (pub, ZMQ_PUB_ENDPOINT);
    assert (rc == 0);

    srand((unsigned) time (NULL));
    while (1) {

        int zipcode, temperature, humidity;
        zipcode = rand() % 100000;
        temperature = rand() % 215 - 80;
        humidity = rand() % 50 + 10;

        sprintf(buffer,  "%05d %d %d", zipcode, temperature, humidity);
        rc = zmq_send(pub, buffer, strlen(buffer) + 1, 0);
        if( rc == -1 )
        {
            printf("zmq_send failed: %s\n", strerror(errno));
        }
        if(zipcode == CITY1_ZIP)
        {
            printf("server sent: %s\n", buffer);
            ++count1;
        }
        if(zipcode == CITY2_ZIP)
        {
            printf("server sent: %s\n", buffer);
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

    const char *filter = "10001 ";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert(rc == 0);

    int count = 0;
    for (count = 0; count < 10; ++count)
    {
        char buffer[20];
        rc = zmq_recv(sub, buffer, 20, 0);
        if (rc != -1)
        {
            printf("Client1 rcvd: %s (%d)\n", buffer, count);
        }
        else
        {
            printf("zmq_recv failed: %s\n", strerror(errno));
        }
    }
    printf("Close subscriber socket\n");
    zmq_close(sub);
}

void client2(void *ctx)
{
    void *sub = zmq_socket (ctx, ZMQ_SUB);
    int rc = zmq_connect (sub, ZMQ_PUB_ENDPOINT);
    assert (rc == 0);

    const char *filter = "10002 ";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert(rc == 0);

    int count = 0;
    for (count = 0; count < 10; ++count)
    {
        char buffer[20];
        rc = zmq_recv(sub, buffer, 20, 0);
        if (rc != -1)
        {
            printf("Client2 rcvd: %s (%d)\n", buffer, count);
        }
        else
        {
            printf("zmq_recv failed: %s\n", strerror(errno));
        }
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

