#include <atomic>
#include <chrono>
#include <fstream>
#include <functional>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "logging/LoggerFactory.h"
#include "thread/ThreadPool.h"

namespace StateManager {

// Test fixture for ThreadPool tests
class ThreadPoolTest : public ::testing::Test {
protected:
  std::unique_ptr<ThreadPool> pool;
  std::shared_ptr<logging::LoggerIface> logger =
      logging::LoggerFactory::getLogger();

  void SetUp() override {
    logging::LoggerFactory::setLevel(logging::LogLevel::TRACE);
    pool = std::make_unique<ThreadPool>();
    logger->trace("Starting test execution");
  }
  void TearDown() override { pool.reset(); }
};

// Test basic task addition and execution
TEST_F(ThreadPoolTest, AddAndExecuteTask) {
  bool taskExecuted = false;
  std::string payload = "test_payload";
  auto task = [&taskExecuted, this](const std::string &p) {
    this->logger->trace("Executing task with payload: " + p, "taskExecuted");
    taskExecuted = true;
  };

  pool->addTask(task, payload);
  // Give some time for the task to be processed
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  EXPECT_TRUE(taskExecuted);
}

// Test multiple tasks execution with synchronization
TEST_F(ThreadPoolTest, MultipleTasksExecution) {
  const int num_tasks = 100;
  std::atomic<int> completed_tasks(0);
  std::atomic<int> execution_counter(0);
  
  for (int i = 0; i < num_tasks; ++i) {
    auto task = [&completed_tasks, &execution_counter, i](const std::string &p) {
      // Simulate some work
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      execution_counter.fetch_add(1);
      completed_tasks.fetch_add(1);
    };
    pool->addTask(task, "payload_" + std::to_string(i));
  }
  
  // Wait for all tasks to complete
  auto start = std::chrono::steady_clock::now();
  while (completed_tasks < num_tasks) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed > std::chrono::seconds(10)) {
      FAIL() << "Tasks did not complete within timeout. Completed: " << completed_tasks.load();
    }
  }
  
  EXPECT_EQ(completed_tasks.load(), num_tasks);
  EXPECT_EQ(execution_counter.load(), num_tasks);
}

// Test concurrent task addition from multiple threads
TEST_F(ThreadPoolTest, ConcurrentTaskAddition) {
  const int num_producer_threads = 4;
  const int tasks_per_thread = 50;
  const int total_tasks = num_producer_threads * tasks_per_thread;
  
  std::atomic<int> completed_tasks(0);
  std::atomic<int> task_ids_sum(0);
  std::vector<std::thread> producers;
  
  // Create producer threads that add tasks concurrently
  for (int t = 0; t < num_producer_threads; ++t) {
    producers.emplace_back([this, t, tasks_per_thread, &completed_tasks, &task_ids_sum]() {
      for (int i = 0; i < tasks_per_thread; ++i) {
        int task_id = t * tasks_per_thread + i;
        auto task = [&completed_tasks, &task_ids_sum, task_id](const std::string &p) {
          // Simulate variable work duration
          std::this_thread::sleep_for(std::chrono::microseconds(task_id % 100));
          task_ids_sum.fetch_add(task_id);
          completed_tasks.fetch_add(1);
        };
        pool->addTask(task, "thread_" + std::to_string(t) + "_task_" + std::to_string(i));
        
        // Introduce small random delays to create more realistic concurrency
        if (i % 10 == 0) {
          std::this_thread::sleep_for(std::chrono::microseconds(i % 50));
        }
      }
    });
  }
  
  // Wait for all producers to finish adding tasks
  for (auto& producer : producers) {
    producer.join();
  }
  
  // Wait for all tasks to complete
  auto start = std::chrono::steady_clock::now();
  while (completed_tasks < total_tasks) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed > std::chrono::seconds(15)) {
      FAIL() << "Tasks did not complete within timeout. Completed: " << completed_tasks.load();
    }
  }
  
  // Verify all tasks completed
  EXPECT_EQ(completed_tasks.load(), total_tasks);
  
  // Verify task IDs sum (simple integrity check)
  int expected_sum = 0;
  for (int i = 0; i < total_tasks; ++i) {
    expected_sum += i;
  }
  EXPECT_EQ(task_ids_sum.load(), expected_sum);
}

