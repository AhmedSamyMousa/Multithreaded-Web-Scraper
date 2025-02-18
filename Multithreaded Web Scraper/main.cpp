#include <iostream>
#include <thread>

void Hello_World()
{
	std::string name = "Ahmed";
	std::cout << "Hello World, " << name;
}

int main()
{
	std::thread t(Hello_World);
	t.join();
}
