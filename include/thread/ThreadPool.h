#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <string>
#include <queue>
#include <condition_variable>

#include "logging/LoggerIface.h"

namespace StateManager {

class ThreadPool {
    private:
        std::shared_ptr<logging::LoggerIface> logger_;
        void log(logging::LogLevel level, const std::string &message) const {
            logger_->log(level, message, "ThreadPool");
        }
        
        std::atomic<bool> running_{false};
        std::thread managementThread_;

        // TODO: try to make this WorkerTask queue templated
        struct WorkerTask {
            std::function<void(const std::string&)> task;
            std::string payload;
        };
        std::mutex taskMutex_;
        std::queue<WorkerTask> taskQueue_;
        std::condition_variable taskCondition_;
        std::atomic<uint> killThread{0};

        std::function<bool(const uint queueSize, const uint workerCount)> scaleUp = [](uint queueSize, uint workerCount) {
            return (queueSize > 10 * workerCount && workerCount < 10);
        };
        std::function<bool(const uint queueSize, const uint workerCount)> scaleDown = [](uint queueSize, uint workerCount) {
            return (queueSize < 3 * workerCount && workerCount > 1);
        };

        void start();
        void shutdown();
        void workerFunction(std::atomic<bool> &finished);
        void managerFunction();

    public:
        ThreadPool();
        ~ThreadPool();
        void addTask(std::function<void(const std::string&)> task, std::string payload);
        void setScaleUp(std::function<bool(const uint queueSize, const uint workerCount)> func);
        void setScaleDown(std::function<bool(const uint queueSize, const uint workerCount)> func);
};

} // namespace StateManager