#pragma once

#include "core/Macros.hpp"
#include "core/ITickable.hpp"
#include <memory>
#include <string>
#include <map>

namespace fury
{
  class Scene;
  class UiComponent;

  class Ui : public ITickable
  {
  public:
    Ui();
    Ui(Scene* scene);
    FURY_OnlyMovable(Ui)
    ~Ui();
    void init(Scene* scene);
    void tick(float dt) override;
    UiComponent* get_component(const std::string& name);
    template<typename T>
    T* get_component(const std::string& name);
  private:
    std::map<std::string, std::unique_ptr<UiComponent>> m_components;
  };
} // namespace fury
