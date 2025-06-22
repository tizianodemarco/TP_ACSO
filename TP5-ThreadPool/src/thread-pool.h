#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>
#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include "Semaphore.h"

using namespace std;

typedef struct worker {
    thread ts;
    Semaphore sem;
    std::function<void(void)> job;
    std::atomic<bool> available{true};
    std::mutex job_mtx; // para que no se le pisen la asignación de tareas
} worker_t;

class ThreadPool {
  public:
    ThreadPool(size_t numThreads);
    void schedule(const function<void(void)>& thunk);
    void wait();
    ~ThreadPool();

  private:
    void worker_loop(int id);
    void dispatcher();

    std::thread dt;
    std::vector<std::unique_ptr<worker_t>> wts;
    std::queue<std::function<void(void)>> tasks;
    std::mutex mtx;
    Semaphore taskAvailable;
    std::condition_variable cv_done; // para esperar a que terminen
    std::mutex mtx_done; // mutex para lo de arriba
    std::atomic<size_t> remainingTasks{0};
    std::atomic<bool> stopping{false};

    // 🔁 NUEVO: cola de workers disponibles
    std::queue<worker_t*> available_workers;
    std::mutex mtx_workers;
    Semaphore workerAvailable;

    ThreadPool(const ThreadPool& original) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
};

#endif
