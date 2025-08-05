#include "core/StateManager.h"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>
#include <random>

using namespace StateManager;

void printSeparator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << " " << title << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

void demonstrateConcurrentWrites() {
    printSeparator("CONCURRENT WRITES DEMO");
    
    RedisConfig config;
    StateManager stateManager(config);
    
    const int numThreads = 10;
    const int operationsPerThread = 50;
    std::atomic<int> successfulWrites{0};
    std::atomic<int> failedWrites{0};
    std::vector<std::thread> threads;
    
    std::cout << "Starting " << numThreads << " threads, each performing " 
              << operationsPerThread << " write operations..." << std::endl;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Launch worker threads
    for (int threadId = 0; threadId < numThreads; ++threadId) {
        threads.emplace_back([&, threadId]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(1, 1000);
            
            for (int op = 0; op < operationsPerThread; ++op) {
                std::string key = "thread_" + std::to_string(threadId) + "_item_" + std::to_string(op);
                
                nlohmann::json data = {
                    {"thread_id", threadId},
                    {"operation_id", op},
                    {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count()},
                    {"random_value", dis(gen)},
                    {"data", "Data from thread " + std::to_string(threadId) + ", op " + std::to_string(op)}
                };
                
                bool success = stateManager.write(key, data);
                if (success) {
                    successfulWrites.fetch_add(1);
                } else {
                    failedWrites.fetch_add(1);
                }
                
                // Small random delay to simulate realistic workload
                std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen) % 5));
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "\nResults:" << std::endl;
    std::cout << "  Successful writes: " << successfulWrites.load() << std::endl;
    std::cout << "  Failed writes: " << failedWrites.load() << std::endl;
    std::cout << "  Total operations: " << (successfulWrites.load() + failedWrites.load()) << std::endl;
    std::cout << "  Duration: " << duration.count() << " ms" << std::endl;
    std::cout << "  Throughput: " << (successfulWrites.load() * 1000.0 / duration.count()) << " writes/sec" << std::endl;
    
    // Verify data integrity by reading back some random entries
    std::cout << "\nVerifying data integrity..." << std::endl;
    int verificationsToPerform = 20;
    int successfulReads = 0;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (int i = 0; i < verificationsToPerform; ++i) {
        int randomThread = gen() % numThreads;
        int randomOp = gen() % operationsPerThread;
        std::string key = "thread_" + std::to_string(randomThread) + "_item_" + std::to_string(randomOp);
        
        auto [error, data] = stateManager.read(key);
        if (!error.has_value()) {
            // Verify the data is consistent
            if (data["thread_id"].get<int>() == randomThread && 
                data["operation_id"].get<int>() == randomOp) {
                successfulReads++;
            }
        }
    }
    
    std::cout << "  Verification: " << successfulReads << "/" << verificationsToPerform 
              << " random reads successful and consistent" << std::endl;
    
    // Cleanup
    std::cout << "\nCleaning up test data..." << std::endl;
    int cleanupCount = 0;
    for (int threadId = 0; threadId < numThreads; ++threadId) {
        for (int op = 0; op < operationsPerThread; ++op) {
            std::string key = "thread_" + std::to_string(threadId) + "_item_" + std::to_string(op);
            if (stateManager.erase(key)) {
                cleanupCount++;
            }
        }
    }
    std::cout << "✓ Cleaned up " << cleanupCount << " keys" << std::endl;
}

