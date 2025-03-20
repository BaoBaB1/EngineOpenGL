#pragma once

#include <string>

class Entity
{
public:
  void set_name(const std::string& name) { m_name = name; }
  const std::string& get_name() const { return m_name; }
protected:
  Entity(const std::string& name) : m_name(name) {}
private:
  std::string m_name;
};
