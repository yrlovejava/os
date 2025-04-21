#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define N 10 // 缓冲区数量
#define NUM_PRODUCERS 3
#define NUM_CONSUMERS 4

pthread_t producer_threads[NUM_PRODUCERS];
pthread_t consumer_threads[NUM_CONSUMERS];

sem_t full;
sem_t empty;
pthread_mutex_t mutex;
int buffer[N];

int in = 0;
int out = 0;

// 生产者线程函数
void *producer(void *arg) {
    int p_num = *(int *)arg;
    int item;
    for (item = 0; item < 5; item++) {
        sem_wait(&empty);
        // 获取锁
        pthread_mutex_lock(&mutex);
        // 生产数据
        buffer[in] = item;
        printf("Producer %d produced item %d in buffer[%d]\n", p_num, item, in);
        in = (in + 1) % N;
        sleep(2);
        // 释放锁
        pthread_mutex_unlock(&mutex);
        // 增加信号量
        sem_post(&full);
    }
    free(arg);
    return NULL;
}

// 消费者线程函数
void *consumer(void *arg) {
    int c_num = *(int *)arg;
    int item;
    for (item = 0; item < 5; item++) {
        sem_wait(&full);
        // 获取锁
        pthread_mutex_lock(&mutex);
        // 消费数据
        int consume = buffer[out];
        printf("Consumer %d consumed item %d in buffer[%d]\n", c_num, consume, out);
        out = (out + 1) % N;
        sleep(1);
        // 释放锁
        pthread_mutex_unlock(&mutex);
        // 增加信号量
        sem_post(&empty);
    }
    free(arg);
    return NULL;
}

int main() {
    int p_num, c_num;
    // 初始化信号量
    sem_init(&full, 0, 0);
    sem_init(&empty, 0, N);
    // 初始化互斥锁
    pthread_mutex_init(&mutex, NULL);

    // 创建生产者线程
    for (p_num = 0; p_num < NUM_PRODUCERS; p_num++) {
        int *arg = malloc(sizeof(int));
        *arg = p_num;
        pthread_create(&producer_threads[p_num], NULL, producer, arg);
    }
    // 创建消费者线程
    for (c_num = 0; c_num < NUM_CONSUMERS; c_num++) {
        int *arg = malloc(sizeof(int));
        *arg = c_num;
        pthread_create(&consumer_threads[c_num], NULL, consumer, arg);
    }
    // 等待线程结束
    for (p_num = 0; p_num < NUM_PRODUCERS; p_num++) {
        pthread_join(producer_threads[p_num], NULL);
    }
    for (c_num = 0; c_num < NUM_CONSUMERS; c_num++) {
        pthread_join(consumer_threads[c_num], NULL);
    }

    // 销毁信号量
    sem_destroy(&full);
    sem_destroy(&empty);
    // 销毁互斥锁
    pthread_mutex_destroy(&mutex);
    return 0;
}