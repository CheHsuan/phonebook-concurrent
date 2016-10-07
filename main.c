#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include "malloc.h"

#include IMPL

#define DICT_FILE "./dictionary/words.txt"

#if defined(OPT)
#include "file.c"
#include "debug.h"
#include <fcntl.h>
#define ALIGN_FILE "align.txt"
#endif

#ifndef THREAD_NUM
#define THREAD_NUM 4
#endif

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

int main(int argc, char *argv[])
{
#ifndef OPT
    FILE *fp;
    int i = 0;
    char line[MAX_LAST_NAME_SIZE];
#else
    struct timespec mid;
#endif
    struct timespec start, end;
    double cpu_time1, cpu_time2;

#ifndef OPT
    /* check file opening */
    fp = fopen(DICT_FILE, "r");
    if (!fp) {
        printf("cannot open the file\n");
        return -1;
    }
#else
    file_align(DICT_FILE, ALIGN_FILE, MAX_LAST_NAME_SIZE);
    int fd = open(ALIGN_FILE, O_RDWR | O_NONBLOCK);
    off_t fs = fsize( ALIGN_FILE);
#endif

    /* build the entry */
    entry *pHead, *e;
#if defined(OPT)
    pHead = init_entry(sizeof(entry), fs / MAX_LAST_NAME_SIZE);
#else
    pHead = init_entry(sizeof(entry), 1);
#endif
    assert(pHead && "pHead error");
    printf("size of entry : %lu bytes\n", sizeof(entry));
    e = pHead;

#if defined(__GNUC__)
    __builtin___clear_cache((char *) pHead, (char *) pHead + sizeof(entry));
#endif

#if defined(OPT)
    clock_gettime(CLOCK_REALTIME, &start);

    char *map = mmap(NULL, fs, PROT_READ, MAP_SHARED, fd, 0);

    assert(map!=MAP_FAILED && "mmap error");

    pthread_setconcurrency(THREAD_NUM + 1);

    pthread_t *tid = (pthread_t *) malloc(sizeof( pthread_t) * THREAD_NUM);
    thread_task **task = (thread_task **) malloc(sizeof(thread_task *) * THREAD_NUM);
    for (int i = 0; i < THREAD_NUM; i++)
        task[i] = assign_thread_task(map + MAX_LAST_NAME_SIZE * i, map + fs, i, THREAD_NUM, pHead + i);

    clock_gettime(CLOCK_REALTIME, &mid);
    //clock_gettime(CLOCK_REALTIME, &mid);
    for (int i = 0; i < THREAD_NUM; i++)
        pthread_create( &tid[i], NULL, (void *) &append, (void *) task[i]);

    for (int i = 0; i < THREAD_NUM; i++)
        pthread_join(tid[i], NULL);

    entry *etmp;
    //pHead = pHead->pNext;
    for (int i = 0; i < THREAD_NUM; i++) {
        if (i == 0) {
            pHead = task[i]->pHead->pNext;
            dprintf("Connect %d head string %s %p\n", i, task[i]->pHead->pNext->lastName, task[i]->start);
        } else {
            etmp->pNext = task[i]->pHead->pNext;
            dprintf("Connect %d head string %s %p\n", i, task[i]->pHead->pNext->lastName, task[i]->start);
        }

        etmp = task[i]->pLast;
        dprintf("Connect %d tail string %s %p\n", i, task[i]->pLast->lastName, task[i]->start);
        dprintf("round %d\n", i);
    }

    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time1 = diff_in_second(start, end);

#else
    clock_gettime(CLOCK_REALTIME, &start);
    while (fgets(line, sizeof(line), fp)) {
        while (line[i] != '\0')
            i++;
        line[i - 1] = '\0';
        i = 0;
        e = append(line, e);
    }

    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time1 = diff_in_second(start, end);

#endif

#ifndef OPT
    /* close file as soon as possible */
    fclose(fp);
#endif

    /* the givn last name to find */
    char input[MAX_LAST_NAME_SIZE] = "zyxel";
    e = pHead;

    assert(findName(input, e) &&
           "Did you implement findName() in " IMPL "?");
    assert(0 == strcmp(findName(input, e)->lastName, "zyxel"));

#if defined(__GNUC__)
    __builtin___clear_cache((char *) pHead, (char *) pHead + sizeof(entry));
#endif
    /* compute the execution time */
    clock_gettime(CLOCK_REALTIME, &start);
    findName(input, e);
    clock_gettime(CLOCK_REALTIME, &end);
    cpu_time2 = diff_in_second(start, end);

    FILE *output;
#if defined(OPT)
    output = fopen("opt.txt", "a");
#else
    output = fopen("orig.txt", "a");
#endif
    fprintf(output, "append() findName() %lf %lf\n", cpu_time1, cpu_time2);
    fclose(output);

    printf("execution time of append() : %lf sec\n", cpu_time1);
    printf("execution time of findName() : %lf sec\n", cpu_time2);

    free_entry(pHead);

#if defined(OPT)
    free(tid);
    free(task);
    munmap(map, fs);
#endif
    return 0;
}
