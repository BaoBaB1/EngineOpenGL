#pragma once

#include "VertexArrayObject.hpp"
#include "VertexBufferObject.hpp"
#include "ElementBufferObject.hpp"
#include "SSBO.hpp"
#include "UniformBuffer.hpp"
#include <unordered_map>
#include <vector>

struct PipelineBuffersRecord
{
	VertexArrayObject vao;
	VertexBufferObject vbo;
	ElementBufferObject ebo;
	std::vector<SSBO> ssbos;
	std::vector<UniformBuffer> ubos;
};

class PipelineBuffersManager
{
public:
	static PipelineBuffersManager& instance() { static PipelineBuffersManager m; return m; }
	PipelineBuffersRecord& get(const std::string& name) { return records[name]; }
	std::unordered_map<std::string, PipelineBuffersRecord>& get_records() { return records; }
	bool contains(const std::string& name) { return records.count(name) != 0; }
private:
	PipelineBuffersManager() = default;
	inline static std::unordered_map<std::string, PipelineBuffersRecord> records;
};
