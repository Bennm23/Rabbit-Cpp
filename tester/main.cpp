#include <iostream>

#include "rabbit_channel.h"
#include <thread>
#include <chrono>

using namespace RabbitChannel;

class Tester
{
	public:
		Tester() : channel("test_app", &Tester::cb, this) {

			channel.subscribe("tester2");
		};

		void start()
		{
			channel.runInBackground();
		}

	private:
		RabbitMQChannel channel;

		static void cb(void* cls, size_t size, void* buf)
		{

			Tester *tester = static_cast<Tester*>(cls);

			char* msg = static_cast<char*>(buf);

			std::cout << "Tester1 received buf = " << msg << std::endl;

			tester->channel.publish("tester1", "I got your message");
		}
};
class Tester2
{
	public:
		Tester2() : channel("test_app2", &Tester2::cb, this) {

			channel.subscribe("tester1");
		};

		void start()
		{
			channel.runInBackground();
		}

		void publish(const std::string &msg)
		{
			channel.publish("tester2", msg);
		}

	private:
		RabbitMQChannel channel;

		static void cb(void* cls, size_t size, void* buf)
		{

			char* msg = static_cast<char*>(buf);

			std::cout << "Tester2 received buf = " << msg << std::endl;
		}
};

int main()
{

	std::cout << "Running Test Application" << std::endl;

	Tester tester;
	tester.start();

	Tester2 t2;
	t2.start();

	while(1)
	{
		t2.publish("hello t1");

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}
