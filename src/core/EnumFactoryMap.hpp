#pragma once

#include <functional>
#include <memory>

namespace fury
{
  template<typename Enum, typename BaseClass>
  class EnumFactoryMap
  {
  public:
    template<typename ConcreteClass, std::enable_if_t<std::is_base_of_v<BaseClass, ConcreteClass>, int> = 0>
    static void add_entry(Enum val) {
      mp.insert({ val, []() { return std::make_unique<ConcreteClass>(); } });
    }
    static std::unique_ptr<BaseClass> create(Enum val) { return mp.at(val)(); }
  private:
    using FactoryMethod = std::function<std::unique_ptr<BaseClass>()>;
    inline static std::unordered_map<Enum, FactoryMethod> mp;
  };
}
