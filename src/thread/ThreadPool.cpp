#include <thread>
#include <vector>

#include "logging/LoggerFactory.h"
#include "thread/ThreadPool.h"

namespace StateManager {

ThreadPool::ThreadPool() : logger_(logging::LoggerFactory::getLogger()) {
  this->log(logging::LogLevel::TRACE, "Initialized ThreadPool");
  this->start();
}

ThreadPool::~ThreadPool() {
  this->shutdown();
  this->log(logging::LogLevel::TRACE, "Destroyed ThreadPool");
}

void ThreadPool::start() {
  if (this->running_)
    return;
  this->running_ = true;
  this->managementThread_ = std::thread(&ThreadPool::managerFunction, this);
}

void ThreadPool::shutdown() {
  this->log(logging::LogLevel::TRACE, "Begining shutdown ThreadPool");
  this->running_ = false;
  this->taskCondition_.notify_all();
  // TODO: improve safe shutdown
  while (this->managementThread_.joinable()) {
    this->managementThread_.join();
  }
}

void ThreadPool::setScaleUp(
    std::function<bool(const uint queueSize, const uint workerCount)> func) {
  std::unique_lock<std::mutex> lock(this->taskMutex_);
  this->scaleUp = func;
}

void ThreadPool::setScaleDown(
    std::function<bool(const uint queueSize, const uint workerCount)> func) {
  std::unique_lock<std::mutex> lock(this->taskMutex_);
  this->scaleDown = func;
}

void ThreadPool::workerFunction(std::atomic<bool> &finished) {
  this->log(logging::LogLevel::TRACE, "Starting worker thread");
  while (true) {
    std::unique_lock<std::mutex> lock(this->taskMutex_);
    taskCondition_.wait(lock, [this] {
      return !taskQueue_.empty() || !this->running_ ||
             this->killThread.load() > 0;
    });

    uint killCount = this->killThread.load();
    if ((killCount > 0 &&
         this->killThread.compare_exchange_weak(killCount, killCount - 1)) ||
        (!this->running_ && this->taskQueue_.empty())) {
      this->log(logging::LogLevel::INFO, "Finishing worker");
      finished.store(true);
      break;
    }

    if (!this->taskQueue_.empty()) {
      WorkerTask task = std::move(this->taskQueue_.front());
      this->taskQueue_.pop();
      lock.unlock();
      this->log(logging::LogLevel::TRACE, "Processing task");
      try {
        task.task(task.payload);
      } catch (const std::exception &e) {
        this->log(logging::LogLevel::ERROR,
                  "Error processing task: " + std::string(e.what()));
      }
    } else {
      this->log(logging::LogLevel::TRACE, "Empty queue");
    }
  }
}

void ThreadPool::managerFunction() {
  int queueSize = 0;
  int workerCount = 0;

  struct Worker {
    std::thread thread;
    std::unique_ptr<std::atomic<bool>> finished;
    Worker() : finished(std::make_unique<std::atomic<bool>>(false)) {}
    Worker(Worker &&) = default;
    Worker &operator=(Worker &&) = default;
  };
  std::vector<Worker> workerPool;
  workerPool.reserve(10);

  while (this->running_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::unique_lock<std::mutex> lock(this->taskMutex_);
    queueSize = this->taskQueue_.size();
    lock.unlock();

    if (this->scaleUp(queueSize, workerCount)) {
      workerPool.emplace_back();
      Worker &worker = workerPool.back();
      worker.thread = std::thread(&ThreadPool::workerFunction, this,
                                  std::ref(*worker.finished));
      workerCount++;
      this->log(logging::LogLevel::INFO, "Scaled up worker thread, total: " +
                                             std::to_string(workerCount));
    } else if (this->scaleDown(queueSize, workerCount)) {
      // Send kill signal to one worker thread
      uint killCount = this->killThread.load();
      if (killCount == 0) {
        this->log(logging::LogLevel::TRACE, "Sent Thread kill signal.");
        this->killThread.compare_exchange_weak(killCount, killCount + 1);
        this->taskCondition_.notify_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      // Use iterator-based loop to safely erase while iterating
      for (auto it = workerPool.begin(); it != workerPool.end();) {
        if (it->finished->load() && it->thread.joinable()) {
          it->thread.join();
          it = workerPool.erase(it);
          workerCount--;
          this->log(logging::LogLevel::INFO,
                    "Scaled down worker thread, total: " +
                        std::to_string(workerCount));
        } else {
          it++;
        }
      }
    }
  }
  
  // Wait for all threads to be joined after cleanup queue
  this->taskCondition_.notify_all();
  while (workerCount) {
    // Use iterator-based loop to safely erase while iterating
    for (auto it = workerPool.begin(); it != workerPool.end();) {
      if (it->finished->load() && it->thread.joinable()) {
        it->thread.join();
        it = workerPool.erase(it);
        workerCount--;
        this->log(logging::LogLevel::INFO,
                  "Shuttind down thread, total: " +
                      std::to_string(workerCount));
      } else {
        it++;
      }
    }
  }
  
  this->log(logging::LogLevel::TRACE, "Leaving");
}

void ThreadPool::addTask(std::function<void(const std::string &)> task,
                         std::string payload) {
  std::unique_lock<std::mutex> lock(this->taskMutex_);
  this->log(logging::LogLevel::TRACE, "Pushing task & notifying");
  this->taskQueue_.push({task, payload});
  lock.unlock();
  taskCondition_.notify_one();
}

} // namespace StateManager
