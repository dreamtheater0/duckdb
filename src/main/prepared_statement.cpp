#include "duckdb/main/prepared_statement.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/main/client_context.hpp"
#include "duckdb/main/prepared_statement_data.hpp"

namespace duckdb {

PreparedStatement::PreparedStatement(shared_ptr<ClientContext> context, shared_ptr<PreparedStatementData> data_p,
                                     string query, idx_t n_param, case_insensitive_map_t<idx_t> named_param_map_p)
    : context(std::move(context)), data(std::move(data_p)), query(std::move(query)), success(true), n_param(n_param),
      named_param_map(std::move(named_param_map_p)) {
	D_ASSERT(data || !success);
}

PreparedStatement::PreparedStatement(PreservedError error) : context(nullptr), success(false), error(std::move(error)) {
}

PreparedStatement::~PreparedStatement() {
}

const string &PreparedStatement::GetError() {
	D_ASSERT(HasError());
	return error.Message();
}

PreservedError &PreparedStatement::GetErrorObject() {
	return error;
}

bool PreparedStatement::HasError() const {
	return !success;
}

idx_t PreparedStatement::ColumnCount() {
	D_ASSERT(data);
	return data->types.size();
}

StatementType PreparedStatement::GetStatementType() {
	D_ASSERT(data);
	return data->statement_type;
}

StatementProperties PreparedStatement::GetStatementProperties() {
	D_ASSERT(data);
	return data->properties;
}

const vector<LogicalType> &PreparedStatement::GetTypes() {
	D_ASSERT(data);
	return data->types;
}

const vector<string> &PreparedStatement::GetNames() {
	D_ASSERT(data);
	return data->names;
}

vector<LogicalType> PreparedStatement::GetExpectedParameterTypes() const {
	D_ASSERT(data);
	vector<LogicalType> expected_types(data->value_map.size());
	for (auto &it : data->value_map) {
		D_ASSERT(it.first >= 1);
		idx_t param_index = it.first - 1;
		D_ASSERT(param_index < expected_types.size());
		D_ASSERT(it.second);
		expected_types[param_index] = it.second->value.type();
	}
	return expected_types;
}

unique_ptr<QueryResult> PreparedStatement::Execute(vector<Value> &unnamed_values,
                                                   case_insensitive_map_t<Value> &named_values,
                                                   bool allow_stream_result) {
	auto pending = PendingQuery(unnamed_values, named_values, allow_stream_result);
	if (pending->HasError()) {
		return make_uniq<MaterializedQueryResult>(pending->GetErrorObject());
	}
	return pending->Execute();
}

unique_ptr<QueryResult> PreparedStatement::Execute(vector<Value> &values, bool allow_stream_result) {
	auto pending = PendingQuery(values, allow_stream_result);
	if (pending->HasError()) {
		return make_uniq<MaterializedQueryResult>(pending->GetErrorObject());
	}
	return pending->Execute();
}

unique_ptr<PendingQueryResult> PreparedStatement::PendingQuery(vector<Value> &values, bool allow_stream_result) {
	case_insensitive_map_t<Value> no_named_values;
	return PendingQuery(values, no_named_values, allow_stream_result);
}

unique_ptr<PendingQueryResult> PreparedStatement::PendingQuery(vector<Value> &unnamed_values,
                                                               case_insensitive_map_t<Value> &named_values,
                                                               bool allow_stream_result) {
	if (!success) {
		throw InvalidInputException("Attempting to execute an unsuccessfully prepared statement!");
	}
	D_ASSERT(data);
	PendingQueryParameters parameters;
	reference<vector<Value>> prepared_parameters(unnamed_values);
	vector<Value> mapped_named_values;
	if (named_param_map.size() != named_values.size()) {
		// Lookup the parameter index from the vector index
		for (idx_t i = 0; i < unnamed_values.size(); i++) {
			named_values[StringUtil::Format("%d", i + 1)] = std::move(unnamed_values[i]);
		}
	}
	if (!named_values.empty()) {
		mapped_named_values = PrepareParameters(named_values, named_param_map);
		prepared_parameters = mapped_named_values;
	}

	parameters.parameters = &prepared_parameters.get();
	parameters.allow_stream_result = allow_stream_result && data->properties.allow_stream_result;
	auto result = context->PendingQuery(query, data, parameters);
	// The result should not contain any reference to the 'vector<Value> parameters.parameters'
	return result;
}

} // namespace duckdb
