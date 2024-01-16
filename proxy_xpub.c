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
 * proxy is a better solution of multiple points of PUB/SUB pattern
*/
#define PROXY_FRONT    ("inproc://frontend")
#define PROXY_BACK     ("inproc://backend")

void publisher1(void *ctx)
{
    char buffer [20];
    void *pub = zmq_socket (ctx, ZMQ_PUB);
    int rc = zmq_connect(pub, PROXY_FRONT);
    assert (rc == 0);

    srand((unsigned) time (NULL));
    while (1) {

        int zipcode, temperature, humidity;
        zipcode = rand() % 10000 + 10000;
        temperature = rand() % 215 - 80;
        humidity = rand() % 50 + 10;

        sprintf(buffer,  "%05d ", zipcode);
        rc = zmq_send(pub, buffer, strlen(buffer) + 1, ZMQ_SNDMORE);
        sprintf(buffer,  "%d %d", temperature, humidity);
        zmq_send(pub, buffer, strlen(buffer) + 1, 0);

        if(zipcode == 10001 || zipcode == 10002)
        {
            printf("Publish: %05d %d %d\n", zipcode, temperature, humidity);
        }
        usleep(1);
    }
    printf("Close publisher1 socket\n");
    zmq_close(pub);
}

void publisher2(void *ctx)
{
    char buffer [20];
    void *pub = zmq_socket (ctx, ZMQ_PUB);
    int rc = zmq_connect(pub, PROXY_FRONT);
    assert (rc == 0);

    srand((unsigned) time (NULL));
    while (1) {

        int zipcode, temperature, humidity;
        zipcode = rand() % 10000 + 20000;
        temperature = rand() % 215 - 80;
        humidity = rand() % 50 + 10;

        sprintf(buffer,  "%05d ", zipcode);
        rc = zmq_send(pub, buffer, strlen(buffer) + 1, ZMQ_SNDMORE);
        sprintf(buffer,  "%d %d", temperature, humidity);
        zmq_send(pub, buffer, strlen(buffer) + 1, 0);
        if(zipcode == 20001 || zipcode == 20002)
        {
            printf("Publish: %05d %d %d\n", zipcode, temperature, humidity);
        }
        usleep(1);
    }
    printf("Close publisher2 socket\n");
    zmq_close(pub);
}

void subscriber1(void *ctx)
{
    void *sub = zmq_socket (ctx, ZMQ_SUB);
    int rc;
    const char *filter;

    rc = zmq_connect(sub, PROXY_BACK);
    assert (rc == 0);
    filter = "10001";
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
        printf("Client1 rcvd: %s (%d)\n", buffer, count);
    }
    printf("Close client1 socket\n");
    zmq_close(sub);
}

void subscriber2(void *ctx)
{
    void *sub = zmq_socket (ctx, ZMQ_SUB);
    int rc;
    const char *filter;

    rc = zmq_connect(sub, PROXY_BACK);
    assert (rc == 0);
    filter = "10002";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert(rc == 0);

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
        printf("Client2 rcvd: %s (%d)\n", buffer, count);
    }
    printf("Close client2 socket\n");
    zmq_close(sub);
}

void proxy(void *ctx)
{
    void *front = zmq_socket (ctx, ZMQ_SUB);
    int rc = zmq_bind (front, PROXY_FRONT);
    assert (rc == 0);

    rc = zmq_setsockopt( front, ZMQ_SUBSCRIBE, "", 0);
    assert(rc == 0);

    void *back = zmq_socket (ctx, ZMQ_XPUB);
    rc = zmq_bind (back, PROXY_BACK);
    assert (rc == 0);

    std::map<int, string> msgmap;
    zmq_pollitem_t items[2] = {
        {front, 0, ZMQ_POLLIN, 0},
        {back, 0, ZMQ_POLLIN, 0}
    };

    char topic_buf[30];
    char data_buf[30];
    int topic;
    char event;
    while (1)
    {
        if (zmq_poll(items, 2, 1000) == -1)
        {
            printf("zmq_poll failed: %s\n", strerror(errno));
            break;
        }

        /*  Any new topic data we cache and then forward */
        if( items[0].revents & ZMQ_POLLIN )
        {
            zmq_recv(front, topic_buf, 30, 0);
            int topic = atoi(topic_buf);
            zmq_recv(front, data_buf, 30, 0);
            msgmap[topic] = data_buf;
            //printf("Update and forward: \"%s %s\"\n",  topic_buf, data_buf);
            zmq_send(back, topic_buf, strlen(topic_buf), ZMQ_SNDMORE);
            zmq_send(back, data_buf, strlen(data_buf) + 1, 0);
        }

        /* new subscription */
        if( items[1].revents & ZMQ_POLLIN)
        {
            zmq_recv(back, topic_buf, 30, 0);
            event = topic_buf[0];
            topic = atoi(&topic_buf[1]);
            printf("event=%d, topic=%d\n", event, topic);

            if (event == 1)
            {
                if (auto search = msgmap.find(topic); search != msgmap.end())
                {
                    sprintf(data_buf, "%s", search->second.c_str());
                    printf("Sending last cached value: \"%s %s\"\n",
                           topic_buf + 1, data_buf);
                    zmq_send(back, topic_buf + 1, strlen(topic_buf + 1), ZMQ_SNDMORE);
                    zmq_send(back, data_buf, strlen(data_buf) + 1, 0);
                }
            }
        }
    }
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

    std::thread server(proxy, ctx);
    std::thread pub1(publisher1, ctx);
    std::thread pub2(publisher2, ctx);
    /* wait for database updated with publish value */
    sleep(10);
    std::thread sub1(subscriber1, ctx);
    std::thread sub2(subscriber2, ctx);

    server.join();
    pub1.join();
    pub2.join();
    sub1.join();
    sub2.join();

    zmq_ctx_destroy(ctx);
    return 0;
}


