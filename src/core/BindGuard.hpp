#pragma once

#include "OpenGLObject.hpp"
#include <initializer_list>

template<typename T>
class BindGuard
{
public:
	BindGuard(const T& bindable) : m_bindable(bindable) { m_bindable.bind(); }
	~BindGuard() { m_bindable.unbind(); }
private:
	const T& m_bindable;
};

template<typename T>
class BindGuard<T*>
{
public:
	BindGuard(const T* bindable) { m_bindable = bindable; m_bindable->bind(); }
	~BindGuard() { m_bindable->unbind(); }
private:
	T* m_bindable;
};

class BindChainFIFO
{
public:
	BindChainFIFO(std::initializer_list<OpenGLObject*> bindables)
	{
		m_bindables = std::move(bindables);
		for (const auto bindable : bindables)
		{
			bindable->bind();
		}
	}
	~BindChainFIFO()
	{
		for (const auto bindable : m_bindables)
		{
			bindable->unbind();
		}
	}
private:
	std::initializer_list<OpenGLObject*> m_bindables;
};