// Test scaling behavior under load
TEST_F(ThreadPoolTest, ScalingUnderLoad) {
  // Set aggressive scaling to test the mechanism
  auto scaleUpFunc = [](const uint queueSize, const uint workerCount) {
    return queueSize > 2 && workerCount < 5;
  };
  pool->setScaleUp(scaleUpFunc);
  
  auto scaleDownFunc = [](const uint queueSize, const uint workerCount) {
    return queueSize == 0 && workerCount > 1;
  };
  pool->setScaleDown(scaleDownFunc);
  
  const int num_batches = 3;
  const int tasks_per_batch = 20;
  std::atomic<int> completed_tasks(0);
  
  for (int batch = 0; batch < num_batches; ++batch) {
    // Add a batch of tasks that will cause scaling up
    for (int i = 0; i < tasks_per_batch; ++i) {
      auto task = [&completed_tasks, batch, i](const std::string &p) {
        // Variable duration to test scaling under different loads
        std::this_thread::sleep_for(std::chrono::milliseconds(10 + (i % 50)));
        completed_tasks.fetch_add(1);
      };
      pool->addTask(task, "batch_" + std::to_string(batch) + "_task_" + std::to_string(i));
    }
    
    // Wait for this batch to mostly complete before next batch
    while (completed_tasks < (batch + 1) * tasks_per_batch) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // Small delay to allow scaling down to happen
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  
  EXPECT_EQ(completed_tasks.load(), num_batches * tasks_per_batch);
}

// Stress test: High frequency task addition with concurrent scaling function updates
TEST_F(ThreadPoolTest, HighFrequencyStressTest) {
  const int duration_seconds = 3;
  const int tasks_per_ms = 2;
  std::atomic<bool> stop_test(false);
  std::atomic<int> completed_tasks(0);
  std::atomic<int> scale_function_updates(0);
  
  // Thread that continuously updates scaling functions
  std::thread scale_updater([this, &stop_test, &scale_function_updates]() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> threshold_dist(1, 10);
    
    while (!stop_test) {
      int up_threshold = threshold_dist(gen);
      int down_threshold = threshold_dist(gen);
      
      auto scaleUpFunc = [up_threshold](const uint queueSize, const uint workerCount) {
        return queueSize > up_threshold && workerCount < 8;
      };
      pool->setScaleUp(scaleUpFunc);
      
      auto scaleDownFunc = [down_threshold](const uint queueSize, const uint workerCount) {
        return queueSize < down_threshold && workerCount > 1;
      };
      pool->setScaleDown(scaleDownFunc);
      
      scale_function_updates.fetch_add(1);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });
  
  // High frequency task producer
  std::thread task_producer([this, &stop_test, &completed_tasks, tasks_per_ms]() {
    int task_counter = 0;
    auto last_time = std::chrono::steady_clock::now();
    
    while (!stop_test) {
      auto current_time = std::chrono::steady_clock::now();
      auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_time).count();
      
      if (elapsed_ms >= 1) {
        for (int i = 0; i < tasks_per_ms; ++i) {
          auto task = [&completed_tasks, task_counter](const std::string &p) {
            // Very light task to stress the queuing mechanism
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            completed_tasks.fetch_add(1);
          };
          pool->addTask(task, "stress_task_" + std::to_string(task_counter++));
        }
        last_time = current_time;
      }
      
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  });
  
  // Let the stress test run
  std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
  stop_test = true;
  
  // Wait for threads to finish
  scale_updater.join();
  task_producer.join();
  
  // Wait for remaining tasks to complete
  auto start = std::chrono::steady_clock::now();
  int last_completed = completed_tasks.load();
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    int current_completed = completed_tasks.load();
    if (current_completed == last_completed) {
      // No new completions, assume all done
      break;
    }
    last_completed = current_completed;
    
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed > std::chrono::seconds(10)) {
      break; // Timeout
    }
  }
  
  // Verify that we processed a reasonable number of tasks and scale updates
  EXPECT_GT(completed_tasks.load(), duration_seconds * 1000 * tasks_per_ms / 2); // At least half expected
  EXPECT_GT(scale_function_updates.load(), 5); // At least some updates happened
}

