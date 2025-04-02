#pragma once

namespace furyutils
{
  template<typename T>
  class Singleton
  {
  public:
    static T& instance()
    {
      static T t;
      return t;
    }
  private:
    Singleton() = delete;
  };
}
