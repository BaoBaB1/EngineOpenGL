#pragma once

#include "opengl/VertexArrayObject.hpp"
#include "opengl/VertexBufferObject.hpp"
#include "opengl/ElementBufferObject.hpp"
#include "opengl/SSBO.hpp"
#include "opengl/UniformBuffer.hpp"
#include <unordered_map>
#include <string>

template<typename T>
class PipelineBuffersManager
{
public:
	static T& get(const std::string& name) { return records[name]; }
private:
	inline static std::unordered_map<std::string, T> records;
};

using PipelineVAOManager = PipelineBuffersManager<VertexArrayObject>;
using PipelineVBOManager = PipelineBuffersManager<VertexBufferObject>;
using PipelineEBOManager = PipelineBuffersManager<ElementBufferObject>;
using PipelineSSBOManager = PipelineBuffersManager<SSBO>;
using PipelineUBOManager = PipelineBuffersManager<UniformBuffer>;
