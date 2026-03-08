#include "app_task.h"
#include <mqueue.h>
#include <pthread.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include "thirdparty/log.c/log.h"

#define MSG_LEN sizeof(struct TaskStruct)

struct TaskStruct
{
    Task task;
    void *argv;
};

static pthread_t *executor_ptr;
static int executors_count = 0;

static mqd_t mq;

static void *app_task_executor(void *argv)
{
    int count = argv;
    log_info("Executor %d start!", count);
    struct TaskStruct task_struct;

    while (1)
    {
        if (mq_receive(mq, (char *)&task_struct, MSG_LEN, 0) < 0)
        {
            continue;
        }
        task_struct.task(task_struct.argv);
    }
    return NULL;
}

int app_task_init(int executors)
{
    // 首先开启mq
    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MSG_LEN;
    mq = mq_open("/gateway-mqueue", O_RDWR | O_CREAT, 0644, &attr);
    if (mq < 0)
    {
        log_error("mq_open error");
        goto EXIT;
    }

    // 启动后台executor
    executors_count = executors;
    executor_ptr = malloc(executors_count * sizeof(pthread_t));
    if (!executor_ptr)
    {
        log_error("Not enough memory for task manager");
        goto MQ_EXIT;
    }
    memset(executor_ptr, 0, executors_count * sizeof(pthread_t));
    int i;
    for (i = 0; i < executors_count; i++)
    {
        if (pthread_create(executor_ptr + i, NULL, app_task_executor, (void *)i) < 0)
        {
            goto FREE_EXIT;
        }
    }
    log_info("Task manager started");

    return 0;
FREE_EXIT:
    for (i = 0; i < executors_count; i++)
    {
        if (executor_ptr[i])
        {
            pthread_cancel(executor_ptr[i]);
            pthread_join(executor_ptr[i], NULL);
        }
    }

    free(executor_ptr);
MQ_EXIT:
    mq_unlink("/gateway-mqueue");
EXIT:
    return -1;
}

int app_task_register(Task task, void *args)
{
    struct TaskStruct task_struct = {
        .task = task,
        .argv = args,
    };
    int result = mq_send(mq, (char *)&task_struct, MSG_LEN, 0);
    log_debug("Task %p registered", task);
    return result;
}

void app_task_wait(void)
{
    for (int i = 0; i < executors_count; i++)
    {
        if (executor_ptr[i])
        {
            pthread_join(executor_ptr[i], NULL);
        }
    }

    free(executor_ptr);
    mq_unlink("/gateway-mqueue");
    log_info("Task manager closed.");
}

void app_task_close()
{
    log_info("Closing task manager...");
    for (int i = 0; i < executors_count; i++)
    {
        if (executor_ptr[i])
        {
            pthread_cancel(executor_ptr[i]);
        }
    }
}
