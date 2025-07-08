#pragma once

#include <algorithm>
#include <atomic>
#include <cstring>
#include <vector>

namespace Dynamix {

    /**
     * CircularBuffer - Lock-free ring buffer for real-time audio processing
     *
     * This is a single-producer, single-consumer lock-free circular buffer
     * optimized for real-time audio applications. It uses atomic operations
     * to ensure thread safety without locks.
     */
    template <typename T> class CircularBuffer {
    public:
        /**
         * Constructor
         * @param capacity Buffer capacity in samples (will be rounded up to power of 2)
         */
        explicit CircularBuffer(size_t capacity = 8192)
            : m_capacity(nextPowerOfTwo(capacity)), m_mask(m_capacity - 1), m_buffer(m_capacity),
              m_readIndex(0), m_writeIndex(0) {}

        /**
         * Write samples to the buffer
         * @param data Pointer to input samples
         * @param count Number of samples to write
         * @return Number of samples actually written
         */
        size_t write(const T* data, size_t count) {
            const size_t writeIndex = m_writeIndex.load(std::memory_order_relaxed);
            const size_t readIndex = m_readIndex.load(std::memory_order_acquire);

            const size_t available_space = space_internal(writeIndex, readIndex);
            const size_t to_write = std::min(count, available_space);

            if (to_write == 0) {
                return 0;
            }

            const size_t write_pos = writeIndex & m_mask;
            const size_t first_chunk = std::min(to_write, m_capacity - write_pos);

            // Copy first chunk
            std::memcpy(&m_buffer[write_pos], data, first_chunk * sizeof(T));

            // Copy second chunk if wrapping around
            if (to_write > first_chunk) {
                const size_t second_chunk = to_write - first_chunk;
                std::memcpy(&m_buffer[0], data + first_chunk, second_chunk * sizeof(T));
            }

            // Update write index
            m_writeIndex.store(writeIndex + to_write, std::memory_order_release);

            return to_write;
        }

        /**
         * Read samples from the buffer
         * @param data Pointer to output buffer
         * @param count Number of samples to read
         * @return Number of samples actually read
         */
        size_t read(T* data, size_t count) {
            const size_t readIndex = m_readIndex.load(std::memory_order_relaxed);
            const size_t writeIndex = m_writeIndex.load(std::memory_order_acquire);

            const size_t available_data = available_internal(readIndex, writeIndex);
            const size_t to_read = std::min(count, available_data);

            if (to_read == 0) {
                return 0;
            }

            const size_t read_pos = readIndex & m_mask;
            const size_t first_chunk = std::min(to_read, m_capacity - read_pos);

            // Copy first chunk
            std::memcpy(data, &m_buffer[read_pos], first_chunk * sizeof(T));

            // Copy second chunk if wrapping around
            if (to_read > first_chunk) {
                const size_t second_chunk = to_read - first_chunk;
                std::memcpy(data + first_chunk, &m_buffer[0], second_chunk * sizeof(T));
            }

            // Update read index
            m_readIndex.store(readIndex + to_read, std::memory_order_release);

            return to_read;
        }

        /**
         * Get number of samples available for reading
         */
        size_t available() const {
            const size_t readIndex = m_readIndex.load(std::memory_order_relaxed);
            const size_t writeIndex = m_writeIndex.load(std::memory_order_relaxed);
            return available_internal(readIndex, writeIndex);
        }

        /**
         * Get number of samples that can be written
         */
        size_t space() const {
            const size_t readIndex = m_readIndex.load(std::memory_order_relaxed);
            const size_t writeIndex = m_writeIndex.load(std::memory_order_relaxed);
            return space_internal(writeIndex, readIndex);
        }

        /**
         * Get buffer capacity
         */
        size_t capacity() const { return m_capacity; }

        /**
         * Clear the buffer
         */
        void clear() {
            m_readIndex.store(0, std::memory_order_relaxed);
            m_writeIndex.store(0, std::memory_order_relaxed);
        }

        /**
         * Check if buffer is empty
         */
        bool empty() const { return available() == 0; }

        /**
         * Check if buffer is full
         */
        bool full() const { return space() == 0; }

    private:
        size_t available_internal(size_t read_idx, size_t write_idx) const {
            return write_idx - read_idx;
        }

        size_t space_internal(size_t write_idx, size_t read_idx) const {
            return m_capacity - (write_idx - read_idx) - 1;
        }

        static size_t nextPowerOfTwo(size_t value) {
            if (value <= 1)
                return 2;

            size_t power = 1;
            while (power < value) {
                power <<= 1;
            }
            return power;
        }

        const size_t m_capacity;
        const size_t m_mask;
        std::vector<T> m_buffer;

        std::atomic<size_t> m_readIndex;
        std::atomic<size_t> m_writeIndex;
    };

} // namespace Dynamix