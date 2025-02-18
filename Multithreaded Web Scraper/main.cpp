#include <iostream>
#include <thread>

class Work
{
public:
	void operator()()
	{
		while (true) { std::cout << "I'm Working... \n"; }
	}
};

int main()
{
	Work work;
	std::thread t(work);
	t.join();
}
