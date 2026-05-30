#pragma once

namespace fury
{
  template<typename T>
  class Singleton
  {
  public:
    static T& instance()
    {
      static T inst;
      return inst;
    }
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) noexcept = default;
    Singleton& operator=(Singleton&&) noexcept = default;
    ~Singleton() = default;
  protected:
    Singleton() = default;
  };
} // namespace fury
