#pragma once

#include "VertexArrayObject.hpp"
#include "VertexBufferObject.hpp"
#include "ElementBufferObject.hpp"
#include "SSBO.hpp"
#include "UniformBuffer.hpp"
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