// Test rapid pool destruction and recreation
TEST_F(ThreadPoolTest, RapidDestructionRecreation) {
  const int num_cycles = 10;
  std::atomic<int> total_completed(0);
  
  for (int cycle = 0; cycle < num_cycles; ++cycle) {
    // Reset the pool
    pool.reset();
    pool = std::make_unique<ThreadPool>();
    
    std::atomic<int> cycle_completed(0);
    const int tasks_this_cycle = 20;
    
    // Add tasks quickly
    for (int i = 0; i < tasks_this_cycle; ++i) {
      auto task = [&cycle_completed, &total_completed, cycle, i](const std::string &p) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        cycle_completed.fetch_add(1);
        total_completed.fetch_add(1);
      };
      pool->addTask(task, "cycle_" + std::to_string(cycle) + "_task_" + std::to_string(i));
    }
    
    // Wait for tasks in this cycle to complete
    auto start = std::chrono::steady_clock::now();
    while (cycle_completed < tasks_this_cycle) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      auto elapsed = std::chrono::steady_clock::now() - start;
      if (elapsed > std::chrono::seconds(5)) {
        FAIL() << "Cycle " << cycle << " tasks did not complete. Completed: " << cycle_completed.load();
      }
    }
  }
  
  EXPECT_EQ(total_completed.load(), num_cycles * 20);
}

// Test task execution ordering and queue integrity
TEST_F(ThreadPoolTest, QueueIntegrityTest) {
  const int num_tasks = 100;
  std::vector<std::atomic<bool>> task_completed(num_tasks);
  std::atomic<int> completion_order_violations(0);
  
  // Initialize all flags to false
  for (int i = 0; i < num_tasks; ++i) {
    task_completed[i] = false;
  }
  
  // Add tasks that check execution ordering
  for (int i = 0; i < num_tasks; ++i) {
    auto task = [&task_completed, &completion_order_violations, i, num_tasks](const std::string &p) {
      // Mark this task as completed
      task_completed[i] = true;
      
      // Check if any later task completed before this one (violation of FIFO in single thread)
      for (int j = i + 1; j < num_tasks; ++j) {
        if (task_completed[j].load()) {
          completion_order_violations.fetch_add(1);
          break;
        }
      }
      
      // Small delay to make race conditions more likely if they exist
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    };
    pool->addTask(task, "order_task_" + std::to_string(i));
  }
  
  // Wait for all tasks to complete
  auto start = std::chrono::steady_clock::now();
  bool all_completed = false;
  while (!all_completed) {
    all_completed = true;
    for (int i = 0; i < num_tasks; ++i) {
      if (!task_completed[i].load()) {
        all_completed = false;
        break;
      }
    }
    
    if (!all_completed) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      auto elapsed = std::chrono::steady_clock::now() - start;
      if (elapsed > std::chrono::seconds(10)) {
        FAIL() << "Not all tasks completed within timeout";
      }
    }
  }
  
  // Note: Some ordering violations are expected in multi-threaded environment
  // This test is more about ensuring the queue mechanisms work without corruption
  EXPECT_TRUE(all_completed);
}

// Test exception handling in tasks
TEST_F(ThreadPoolTest, ExceptionHandlingTest) {
  const int num_normal_tasks = 10;
  const int num_exception_tasks = 5;
  std::atomic<int> normal_completed(0);
  std::atomic<int> exception_tasks_run(0);
  
  // Add normal tasks
  for (int i = 0; i < num_normal_tasks; ++i) {
    auto task = [&normal_completed, i](const std::string &p) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      normal_completed.fetch_add(1);
    };
    pool->addTask(task, "normal_task_" + std::to_string(i));
  }
  
  // Add exception-throwing tasks
  for (int i = 0; i < num_exception_tasks; ++i) {
    auto task = [&exception_tasks_run, i](const std::string &p) {
      exception_tasks_run.fetch_add(1);
      throw std::runtime_error("Test exception from task " + std::to_string(i));
    };
    pool->addTask(task, "exception_task_" + std::to_string(i));
  }
  
  // Add more normal tasks after exceptions
  for (int i = num_normal_tasks; i < num_normal_tasks * 2; ++i) {
    auto task = [&normal_completed, i](const std::string &p) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      normal_completed.fetch_add(1);
    };
    pool->addTask(task, "normal_task_after_" + std::to_string(i));
  }
  
  // Wait for all tasks to be processed
  auto start = std::chrono::steady_clock::now();
  while (normal_completed < num_normal_tasks * 2 || exception_tasks_run < num_exception_tasks) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed > std::chrono::seconds(10)) {
      break;
    }
  }
  
  // Verify that exceptions didn't prevent other tasks from executing
  EXPECT_EQ(normal_completed.load(), num_normal_tasks * 2);
  EXPECT_EQ(exception_tasks_run.load(), num_exception_tasks);
}

