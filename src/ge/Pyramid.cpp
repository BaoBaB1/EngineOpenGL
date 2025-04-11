#include "Pyramid.hpp"

namespace fury
{
  Pyramid::Pyramid() : Object3D("Pyramid")
  {
    auto& mesh = emplace_mesh();
    mesh.vertices().reserve(5);
    mesh.faces().reserve(6);
    mesh.vertices().emplace_back(-0.5f, -0.5f, -0.5f);
    mesh.vertices().emplace_back(0.5f, -0.5f, -0.5f);
    mesh.vertices().emplace_back(0.5f, -0.5f, 0.5f);
    mesh.vertices().emplace_back(-0.5f, -0.5f, 0.5f);
    mesh.vertices().emplace_back(0.f, 0.5f, 0.f);
    mesh.faces().emplace_back(Face({ 0, 1, 2 }));
    mesh.faces().emplace_back(Face({ 0, 2, 3 }));
    mesh.faces().emplace_back(Face({ 0, 4, 1 }));
    mesh.faces().emplace_back(Face({ 1, 4, 2 }));
    mesh.faces().emplace_back(Face({ 2, 4, 3 }));
    mesh.faces().emplace_back(Face({ 3, 4, 0 }));
  }
}
