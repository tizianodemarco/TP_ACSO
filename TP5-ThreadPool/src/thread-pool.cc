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
    wts.reserve(numThreads);
    // Inicializar todos los unique_ptr
    for (size_t i = 0; i < numThreads; ++i) {
        wts.emplace_back(std::make_unique<worker_t>()); // sin argumentos
    }
    // Lanzar los workers 
    for (size_t i = 0; i < numThreads; ++i) {
        wts[i]->ts = std::thread([this, i]{ worker_loop(i); });
    }
    // Lanzar dispatcher
    dt = std::thread([this] { dispatcher(); });
}

// Encola tareas y avisa al dispatcher
void ThreadPool::schedule(const function<void(void)>& thunk) {
    if (stopping) throw std::runtime_error("ThreadPool is stopping; can't schedule new tasks"); // evitar agregar tareas si se está deteniendo
    if (!thunk) throw std::invalid_argument("Cannot schedule nullptr task"); // Validar que la tarea no sea nula
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
                if (w->available.exchange(false)) {
                    {
                        std::lock_guard<std::mutex> lock(w->job_mtx);
                        w->job = task;
                    }
                    w->sem.signal();  // Despertar worker
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

        std::function<void(void)> task_to_run;

        {
            std::lock_guard<std::mutex> lock(wts[id]->job_mtx);
            task_to_run = wts[id]->job;
            wts[id]->job = nullptr;
        }

        if (task_to_run) {
            task_to_run();  // Ejecutar fuera del lock
            {
                std::lock_guard<std::mutex> lock(mtx_done);
                if (--remainingTasks == 0){
                    cv_done.notify_all();
                }
            }   
            wts[id]->available = true;  // Se libera
        }
    }
}

void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(mtx_done);
    cv_done.wait(lock, [this]() { return remainingTasks == 0; });
    // todos los que estan esperando por wait se desbloquean cuando terminan las tareas 
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