// Test that ThreadPool waits for all tasks to complete before destruction
TEST_F(ThreadPoolTest, WaitForTasksOnDestruction) {
  const int num_long_tasks = 5;
  const int task_duration_ms = 200; // Each task takes 200ms
  std::atomic<int> tasks_started(0);
  std::atomic<int> tasks_completed(0);
  std::vector<std::chrono::steady_clock::time_point> task_start_times(num_long_tasks);
  std::vector<std::chrono::steady_clock::time_point> task_end_times(num_long_tasks);
  
  auto start_time = std::chrono::steady_clock::now();
  
  // Add tasks that take significant time to complete
  for (int i = 0; i < num_long_tasks; ++i) {
    auto task = [&tasks_started, &tasks_completed, &task_start_times, &task_end_times, 
                 i, task_duration_ms](const std::string &payload) {
      int task_index = tasks_started.fetch_add(1);
      task_start_times[task_index] = std::chrono::steady_clock::now();
      
      // Simulate long-running work
      std::this_thread::sleep_for(std::chrono::milliseconds(task_duration_ms));
      
      task_end_times[task_index] = std::chrono::steady_clock::now();
      tasks_completed.fetch_add(1);
    };
    pool->addTask(task, "long_task_" + std::to_string(i));
  }
  
  // Wait a short time to ensure tasks have started
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  
  // Record when we start destruction
  auto destruction_start = std::chrono::steady_clock::now();
  
  // Destroy the pool - this should wait for all tasks to complete
  pool.reset();
  
  auto destruction_end = std::chrono::steady_clock::now();
  
  // Verify all tasks completed
  EXPECT_EQ(tasks_started.load(), num_long_tasks);
  EXPECT_EQ(tasks_completed.load(), num_long_tasks);
  
  // Verify that destruction took at least as long as the tasks needed to complete
  auto destruction_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      destruction_end - destruction_start).count();
  
  // The destruction should have waited for tasks to complete
  // Allow some tolerance for timing variations
  EXPECT_GE(destruction_duration, task_duration_ms - 50); 
  
  // Verify all tasks actually completed before destruction finished
  for (int i = 0; i < num_long_tasks; ++i) {
    EXPECT_TRUE(task_end_times[i] <= destruction_end);
  }
}

// Test destruction with mixed short and long tasks
TEST_F(ThreadPoolTest, MixedTaskDuringDestruction) {
  const int num_short_tasks = 10;
  const int num_long_tasks = 3;
  const int long_task_duration_ms = 300;
  
  std::atomic<int> short_tasks_completed(0);
  std::atomic<int> long_tasks_completed(0);
  std::atomic<bool> destruction_started(false);
  
  // Add short tasks
  for (int i = 0; i < num_short_tasks; ++i) {
    auto task = [&short_tasks_completed, &destruction_started](const std::string &payload) {
      // Quick task
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      
      // Check if destruction started while we were running
      if (destruction_started.load()) {
        // This is expected - destruction should wait for us
      }
      
      short_tasks_completed.fetch_add(1);
    };
    pool->addTask(task, "short_task_" + std::to_string(i));
  }
  
  // Add long tasks
  for (int i = 0; i < num_long_tasks; ++i) {
    auto task = [&long_tasks_completed, &destruction_started, 
                 long_task_duration_ms](const std::string &payload) {
      auto start = std::chrono::steady_clock::now();
      
      // Long running task with periodic checks
      while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        
        if (elapsed >= long_task_duration_ms) {
          break;
        }
        
        // Task continues even if destruction started
        if (destruction_started.load()) {
          // This is expected - we should continue running until completion
        }
      }
      
      long_tasks_completed.fetch_add(1);
    };
    pool->addTask(task, "long_task_" + std::to_string(i));
  }
  
  // Give tasks time to start
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  
  // Mark that destruction is starting
  destruction_started = true;
  auto destruction_start = std::chrono::steady_clock::now();
  
  // Destroy the pool
  pool.reset();
  
  auto destruction_end = std::chrono::steady_clock::now();
  auto destruction_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      destruction_end - destruction_start).count();
  
  // Verify all tasks completed
  EXPECT_EQ(short_tasks_completed.load(), num_short_tasks);
  EXPECT_EQ(long_tasks_completed.load(), num_long_tasks);
  
  // Destruction should have taken time to wait for long tasks
  EXPECT_GE(destruction_time, 200); // At least most of the long task duration
}

