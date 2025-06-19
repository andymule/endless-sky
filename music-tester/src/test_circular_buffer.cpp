#include "CircularBuffer.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

using namespace AudioTester;

// Test basic functionality
void test_basic_operations() {
    std::cout << "Testing basic operations..." << std::endl;

    CircularBuffer<float> buffer(1024);

    // Test initial state
    assert(buffer.empty());
    assert(!buffer.full());
    assert(buffer.available() == 0);
    assert(buffer.space() > 0);
    assert(buffer.capacity() >= 1024); // Should be power of 2

    // Test simple write/read
    std::vector<float> input = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    size_t written = buffer.write(input.data(), input.size());
    assert(written == input.size());
    assert(buffer.available() == input.size());
    assert(!buffer.empty());

    std::vector<float> output(input.size());
    size_t read = buffer.read(output.data(), output.size());
    assert(read == input.size());
    assert(output == input);
    assert(buffer.empty());

    std::cout << "✓ Basic operations passed" << std::endl;
}

// Test buffer wrap-around
void test_wrap_around() {
    std::cout << "Testing wrap-around..." << std::endl;

    CircularBuffer<float> buffer(8); // Small buffer for easy testing

    // Fill buffer almost to capacity
    std::vector<float> data1 = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    size_t written1 = buffer.write(data1.data(), data1.size());
    assert(written1 == data1.size());

    // Read some data to create space at the beginning
    std::vector<float> temp(3);
    size_t read1 = buffer.read(temp.data(), temp.size());
    assert(read1 == temp.size());

    // Write data that should wrap around
    std::vector<float> data2 = {7.0f, 8.0f, 9.0f, 10.0f};
    size_t written2 = buffer.write(data2.data(), data2.size());
    assert(written2 == data2.size());

    // Read all remaining data and verify order
    std::vector<float> result(data1.size() - temp.size() + data2.size());
    size_t read2 = buffer.read(result.data(), result.size());
    assert(read2 == result.size());

    // Should be: [4, 5, 6, 7, 8, 9, 10]
    std::vector<float> expected = {4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
    assert(result == expected);

    std::cout << "✓ Wrap-around test passed" << std::endl;
}

// Test overflow protection
void test_overflow_protection() {
    std::cout << "Testing overflow protection..." << std::endl;

    CircularBuffer<float> buffer(8);

    // Try to write more than capacity
    std::vector<float> large_data(20, 1.0f);
    size_t written = buffer.write(large_data.data(), large_data.size());

    // Should only write what fits (capacity - 1 due to implementation)
    assert(written < large_data.size());
    assert(!buffer.empty());

    // Try to read more than available
    std::vector<float> large_output(20);
    size_t read = buffer.read(large_output.data(), large_output.size());
    assert(read == written);
    assert(buffer.empty());

    std::cout << "✓ Overflow protection test passed" << std::endl;
}

// Test threading with producer/consumer
void test_threading() {
    std::cout << "Testing threading..." << std::endl;

    CircularBuffer<float> buffer(4096);
    constexpr size_t total_samples = 10000;
    constexpr size_t chunk_size = 100;

    std::atomic<bool> producer_done{false};
    std::atomic<bool> consumer_done{false};
    std::atomic<size_t> total_written{0};
    std::atomic<size_t> total_read{0};

    // Producer thread
    std::thread producer([&]() {
        std::vector<float> data(chunk_size);
        for (size_t i = 0; i < total_samples; i += chunk_size) {
            // Fill with incremental values
            for (size_t j = 0; j < chunk_size; ++j) {
                data[j] = static_cast<float>(i + j);
            }

            // Write with retry on full buffer
            size_t remaining = std::min(chunk_size, total_samples - i);
            size_t offset = 0;
            while (offset < remaining) {
                size_t written = buffer.write(data.data() + offset, remaining - offset);
                offset += written;
                total_written += written;

                if (written == 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            }
        }
        producer_done = true;
    });

    // Consumer thread
    std::thread consumer([&]() {
        std::vector<float> data(chunk_size);
        while (!producer_done || buffer.available() > 0) {
            size_t read = buffer.read(data.data(), chunk_size);
            total_read += read;

            if (read == 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        }
        consumer_done = true;
    });

    producer.join();
    consumer.join();

    assert(total_written == total_samples);
    assert(total_read == total_samples);
    assert(buffer.empty());

    std::cout << "✓ Threading test passed (written: " << total_written << ", read: " << total_read
              << ")" << std::endl;
}

// Test power-of-two capacity adjustment
void test_capacity_adjustment() {
    std::cout << "Testing capacity adjustment..." << std::endl;

    // Test various sizes get adjusted to power of 2
    struct TestCase {
        size_t input;
        size_t expected_min;
    };
    std::vector<TestCase> tests = {{1, 2}, {100, 128}, {1000, 1024}, {1024, 1024}, {2000, 2048}};

    for (const auto& test : tests) {
        CircularBuffer<float> buffer(test.input);
        assert(buffer.capacity() >= test.expected_min);

        // Verify it's actually a power of 2
        size_t capacity = buffer.capacity();
        assert((capacity & (capacity - 1)) == 0); // Power of 2 check
    }

    std::cout << "✓ Capacity adjustment test passed" << std::endl;
}

// Performance test
void test_performance() {
    std::cout << "Testing performance..." << std::endl;

    CircularBuffer<float> buffer(65536);
    constexpr size_t iterations = 10000;
    constexpr size_t chunk_size = 1024;

    std::vector<float> data(chunk_size, 1.0f);

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < iterations; ++i) {
        // Write a chunk
        buffer.write(data.data(), chunk_size);

        // Read it back
        buffer.read(data.data(), chunk_size);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double samples_per_sec = (iterations * chunk_size * 1000000.0) / duration.count();
    std::cout << "✓ Performance test: " << (samples_per_sec / 1000000.0) << " million samples/sec"
              << std::endl;
}

int main() {
    std::cout << "=== CircularBuffer Unit Tests ===" << std::endl;

    try {
        test_basic_operations();
        test_wrap_around();
        test_overflow_protection();
        test_threading();
        test_capacity_adjustment();
        test_performance();

        std::cout << "\n✅ All tests passed!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\n❌ Test failed: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\n❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}