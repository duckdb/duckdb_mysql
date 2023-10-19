#include "storage/mysql_insert.hpp"
#include "storage/mysql_catalog.hpp"
#include "storage/mysql_transaction.hpp"
#include "duckdb/planner/operator/logical_insert.hpp"
#include "duckdb/planner/operator/logical_create_table.hpp"
#include "storage/mysql_table_entry.hpp"
#include "duckdb/planner/parsed_data/bound_create_table_info.hpp"
#include "duckdb/execution/operator/projection/physical_projection.hpp"
#include "duckdb/execution/operator/scan/physical_table_scan.hpp"
#include "duckdb/planner/expression/bound_cast_expression.hpp"
#include "duckdb/planner/expression/bound_reference_expression.hpp"
#include "mysql_connection.hpp"
#include "mysql_scanner.hpp"

namespace duckdb {

MySQLInsert::MySQLInsert(LogicalOperator &op, TableCatalogEntry &table,
                           physical_index_vector_t<idx_t> column_index_map_p)
    : PhysicalOperator(PhysicalOperatorType::EXTENSION, op.types, 1), table(&table), schema(nullptr),
      column_index_map(std::move(column_index_map_p)) {
}

MySQLInsert::MySQLInsert(LogicalOperator &op, SchemaCatalogEntry &schema, unique_ptr<BoundCreateTableInfo> info)
    : PhysicalOperator(PhysicalOperatorType::EXTENSION, op.types, 1), table(nullptr), schema(&schema),
      info(std::move(info)) {
}

//===--------------------------------------------------------------------===//
// States
//===--------------------------------------------------------------------===//
class MySQLInsertGlobalState : public GlobalSinkState {
public:
	explicit MySQLInsertGlobalState(ClientContext &context, MySQLTableEntry *table) : table(table), insert_count(0) {
	}

	MySQLTableEntry *table;
	DataChunk varchar_chunk;
	idx_t insert_count;
};

unique_ptr<GlobalSinkState> MySQLInsert::GetGlobalSinkState(ClientContext &context) const {
	throw InternalException("MySQLInsert::GetGlobalSinkState");
}

//===--------------------------------------------------------------------===//
// Sink
//===--------------------------------------------------------------------===//
SinkResultType MySQLInsert::Sink(ExecutionContext &context, DataChunk &chunk, OperatorSinkInput &input) const {
	throw InternalException("MySQLInsert::Sink");
}

//===--------------------------------------------------------------------===//
// Finalize
//===--------------------------------------------------------------------===//
SinkFinalizeType MySQLInsert::Finalize(Pipeline &pipeline, Event &event, ClientContext &context,
						  OperatorSinkFinalizeInput &input) const {
	throw InternalException("MySQLInsert::Finalize");
}

//===--------------------------------------------------------------------===//
// GetData
//===--------------------------------------------------------------------===//
SourceResultType MySQLInsert::GetData(ExecutionContext &context, DataChunk &chunk, OperatorSourceInput &input) const {
	auto &insert_gstate = sink_state->Cast<MySQLInsertGlobalState>();
	chunk.SetCardinality(1);
	chunk.SetValue(0, 0, Value::BIGINT(insert_gstate.insert_count));

	return SourceResultType::FINISHED;
}

//===--------------------------------------------------------------------===//
// Helpers
//===--------------------------------------------------------------------===//
string MySQLInsert::GetName() const {
	return table ? "INSERT" : "CREATE_TABLE_AS";
}

string MySQLInsert::ParamsToString() const {
	return table ? table->name : info->Base().table;
}

//===--------------------------------------------------------------------===//
// Plan
//===--------------------------------------------------------------------===//
unique_ptr<PhysicalOperator> AddCastToMySQLTypes(ClientContext &context, unique_ptr<PhysicalOperator> plan) {
	// check if we need to cast anything
	bool require_cast = false;
	auto &child_types = plan->GetTypes();
	for (auto &type : child_types) {
		auto mysql_type = MySQLUtils::ToMySQLType(type);
		if (mysql_type != type) {
			require_cast = true;
			break;
		}
	}
	if (require_cast) {
		vector<LogicalType> mysql_types;
		vector<unique_ptr<Expression>> select_list;
		for (idx_t i = 0; i < child_types.size(); i++) {
			auto &type = child_types[i];
			unique_ptr<Expression> expr;
			expr = make_uniq<BoundReferenceExpression>(type, i);

			auto mysql_type = MySQLUtils::ToMySQLType(type);
			if (mysql_type != type) {
				// add a cast
				expr = BoundCastExpression::AddCastToType(context, std::move(expr), mysql_type);
			}
			mysql_types.push_back(std::move(mysql_type));
			select_list.push_back(std::move(expr));
		}
		// we need to cast: add casts
		auto proj =
		    make_uniq<PhysicalProjection>(std::move(mysql_types), std::move(select_list), plan->estimated_cardinality);
		proj->children.push_back(std::move(plan));
		plan = std::move(proj);
	}

	return plan;
}

void MySQLCatalog::MaterializeMySQLScans(PhysicalOperator &op) {
	if (op.type == PhysicalOperatorType::TABLE_SCAN) {
		auto &table_scan = op.Cast<PhysicalTableScan>();
		if (table_scan.function.name == "mysql_scan" || table_scan.function.name == "mysql_scan_pushdown") {
			auto &bind_data = table_scan.bind_data->Cast<MySQLBindData>();
			bind_data.requires_materialization = true;
			bind_data.max_threads = 1;
		}
	}
	for(auto &child : op.children) {
		MaterializeMySQLScans(*child);
	}
}

unique_ptr<PhysicalOperator> MySQLCatalog::PlanInsert(ClientContext &context, LogicalInsert &op,
                                                       unique_ptr<PhysicalOperator> plan) {
	if (op.return_chunk) {
		throw BinderException("RETURNING clause not yet supported for insertion into MySQL table");
	}
	if (op.action_type != OnConflictAction::THROW) {
		throw BinderException("ON CONFLICT clause not yet supported for insertion into MySQL table");
	}
	MaterializeMySQLScans(*plan);

	plan = AddCastToMySQLTypes(context, std::move(plan));

	auto insert = make_uniq<MySQLInsert>(op, op.table, op.column_index_map);
	insert->children.push_back(std::move(plan));
	return std::move(insert);
}

unique_ptr<PhysicalOperator> MySQLCatalog::PlanCreateTableAs(ClientContext &context, LogicalCreateTable &op,
                                                              unique_ptr<PhysicalOperator> plan) {
	plan = AddCastToMySQLTypes(context, std::move(plan));

	auto insert = make_uniq<MySQLInsert>(op, op.schema, std::move(op.info));
	insert->children.push_back(std::move(plan));
	return std::move(insert);
}

} // namespace duckdb