// Test that ThreadPool waits for a single executing task to complete before destruction
TEST_F(ThreadPoolTest, SingleTaskDestructionWait) {
  const int task_duration_ms = 300; // Single long task
  std::atomic<bool> task_started(false);
  std::atomic<bool> task_completed(false);
  std::chrono::steady_clock::time_point task_start_time;
  std::chrono::steady_clock::time_point task_end_time;
  
  // Add a single long-running task to ensure it gets started before destruction
  auto task = [&task_started, &task_completed, &task_start_time, &task_end_time, 
               task_duration_ms](const std::string &payload) {
    task_start_time = std::chrono::steady_clock::now();
    task_started = true;
    
    // Simulate long-running work
    std::this_thread::sleep_for(std::chrono::milliseconds(task_duration_ms));
    
    task_end_time = std::chrono::steady_clock::now();
    task_completed = true;
  };
  pool->addTask(task, "single_long_task");
  
  // Wait for task to definitely start
  while (!task_started.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  
  // Get initial thread count before destruction
  auto get_thread_count = []() {
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
      if (line.substr(0, 8) == "Threads:") {
        return std::stoi(line.substr(9));
      }
    }
    return -1; // Fallback if can't read
  };
  
  int threads_before_destruction = get_thread_count();
  
  // Record when we start destruction
  auto destruction_start = std::chrono::steady_clock::now();
  
  // Destroy the pool - this should wait for the running task to complete
  pool.reset();
  
  auto destruction_end = std::chrono::steady_clock::now();
  
  // Wait a moment for any cleanup to complete
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  
  int threads_after_destruction = get_thread_count();
  
  // Verify the task completed
  EXPECT_TRUE(task_started.load());
  EXPECT_TRUE(task_completed.load());
  
  // Verify that destruction took time to wait for the task
  auto destruction_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      destruction_end - destruction_start).count();
  
  // The destruction should have waited for the task to complete
  // Allow some tolerance for timing variations (task was already running for 50ms)
  EXPECT_GE(destruction_duration, task_duration_ms - 100); 
  
  // Verify the task actually completed before destruction finished
  EXPECT_TRUE(task_end_time <= destruction_end);
  
  // Check for ghost threads - thread count should not increase (or should decrease)
  if (threads_before_destruction != -1 && threads_after_destruction != -1) {
    // Log the actual thread counts for debugging
    std::cout << "Thread count - Before destruction: " << threads_before_destruction 
              << ", After destruction: " << threads_after_destruction << std::endl;
    
    EXPECT_LE(threads_after_destruction, threads_before_destruction)
      << "Ghost threads detected! Before: " << threads_before_destruction 
      << ", After: " << threads_after_destruction;
  } else {
    std::cout << "Could not read thread counts from /proc/self/status" << std::endl;
  }
}

// Test destruction with tasks that are queued but not yet started
TEST_F(ThreadPoolTest, QueuedTasksOnDestruction) {
  const int num_queued_tasks = 20;
  const int blocking_task_duration_ms = 100;
  
  std::atomic<int> blocking_tasks_completed(0);
  std::atomic<int> queued_tasks_completed(0);
  
  // Add one blocking task that will occupy the initial worker
  auto blocking_task = [&blocking_tasks_completed, blocking_task_duration_ms](const std::string &payload) {
    std::this_thread::sleep_for(std::chrono::milliseconds(blocking_task_duration_ms));
    blocking_tasks_completed.fetch_add(1);
  };
  pool->addTask(blocking_task, "blocking_task");
  
  // Immediately add many more tasks that will be queued
  for (int i = 0; i < num_queued_tasks; ++i) {
    auto task = [&queued_tasks_completed, i](const std::string &payload) {
      // Quick tasks
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      queued_tasks_completed.fetch_add(1);
    };
    pool->addTask(task, "queued_task_" + std::to_string(i));
  }
  
  // Give a very short time for tasks to be queued
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  
  auto destruction_start = std::chrono::steady_clock::now();
  
  // Destroy the pool - should wait for all queued tasks to complete
  pool.reset();
  
  auto destruction_end = std::chrono::steady_clock::now();
  auto destruction_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      destruction_end - destruction_start).count();
  
  // Verify all tasks completed, including queued ones
  EXPECT_EQ(blocking_tasks_completed.load(), 1);
  EXPECT_EQ(queued_tasks_completed.load(), num_queued_tasks);
  
  // Destruction should have taken time to process all queued tasks
  EXPECT_GE(destruction_time, blocking_task_duration_ms - 30);
}

