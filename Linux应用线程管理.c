/*
什么是线程？
    线程是进程中的一个执行单元，是CPU调度的最大单位。
    与进程不同，同一个进程中的多个线程共享内存空间（
    包括代码段、数据段和堆），但每个线程都有独立的栈
    和寄存器。

优点：
    线程创建和切换头部比进程小。线程间通信消耗特殊机制，直接共享内存。
缺点：
    线程间共享内存可能导致数据竞争，需要同步机制。


Pthreads简介
    Pthreads（POSIX Threads）是 POSIX 标准定义的线程库，在 Linux 上通
    过<pthread.h>头文件使用。编译时需链接线程库-pthread。
*/

======================================================================
/*
创建线程

函数原型
    int pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
                       void *(*start_routine)(void *), void *arg);

    thread ：线程标识符。
    attr：线程属性（NULL 表示默认）。
    start_routine ：线程执行的函数。
    arg：传递给线程函数的参数。
*/
#include <stdio.h>
#include <pthread.h>

void *thread_func(void *arg)
{
    printf("线程运行中，线程 ID: %lu\n", pthread_self());
    return NULL;
}

int main() {
    pthread_t tid;
    int ret = pthread_create(&tid, NULL, thread_func, NULL);
    if (ret != 0) {
        perror("pthread_create 失败");
        return 1;
    }
    printf("主线程 ID: %lu\n", pthread_self());
    pthread_join(tid, NULL); // 等待线程结束
    return 0;
}

/*
编译：gcc -o thread thread.c -pthread
显示：
      主线程 ID: 140735673651008
      线程运行中，线程 ID: 140735665258240
*/
==================================================================
/*
线程终止
    线程可以通过以下方式终止：
        线程函数返回，调用pthread_exit
        被其他线程通过pthread_cancel取消

函数原型
    void pthread_exit(void *retval);       // 线程主动退出
    int pthread_cancel(pthread_t thread);  // 取消指定线程
*/
#include <stdio.h>
#include <pthread.h>

void *thread_func(void *arg)
{
    printf("线程开始运行\n");
    sleep(1);                 // 模拟工作
    pthread_exit((void *)42); // 退出并返回值
    printf("这行不会执行\n"); // 不会到达
}

int main() {
    pthread_t tid;
    void *retval;
    pthread_create(&tid, NULL, thread_func, NULL);
    pthread_join(tid, &retval); // 等待线程并获取返回值
    printf("线程返回值: %ld\n", (long)retval);
    return 0;
}

/*
等待线程结束
    使用pthread_join等待线程结束并恢复资源。

函数原型
    int pthread_join(pthread_t thread, void **retval);

    thread：要等待的线程。
    retval：接收线程的返回值。

注
    如果不调用pthread_join，线程结束后会变成僵尸线程，浪费资源。
*/
=================================================================
/*
线程介绍

    默认情况下，线程是可加入的（joinable）。如果线程被等待，可
    设置为分离状态（detached），结束后自动释放资源。

函数原型
    int pthread_detach(pthread_t thread);
*/
#include <stdio.h>
#include <pthread.h>

void *thread_func(void *arg)
{
    printf("分离线程运行中\n");
    return NULL;
}

int main() {
    pthread_t tid;
    pthread_create(&tid, NULL, thread_func, NULL);
    pthread_detach(tid); // 设置为分离状态
    sleep(1); // 确保线程运行完成
    printf("主线程结束\n");
    return 0;
}

====================================================================
/*
线程同步 - 互斥锁
    多线程共享内存时，需避免数据竞争。互斥锁（Mutex）是异步的同步工具。

相关函数
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;                               // 静态初始化
    int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr); // 动态初始化
    int pthread_mutex_lock(pthread_mutex_t *mutex);                                  // 加锁
    int pthread_mutex_unlock(pthread_mutex_t *mutex);                                // 解锁
    int pthread_mutex_destroy(pthread_mutex_t *mutex);                               // 销毁
*/

#include <stdio.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int shared_counter = 0;

void *thread_func(void *arg) {
    for (int i = 0; i < 100000; i++) {
        pthread_mutex_lock(&mutex);
        shared_counter++;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, thread_func, NULL);
    pthread_create(&t2, NULL, thread_func, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("最终计数: %d\n", shared_counter);
    return 0;
}

/*
显示：最终数量： 200000

说明：若无锁，shared_counter的价值会因竞争而不确定。
*/
=====================================================================

/*
线程同步-条件变量
    用于线程间协作的条件变量，解决“等待特定条件成立”的问题，常与互斥锁结合使用。

相关函数
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER; // 静态初始化
    int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex); // 等待条件
    int pthread_cond_signal(pthread_cond_t *cond); // 唤醒一个等待线程
    int pthread_cond_broadcast(pthread_cond_t *cond); // 唤醒所有等待线程
*/

#include <stdio.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int buffer = 0;

void *producer(void *arg)
{
    pthread_mutex_lock(&mutex);
    buffer = 1; // 生产数据
    printf("生产者生产数据: %d\n", buffer);
    pthread_cond_signal(&cond); // 通知消费者
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void *consumer(void *arg)
{
    pthread_mutex_lock(&mutex);
    while (buffer == 0) { // 等待数据
        pthread_cond_wait(&cond, &mutex);
    }
    printf("消费者消费数据: %d\n", buffer);
    buffer = 0;
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main()
{
    pthread_t prod, cons;
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);
    return 0;
}

/*
显示
    生产者生产数据: 1
    消费者消费数据: 1
*/

================================================================
/*
线程属性
    线程属性通过pthread_attr_t设置，如栈大小、分离状态等。

相关函数
    int pthread_attr_init(pthread_attr_t *attr);
    int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
    int pthread_attr_destroy(pthread_attr_t *attr);
*/
#include <stdio.h>
#include <pthread.h>

void *thread_func(void *arg) {
    printf("分离线程运行\n");
    return NULL;
}

int main() {
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); // 设置为分离
    pthread_create(&tid, &attr, thread_func, NULL);
    pthread_attr_destroy(&attr);
    sleep(1);
    printf("主线程结束\n");
    return 0;
}

/*
显示：
    分离线程运行
    主线程结束
*/
===============================================================

/*
综合实例：多线程成员
    下面是一个综合实例，结合锁、条件变量和线程分离。
*/
#include <stdio.h>
#include <pthread.h>

#define THREAD_NUM 3

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int counter = 0;

void *worker(void *arg)
{
    int id = *(int *)arg;
    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&mutex);
        while (counter % THREAD_NUM != id) { // 等待轮到自己
            pthread_cond_wait(&cond, &mutex);
        }
        printf("线程 %d: 计数 = %d\n", id, counter);
        counter++;
        pthread_cond_broadcast(&cond);      // 唤醒所有线程
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
    return NULL;
}

int main() {
    pthread_t threads[THREAD_NUM];
    int ids[THREAD_NUM];
    for (int i = 0; i < THREAD_NUM; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, worker, &ids[i]);
        pthread_detach(threads[i]); // 设置分离
    }
    sleep(16); // 等待所有线程完成
    printf("主线程结束\n");
    return 0;
}

/*
显示：
    线程 0: 计数 = 0
    线程 1: 计数 = 1
    线程 2: 计数 = 2
    线程 0: 计数 = 3
    ...
    主线程结束
*/