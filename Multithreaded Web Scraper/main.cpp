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

void Task()
{
	std::cout << "Task is running.... \n";
}

int main()
{
	std::thread t(Task);
	Guard guard(t);
}