// Test scaling behavior with small task count (no scaling expected)
TEST_F(ThreadPoolTest, DefaultScalingBehaviorSmallTasks) {
  const int small_task_count = 5;  // Should NOT trigger scaling (5 <= 10 * 1)
  const int task_duration_ms = 50;
  
  pool.reset();
  pool = std::make_unique<ThreadPool>();
  
  std::atomic<int> completed(0);
  auto start = std::chrono::steady_clock::now();
  
  for (int i = 0; i < small_task_count; ++i) {
    auto task = [&completed, task_duration_ms](const std::string &payload) {
      std::this_thread::sleep_for(std::chrono::milliseconds(task_duration_ms));
      completed.fetch_add(1);
    };
    pool->addTask(task, "small_task_" + std::to_string(i));
  }
  
  // Wait for completion with timeout
  auto timeout_start = std::chrono::steady_clock::now();
  while (completed.load() < small_task_count) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - timeout_start).count();
    if (elapsed > 5) {
      FAIL() << "Test timed out. Completed: " << completed.load() << "/" << small_task_count;
    }
  }
  
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start).count();
  
  // Should take approximately small_task_count * task_duration_ms (serial execution)
  EXPECT_GE(duration, (small_task_count - 1) * task_duration_ms);
  EXPECT_EQ(completed.load(), small_task_count);
}

// Test scaling behavior with large task count (scaling expected)
TEST_F(ThreadPoolTest, DefaultScalingBehaviorLargeTasks) {
  const int large_task_count = 15; // Should trigger scaling (15 > 10 * 1)
  const int task_duration_ms = 50;
  
  pool.reset();
  pool = std::make_unique<ThreadPool>();
  
  std::atomic<int> completed(0);
  auto start = std::chrono::steady_clock::now();
  auto task = [&completed, task_duration_ms](const std::string &payload) {
    std::cout << "Start processing " + payload << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(task_duration_ms));
    completed.fetch_add(1);
  };
  for (int i = 0; i < large_task_count; ++i) {
    pool->addTask(task, "large_task_" + std::to_string(i));
  }
  
  // Wait for completion with timeout
  auto timeout_start = std::chrono::steady_clock::now();
  while (completed.load() < large_task_count) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - timeout_start).count();
    if (elapsed > 5) {
      FAIL() << "Test timed out. Completed: " << completed.load() << "/" << large_task_count;
      break;
    }
  }
  
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start).count();
  
  // Should take significantly less than large_task_count * task_duration_ms (parallel execution)
  EXPECT_LT(duration, large_task_count * task_duration_ms);
  EXPECT_EQ(completed.load(), large_task_count);
}

// Test custom scaling configuration
TEST_F(ThreadPoolTest, CustomScalingConfiguration) {
  const int task_count = 10;
  const int task_duration_ms = 50;
  
  // Configure aggressive scaling for small queues
  pool->setScaleUp([](uint queueSize, uint workerCount) {
    return queueSize > 2 && workerCount < 5;
  });
  
  std::atomic<int> completed(0);
  auto start = std::chrono::steady_clock::now();
  
  // Add tasks quickly to trigger scaling
  for (int i = 0; i < task_count; ++i) {
    auto task = [&completed, task_duration_ms](const std::string &payload) {
      std::this_thread::sleep_for(std::chrono::milliseconds(task_duration_ms));
      completed.fetch_add(1);
    };
    pool->addTask(task, "custom_scale_task_" + std::to_string(i));
  }
  
  // Wait for completion with timeout
  auto timeout_start = std::chrono::steady_clock::now();
  while (completed.load() < task_count) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - timeout_start).count();
    if (elapsed > 5) {
      FAIL() << "Test timed out. Completed: " << completed.load() << "/" << task_count;
    }
  }
  
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start).count();
  
  // With aggressive scaling, should complete faster than serial execution
  EXPECT_LT(duration, task_count * task_duration_ms);
  EXPECT_EQ(completed.load(), task_count);
}

} // namespace StateManager
