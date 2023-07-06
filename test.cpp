#include <thread>

#include "loglog.h"

void call_3()
{
	CALL_START("");
	sleep(1);
	A_INFO("this is call_3");
}

void call_2()
{
	CALL_START("");
	sleep(1);
	call_3();
}

void call_1()
{
	CALL_START("");
	sleep(1);
	call_2();
}

int main()
{
	LogTitle::set("Test");
	LogLevel::set(INFO_LEVEL);
	
	std::thread t_1 = std::thread([&]{
		call_1();
	});
	
	std::thread t_2 = std::thread([&]{
		call_1();
	});
	
	std::thread t_3 = std::thread([&]{
		call_1();
	});
	
	t_1.join();
	t_2.join();
	t_3.join();
	
	
	return 0;
}
