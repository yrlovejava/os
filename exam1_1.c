#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define N 5  // 哲学家数量
pthread_mutex_t chopsticks[N];  // 定义互斥锁表示筷子

// 哲学家线程函数
void *philosopher(void *num) {
    int id = *(int *)num;
    int left = id;
    int right = (id + 1) % N;

    while (1) {
        // 思考
        printf("哲学家 %d 正在思考\n", id);
        sleep(1);

        // 尝试拿起筷子
        if (id == N - 1) {
            // 最后一个哲学家先拿右边的筷子，避免死锁
            pthread_mutex_lock(&chopsticks[right]);
            printf("哲学家 %d 拿起了右边的筷子 %d\n", id, right);
            pthread_mutex_lock(&chopsticks[left]);
            printf("哲学家 %d 拿起了左边的筷子 %d\n", id, left);
        } else {
            pthread_mutex_lock(&chopsticks[left]);
            printf("哲学家 %d 拿起了左边的筷子 %d\n", id, left);
            pthread_mutex_lock(&chopsticks[right]);
            printf("哲学家 %d 拿起了右边的筷子 %d\n", id, right);
        }

        // 进餐
        printf("哲学家 %d 正在进餐\n", id);
        sleep(1);

        // 放下筷子
        pthread_mutex_unlock(&chopsticks[left]);
        printf("哲学家 %d 放下了左边的筷子 %d\n", id, left);
        pthread_mutex_unlock(&chopsticks[right]);
        printf("哲学家 %d 放下了右边的筷子 %d\n", id, right);
    }

    return NULL;
}

int main() {
    pthread_t threads[N];
    int ids[N];

    // 初始化互斥锁
    for (int i = 0; i < N; i++) {
        pthread_mutex_init(&chopsticks[i], NULL);
    }

    // 创建哲学家线程
    for (int i = 0; i < N; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, philosopher, &ids[i]);
    }

    // 等待线程结束（实际上不会结束）
    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }

    // 销毁互斥锁
    for (int i = 0; i < N; i++) {
        pthread_mutex_destroy(&chopsticks[i]);
    }

    return 0;
}