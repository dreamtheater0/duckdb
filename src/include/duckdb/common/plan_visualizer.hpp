//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/common/plan_visualizer/plan_visualizer.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/common.hpp"
#include "duckdb/common/assert.hpp"
#include "duckdb/common/map.hpp"

#include <ostream>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <functional>
#include <unordered_set>

namespace duckdb {

//!  Key-value pair type for providing properties of a plan node
using PlanProperty = std::pair<string, string>;

//!  Base graph data structure for visualizing the plan
template <typename T>
class PlanVisualizerBaseGraph {
private:
	//! Internal edge struct
	struct Edge {
		idx_t from;
		idx_t to;
		std::string label;

		Edge(const T &from, const T &to, std::string &label)
		    : from {reinterpret_cast<idx_t>(&from)}, to {reinterpret_cast<idx_t>(&to)}, label(label) {
		}
	};

	//! Internal node struct
	struct Node {
		//! Internal node unique id. This is generated from the node pointer,
		//! which is a unique value easy to derive from a give node.
		idx_t id;
		std::string name;
		std::vector<PlanProperty> properties;

		Node(const T &op, const std::string &name, std::vector<PlanProperty> properties)
		    : id {reinterpret_cast<idx_t>(&op)}, name {name}, properties {std::move(properties)} {
		}
	};

	std::string graph_name;

	std::vector<Edge> edges;
	std::vector<Node> nodes;

	//! A set to track already visited nodes
	std::unordered_set<idx_t> registered_nodes;

public:
	explicit PlanVisualizerBaseGraph(const std::string &name) : graph_name {name} {
	}

	PlanVisualizerBaseGraph(const PlanVisualizerBaseGraph &other) = delete;
	PlanVisualizerBaseGraph &operator=(const PlanVisualizerBaseGraph &rhs) = delete;

	PlanVisualizerBaseGraph(PlanVisualizerBaseGraph &&other) = delete;
	PlanVisualizerBaseGraph &operator=(PlanVisualizerBaseGraph &&rhs) = delete;

	//! Add a plan node to the base graph. If the node is already vistied, this function just returns false.
	bool AddNode(const T &plan_node) {
		idx_t node_id = reinterpret_cast<idx_t>(&plan_node);

		// Check if this node has been visited before
		if (registered_nodes.find(node_id) != registered_nodes.end()) {
			return false;
		}

		std::vector<PlanProperty> properties;
		// Get the plan node properties
		plan_node.GetPlanProperties(properties);
		registered_nodes.insert(node_id);
		nodes.emplace_back(plan_node, plan_node.GetName(), properties);

		return true;
	}

	//! Add a plan edge between two nodes to the base graph
	void AddEdge(const T &from, const T &to, std::string &label) {
		edges.emplace_back(from, to, label);
	}

	const std::vector<Node> &GetNodes() {
		return nodes;
	}
	const std::vector<Edge> &GetEdges() {
		return edges;
	}
};

//!  This is used to provide information for a plan edge
template <typename T>
struct PlanChildOperatorInfo {
	//! Edge label
	std::string label;
	//! Pointer to the child node
	T *child_operator;
	//! Is this edge drawn as backward edge
	bool backward;

	PlanChildOperatorInfo(const std::string &label, T *child_operator, bool backward)
	    : label(label), child_operator(child_operator), backward(backward) {
	}
};

//!  Plan visualizer which draws a plan structure to a svg image file
template <typename T>
class PlanVisualizer {
	friend class PipelineVisualizer;

public:
	std::string plan_name;

	//! Base graph structure built from a given plan
	PlanVisualizerBaseGraph<T> base_graph;

	//! Build a base graph structure from a plan structure
	void BuildGraphFromPlan(T &root);
	//! Recursive helper function for BuildGraphFromPlan
	void BuildGraphRecursive(const T &op, const T &parent);
	//! Add child operators of a plan node to the base graph
	void ProcessChildOperator(const T &op);

	//! Generate the intermediate dot file from the base graph
	void GenerateDotFile(const std::string &dot_filename);
	//! Run the dot command to generate a svg image file
	void ExecuteDotCommand(const std::string &dot_filename);

	explicit PlanVisualizer(const std::string &plan_name) : plan_name {plan_name}, base_graph {plan_name} {
	}

	//! Visualize a plan
	void Visualize(T &root);

private:
	//! Dump the base graph structure to an intermediate dot file
	virtual void DumpGraphToDotFile(std::ofstream& ofs);
	virtual void DumpGraphHeader(std::ofstream& ofs);
	virtual void DumpGraphNodes(std::ofstream& ofs);
	virtual void DumpGraphEdges(std::ofstream& ofs);
	virtual void DumpGraphFooter(std::ofstream& ofs);
};

} // namespace duckdb
