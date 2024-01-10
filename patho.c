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

#define PATHO_FRONT    ("inproc://frontend")
#define PATHO_BACK     ("inproc://backend")

void publisher(void *ctx)
{
    char topic[20];
    char msg[30];
    void *pub = zmq_socket (ctx, ZMQ_PUB);
    int rc = zmq_connect (pub, PATHO_FRONT);
    assert (rc == 0);

    srand((unsigned) time (NULL));
    for (int num = 0; num < 1000; ++num)
    {
        sprintf(topic, "%03d", num);
        zmq_send (pub, topic, strlen(topic) + 1, ZMQ_SNDMORE);
        sprintf(msg, "Save Roger-%d", num);
        zmq_send(pub, msg, strlen(msg) + 1, 0);
    }

    while (1) {
        sprintf(topic, "%03d", rand() % 1000);
        rc = zmq_send (pub, topic, strlen(topic) + 1, ZMQ_SNDMORE);
        if( rc == -1 )
        {
            printf("zmq_send failed: %s\n", strerror(errno));
        }
        sprintf(msg, "Off with his head!");
        rc = zmq_send(pub, msg, strlen(msg) + 1, 0);
        if( rc == -1 )
        {
            printf("zmq_send failed: %s\n", strerror(errno));
        }
        sleep(1);
    }
    zmq_close(pub);
}

void subscriber(void *ctx)
{
    void *sub = zmq_socket (ctx, ZMQ_SUB);
    int rc = zmq_connect (sub, PATHO_BACK);
    assert (rc == 0);

    char topic[20];
    char data[30];

    const char *filter = "488";
    rc = zmq_setsockopt( sub, ZMQ_SUBSCRIBE, filter, strlen(filter));
    assert(rc == 0);

    while (1)
    {
        rc = zmq_recv(sub, topic, 20, 0);
        if (rc == -1)
        {
            printf("zmq_recv failed: %s\n", strerror(errno));
            break;
        }

        rc = zmq_recv(sub, data, 20, 0);
        if (rc == -1)
        {
            printf("zmq_recv failed: %s\n", strerror(errno));
            break;
        }

        printf("received topic 488 = %s\n", data);
    }

    zmq_close(sub);
}

void proxy(void *ctx)
{
    void *front = zmq_socket (ctx, ZMQ_SUB);
    int rc = zmq_bind (front, PATHO_FRONT);
    assert (rc == 0);

    rc = zmq_setsockopt( front, ZMQ_SUBSCRIBE, "", 0);
    assert(rc == 0);

    void *back = zmq_socket (ctx, ZMQ_XPUB);
    rc = zmq_bind (back, PATHO_BACK);
    assert (rc == 0);

    std::map<int, string> msgmap;
    zmq_pollitem_t items[2] = {
        {front, 0, ZMQ_POLLIN, 0},
        {back, 0, ZMQ_POLLIN, 0}
    };

    char topic_buf[30];
    char msg_buf[30];
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
            zmq_recv(front, msg_buf, 30, 0);
            msgmap[topic] = msg_buf;

            printf("Forward msg: \"%s %s\" after updating the msgmap\n",
                   topic_buf,msg_buf);
            zmq_send(back, topic_buf, strlen(topic_buf) + 1, ZMQ_SNDMORE);
            zmq_send(back, msg_buf, strlen(msg_buf) + 1, 0);
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
                sprintf(topic_buf, "%03d", topic);
                if (auto search = msgmap.find(topic); search != msgmap.end())
                {
                    sprintf(msg_buf, "%s", search->second.c_str());
                    printf("Sending cached value on subscription: \"%s %s\"\n",
                           topic_buf, msg_buf);
                    zmq_send(back, topic_buf, strlen(topic_buf) + 1, ZMQ_SNDMORE);
                    zmq_send(back, msg_buf, strlen(msg_buf) + 1, 0);
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
    std::thread pub1(publisher, ctx);
    std::thread pub2(publisher, ctx);
    sleep(1);
    std::thread sub(subscriber, ctx);

    while (1){}
    zmq_ctx_destroy(ctx);
    return 0;
}


