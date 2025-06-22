#include "thread-pool.h"
#include "Semaphore.h"

#include <functional>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

using namespace std;

ThreadPool::ThreadPool(size_t numThreads) {
    wts.reserve(numThreads);  // reservamos lugar para los workers

    // creamos los workers y los guardamos
    for (size_t i = 0; i < numThreads; ++i) {
        wts.emplace_back(std::unique_ptr<worker_t>(new worker_t()));
    }

    // arrancamos los hilos que corren el loop del worker
    for (size_t i = 0; i < numThreads; ++i) {
        wts[i]->ts = std::thread([this, i]{ worker_loop(i); });
    }

    // metemos todos los workers en la cola de disponibles
    for (size_t i = 0; i < numThreads; ++i) {
        {
            std::lock_guard<std::mutex> lock(mtx_workers);
            available_workers.push(wts[i].get());
        }
        workerAvailable.signal(); // avisamos que hay uno libre
    }

    // arrancamos el dispatcher en su propio hilo
    dt = std::thread([this] { dispatcher(); });
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    if (stopping) throw std::runtime_error("el threadpool se está apagando");
    if (!thunk) throw std::invalid_argument("tarea vacía, no va");

    {
        std::unique_lock<std::mutex> lock(mtx);
        tasks.push(thunk);     // metemos la tarea en la cola
        remainingTasks++;      // subimos el contador
    }

    taskAvailable.signal();    // avisamos que hay tarea nueva
}

void ThreadPool::dispatcher() {
    while (true) {
        taskAvailable.wait();    // esperamos que haya tareas

        if (stopping) break;     // si se apagó, salimos

        std::function<void(void)> task;

        {
            std::unique_lock<std::mutex> lock(mtx);
            if (tasks.empty()) continue;  // a veces no hay tareas, seguimos esperando
            task = tasks.front();         // agarramos la primera
            tasks.pop();                  // la sacamos
        }

        workerAvailable.wait();  // esperamos que haya un worker libre

        worker_t* free_worker = nullptr;
        {
            std::lock_guard<std::mutex> lock(mtx_workers);
            if (!available_workers.empty()) {
                free_worker = available_workers.front();
                available_workers.pop();
            }
        }

        if (!free_worker) continue; // seguridad extra por si falla algo

        {
            std::lock_guard<std::mutex> lock(free_worker->job_mtx);
            free_worker->job = task;  // le asignamos la tarea al worker
        }

        free_worker->sem.signal();  // le decimos que arranque
    }
}

void ThreadPool::worker_loop(int id) {
    while (true) {
        wts[id]->sem.wait();  // espera a que lo despierten

        if (stopping) break;  // si cerramos, sale

        std::function<void(void)> task_to_run;
        {
            std::lock_guard<std::mutex> lock(wts[id]->job_mtx);
            task_to_run = wts[id]->job;  // agarramos la tarea
            wts[id]->job = nullptr;      // la limpiamos
        }

        if (task_to_run) {
            task_to_run();  // la ejecutamos

            {
                std::lock_guard<std::mutex> lock(mtx_done);
                if (--remainingTasks == 0){
                    cv_done.notify_all();  // si no quedan tareas, avisamos
                }
            }

            {
                std::lock_guard<std::mutex> lock(mtx_workers);
                available_workers.push(wts[id].get());  // lo devolvemos a la cola de libres
            }
            workerAvailable.signal(); // avisamos que hay un worker más libre
        }
    }
}

void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(mtx_done);
    // esperamos hasta que no quede nada pendiente
    cv_done.wait(lock, [this]() { return remainingTasks == 0; });
}

ThreadPool::~ThreadPool() {
    wait();              // esperamos a que terminen todas
    stopping = true;     // avisamos que se cierra

    taskAvailable.signal();  // despertamos al dispatcher

    dt.join();           // lo esperamos

    // despertamos a los workers para que salgan y los esperamos
    for (auto& w : wts) {
        w->sem.signal(); // por si están dormidos esperando tareas
        if (w->ts.joinable())
            w->ts.join();
    }
}
