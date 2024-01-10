
#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <vector>
#include <memory>
#include <thread>

/**
* In this example we use pub/sub to fan out the information, while using
* req/rep pair to sync up server and client with heartbeat
*/
#define SERVER_PUB_ENDPOINT  ("inproc://server.pub")
#define SERVER_SYNC_ENDPOINT  ("inproc://server.sync")
#define CITY1_ZIP   10001
#define CITY2_ZIP   10002

void server(void *ctx)
{
    void *pub = zmq_socket (ctx, ZMQ_PUB);
    int rc = zmq_bind(pub, SERVER_PUB_ENDPOINT);
    assert (rc == 0);

    void *sync = zmq_socket (ctx, ZMQ_REP);
    rc = zmq_bind(sync, SERVER_SYNC_ENDPOINT);
    assert (rc == 0);

    zmq_pollitem_t items[] = {
        {sync, 0, ZMQ_POLLIN, 0}
    };
    char buf[20];
    struct timeval start, now;
    gettimeofday(&start, NULL);
    srand((unsigned) time (NULL));
    while (1) {
        /**
        * handle heartbeat.
        * server will be blocked if there is no client
        */
        if (zmq_poll(items, 1, -1) == -1)
        {
            printf("zmq_poll failed: %s\n", strerror(errno));
            break;
        }

        if( items[0].revents & ZMQ_POLLIN )
        {
            zmq_recv(sync, buf, 20, 0);
            zmq_send(sync, buf, 20 ,0);
        }

        int zipcode, temperature, humidity;
        zipcode = rand() % 100000;
        temperature = rand() % 215 - 80;
        humidity = rand() % 50 + 10;

        sprintf(buf, "%05d %d %d", zipcode, temperature, humidity);
        rc = zmq_send(pub, buf, strlen(buf) + 1, 0);
        if( rc == -1 )
        {
            printf("zmq_send failed: %s\n", strerror(errno));
        }

        /* finish the server after 1 minute */
        gettimeofday(&now, NULL);
        int elapsed = now.tv_sec - start.tv_sec;
        if( elapsed > 60)
        {
            printf("Server running out of time, exit...\n");
            break;
        }
    }
    printf("Server close pub and sync socket\n");
    zmq_close(pub);
    zmq_close(sync);
}

void client1(void *ctx)
{
    void *sub = zmq_socket (ctx, ZMQ_SUB);
    int rc = zmq_connect (sub, SERVER_PUB_ENDPOINT);
    assert (rc == 0);

    const char *filter = "10001 ";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert(rc == 0);

    void *sync = zmq_socket (ctx, ZMQ_REQ);
    rc = zmq_connect (sync, SERVER_SYNC_ENDPOINT);
    assert (rc == 0);

    char buf[20];
    zmq_send(sync, buf, 20, 0);

    zmq_pollitem_t items[] = {
        {sync, 0, ZMQ_POLLIN, 0},
        {sub, 0, ZMQ_POLLIN, 0}
    };

    while(1)
    {
        /* heart beat timeout 1 sec */
        int rc = zmq_poll(items, 2, 1000);
        if (rc == -1)
        {
            printf("zmq_poll failed: %s\n", strerror(errno));
            break;
        }
        else if(rc == 0)
        {
            printf("Client1 Heartbeat timeout!\n");
            break;
        }

        if( items[0].revents & ZMQ_POLLIN )
        {
            zmq_recv(sync, buf, 20, 0);
            zmq_send(sync, buf, 20 ,0);
        }

        if( items[1].revents & ZMQ_POLLIN )
        {
            rc = zmq_recv(sub, buf, 20, 0);
            if (rc != -1)
            {
                printf("Client1 rcvd: %s\n", buf);
            }
            else
            {
                printf("zmq_recv failed: %s\n", strerror(errno));
            }
        }
    }
    printf("Client1 close sub and sync socket\n");
    zmq_close(sub);
    zmq_close(sync);
}

void client2(void *ctx)
{
    void *sub = zmq_socket (ctx, ZMQ_SUB);
    int rc = zmq_connect (sub, SERVER_PUB_ENDPOINT);
    assert (rc == 0);

    const char *filter = "10002 ";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert(rc == 0);

    void *sync = zmq_socket (ctx, ZMQ_REQ);
   rc = zmq_connect (sync, SERVER_SYNC_ENDPOINT);
    assert (rc == 0);

    char buf[20];
    zmq_send(sync, buf, 20, 0);

    zmq_pollitem_t items[] = {
        {sync, 0, ZMQ_POLLIN, 0},
        {sub, 0, ZMQ_POLLIN, 0}
    };

    while(1)
    {
        /* heart beat timeout 1 sec */
        int rc = zmq_poll(items, 2, 1000);
        if (rc == -1)
        {
            printf("zmq_poll failed: %s\n", strerror(errno));
            break;
        }
        else if(rc == 0)
        {
            printf("Client2 Heartbeat timeout!\n");
            break;
        }

        if( items[0].revents & ZMQ_POLLIN )
        {
            zmq_recv(sync, buf, 20, 0);
            zmq_send(sync, buf, 20 ,0);
        }

        if( items[1].revents & ZMQ_POLLIN )
        {
            rc = zmq_recv(sub, buf, 20, 0);
            if (rc != -1)
            {
                printf("Client2 rcvd: %s\n", buf);
            }
            else
            {
                printf("zmq_recv failed: %s\n", strerror(errno));
            }
        }
    }
    printf("Client2 close sub and sync socket\n");
    zmq_close(sub);
    zmq_close(sync);
}

int main()
{
    void *ctx = zmq_ctx_new ();
    assert(ctx);

    std::thread s(server, ctx);
    std::thread c1(client1, ctx);
    std::thread c2(client2, ctx);

    s.join();
    c1.join();
    c2.join();

    printf("Destroy zmq ctx\n");
    zmq_ctx_destroy(ctx);
    return 0;
}

