#pragma once

namespace furyutils 
{
  class IObserver
  {
  public:
    virtual void notify(bool value) = 0;
    virtual ~IObserver() = default;
  };
}
