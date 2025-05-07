#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 10
#define NUM_PRODUCERS 3
#define NUM_CONSUMERS 4

/* 共享缓冲区结构 */
char buffer[BUFFER_SIZE];       // 未显式初始化（内容无关紧要）
int in = 0;                     // 显式初始化为0（下一个写入位置）
int out = 0;                    // 显式初始化为0（下一个读取位置）
int count = 0;                  // 显式初始化为0（缓冲区元素计数）

/* 同步机制 */
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;    // 静态初始化互斥锁
pthread_cond_t buffer_not_full = PTHREAD_COND_INITIALIZER;   // 静态初始化条件变量
pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;  // 静态初始化条件变量

/* 文件相关 */
int fd;                         // 通过open()函数初始化
off_t file_offset = 0;          // 显式初始化为0（文件读取偏移量）
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;       // 静态初始化

/* 生产者状态 */
int producers_done = 0;         // 显式初始化为0（已完成生产者计数）
pthread_mutex_t producers_done_mutex = PTHREAD_MUTEX_INITIALIZER; // 静态初始化

void* producer(void* arg) {
    while (1) {
        char data;
        ssize_t bytes_read;

        /* 同步文件读取 */
        pthread_mutex_lock(&file_mutex);
        bytes_read = pread(fd, &data, 1, file_offset);
        if (bytes_read <= 0) {
            pthread_mutex_unlock(&file_mutex);
            break;
        }
        file_offset++;
        pthread_mutex_unlock(&file_mutex);

        /* 缓冲区写入操作 */
        pthread_mutex_lock(&buffer_mutex);
        while (count == BUFFER_SIZE) {
            pthread_cond_wait(&buffer_not_full, &buffer_mutex);
        }
        buffer[in] = data;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        pthread_cond_signal(&buffer_not_empty);
        pthread_mutex_unlock(&buffer_mutex);
    }

    /* 更新完成状态 */
    pthread_mutex_lock(&producers_done_mutex);
    producers_done++;
    if (producers_done == NUM_PRODUCERS) {
        pthread_mutex_lock(&buffer_mutex);
        pthread_cond_broadcast(&buffer_not_empty); // 唤醒所有等待消费者
        pthread_mutex_unlock(&buffer_mutex);
    }
    pthread_mutex_unlock(&producers_done_mutex);

    return NULL;
}

void* consumer(void* arg) {
    pthread_t tid = pthread_self(); // 获取当前线程ID
    while (1) {
        pthread_mutex_lock(&buffer_mutex);

        /* 终止条件判断 */
        while (count == 0) {
            pthread_mutex_lock(&producers_done_mutex);
            int done = (producers_done == NUM_PRODUCERS);
            pthread_mutex_unlock(&producers_done_mutex);

            if (done) {
                pthread_mutex_unlock(&buffer_mutex);
                return NULL;
            }
            pthread_cond_wait(&buffer_not_empty, &buffer_mutex);
        }

        /* 缓冲区读取操作 */
        char data = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;

        pthread_cond_signal(&buffer_not_full);
        pthread_mutex_unlock(&buffer_mutex);

        printf("Consumer[%lu] >> %c\n", (unsigned long)tid, data); // 带ID输出
        struct timespec delay = {0, 1000000}; // 1ms = 1,000,000纳秒
        nanosleep(&delay, NULL);
    }
    return NULL;
}

// 启动主程序
int main() {
    /* 初始化文件访问 */
    if ((fd = open("data.txt", O_RDONLY)) == -1) {
        perror("File open failed");
        exit(EXIT_FAILURE);
    }

    pthread_t producers[NUM_PRODUCERS], consumers[NUM_CONSUMERS];

    /* 创建生产者线程 */
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        if (pthread_create(&producers[i], NULL, producer, NULL) != 0) {
            perror("Producer creation failed");
            exit(EXIT_FAILURE);
        }
    }

    /* 创建消费者线程 */
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        if (pthread_create(&consumers[i], NULL, consumer, NULL) != 0) {
            perror("Consumer creation failed");
            exit(EXIT_FAILURE);
        }
    }

    /* 等待线程结束 */
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(consumers[i], NULL);
    }

    /* 资源清理 */
    close(fd);
    pthread_mutex_destroy(&buffer_mutex);
    pthread_cond_destroy(&buffer_not_full);
    pthread_cond_destroy(&buffer_not_empty);
    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&producers_done_mutex);

    return 0;
}