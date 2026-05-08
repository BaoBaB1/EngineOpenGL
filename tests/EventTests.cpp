#include "core/Event.hpp"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <tuple>

using namespace fury;
using namespace testing;

namespace
{
  Event<int> on_bar_called_event;
  void bar(int a)
  {
    on_bar_called_event.notify(a);
  }

  struct TestClassWithEvent
  {
    void foo(double a, float b, bool c) { on_foo_called_event.notify(a, b, c); }
    Event<double, float, bool> on_foo_called_event;
  };

  struct TestClassEventListener
  {
    void foo(double a, float b, bool c)
    {
      this->a = a;
      this->b = b;
      this->c = c;
    }

    double a = 0.;
    float b = 0.f;
    bool c = false;
  };

} // namespace

TEST(EventTest, CallFunctionListener)
{
  int call_count = 0;
  on_bar_called_event += new FunctionListener(std::function([&](int a) { call_count++; }));
  bar(1);
  bar(2);
  bar(3);
  EXPECT_EQ(call_count, 3);
}

TEST(EventTest, CallInstanceListener)
{
  TestClassWithEvent te;
  TestClassEventListener t;
  te.on_foo_called_event += new InstanceListener(&t, &TestClassEventListener::foo);
  te.foo(2.2, 1.11f, true);
  EXPECT_THAT(std::tuple(t.a, t.b, t.c), FieldsAre(2.2, 1.11f, true));
}

TEST(EventTest, RemoveAllListeners)
{
  Event<int> event;
  event += new FunctionListener(&bar);
  event += new FunctionListener(&bar);
  EXPECT_TRUE(event.listeners_count() == 2);
  event.remove_all_listeners();
  EXPECT_TRUE(event.listeners_count() == 0);
}

TEST(EventTest, RemoveListenerByPointer)
{
  Event<int> event;
  event += new FunctionListener(&bar);
  auto lptr = event.get_listener(0);
  event -= lptr;
  EXPECT_TRUE(event.listeners_count() == 0);
}

TEST(EventTest, RemoveListenerByInstance)
{
  TestClassWithEvent te;
  TestClassEventListener t;
  te.on_foo_called_event += new InstanceListener(&t, &TestClassEventListener::foo);
  te.on_foo_called_event.remove_listener_by_instance(&t);
  EXPECT_TRUE(te.on_foo_called_event.listeners_count() == 0);
}

TEST(EventTest, RemoveFunctionListenerByName)
{
  Event<int> event;
  event += new NamedFunctionListener("Name1", &bar);
  event += new NamedFunctionListener("Name2", &bar);
  EXPECT_TRUE(event.get_listener(0)->get_name() == "Name1" && event.get_listener(1)->get_name() == "Name2");
  event.remove_listener_by_name("Name1");
  EXPECT_TRUE(event.listeners_count() == 1 && event.get_listener(0)->get_name() == "Name2");
  event.remove_listener_by_name("Name2");
  EXPECT_TRUE(event.listeners_count() == 0);
}

TEST(EventTest, RemoveInstanceListenerByName)
{
  TestClassWithEvent observable;
  TestClassEventListener observer, observer2;
  observable.on_foo_called_event += new NamedInstanceListener("Observer1", &observer, &TestClassEventListener::foo);
  observable.on_foo_called_event += new NamedInstanceListener("Observer2", &observer2, &TestClassEventListener::foo);
  EXPECT_EQ(observable.on_foo_called_event.get_listener(0)->get_name(), "Observer1");
  EXPECT_EQ(observable.on_foo_called_event.get_listener(1)->get_name(), "Observer2");

  observable.on_foo_called_event.remove_listener_by_name("Observer2");
  EXPECT_EQ(observable.on_foo_called_event.get_listener(0)->get_name(), "Observer1");
  EXPECT_TRUE(observable.on_foo_called_event.listeners_count() == 1);

  observable.on_foo_called_event.remove_listener_by_name("Observer1");
  EXPECT_TRUE(observable.on_foo_called_event.listeners_count() == 0);
}

TEST(EventTest, RemoveListenerByNotExistingName)
{
  Event<int> event;
  event.remove_listener_by_name("Name1");
  EXPECT_TRUE(event.listeners_count() == 0);
  TestClassWithEvent observable;
  observable.on_foo_called_event.remove_listener_by_name("Name1");
  EXPECT_TRUE(observable.on_foo_called_event.listeners_count() == 0);
}
