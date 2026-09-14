#include "AssetManager.hpp"
#include "opengl/Texture2D.hpp"
#include "core/Application.hpp"
#include "core/Scene.hpp"

extern std::thread::id g_main_thread_id;

namespace fury
{
  template<>
  AssetHandle<Texture2D> AssetManager<Texture2D>::load(const std::filesystem::path& path)
  {
    const AssetId id = get_asset_id(path);
    auto it = m_assets.find(id);
    if (it != m_assets.end() && !it->second.expired())
    {
      return { id, it->second.lock() };
    }
    if (!std::filesystem::exists(path))
    {
      Logger::warn("[AssetManager::load]: Texture path {} does not exist.", path);
      return {};
    }
    Logger::debug("[AssetManager::load]: Loading texture {}.", path);
    auto tex = std::make_shared<Texture2D>();
    // Only main thread can do OpenGL calls
    if (::g_main_thread_id == std::this_thread::get_id())
    {
      tex->create(path);
    }
    else
    {
      auto [pixels, image_data] = Texture2D::read_image(path);
      Application::instance().get_scene().enqueue_job_from_async_thread(
          [tex, pixels_ = std::move(pixels), image_data_ = std::move(image_data)]()
          { tex->create(pixels_, image_data_); });
    }
    m_assets[id] = tex;
    return { id, tex };
  }

} // namespace fury
