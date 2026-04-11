#pragma once

namespace fury
{
  template<typename T>
  class Singletone {
  public:
    static T& instance() {
      static T inst;
      return inst;
    }
    Singletone(const Singletone&) = delete;
    Singletone& operator=(const Singletone&) = delete;
    Singletone(Singletone&&) = default;
    Singletone& operator=(Singletone&&) = default;
    ~Singletone() = default;
  protected:
    Singletone() = default;
  };
}
