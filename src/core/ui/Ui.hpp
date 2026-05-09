#pragma once

#include "core/Macros.hpp"
#include "UiComponent.hpp"
#include "SceneInfo.hpp"
#include "MenuBar.hpp"
#include "FileExplorer.hpp"
#include "Gizmo.hpp"
#include "core/ITickable.hpp"
#include <memory>
#include <string>
#include <map>

namespace fury
{
  class Scene;

  class Ui : public ITickable
  {
  public:
    Ui() = default;
    Ui(Scene* scene);
    FURY_OnlyMovable(Ui)
    ~Ui();
    void init(Scene* scene);
    void tick(float dt) override;
    UiComponent* get_component(const std::string& name) { return m_components.at(name).get(); }
    template<typename T> 
    T* get_component(const std::string& name)
    {
      return static_cast<T*>(m_components.at(name).get());
    }
  private:
    std::map<std::string, std::unique_ptr<UiComponent>> m_components;
  };
}
