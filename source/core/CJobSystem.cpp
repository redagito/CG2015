#include "CJobSystem.h"

CJobSystem::CJobSystem()
{
	// Number of threads is based on available processors.
	m_threadCount = std::thread::hardware_concurrency();
	if (m_threadCount == 0)
	{
		// If hardware stats are unavailable, force 4 threads as default.
		m_threadCount = 4;
	}
	// Create threads and queues
	for (unsigned int i = 0; i < m_threadCount; ++i)
	{
		m_threads.push_back(std::thread(&CJobSystem::work, this));
	}
}

CJobSystem::~CJobSystem()
{
	// Stop job retrieval
	m_done = true;
	// Finish current tasks
	for (auto& thread : m_threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
}

void CJobSystem::addJob(const CJobSystem::JobFunction& job)
{
	m_jobQueue.push(job);
	return;
}

void CJobSystem::stop()
{
	m_stop = true;
}

void CJobSystem::start()
{
	m_stop = false;
}

void CJobSystem::finishJobs()
{
	while (!m_jobQueue.empty())
	{
		std::this_thread::yield();
	}
}

void CJobSystem::work()
{
	while (!m_done)
	{
		JobFunction job;
		if (!m_stop && m_jobQueue.pop(job))
		{
			job();
		}
		else
		{
			std::this_thread::yield();
		}
	}
}