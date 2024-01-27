#include <zmq.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <thread>
#include <czmq.h>

/**
 * When usepub/sub pattern, the topic and data should always be seperated
 * to make sure the subscription filter only matches topic
*/
void speaker(int verbose)
{
    //  Create speaker beacon to broadcast our service
    printf("Start speaker\n");
    zactor_t *speaker = zactor_new (zbeacon, NULL);
    assert (speaker);
    if (verbose)
        zstr_sendx (speaker, "VERBOSE", NULL);

    zsock_send (speaker, "si", "CONFIGURE", 9999);
    char *hostname = zstr_recv (speaker);
    if (!*hostname) {
        printf ("OK (skipping test, no UDP broadcasting)\n");
        zactor_destroy (&speaker);
        freen (hostname);
        return;
    }
    freen (hostname);
    //  We will broadcast the magic value 0xCAFE
    byte announcement [2] = { 0xCA, 0xFE };
    zsock_send (speaker, "sbi", "PUBLISH", announcement, 2, 100);
    sleep(10);
    zstr_sendx (speaker, "SILENCE", NULL);
    zactor_destroy (&speaker);
}

void listener(int verbose)
{
    //  Create listener beacon on port 9999 to lookup service
    printf("Start listener\n");
    zactor_t *listener = zactor_new (zbeacon, NULL);
    assert (listener);
    if (verbose)
        zstr_sendx (listener, "VERBOSE", NULL);
    zsock_send (listener, "si", "CONFIGURE", 9999);
    char *hostname = zstr_recv (listener);
    assert (*hostname);
    freen (hostname);

    //  We will listen to anything (empty subscription)
    zsock_send (listener, "sb", "SUBSCRIBE", "", 0);

    //  Wait for at most 1/2 second if there's no broadcasting
    zsock_set_rcvtimeo (listener, 500);
    char *ipaddress = zstr_recv (listener);
    if (ipaddress) {
        zframe_t *content = zframe_recv (listener);
        assert (zframe_size (content) == 2);
        assert (zframe_data (content) [0] == 0xCA);
        assert (zframe_data (content) [1] == 0xFE);
        printf("ip = %s\n", ipaddress);
        zframe_destroy (&content);
        zstr_free (&ipaddress);
    }
    zactor_destroy (&listener);
}

int main()
{
    int verbose = 1;
    std::thread spk(speaker, verbose);
    std::thread lis(listener, verbose);

    spk.join();
    lis.join();
    return 0;
}

