#pragma once

#include "util/TThreadSafeQueue.h"

#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include <functional>

class CJobSystem
{
public:
	typedef std::function<void(void)> JobFunction;

	/**
	* \brief Starts job system.
	* Creates thread pool.
	*/
	CJobSystem();

	/**
	* \brief Finishes pending tasks.
	*/
	~CJobSystem();

	/**
	* \brief Stops pending job execution.
	*/
	void stop();

	/**
	* \brief Starts job execution.
	*/
	void start();

	/**
	* \brief Adds job to execution queue.
	*/
	void addJob(const JobFunction& job);

	/**
	* \brief Blocks until all jobs are finished.
	*/
	void finishJobs();

private:
	/**
	* \brief Work function executed by threads.
	*/
	void work();

	std::vector<std::thread> m_threads; /**< Thread pool. */
	TThreadSafeQueue<JobFunction> m_jobQueue; /**< Stores pending jobs. */

	unsigned int m_threadCount = 0; /**< Number of threads used. */

	std::atomic<bool> m_done = false; /**< Flag for stopping worker threads. */
	std::atomic<bool> m_stop = false; /**< Execution started/stopped flag. */
};