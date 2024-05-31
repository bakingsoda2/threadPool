#include "threadPool.h"

threadPool::threadPool(unsigned int num):stop_(false)
{
	if (num <= 1)
		thread_num_ = 2;
	else
		thread_num_ = num;
}

void threadPool::start()
{
	for (int i = 0; i < thread_num_; i++)
	{
		//创建一个线程，这个线程的任务就是在线程池没有停的情况下去获取任务队列里的任务并且执行
		//其中发生了隐式转换，因为你传入的是一个可调用lambda函数，但是实际是创建了一个线程
		pool_.emplace_back(
			[this]()
			{
				while (!stop_.load())
				{
					Task task;
					//异步获取任务
					{
						std::unique_lock<std::mutex> rw_mt(rw_mt_);
						//只有当线程池没关闭，而且任务队列非空的时候，才会解除阻塞往下走
						rw_lock_.wait(rw_mt, [this]()
							{
								return stop_.load() || !tasks_.empty();
							});
						if (tasks_.empty())
							return;
						task = std::move(tasks_.front());
						tasks_.pop();
					}
					//执行任务
					thread_num_--;
					task();
					thread_num_++;
				}
			}
		);
	}
}

void threadPool::stop()
{
	stop_.store(true);
	rw_lock_.notify_all();
	for (auto& td : pool_)
	{
		if (td.joinable())
		{
			std::cout << "join thread" << td.get_id() << std::endl;
			td.join();
		}
	}
}

threadPool& threadPool::getInstance()
{
	//static threadPool();
	// TODO: 在此处插入 return 语句
}
