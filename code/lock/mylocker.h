#pragma once

#include <pthread.h>
#include <semaphore.h>

#include <exception>

class MySem {
   public:
    MySem(int num = 0) {
        if (sem_init(&my_sem, 0, num) != 0) {
            throw std::exception();
        }
    }
    ~MySem() {
        sem_destroy(&my_sem);
    }

    MySem(MySem&) = delete;
    MySem& operator=(MySem&) = delete;

    bool wait() {
        return sem_wait(&my_sem) == 0;
    }
    bool post() {
        return sem_post(&my_sem) == 0;
    }

   private:
    sem_t my_sem;
};

class MyLocker {
   public:
    MyLocker() {
        pthread_mutex_init(&my_mutex, nullptr);
    }
    ~MyLocker() {
        pthread_mutex_destroy(&my_mutex);
    }

    MyLocker(MyLocker&) = delete;
    MyLocker& operator=(MyLocker&) = delete;

    bool myLock() {
        return pthread_mutex_lock(&my_mutex) == 0;
    }
    bool myUnlock() {
        return pthread_mutex_unlock(&my_mutex) == 0;
    }
    pthread_mutex_t* myMutexGet() {
        return &my_mutex;
    }

   private:
    pthread_mutex_t my_mutex;
};

class MyCond {
   public:
    MyCond() {
        pthread_cond_init(&my_cond, nullptr);
    }
    ~MyCond() {
        pthread_cond_destroy(&my_cond);
    }

    MyCond(MyCond&) = delete;
    MyCond& operator=(MyCond&&) = delete;

    bool myWait(pthread_mutex_t* mymutex) {
        return pthread_cond_wait(&my_cond, mymutex) == 0;
    }
    bool myTimeWait(pthread_mutex_t* mymutex, const timespec tm) {
        return pthread_cond_timedwait(&my_cond, mymutex, &tm) == 0;
    }
    bool mySignal() {
        return pthread_cond_signal(&my_cond) == 0;
    }
    bool myBroadcast() {
        return pthread_cond_broadcast(&my_cond) == 0;
    }

   private:
    pthread_cond_t my_cond;
};
