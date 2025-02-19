#include <iostream>
#include <thread>

class Guard
{
private:
	std::thread& thrd;
	bool detachOnDestroy = false;
public:
	explicit Guard(std::thread& thread,bool detach = false) : thrd(thread),
		detachOnDestroy(detach) {}
    ~Guard() {
        if (thrd.joinable())
        {
            try {
                if (!detachOnDestroy)
                {
                    thrd.join();
                    std::cout << "Thread Joined \n";
                }
                else {
                    thrd.detach();
                    std::cout << "Thread Detached \n";
                }
            }
            catch (const std::system_error& e) {
                std::cerr << "Thread operation failed: " << e.what() << '\n';
            }
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
	std::this_thread::sleep_for(std::chrono::seconds(1));
}