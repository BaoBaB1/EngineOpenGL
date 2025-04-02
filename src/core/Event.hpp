#pragma once

#include <vector>
#include <functional>
#include <memory>

namespace fury
{
	template<typename... Args>
	class EventListener
	{
	public:
		virtual void notify(Args... args) const = 0;
		virtual ~EventListener() = default;
	};

	template<typename ReturnType, typename... Args>
	class FunctionListener : public EventListener<Args...>
	{
	public:
		using Func = std::function<ReturnType(Args...)>;
		using FuncPtr = ReturnType(*)(Args...);
		FunctionListener(const Func& func) : m_func(func) {}
		FunctionListener(FuncPtr func_ptr) : m_func(func_ptr) {}
		void notify(Args... args) const override
		{
			m_func(args...);
		}
	private:
		Func m_func;
	};

	template<typename Class, typename ReturnType, typename... Args>
	class InstanceListener : public EventListener<Args...>
	{
	public:
		using ClassFunc = ReturnType(Class::*)(Args...);
		InstanceListener(Class* instance, ClassFunc func) : m_func(func), m_instance(instance) {}
		void notify(Args... args) const override
		{
			(m_instance->*m_func)(args...);
		}
	private:
		Class* m_instance;
		ClassFunc m_func;
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

		Event& operator +=(Listener* listener)
		{
			m_listeners.emplace_back(listener);
			return *this;
		}

		bool operator -=(Listener* listener)
		{
			return this->operator-=(static_cast<void*>(listener));
		}

		bool operator -=(void* listener)
		{
			for (size_t i = 0; i < m_listeners.size(); i++)
			{
				if (m_listeners[i].get() == listener)
				{
					m_listeners.erase(m_listeners.begin() + i);
					return true;
				}
			}
			return false;
		}

		Listener* get(size_t idx) { return m_listeners[idx].get(); }

		Listener* back() { return m_listeners.back().get(); }

		void unregister_all()
		{
			m_listeners.clear();
		}

		size_t listeners_count() const { return m_listeners.size(); }

	private:
		std::vector<std::unique_ptr<Listener>> m_listeners;
	};
}
