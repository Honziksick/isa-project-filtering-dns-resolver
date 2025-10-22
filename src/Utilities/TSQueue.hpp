/*******************************************************************************
 *                                                                             *
 * Project:      Filtering DNS Resolver                                        *
 * University:   Faculty of Information Technology, BUT                        *
 * Subject:      ISA: Network Applications and Network Administration          *
 *                                                                             *
 * File:         TSQueue.hpp                                                   *
 * Author:       Jan Kalina <xkalinj00>                                        *
 *                                                                             *
 * Created:      12.10.2025                                                    *
 * Last edit:    17.10.2025                                                    *
 *                                                                             *
 * Description:  This header file provides a generic thread-safe queue         *
 *               implementation (`TSQueue`) with bounded capacity, blocking    *
 *               push/pop operations, and safe shutdown for use in concurrent  *
 *               producer-consumer scenarios. The queue is designed for        *
 *               multi-threaded environments, ensuring data integrity and      *
 *               efficient synchronization using mutexes and condition         *
 *               variables.                                                    *
 *                                                                             *
 ******************************************************************************/
/**
 * @file TSQueue.hpp
 * @author Jan Kalina \<xkalinj00>
 * @brief Header file declaring the `TSQueue` class template for a thread-safe,
 *        bounded-capacity queue with blocking operations and shutdown support.
 *
 * @note This module is inspired by the article by Medieum by Xin (slightly modified):
 * `https://medium.com/@lixin_78505/c-implementing-a-simple-thread-safe-queue-b0cdfec40e71`
 */

#ifndef TSQUEUE_HPP
#define TSQUEUE_HPP

#include <condition_variable> // std::condition_variable
#include <optional> // std::optional
#include <mutex>    // std::mutex, std::unique_lock, std::lock_guard
#include <deque>    // std::deque

namespace FilteringDnsResolver::Utilities
{
    /**
     * @class TSQueue
     * @brief Thread-safe, bounded-capacity queue for concurrent producer-consumer usage.
     *
     * @details Provides a generic queue with blocking push and pop operations,
     *          backpressure (capacity limit), and safe shutdown. Designed for
     *          multi-threaded environments, it uses mutexes and condition variables
     *          to synchronize access and ensure data integrity.
     *
     * @tparam T Type of elements stored in the queue.
     */
    template <typename T>
    class TSQueue {
    public:
        /**
         * @brief Constructs a thread-safe queue with a given capacity.
         * @param capacity Maximum number of elements the queue can hold.
         */
        explicit TSQueue(const size_t capacity)
            : mCapacity(capacity) {}

        /**
         * @brief Pushes a new element to the back of the queue (blocking).
         *
         * @details Waits if the queue is full (backpressure). If the queue is closed,
         *          the operation is ignored. Notifies one waiting consumer after push.
         *
         * @param inValue Element to be pushed (rvalue reference).
         */
        void push(T &&inValue) {
            std::unique_lock<std::mutex> lock{mMutex};

            // Wait until not full or closed
            mCvNotFull.wait(lock, [&] {
                return mIsClosed || mQueue.size() < mCapacity;
            });

            // If closed, ignore push
            if(mIsClosed) {
                return;
            }

            // Push the element
            mQueue.emplace_back(std::move(inValue));

            // Notify one waiting consumer
            lock.unlock();
            mCvNotEmpty.notify_one();
        } // TSQueue::push

        /**
         * @brief Pops an element from the front of the queue (blocking).
         *
         * @details Waits if the queue is empty. Returns false if the queue is closed
         *          and empty (shutdown). Notifies one waiting producer after pop.
         *
         * @param outValue Reference to store the popped element.
         * @return true if an element was popped, false if shutdown and empty.
         */
        bool pop(T &outValue) {
            std::unique_lock<std::mutex> lock{mMutex};

            // Wait until not empty or closed
            mCvNotEmpty.wait(lock, [&] {
                return mIsClosed || !mQueue.empty();
            });

            // If closed and empty, return false
            if(mQueue.empty()) {
                return false;
            }

            // Pop the element
            outValue = std::move(mQueue.front());
            mQueue.pop_front();

            // Notify one waiting producer
            lock.unlock();
            mCvNotFull.notify_one();
            return true;
        } // TSQueue::pop

        /**
         * @brief Closes the queue for further pushes and wakes all waiting threads.
         *
         * @details After calling close, no new elements can be pushed. All waiting
         *          threads (producers and consumers) are notified to finish.
         */
        void close() {
            std::lock_guard<std::mutex> lock{mMutex};
            mIsClosed = true;
            mCvNotEmpty.notify_all();
            mCvNotFull.notify_all();
        } // TSQueue::close

        /**
         * @brief Returns the current number of elements in the queue.
         * @return Number of elements in the queue.
         */
        size_t size() const {
            std::lock_guard<std::mutex> lock{mMutex};
            return mQueue.size();
        } // TSQueue::size

    private:
        size_t mCapacity{};                     /**< Maximum queue capacity.                  */
        mutable std::mutex mMutex{};            /**< Mutex for synchronizing access.          */
        std::condition_variable mCvNotEmpty{};  /**< Condition variable for not-empty state.  */
        std::condition_variable mCvNotFull{};   /**< Condition variable for not-full state.   */
        std::deque<T> mQueue{};                 /**< Underlying container for queue elements. */
        bool mIsClosed{false};                  /**< Indicates if the queue is closed.        */
    }; // TSQueue
} // FilteringDnsResolver::Utilities

#endif // TSQUEUE_HPP

/*** end of file TSQueue.hpp ***/
