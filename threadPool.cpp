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
		//����һ���̣߳�����̵߳�����������̳߳�û��ͣ�������ȥ��ȡ����������������ִ��
		//���з�������ʽת������Ϊ�㴫�����һ���ɵ���lambda����������ʵ���Ǵ�����һ���߳�
		pool_.emplace_back(
			[this]()
			{
				while (!stop_.load())
				{
					Task task;
					//�첽��ȡ����
					{
						std::unique_lock<std::mutex> rw_mt(rw_mt_);
						//ֻ�е��̳߳�û�رգ�����������зǿյ�ʱ�򣬲Ż�������������
						rw_lock_.wait(rw_mt, [this]()
							{
								return stop_.load() || !tasks_.empty();
							});
						if (tasks_.empty())
							return;
						task = std::move(tasks_.front());
						tasks_.pop();
					}
					//ִ������
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
	// TODO: �ڴ˴����� return ���
}
