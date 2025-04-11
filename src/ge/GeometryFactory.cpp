#include "GeometryFactory.hpp"
#include "ge/Cube.hpp"
#include "ge/Polyline.hpp"
#include "ge/Pyramid.hpp"
#include "ge/Icosahedron.hpp"
#include "ge/BezierCurve.hpp"
#include "core/Logger.hpp"

namespace fury
{
	std::unique_ptr<Object3D> GeometryFactory::create_from_type(int32_t type)
	{
		switch (type)
		{
		case Cube::type:
			return std::make_unique<Cube>();
		case Pyramid::type:
			return std::make_unique<Pyramid>();
		case Icosahedron::type:
			return std::make_unique<Icosahedron>();
		case Polyline::type:
			return std::make_unique<Polyline>();
		case BezierCurve::type:
			return std::make_unique<BezierCurve>();
		case Object3D::type:
			return std::make_unique<Object3D>();
		default:
			Logger::critical("Unknown object type {}.", type);
			throw std::runtime_error("Unknown object type");
		}
	}
}
