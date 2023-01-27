#include "duckdb/common/plan_visualizer.hpp"
#include "duckdb/common/pipeline_visualizer.hpp"
#include "duckdb/common/assert.hpp"
#include "duckdb/execution/operator/helper/physical_result_collector.hpp"
#include "duckdb/execution/physical_operator.hpp"
#include "duckdb/parallel/pipeline.hpp"
#include "duckdb/planner/logical_operator.hpp"

namespace duckdb {

template <typename T>
void PlanVisualizer<T>::BuildGraphFromPlan(T &root) {
	base_graph.AddNode(root);
	ProcessChildOperator(root);
}

template <typename T>
void PlanVisualizer<T>::BuildGraphRecursive(const T &op, const T &parent) {
	if (base_graph.AddNode(op)) {
		ProcessChildOperator(op);
	}
}

template <typename T>
void PlanVisualizer<T>::ProcessChildOperator(const T &op) {
	for (auto &child_info : op.GetChildOperatorInfo()) {
		if (child_info.backward) {
			base_graph.AddEdge(op, *child_info.child_operator, child_info.label);
		} else {
			base_graph.AddEdge(*child_info.child_operator, op, child_info.label);
		}
		BuildGraphRecursive(*child_info.child_operator, op);
	}
}

//! Helper function to encode some special characters for HTML format
static void HtmlEncode(std::string &s) {
	std::string::size_type pos = 0;
	while ((pos = s.find_first_of("<>&\n", pos)) != std::string::npos) {
		std::string replacement;
		switch (s[pos]) {
		case '<':
			replacement = "&lt;";
			break;
		case '>':
			replacement = "&gt;";
			break;
		case '&':
			replacement = "&amp;";
			break;
		case '\n':
			replacement = "<br />";
			break;
		}
		s.replace(pos, 1, replacement);
		pos += replacement.size();
	}
}

template <typename T>
void PlanVisualizer<T>::DumpGraphToDotFile(std::ofstream& ofs) {
	DumpGraphHeader(ofs);
	DumpGraphNodes(ofs);
	DumpGraphEdges(ofs);
	DumpGraphFooter(ofs);
}

template <typename T>
void PlanVisualizer<T>::DumpGraphHeader(std::ofstream& ofs) {
	// Output header information
	ofs << "digraph duckdb_plan_graph {" << std::endl
	    << "  graph [label=\"Duck DB Plan Graph - " << plan_name << "\"" << std::endl
	    << "         rankdir=\"BT\"" << std::endl
	    << "         labelloc=\"t\"" << std::endl
	    << "         fontname=\"Helvetica,Arial,sans-serif\"]" << std::endl
	    << "  fontname=\"Helvetica,Arial,sans-serif\"" << std::endl
	    << "  node [fontname=\"Helvetica,Arial,sans-serif\"" << std::endl
	    << "        style=filled" << std::endl
	    << "        fillcolor=gray95" << std::endl
	    << "        shape=record]" << std::endl
	    << "  edge [fontname=\"Helvetica,Arial,sans-serif\"]" << std::endl;
}

template <typename T>
void PlanVisualizer<T>::DumpGraphNodes(std::ofstream& ofs) {
	// Output node information
	for (auto &node : base_graph.GetNodes()) {
		ofs << node.id << " [label=<{<b>" << node.name << "</b> ";
		for (auto prop : node.properties) {
			HtmlEncode(prop.second);
			ofs << "| {" << prop.first << " | " << prop.second << "} ";
		}
		ofs << "}>];" << std::endl;
	}
}

template <typename T>
void PlanVisualizer<T>::DumpGraphEdges(std::ofstream& ofs) {
	// Output edge information
	for (auto &edge : base_graph.GetEdges()) {
		ofs << edge.from << " -> " << edge.to << " [label=\"" + edge.label + "\"];" << std::endl;
	}
}

template <typename T>
void PlanVisualizer<T>::DumpGraphFooter(std::ofstream& ofs) {
	ofs << "}";
}

template <typename T>
void PlanVisualizer<T>::GenerateDotFile(const std::string &dot_filename) {
	std::ofstream outfile {dot_filename};
	DumpGraphToDotFile(outfile);
}

template <typename T>
void PlanVisualizer<T>::ExecuteDotCommand(const std::string &dot_filename) {
	auto command = "dot -T svg \"" + dot_filename + "\" > \"" + dot_filename + ".svg\"";
	auto ret = system(command.c_str());
	D_ASSERT(ret == 0);
}

template <typename T>
void PlanVisualizer<T>::Visualize(T &root) {
	BuildGraphFromPlan(root);

	std::string dot_filename {"/tmp/duckdb_plan_" + plan_name + ".dot"};
	GenerateDotFile(dot_filename);
	ExecuteDotCommand(dot_filename);
}

template class PlanVisualizer<LogicalOperator>;
template class PlanVisualizer<PhysicalOperator>;

void PipelineVisualizer::AddPlan(PhysicalOperator *root) {
	if (root->type == PhysicalOperatorType::RESULT_COLLECTOR) {
		PhysicalResultCollector *result_collector = (PhysicalResultCollector *) root;
		base_graph.AddNode(*root);
		BuildGraphFromPlan(*result_collector->plan);
	} else {
		BuildGraphFromPlan(*root);
	}
}

void PipelineVisualizer::AddPipeline(const std::vector<shared_ptr<Pipeline>> &plan_pipelines) {
	for (auto &pipeline : plan_pipelines) {
		pipelines.push_back(pipeline->GetOperators());
	}
}

void PipelineVisualizer::Visualize() {
	std::string dot_filename {"/tmp/duckdb_plan_" + plan_name + ".dot"};
	GenerateDotFile(dot_filename);
	ExecuteDotCommand(dot_filename);
}

void PipelineVisualizer::DumpGraphFooter(std::ofstream& ofs) {
    static std::vector<std::string> COLORS(
        {"blue", "brown", "chartreuse2", "darkgoldenrod2", "darkgreen", "darkmagenta", "bisque4", "aquamarine", "darkslategray", "deeppink3"});

	idx_t pipeline_idx = 0;
	for (auto &pipeline : pipelines) {
		idx_t index = 0;
		if (pipeline.size() == 1) {
			PhysicalOperator *op = pipeline[0];
			ofs << reinterpret_cast<idx_t>(op) << " -> " << reinterpret_cast<idx_t>(op);
		}
		else {
			for (auto op : pipeline) {
				if (index > 0) {
					ofs << " -> ";
				}
				ofs << reinterpret_cast<idx_t>(op);
				index++;
			}
		}
		ofs << " [label=\"pipeline_" + to_string(pipeline_idx) + "\" weight=20 penwidth=5 color=\"" + COLORS[pipeline_idx % COLORS.size()] + "\"]" << std::endl;
		pipeline_idx++;
	}
	ofs << "}";
}

} // namespace duckdb