#include "core/Event.hpp"
#include "gtest/gtest.h"

using namespace fury;

namespace
{
	int g_a = 0;
	Event<int> on_bar_called_event;
	void bar(int a)
	{
		a += 10;
		on_bar_called_event.notify(a);
	}

	int bar_listener_func(int a)
	{
		g_a = a;
		return a;
	}

	struct TestClassWithEvent
	{
		void foo(double a, float b, bool c)
		{
			on_foo_called_event.notify(a, b, c);
		}
		Event<double, float, bool> on_foo_called_event;
	};

	struct TestClassEventListener
	{
		TestClassEventListener()
		{
			otherClass.on_foo_called_event += new InstanceListener(this, &TestClassEventListener::foo);
		}

		void foo(double a, float b, bool c)
		{
			this->a = a;
			this->b = b;
			this->c = c;
		}

		double a = 0.;
		float b = 0.f;
		bool c = false;
		TestClassWithEvent otherClass;
	};

}

TEST(EventTest, FunctionEvent)
{
	on_bar_called_event += new FunctionListener(&bar_listener_func);
	bar(20);
	EXPECT_TRUE(g_a == 30);
}

TEST(EventTest, InstancedEvent)
{
	TestClassEventListener t;
	t.otherClass.foo(2.2, 1.11f, true);
	EXPECT_DOUBLE_EQ(t.a, 2.2);
	EXPECT_FLOAT_EQ(t.b, 1.11f);
	EXPECT_TRUE(t.c == true);
}

TEST(EventTest, UnregisterAllListeners)
{
	Event<int> event;
	event += new FunctionListener(&bar);
	event += new FunctionListener(&bar);
	EXPECT_TRUE(event.listeners_count() == 2);
	event.unregister_all();
	EXPECT_TRUE(event.listeners_count() == 0);
}

TEST(EventTest, RemoveListener)
{
	Event<int> event;
	event += new FunctionListener(&bar);
	auto lptr = event.get(0);
	event -= lptr;
	EXPECT_TRUE(event.listeners_count() == 0);
}
