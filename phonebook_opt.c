#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "phonebook_opt.h"
#include "debug.h"

entry *findName(char lastname[], entry *pHead)
{
    size_t len = strlen( lastname);
    while (pHead != NULL) {
        if (strncasecmp(lastname, pHead->lastName, len) == 0
                && (pHead->lastName[len] == '\n' ||
                    pHead->lastName[len] == '\0')) {
            pHead->lastName = (char *) malloc( sizeof(char) * MAX_LAST_NAME_SIZE);
            memset(pHead->lastName, '\0', MAX_LAST_NAME_SIZE);
            strcpy(pHead->lastName, lastname);
            pHead->dtl = (pdetail) malloc( sizeof( detail));
            return pHead;
        }
        dprintf("find string = %s\n", pHead->lastName);
        pHead = pHead->pNext;
    }
    return NULL;
}

thread_task *assign_thread_task(char *sptr, char *eptr, int tid, int ntd, entry *start)
{
    thread_task *task = (thread_task *) malloc(sizeof(thread_task));

    task->start = sptr;
    task->end = eptr;
    task->tid = tid;
    task->nthread = ntd;
    task->entryStart = start;

    task->pHead = (task->pLast = task->entryStart);

    return task;
}

void append(void *arg)
{
    struct timespec start, end;
    double cpu_time;

    clock_gettime( CLOCK_REALTIME, &start);

    thread_task *task = (thread_task *) arg;

    int count = 0;
    entry *j = task->entryStart;
    for (char *i = task->start; i < task->end;
            i += MAX_LAST_NAME_SIZE * task->nthread,
            j += task->nthread,count++) {
        task->pLast->pNext = j;
        task->pLast = task->pLast->pNext;

        task->pLast->lastName = i;
        dprintf("thread %d append string = %s\n", task->tid, task->pLast->lastName);
        task->pLast->pNext = NULL;
    }
    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time = diff_in_second(start, end);

    dprintf("thread take %lf sec, count %d\n", cpu_time, count);

    pthread_exit(NULL);
}

void show_entry(entry *pHead)
{
    while (pHead != NULL) {
        printf("lastName = %s\n", pHead->lastName);
        pHead = pHead->pNext;
    }
}

static double diff_in_second(struct timespec t1, struct timespec t2)
{
    struct timespec diff;
    if (t2.tv_nsec-t1.tv_nsec < 0) {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return (diff.tv_sec + diff.tv_nsec / 1000000000.0);
}
