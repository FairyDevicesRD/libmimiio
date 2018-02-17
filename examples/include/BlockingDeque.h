/*
 * @file BlockingDeque.h
 * \~english
 * @brief Simple thread-safe double ended blocking queue.
 * @note You can use BlockingQueue.h in simple case.
 * \~japanese
 * @brief スレッドセーフな両端ブロッキングキュー
 * @note 単純なブロッキングキューについては、BlockingQueue.h を利用できる
 * \~
 * @copyright Copyright 2018 Fairy Devices Inc. http://www.fairydevices.jp/
 * @copyright Apache License, Version 2.0
 * @author Masato Fujino, created on: 2018/02/17
 *
 * Copyright 2018 Fairy Devices Inc. http://www.fairydevices.jp/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EXAMPLES_INCLUDE_BLOCKINGDEQUE_H_
#define EXAMPLES_INCLUDE_BLOCKINGDEQUE_H_

#include <mutex>
#include <condition_variable>
#include <deque>
#include <stdexcept>

/**
 * @class BlockingDeque
 * \~english
 * @brief Simple double ended blocking queue, thread safe.
 * @tparam T Value type for element of the queue.
 * @tparam Container Container class. The default \e Container is std::deque.
 * @see See also BlockingQueue.h
 *
 * \~japanese
 * @brief @brief スレッドセーフな両端ブロッキングキュー
 * @tparam T キューに格納するデータ型
 * @tparam キューを実現するコンテナ型
 * @see See also BlockinQueue.h
 */
template <class T, class Container = std::deque<T>>
class BlockingDeque
{
public:
	/**
	 * @brief Blocking push_front() for std::deque<>
	 * @param [in] value A value to be pushed.
	 */
    void push_front(T const& value)
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            queue_.push_front(value);
        }
        condition_.notify_one();
    }

    T& front(std::chrono::microseconds timeout)
    {
    	std::unique_lock<std::mutex> lock(mutex_);
    	if(!condition_.wait_for(lock, timeout, [=]{ !queue_.empty(); })){
    		throw std::runtime_error("queue.front() time out");
    	}
    	return queue_.front();
    }

    T& back(std::chrono::microseconds timeout)
    {
    	std::unique_lock<std::mutex> lock(mutex_);
    	if(!condition_.wait_for(lock, timeout, [=]{ !queue_.empty(); })){
    		throw std::runtime_error("queue.back() time out");
    	}
    	return queue_.back();
    }

    /**
     * @brief Pop from queue with timeout
     * @param [out] value A value popped from the queue
     * @param [in] timeout Timeout for pop
     * @return True if successfully popped, false if timed out.
     */
    void pop_back(T& value, std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mutex_);
    	if (!condition_.wait_for(lock, timeout, [=]{ return !queue_.empty(); })) {
    		throw std::runtime_error("queue.pop() time out");
    	}
        value = queue_.back();
        queue_.pop_back();
    }

    /**
     * @brief Return size of container
     * @return The size of the container
     */
    int size() const { return queue_.size(); }

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    Container queue_;
};





#endif /* EXAMPLES_INCLUDE_BLOCKINGDEQUE_H_ */
