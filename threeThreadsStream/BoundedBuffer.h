#pragma once
/**
 * Under the license of LGPL V3.0
 * http://www.gnu.org/licenses/lgpl-3.0.de.html
 * Copyright © 2015 Free Software Foundation, Inc. <http://fsf.org/>
 * Everyone is permitted to copy and distribute verbatim copies or changing
 * of this document is allowed.
 * Changed documents must contain this license notice.
 * You can use another license for your own changes.
 */

#include <mutex>
#include <condition_variable>
#include <iostream>
#include <atomic>
#include <chrono>

/**
 * @brief The BoundedBuffer class
 * A cyclic buffer, which has the read index everytime behind the write index.
 * The buffer size is given in the constructor.
 * It has to be guarantied, that there is only one thread which reads or writes from one buffer!
 *
 * @see http://baptiste-wicht.com/posts/2012/04/c11-concurrency-tutorial-advanced-locking-and-condition-variables.html
 */
template<typename T>
class BoundedBuffer {
public:
	typedef T value_type;
	value_type* buffer;
	size_t capacity;

    std::atomic_size_t front;
	size_t rear;
    std::atomic_size_t count;

	std::mutex lock;

	std::condition_variable not_full;
	std::condition_variable not_empty;

    std::chrono::milliseconds delayCheck;

    BoundedBuffer(size_t capacity
                  , const std::chrono::milliseconds &aDelayCheck = std::chrono::milliseconds(4))
        : capacity(capacity), front(0), rear(0)
    {
        delayCheck = aDelayCheck;
        count.store(0);
		buffer = new value_type[capacity];
        // XXX here you can allocate your frames once for the whole capacity.
        // Or you did not need to allocate, instead just move a pointer.
	}

    ~BoundedBuffer()
    {
		delete[] buffer;
        // XXX here you would deallocate all the frames.
	}

    void deposit(value_type &&data)
    {
		std::unique_lock<std::mutex> l(lock);

        // with this if, it would be correct, because deposite could be called from two threads.
        // And the second thread would not get any notify() again.
        // Because we have only one thread guarantied to call deposite on this buffer,
        // we omit this if for performance reasons.
        //if (count.load() != capacity) {
        not_full.wait_for(l, delayCheck, [this](){return count.load() != capacity; });
        //}

        if (count.load() == capacity)
        {
            // TODO: check, if front.store(front.load()+1) is possible.
            size_t i = front.load();
            front.store((i+1) % capacity);
        } else {
            ++count;
        }

        buffer[rear] = std::move(data);
        rear = (rear + 1) % capacity;

		not_empty.notify_one();
	}

    value_type & fetch()
    {
		std::unique_lock<std::mutex> l(lock);

        not_empty.wait_for(l, delayCheck, [this](){return count.load() != 0; });

        size_t i = front.load();
        value_type result;
        if (count.load() != 0)
        {
            result = buffer[i];     // needs to be before not_full.notify()
            front.store((i+1) % capacity);
            --count;
            not_full.notify_one();
        } else {
            result = buffer[i];
        }

        return result;
	}

    BoundedBuffer<T> & operator<<(typename BoundedBuffer<T>::value_type &&rhs)
    {
        deposit(std::move(rhs));
        // Return-Value-Optimization (RVO) will simplify the return value.
        return *this;
    }

    BoundedBuffer<T> & operator>>(typename BoundedBuffer<T>::value_type &rhs) // const - not possible, because front. Solution: front as mutable.
    {
        rhs = std::move(fetch());
        return *this;
    }

    template <T>
    friend std::ostream & operator<<(std::ostream &lhs, BoundedBuffer<T> &rhs);

    template <T>
    friend std::istream & operator>>(std::istream &lhs, BoundedBuffer<T> &rhs);

};

template<typename T>
std::ostream & operator<<(std::ostream &lhs, BoundedBuffer<T> &rhs)
{
    lhs << rhs.fetch();
    return lhs;
}

template<typename T>
std::istream & operator>>(std::istream &lhs, BoundedBuffer<T> &rhs)
{
    T value;
    lhs >> value;
    rhs.deposit(std::move(value));
    return lhs;
}

