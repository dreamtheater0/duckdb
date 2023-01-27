#include "duckdb/common/field_writer.hpp"
#include "duckdb/planner/operator/logical_delim_join.hpp"

namespace duckdb {

LogicalDelimJoin::LogicalDelimJoin(JoinType type)
    : LogicalComparisonJoin(type, LogicalOperatorType::LOGICAL_DELIM_JOIN) {
}

void LogicalDelimJoin::Serialize(FieldWriter &writer) const {
	LogicalComparisonJoin::Serialize(writer);
	if (type == LogicalOperatorType::LOGICAL_COMPARISON_JOIN) {
		D_ASSERT(duplicate_eliminated_columns.empty());
		// if the delim join has no delim columns anymore it is turned into a regular comparison join
		return;
	}
	writer.WriteSerializableList(duplicate_eliminated_columns);
}

unique_ptr<LogicalOperator> LogicalDelimJoin::Deserialize(LogicalDeserializationState &state, FieldReader &reader) {
	auto result = make_unique<LogicalDelimJoin>(JoinType::INVALID);
	LogicalComparisonJoin::Deserialize(*result, state, reader);
	result->duplicate_eliminated_columns = reader.ReadRequiredSerializableList<Expression>(state.gstate);
	return std::move(result);
}

void LogicalDelimJoin::GetPlanProperties(vector<PlanProperty> &props) const {
	idx_t col_idx = 0;
	props.reserve(duplicate_eliminated_columns.size());
	for (auto &expr : duplicate_eliminated_columns) {
		props.emplace_back("DElimCol[" + to_string(col_idx) + "]", expr->ToString());
	}

	LogicalComparisonJoin::GetPlanProperties(props);
}

} // namespace duckdb
