/**
 * File: thread-pool.h
 * -------------------
 * This class defines the ThreadPool class, which accepts a collection
 * of thunks (which are zero-argument functions that don't return a value)
 * and schedules them in a FIFO manner to be executed by a constant number
 * of child threads that exist solely to invoke previously scheduled thunks.
 */

#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>     // for size_t
#include <functional>  // for the function template used in the schedule signature
#include <thread>      // for thread
#include <vector>      // for vector
#include "Semaphore.h" // for Semaphore
#include <mutex>       // for mutex
#include <queue>       // for queue
#include <atomic>      // for atomic
#include <functional> // for std::function
#include <memory>      // for unique_ptr

using namespace std;

/**
 * @brief Represents a worker in the thread pool.
 * 
 * The `worker_t` struct contains information about a worker 
 * thread in the thread pool. Should be includes the thread object, 
 * availability status, the task to be executed, and a semaphore 
 * (or condition variable) to signal when work is ready for the 
 * worker to process.
 */
typedef struct worker {
    thread ts;
    // function<void(void)> thunk;
    /**
     * Complete the definition of the worker_t struct here...
     **/
    Semaphore sem;                  // Semaphore to signal the worker to start
    std::function<void(void)> job;
    std::atomic<bool> available{true};
    std::mutex job_mtx; // para protejer el acceso a job

} worker_t;

class ThreadPool {
  public:

  /**
  * Constructs a ThreadPool configured to spawn up to the specified
  * number of threads.
  */
    ThreadPool(size_t numThreads);

  /**
  * Schedules the provided thunk (which is something that can
  * be invoked as a zero-argument function without a return value)
  * to be executed by one of the ThreadPool's threads as soon as
  * all previously scheduled thunks have been handled.
  */
    void schedule(const function<void(void)>& thunk);

  /**
  * Blocks and waits until all previously scheduled thunks
  * have been executed in full.
  */
    void wait();

  /**
  * Waits for all previously scheduled thunks to execute, and then
  * properly brings down the ThreadPool and any resources tapped
  * over the course of its lifetime.
  */
    ~ThreadPool();

    // privado 
    // ThreadPool(const ThreadPool& original) = delete;
    // ThreadPool& operator=(const ThreadPool& rhs) = delete;
    
  private:

    void worker_loop(int id);
    void dispatcher();
    std::thread dt;                              // dispatcher thread handle
    std::vector<std::unique_ptr<worker_t>> wts;                 // worker thread handles. you may want to change/remove this
    std::queue<std::function<void(void)>> tasks; // queue of tasks to be executed
    // bool done;                              // flag to indicate the pool is being destroyed
    //mutex queueLock;                        // mutex to protect the queue of tasks

    /* It is incomplete, there should be more private variables to manage the structures... 
    * *
    */
    std::mutex mtx;
    Semaphore taskAvailable;
    // Semaphore allTasksDone;
    std::condition_variable cv_done;
    std::mutex mtx_done;

    std::atomic<size_t> remainingTasks{0};
    std::atomic<bool> stopping{false};
  
    /* ThreadPools are the type of thing that shouldn't be cloneable, since it's
    * not clear what it means to clone a ThreadPool (should copies of all outstanding
    * functions to be executed be copied?).
    *
    * In order to prevent cloning, we remove the copy constructor and the
    * assignment operator.  By doing so, the compiler will ensure we never clone
    * a ThreadPool. */
    ThreadPool(const ThreadPool& original) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
};

#endif

