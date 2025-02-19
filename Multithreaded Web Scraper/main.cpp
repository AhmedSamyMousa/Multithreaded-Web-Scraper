#include <iostream>
#include <thread>

class Guard
{
private:
	std::thread& thrd;
public:
	explicit Guard(std::thread& thread) : thrd(thread) {}
	~Guard() {
		if (thrd.joinable())
		{
			thrd.join();
			std::cout << "Thread Joined \n";
		}
	}
	Guard(const Guard&) = delete;
	Guard& operator = (const Guard&) = delete;
};

void Task(int id)
{
	std::cout << "Task "<<id<<" is running.... \n";
}

int main()
{
	std::thread t(Task,1);
	std::thread t1(Task, 2);
	Guard guard(t);
	Guard guard1(t1);
}