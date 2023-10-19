#include "storage/mysql_update.hpp"
#include "storage/mysql_table_entry.hpp"
#include "duckdb/planner/operator/logical_update.hpp"
#include "storage/mysql_catalog.hpp"
#include "storage/mysql_transaction.hpp"
#include "mysql_connection.hpp"
#include "duckdb/common/types/uuid.hpp"

namespace duckdb {

MySQLUpdate::MySQLUpdate(LogicalOperator &op, TableCatalogEntry &table, vector<PhysicalIndex> columns_p)
    : PhysicalOperator(PhysicalOperatorType::EXTENSION, op.types, 1), table(table), columns(std::move(columns_p)) {
}

//===--------------------------------------------------------------------===//
// States
//===--------------------------------------------------------------------===//
class MySQLUpdateGlobalState : public GlobalSinkState {
public:
	explicit MySQLUpdateGlobalState(MySQLTableEntry &table) : table(table), update_count(0) {
	}

	MySQLTableEntry &table;
	DataChunk insert_chunk;
	DataChunk varchar_chunk;
	string update_sql;
	idx_t update_count;
};

string CreateUpdateTable(const string &name, MySQLTableEntry &table, const vector<PhysicalIndex> &index) {
	string result;
	result = "CREATE LOCAL TEMPORARY TABLE " + KeywordHelper::WriteOptionallyQuoted(name);
	result += "(";
	for (idx_t i = 0; i < index.size(); i++) {
		if (i > 0) {
			result += ", ";
		}
		auto &col = table.GetColumn(LogicalIndex(index[i].index));
		result += KeywordHelper::WriteQuoted(col.GetName(), '`');
		result += " ";
		result += MySQLUtils::TypeToString(col.GetType());
	}
	result += ", __page_id_string VARCHAR) ON COMMIT DROP;";
	return result;
}

string GetUpdateSQL(const string &name, MySQLTableEntry &table, const vector<PhysicalIndex> &index) {
	string result;
	result = "UPDATE " + KeywordHelper::WriteQuoted(table.name, '`');
	result += " SET ";
	for (idx_t i = 0; i < index.size(); i++) {
		if (i > 0) {
			result += ", ";
		}
		auto &col = table.GetColumn(LogicalIndex(index[i].index));
		result += KeywordHelper::WriteQuoted(col.GetName(), '`');
		result += " = ";
		result += KeywordHelper::WriteQuoted(name, '`');
		result += ".";
		result += KeywordHelper::WriteQuoted(col.GetName(), '`');
	}
	result += " FROM " + KeywordHelper::WriteOptionallyQuoted(name);
	result += " WHERE ";
	result += KeywordHelper::WriteQuoted(table.name, '`');
	result += ".ctid=__page_id_string::TID";
	return result;
}


unique_ptr<GlobalSinkState> MySQLUpdate::GetGlobalSinkState(ClientContext &context) const {
	throw InternalException("FIXME MySQLUpdate::GetGlobalSinkState");
}

//===--------------------------------------------------------------------===//
// Sink
//===--------------------------------------------------------------------===//
SinkResultType MySQLUpdate::Sink(ExecutionContext &context, DataChunk &chunk, OperatorSinkInput &input) const {
	throw InternalException("FIXME MySQLUpdate::Sink");
}

//===--------------------------------------------------------------------===//
// Finalize
//===--------------------------------------------------------------------===//
SinkFinalizeType MySQLUpdate::Finalize(Pipeline &pipeline, Event &event, ClientContext &context,
						  OperatorSinkFinalizeInput &input) const {
	throw InternalException("FIXME MySQLUpdate::Finalize");
}

//===--------------------------------------------------------------------===//
// GetData
//===--------------------------------------------------------------------===//
SourceResultType MySQLUpdate::GetData(ExecutionContext &context, DataChunk &chunk, OperatorSourceInput &input) const {
	auto &insert_gstate = sink_state->Cast<MySQLUpdateGlobalState>();
	chunk.SetCardinality(1);
	chunk.SetValue(0, 0, Value::BIGINT(insert_gstate.update_count));

	return SourceResultType::FINISHED;
}

//===--------------------------------------------------------------------===//
// Helpers
//===--------------------------------------------------------------------===//
string MySQLUpdate::GetName() const {
	return "UPDATE";
}

string MySQLUpdate::ParamsToString() const {
	return table.name;
}

//===--------------------------------------------------------------------===//
// Plan
//===--------------------------------------------------------------------===//
unique_ptr<PhysicalOperator> MySQLCatalog::PlanUpdate(ClientContext &context, LogicalUpdate &op,
                                                       unique_ptr<PhysicalOperator> plan) {
	if (op.return_chunk) {
		throw BinderException("RETURNING clause not yet supported for updates of a MySQL table");
	}
	for (auto &expr : op.expressions) {
		if (expr->type == ExpressionType::VALUE_DEFAULT) {
			throw BinderException("SET DEFAULT is not yet supported for updates of a MySQL table");
		}
	}
	MySQLCatalog::MaterializeMySQLScans(*plan);
	auto insert = make_uniq<MySQLUpdate>(op, op.table, std::move(op.columns));
	insert->children.push_back(std::move(plan));
	return std::move(insert);
}

} // namespace duckdb