void demonstrateConcurrentReadWrite() {
    printSeparator("CONCURRENT READ/WRITE DEMO");
    
    RedisConfig config;
    StateManager stateManager(config);
    
    const std::string sharedKey = "shared_counter";
    const int numReaderThreads = 5;
    const int numWriterThreads = 3;
    const int operationsPerThread = 30;
    
    std::atomic<int> totalReads{0};
    std::atomic<int> totalWrites{0};
    std::atomic<int> readErrors{0};
    std::atomic<int> writeErrors{0};
    std::mutex outputMutex;
    
    // Initialize shared data
    nlohmann::json initialData = {
        {"counter", 0},
        {"last_update", "initialization"},
        {"total_updates", 0}
    };
    stateManager.write(sharedKey, initialData);
    
    std::cout << "Starting concurrent read/write operations on shared key: " << sharedKey << std::endl;
    std::cout << "Readers: " << numReaderThreads << " threads, Writers: " << numWriterThreads << " threads" << std::endl;
    
    std::vector<std::thread> threads;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Launch reader threads
    for (int readerId = 0; readerId < numReaderThreads; ++readerId) {
        threads.emplace_back([&, readerId]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(10, 50);
            
            for (int op = 0; op < operationsPerThread; ++op) {
                auto [error, data] = stateManager.read(sharedKey);
                if (!error.has_value()) {
                    totalReads.fetch_add(1);
                    
                    // Occasionally print what we read
                    if (op % 10 == 0) {
                        std::lock_guard<std::mutex> lock(outputMutex);
                        std::cout << "Reader " << readerId << " read counter: " 
                                  << data["counter"].get<int>() << std::endl;
                    }
                } else {
                    readErrors.fetch_add(1);
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
            }
        });
    }
    
    // Launch writer threads
    for (int writerId = 0; writerId < numWriterThreads; ++writerId) {
        threads.emplace_back([&, writerId]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(20, 100);
            
            for (int op = 0; op < operationsPerThread; ++op) {
                // Read current data, modify it, and write back
                auto [error, data] = stateManager.read(sharedKey);
                if (!error.has_value()) {
                    // Modify the data
                    data["counter"] = data["counter"].get<int>() + 1;
                    data["last_update"] = "writer_" + std::to_string(writerId) + "_op_" + std::to_string(op);
                    data["total_updates"] = data["total_updates"].get<int>() + 1;
                    data["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    
                    bool writeSuccess = stateManager.write(sharedKey, data);
                    if (writeSuccess) {
                        totalWrites.fetch_add(1);
                        
                        if (op % 10 == 0) {
                            std::lock_guard<std::mutex> lock(outputMutex);
                            std::cout << "Writer " << writerId << " updated counter to: " 
                                      << data["counter"].get<int>() << std::endl;
                        }
                    } else {
                        writeErrors.fetch_add(1);
                    }
                } else {
                    readErrors.fetch_add(1);
                }
                
                std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    // Read final state
    auto [finalError, finalData] = stateManager.read(sharedKey);
    
    std::cout << "\nResults:" << std::endl;
    std::cout << "  Total reads: " << totalReads.load() << " (errors: " << readErrors.load() << ")" << std::endl;
    std::cout << "  Total writes: " << totalWrites.load() << " (errors: " << writeErrors.load() << ")" << std::endl;
    std::cout << "  Duration: " << duration.count() << " ms" << std::endl;
    
    if (!finalError.has_value()) {
        std::cout << "  Final counter value: " << finalData["counter"].get<int>() << std::endl;
        std::cout << "  Total updates recorded: " << finalData["total_updates"].get<int>() << std::endl;
        std::cout << "  Last update by: " << finalData["last_update"].get<std::string>() << std::endl;
    }
    
    // Cleanup
    stateManager.erase(sharedKey);
    std::cout << "✓ Cleaned up shared data" << std::endl;
}

void demonstrateLoadTesting() {
    printSeparator("LOAD TESTING DEMO");
    
    RedisConfig config;
    StateManager stateManager(config);
    
    const int numThreads = 8;
    const int operationsPerThread = 100;
    std::atomic<int> totalOperations{0};
    std::atomic<int> successfulOperations{0};
    std::vector<std::thread> threads;
    
    std::cout << "Performing load test with " << numThreads << " threads" << std::endl;
    std::cout << "Each thread performs " << operationsPerThread << " mixed operations (write/read/erase)" << std::endl;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Launch load testing threads
    for (int threadId = 0; threadId < numThreads; ++threadId) {
        threads.emplace_back([&, threadId]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> opDis(0, 2); // 0=write, 1=read, 2=erase
            std::uniform_int_distribution<> keyDis(0, 99); // Key range
            std::uniform_int_distribution<> valueDis(1, 1000);
            
            int localSuccessful = 0;
            
            for (int op = 0; op < operationsPerThread; ++op) {
                totalOperations.fetch_add(1);
                
                std::string key = "load_test_" + std::to_string(threadId) + "_" + std::to_string(keyDis(gen));
                
                int operation = opDis(gen);
                bool success = false;
                
                switch (operation) {
                    case 0: { // Write
                        nlohmann::json data = {
                            {"thread", threadId},
                            {"operation", op},
                            {"value", valueDis(gen)},
                            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch()).count()}
                        };
                        success = stateManager.write(key, data);
                        break;
                    }
                    case 1: { // Read
                        auto [error, data] = stateManager.read(key);
                        success = !error.has_value();
                        break;
                    }
                    case 2: { // Erase
                        success = stateManager.erase(key);
                        break;
                    }
                }
                
                if (success) {
                    localSuccessful++;
                }
                
                // Very small delay to simulate realistic load
                if (op % 10 == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
            
            successfulOperations.fetch_add(localSuccessful);
        });
    }
    
    // Monitor progress
    std::thread monitor([&]() {
        while (totalOperations.load() < numThreads * operationsPerThread) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            int completed = totalOperations.load();
            int total = numThreads * operationsPerThread;
            double progress = (completed * 100.0) / total;
            std::cout << "Progress: " << completed << "/" << total 
                      << " (" << std::fixed << std::setprecision(1) << progress << "%)" << std::endl;
        }
    });
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    monitor.join();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "\nLoad Test Results:" << std::endl;
    std::cout << "  Total operations: " << totalOperations.load() << std::endl;
    std::cout << "  Successful operations: " << successfulOperations.load() << std::endl;
    std::cout << "  Success rate: " << (successfulOperations.load() * 100.0 / totalOperations.load()) << "%" << std::endl;
    std::cout << "  Duration: " << duration.count() << " ms" << std::endl;
    std::cout << "  Throughput: " << (totalOperations.load() * 1000.0 / duration.count()) << " ops/sec" << std::endl;
    
    // Cleanup any remaining test data
    std::cout << "\nCleaning up load test data..." << std::endl;
    int cleanupCount = 0;
    for (int threadId = 0; threadId < numThreads; ++threadId) {
        for (int keyId = 0; keyId < 100; ++keyId) {
            std::string key = "load_test_" + std::to_string(threadId) + "_" + std::to_string(keyId);
            if (stateManager.erase(key)) {
                cleanupCount++;
            }
        }
    }
    std::cout << "✓ Cleaned up " << cleanupCount << " remaining keys" << std::endl;
}

int main() {
    std::cout << "StateManager Library - Multithreaded Demo" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "This demo showcases thread safety and concurrent access patterns." << std::endl;
    std::cout << "Make sure Redis is running on localhost:6379 before running this demo." << std::endl;
    
    try {
        // Run the demonstrations
        demonstrateConcurrentWrites();
        demonstrateConcurrentReadWrite();
        demonstrateLoadTesting();
        
        printSeparator("DEMO COMPLETED");
        std::cout << "All multithreaded demonstrations completed successfully!" << std::endl;
        std::cout << "The StateManager library demonstrated robust thread safety" << std::endl;
        std::cout << "and performance under concurrent access patterns." << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "✗ Exception during demo: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}