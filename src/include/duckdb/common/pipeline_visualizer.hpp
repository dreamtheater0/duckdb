#pragma once

#include "duckdb/common/plan_visualizer.hpp"
#include "duckdb/execution/physical_operator.hpp"

namespace duckdb {

class PipelineVisualizer : PlanVisualizer<PhysicalOperator> {
public:
	explicit PipelineVisualizer(const std::string &plan_name) : PlanVisualizer<PhysicalOperator>(plan_name) {
	}
	void Visualize();
	void AddPlan(PhysicalOperator *root);
	void AddPipeline(const std::vector<shared_ptr<Pipeline>> &pipelines);

private:
	void DumpGraphFooter(std::ofstream& ofs) override;

private:
	std::vector<std::vector<PhysicalOperator*>> pipelines;
};

} // namespace duckdb
