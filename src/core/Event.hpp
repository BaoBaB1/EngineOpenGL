#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace fury
{
  template<typename... Args>
  class EventListener
  {
  public:
    virtual void notify(Args... args) const = 0;
    virtual void* get_owner_instance() const { return nullptr; }
    virtual std::string_view get_name() const { return ""; }
    virtual ~EventListener() = default;
  };

  template<typename ReturnType, typename... Args>
  class FunctionListener : public EventListener<Args...>
  {
  public:
    using Func = std::function<ReturnType(Args...)>;
    using FuncPtr = ReturnType (*)(Args...);
    FunctionListener(const Func& func) : m_func(func) {}
    FunctionListener(FuncPtr func_ptr) : m_func(func_ptr) {}
    void notify(Args... args) const override { m_func(args...); }
  private:
    Func m_func;
  };

  template<typename ReturnType, typename... Args>
  class NamedFunctionListener : public FunctionListener<ReturnType, Args...>
  {
  public:
    using Parent = typename FunctionListener<ReturnType, Args...>;
    NamedFunctionListener(const std::string& name, const Parent::Func& func) : Parent(func) { m_name = name; }
    NamedFunctionListener(const std::string& name, const Parent::FuncPtr& func_ptr) : Parent(func_ptr)
    {
      m_name = name;
    }
    std::string_view get_name() const override { return m_name; }
  private:
    std::string m_name;
  };

  template<typename Class, typename ReturnType, typename... Args>
  class InstanceListener : public EventListener<Args...>
  {
  public:
    using ClassFunc = ReturnType (Class::*)(Args...);
    InstanceListener(Class* instance, ClassFunc func) : m_func(func), m_instance(instance) {}
    void* get_owner_instance() const override { return m_instance; }
    void notify(Args... args) const override { (m_instance->*m_func)(args...); }
  private:
    Class* m_instance;
    ClassFunc m_func;
  };

  template<typename Class, typename ReturnType, typename... Args>
  class NamedInstanceListener : public InstanceListener<Class, ReturnType, Args...>
  {
  public:
    using Parent = typename InstanceListener<Class, ReturnType, Args...>;
    NamedInstanceListener(const std::string& name, Class* instance, Parent::ClassFunc func) : Parent(instance, func)
    {
      m_name = name;
    }
    std::string_view get_name() const { return m_name; }
  private:
    std::string m_name;
  };

  template<typename... Args>
  class Event
  {
  public:
    using Listener = EventListener<Args...>;

    void notify(Args... args) const
    {
      for (const auto& h : m_listeners)
      {
        h->notify(args...);
      }
    }

    Event& add_listener(Listener* listener)
    {
      m_listeners.emplace_back(listener);
      return *this;
    }

    Event& remove_listener(Listener* listener)
    {
      std::erase_if(m_listeners, [=](const auto& lstnr) { return lstnr.get() == listener; });
      return *this;
    }

    Event& operator-=(Listener* listener) { return remove_listener(listener); }

    Event& operator+=(Listener* listener) { return add_listener(listener); }

    bool remove_listener_by_name(const std::string& name)
    {
      return std::erase_if(m_listeners, [&](const auto& lstnr) { return lstnr->get_name() == name; }) > 0;
    }

    bool remove_listener_by_instance(void* owner)
    {
      return std::erase_if(m_listeners, [=](const auto& lstnr) { return lstnr->get_owner_instance() == owner; }) > 0;
    }

    Listener* get_listener(size_t idx) { return m_listeners[idx].get(); }

    void remove_all_listeners() { m_listeners.clear(); }

    size_t listeners_count() const { return m_listeners.size(); }

  private:
    std::vector<std::unique_ptr<Listener>> m_listeners;
  };
} // namespace fury
