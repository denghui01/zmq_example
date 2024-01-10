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
* push/pull is a unidirection pipeline pattern.
* With one-to-one pattern it can be used in the scenario that server doesn't
* need reply from the receiver, so the transaction performance should be
* better than req-rep pattern.
*
* With one-to-many or many-to-one pattern, it is different to pub-sub pattern,
* although they are both one direction.
*
* One-to-many push-pull has a load balance message queue between sender and
* receiver, but the pub-sub is on subscription only.
*
* Many-to-one push-pull has a fair-queuing between sender and receiver.

* There is no support for many-to-many natively, because zmq_bind can only
* be called once. but you could use multiple one-to-many to achieve it,
* that needs multiple endpoints
*/
#define TASK_VENT_ENDPOINT  ("inproc://task_vent")
#define TASK_SINK_ENDPOINT  ("inproc://task_sink")

void taskvent(void *ctx, int nworker)
{
    int count1 = 0;
    int count2 = 0;
    void *sender = zmq_socket (ctx, ZMQ_PUSH);
    int rc = zmq_bind(sender, TASK_VENT_ENDPOINT);
    assert (rc == 0);

    void *sink = zmq_socket (ctx, ZMQ_PUSH);
    rc = zmq_connect(sink, TASK_SINK_ENDPOINT);
    assert (rc == 0);

    /* inform tasksinker the task starts */
    zmq_send(sink, "0", 2, 0);

    srand((unsigned) time (NULL));

    int i;
    int total_time = 0;
    int ntask = 100;
    for (i = 0; i < ntask; i++) {
        int workload;
        //  Random workload from 1 to 100 msecs
        workload = rand() % 100 + 1;
        total_time += workload;
        char string [10];
        sprintf (string, "%d", workload);
        zmq_send (sender, string, strlen(string) + 1, 0);
    }

    for(i = 0; i < nworker; ++i)
    {
        // there is no guarantee that every worker will receive this
        zmq_send(sender, "done", 5, 0);
    }

    printf ("Total expected cost: %d msec\n", total_time);
    printf ("Taskvent close sink and sender socket\n");
    zmq_close (sink);
    zmq_close (sender);
}

void taskworker(void *ctx, int n)
{
    assert(ctx);

    printf("Taskworker %d starting...\n", n);
    void *receiver = zmq_socket (ctx, ZMQ_PULL);
    int rc = zmq_connect(receiver, TASK_VENT_ENDPOINT);
    assert (rc == 0);

    void *sender = zmq_socket (ctx, ZMQ_PUSH);
    zmq_connect (sender, TASK_SINK_ENDPOINT);

    int count = 0;
    while (1) {
        char string[20];
        rc = zmq_recv (receiver, string, 20, 0);
        if (rc != -1)
        {
            if(strcmp(string, "done") == 0)
            {
                printf("task worker %d done, process %d tasks!\n", n, count);
                break;
            }
            fflush (stdout);
            //printf("Worker %d is doing a %s msec job\n", n, string);
            usleep (atoi (string) * 1000);
            ++count;
            zmq_send (sender, "", 0, 0);
        }
    }
    printf("Taskworker %d close receiver and sender socket\n", n);
    zmq_close(receiver);
    zmq_close(sender);
}

void tasksink(void *ctx)
{
    char string[20];
    void *receiver = zmq_socket (ctx, ZMQ_PULL);
    int rc = zmq_bind(receiver, TASK_SINK_ENDPOINT);
    assert (rc == 0);

    rc = zmq_recv (receiver, string, 20, 0);
    if (rc != -1)
    {
        struct timespec start, stop;
        clock_gettime(CLOCK_MONOTONIC, &start);
        for(int i = 0; i < 100; ++i)
        {
            zmq_recv (receiver, string, 20 ,0);
        }
        clock_gettime(CLOCK_MONOTONIC, &stop);
        printf("Tasksink elapsed: %d msec\n",
                (int)((stop.tv_sec - start.tv_sec)* 1000 +
                (float)(stop.tv_nsec - start.tv_nsec)/1000000));
    }

    printf("Tasksink close receiver socket\n");
    zmq_close(receiver);
}

int main()
{
    void *ctx = zmq_ctx_new ();
    assert(ctx);

    int worker_nbr;
    std::vector<std::shared_ptr<std::thread>> workers;

    printf ("How many workers do you want to create?");
    scanf("%d", &worker_nbr);

    for(int i = 0; i < worker_nbr; ++i)
    {
        auto worker = std::make_shared<std::thread>(taskworker, ctx, i);
        workers.push_back(worker);
    }

    // the key point is to wait for all worker are ready to receive the request
    // you could use another pipiline from worker to vent to sync up them
    sleep(1);
    std::thread vent(taskvent, ctx, worker_nbr);
    std::thread sink(tasksink, ctx);

    vent.join();
    sink.join();
    for(auto worker: workers)
    {
        worker->join();
    }
    printf("Destroy zmq ctx\n");
    zmq_ctx_destroy(ctx);
    return 0;
}

