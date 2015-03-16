#pragma once

#include <mutex>
#include <queue>

/**
* \brief Thread safe queue.
*/
template <typename T>
class TThreadSafeQueue
{
public:
	TThreadSafeQueue();
	TThreadSafeQueue(const TThreadSafeQueue& rhs);
	TThreadSafeQueue& operator=(const TThreadSafeQueue& rhs);

	/**
	* \brief Attempts to access and remove first element.
	* \parm value Reference to value for output.
	* \return True if element was popped and false if not.
	*/
	bool pop(T& value);

	/**
	* \brief Pushes element at end of queue.
	* \parm value Value to be pushed.
	*/
	void push(const T& value);

	/**
	* \brief Returns true if the queue is empty and false if not.
	* This call is not protected by a mutex.
	*/
	bool empty() const;

protected:
	mutable ::std::mutex d_mutex; /**< Queue mutex. */
	::std::queue<T> d_queue; /**< Data queue. */
};

template <typename T>
TThreadSafeQueue<T>::TThreadSafeQueue()
{
	return;
}

template <typename T>
TThreadSafeQueue<T>::TThreadSafeQueue(const TThreadSafeQueue<T>& rhs)
{
	std::lock_guard<std::mutex> lg(rhs.d_mutex);
	d_queue = rhs.d_queue;
	return;
}

template <typename T>
TThreadSafeQueue<T>& TThreadSafeQueue<T>::operator=(const TThreadSafeQueue<T>& rhs)
{
	// Lock both queues
	std::lock(d_mutex, rhs.d_mutex);
	std::lock_guard<std::mutex>(d_mutex, std::adopt_lock);
	std::lock_guard<std::mutex>(rhs.d_mutex, std::adopt_lock);
	d_queue = rhs.d_queue;
	return *this;
}

template <typename T>
void TThreadSafeQueue<T>::push(const T& value)
{
	std::lock_guard<std::mutex> lg(d_mutex);
	d_queue.push(value);
	return;
}

template <typename T>
bool TThreadSafeQueue<T>::pop(T& value)
{
	// Check without locking
	if (d_queue.empty())
	{
		return false;
	}
	// Lock and check again
	std::lock_guard<std::mutex> lg(d_mutex);
	if (d_queue.empty())
	{
		return false;
	}
	value = d_queue.front();
	d_queue.pop();
	return true;
}

template <typename T>
bool TThreadSafeQueue<T>::empty() const
{
	return d_queue.empty();
}