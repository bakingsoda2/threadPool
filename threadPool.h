#pragma once

#include <atomic>
#include <queue>
#include <iostream>
#include <thread>
#include <future>

using Task = std::packaged_task<void()>;

class threadPool
{
private:
	std::atomic_int thread_num_;		//当前可用线程数量
	std::queue<Task> tasks_;			//任务队列
	std::vector<std::thread> pool_;		//线程池
	std::atomic_bool stop_;				//是否关闭
	std::mutex rw_mt_;					//互斥锁
	std::condition_variable rw_lock_;	//条件变量

private:
	threadPool(unsigned int num = std::thread::hardware_concurrency());
	threadPool(threadPool& other) = delete;
	threadPool& operator=(threadPool& other) = delete;
	void start();				//开启
	void stop();				//关闭

	template <typename T, typename... Args>
	auto commit(T&& f, Args&&... args) -> 
		//推导下返回值类型，给auto
		std::future<decltype(std::forward<T>(f)(std::forward<Args>(args)...))>
	{
		//获取实际的返回类型，用std::forward是为了完美转发以实现更好的兼容性（一般来讲任务传入的都是左值/左值引用）
		using RetType = decltype(std::forward<T>(f)(std::forward<Args>(args)...);
		//如果线程直接停了，就不执行直接返回
		if (stop_.load())
			return std::future(RetType) {};
		//创建一个将传入的任务函数和函数的参数绑定后生成的std::packaged_task的智能指针
		//这里用到了闭包（C++只有伪闭包，注意）因为task接收的是返回值是void，参数是void的任务
		//通过std::bind，将任务函数f转变成一个参数为空的std::packaged_task对象
		auto task = std::make_shared<std::packaged_task<RetType()>>(
			std::bind(std::forward<T>(f), std::forward<Args>(args)...)
		);
		std::future<RetType> ret = task->get_future();

		//异步加入任务队列
		{
			std::lock_guard<std::mutex> rw_mt(rw_mt_);
			tasks_.emplace(
				//不能直接放入task，因为task的返回值是RetType类型的，要把他变成void返回值，用lambda表达式
				[task]
				{
					(*task)();			//执行任务函数
				}
			);
		}
		rw_lock_.notify_one();			//唤醒线程池里的线程，让他们去任务队列里取任务执行
		return ret;
	};
		

public:
	static threadPool& getInstance();

};

