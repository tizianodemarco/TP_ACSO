/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
#include "Semaphore.h"

#include <queue>
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <functional>
#include <atomic>

using namespace std;

ThreadPool::ThreadPool(size_t numThreads) {
    // wts.resize(numThreads);

    // // Lanzar workers
    // for (size_t i = 0; i < numThreads; ++i) {
    //     wts[i].ts = std::thread([this, i] { worker(i); });
    // }
    for (size_t i = 0; i < numThreads; ++i) {
        wts.emplace_back(std::make_unique<worker_t>()); // sin argumentos
        wts[i]->ts = std::thread([this, i]{ worker_loop(i); });
    }
    // Lanzar dispatcher
    dt = std::thread([this] { dispatcher(); });
}

// Encola tareas y avisa al dispatcher
void ThreadPool::schedule(const function<void(void)>& thunk) {
    {
        std::unique_lock<std::mutex> lock(mtx);
        this->tasks.push(thunk);
        this->remainingTasks++;
    }
    taskAvailable.signal();  // Avisar al dispatcher
}

// Extrae tareas, busca un worker libre, le asigna la tarea y lo despierta
void ThreadPool::dispatcher() {
    while (true) {
        taskAvailable.wait();

        if (stopping) break;

        std::function<void(void)> task;
        {
            std::unique_lock<std::mutex> lock(mtx);
            if (tasks.empty()) continue;

            task = tasks.front();
            tasks.pop();
        }

        // Buscar un worker disponible
        while (true) {
            for (auto& w : wts) {
                if (w->available.exchange(false)) {  // Lo marco como ocupado
                    w->job = task;
                    w->sem.signal();  // Le digo al worker que arranque
                    goto next_task;
                }
            }
            std::this_thread::yield();  // Esperar activamente hasta que haya un worker
        }
    next_task:;
    }
}

// Ejecuta una tarea y se marca como disponible nuevamente
void ThreadPool::worker_loop(int id) {
    while (true) {
        wts[id]->sem.wait();  // Esperar señal del dispatcher

        if (stopping) break;

        if (wts[id]->job) {
            wts[id]->job();  // Ejecutar tarea
            wts[id]->job = nullptr;

            remainingTasks--;

            if (remainingTasks == 0)
                allTasksDone.signal();

            wts[id]->available = true;  // Se libera
        }
    }
}

// Espera a que todas las tareas se completen
void ThreadPool::wait() {
    while (remainingTasks > 0) {
    allTasksDone.wait();  // Esperar a que se terminen
    }
}

// Espera tareas, finaliza todos los hilos correctamente
ThreadPool::~ThreadPool() {
    wait();  // Esperar que terminen

    stopping = true;

    // Despertar todos los hilos
    taskAvailable.signal();  // Para el dispatcher

    dt.join();

    for (auto& w : wts) {
        w->sem.signal();  // Despertar a los workers por si están esperando
        if (w->ts.joinable())
            w->ts.join();
    }
}
