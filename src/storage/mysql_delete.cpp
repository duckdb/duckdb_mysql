#include "storage/mysql_delete.hpp"
#include "storage/mysql_table_entry.hpp"
#include "duckdb/planner/operator/logical_delete.hpp"
#include "storage/mysql_catalog.hpp"
#include "storage/mysql_transaction.hpp"
#include "mysql_connection.hpp"
#include "duckdb/planner/expression/bound_reference_expression.hpp"

namespace duckdb {

MySQLDelete::MySQLDelete(LogicalOperator &op, TableCatalogEntry &table, idx_t row_id_index)
    : PhysicalOperator(PhysicalOperatorType::EXTENSION, op.types, 1), table(table), row_id_index(row_id_index) {
}

//===--------------------------------------------------------------------===//
// States
//===--------------------------------------------------------------------===//
string GetDeleteSQL(const string &table_name, const string &ctid_list) {
	string result;
	result = "DELETE FROM " + MySQLUtils::WriteIdentifier(table_name);
	result += " WHERE ctid IN (" + ctid_list + ")";
	return result;
}

class MySQLDeleteGlobalState : public GlobalSinkState {
public:
	explicit MySQLDeleteGlobalState(MySQLTableEntry &table) : table(table), delete_count(0) {
	}

	MySQLTableEntry &table;
	string ctid_list;
	idx_t delete_count;

	void Flush(ClientContext &context) {
		if (ctid_list.empty()) {
			return;
		}
		auto &transaction = MySQLTransaction::Get(context, table.catalog);
		transaction.Query(GetDeleteSQL(table.name, ctid_list));
		ctid_list = "";
	}
};

unique_ptr<GlobalSinkState> MySQLDelete::GetGlobalSinkState(ClientContext &context) const {
	auto &mysql_table = table.Cast<MySQLTableEntry>();

	auto &transaction = MySQLTransaction::Get(context, mysql_table.catalog);
	auto result = make_uniq<MySQLDeleteGlobalState>(mysql_table);
	return std::move(result);
}

//===--------------------------------------------------------------------===//
// Sink
//===--------------------------------------------------------------------===//
SinkResultType MySQLDelete::Sink(ExecutionContext &context, DataChunk &chunk, OperatorSinkInput &input) const {
	auto &gstate = input.global_state.Cast<MySQLDeleteGlobalState>();

	chunk.Flatten();
	auto &row_identifiers = chunk.data[row_id_index];
	auto row_data = FlatVector::GetData<row_t>(row_identifiers);
	for (idx_t i = 0; i < chunk.size(); i++) {
		if (!gstate.ctid_list.empty()) {
			gstate.ctid_list += ",";
		}
		// extract the ctid from the row id
		auto row_in_page = row_data[i] & 0xFFFF;
		auto page_index = row_data[i] >> 16;
		gstate.ctid_list += "'(";
		gstate.ctid_list += to_string(page_index);
		gstate.ctid_list += ",";
		gstate.ctid_list += to_string(row_in_page);
		gstate.ctid_list += ")'";
		if (gstate.ctid_list.size() > 3000) {
			// avoid making too long SQL statements
			gstate.Flush(context.client);
		}
	}
	gstate.delete_count += chunk.size();
	return SinkResultType::NEED_MORE_INPUT;
}

//===--------------------------------------------------------------------===//
// Finalize
//===--------------------------------------------------------------------===//
SinkFinalizeType MySQLDelete::Finalize(Pipeline &pipeline, Event &event, ClientContext &context,
                                       OperatorSinkFinalizeInput &input) const {
	auto &gstate = input.global_state.Cast<MySQLDeleteGlobalState>();
	gstate.Flush(context);
	return SinkFinalizeType::READY;
}

//===--------------------------------------------------------------------===//
// GetData
//===--------------------------------------------------------------------===//
SourceResultType MySQLDelete::GetData(ExecutionContext &context, DataChunk &chunk, OperatorSourceInput &input) const {
	auto &insert_gstate = sink_state->Cast<MySQLDeleteGlobalState>();
	chunk.SetCardinality(1);
	chunk.SetValue(0, 0, Value::BIGINT(insert_gstate.delete_count));

	return SourceResultType::FINISHED;
}

//===--------------------------------------------------------------------===//
// Helpers
//===--------------------------------------------------------------------===//
string MySQLDelete::GetName() const {
	return "DELETE";
}

string MySQLDelete::ParamsToString() const {
	return table.name;
}

//===--------------------------------------------------------------------===//
// Plan
//===--------------------------------------------------------------------===//
unique_ptr<PhysicalOperator> MySQLCatalog::PlanDelete(ClientContext &context, LogicalDelete &op,
                                                      unique_ptr<PhysicalOperator> plan) {
	if (op.return_chunk) {
		throw BinderException("RETURNING clause not yet supported for deletion of a MySQL table");
	}
	auto &bound_ref = op.expressions[0]->Cast<BoundReferenceExpression>();

	auto insert = make_uniq<MySQLDelete>(op, op.table, bound_ref.index);
	insert->children.push_back(std::move(plan));
	return std::move(insert);
}

} // namespace duckdb
