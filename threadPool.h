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
	std::atomic_int thread_num_;		//��ǰ�����߳�����
	std::queue<Task> tasks_;			//�������
	std::vector<std::thread> pool_;		//�̳߳�
	std::atomic_bool stop_;				//�Ƿ�ر�
	std::mutex rw_mt_;					//������
	std::condition_variable rw_lock_;	//��������

private:
	threadPool(unsigned int num = std::thread::hardware_concurrency());
	threadPool(threadPool& other) = delete;
	threadPool& operator=(threadPool& other) = delete;
	void start();				//����
	void stop();				//�ر�

	template <typename T, typename... Args>
	auto commit(T&& f, Args&&... args) -> 
		//�Ƶ��·���ֵ���ͣ���auto
		std::future<decltype(std::forward<T>(f)(std::forward<Args>(args)...))>
	{
		//��ȡʵ�ʵķ������ͣ���std::forward��Ϊ������ת����ʵ�ָ��õļ����ԣ�һ������������Ķ�����ֵ/��ֵ���ã�
		using RetType = decltype(std::forward<T>(f)(std::forward<Args>(args)...);
		//����߳�ֱ��ͣ�ˣ��Ͳ�ִ��ֱ�ӷ���
		if (stop_.load())
			return std::future(RetType) {};
		//����һ����������������ͺ����Ĳ����󶨺����ɵ�std::packaged_task������ָ��
		//�����õ��˱հ���C++ֻ��α�հ���ע�⣩��Ϊtask���յ��Ƿ���ֵ��void��������void������
		//ͨ��std::bind����������fת���һ������Ϊ�յ�std::packaged_task����
		auto task = std::make_shared<std::packaged_task<RetType()>>(
			std::bind(std::forward<T>(f), std::forward<Args>(args)...)
		);
		std::future<RetType> ret = task->get_future();

		//�첽�����������
		{
			std::lock_guard<std::mutex> rw_mt(rw_mt_);
			tasks_.emplace(
				//����ֱ�ӷ���task����Ϊtask�ķ���ֵ��RetType���͵ģ�Ҫ�������void����ֵ����lambda���ʽ
				[task]
				{
					(*task)();			//ִ��������
				}
			);
		}
		rw_lock_.notify_one();			//�����̳߳�����̣߳�������ȥ���������ȡ����ִ��
		return ret;
	};
		

public:
	static threadPool& getInstance();

};

