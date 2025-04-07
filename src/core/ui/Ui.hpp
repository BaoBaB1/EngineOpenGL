#pragma once

#include "utils/Macro.hpp"
#include "UiComponent.hpp"
#include "SceneInfo.hpp"
#include "MenuBar.hpp"
#include "FileSelectionWindow.hpp"
#include "Gizmo.hpp"
#include "core/ITickable.hpp"
#include <memory>
#include <string>
#include <map>

namespace fury
{
  class SceneRenderer;

  class Ui : public ITickable
  {
  public:
    Ui() = default;
    Ui(SceneRenderer* scene);
    OnlyMovable(Ui)
    ~Ui();
    void init(SceneRenderer* scene);
    void tick() override;
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
