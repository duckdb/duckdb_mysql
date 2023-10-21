#include "storage/mysql_catalog.hpp"
#include "storage/mysql_table_entry.hpp"
#include "storage/mysql_transaction.hpp"
#include "duckdb/storage/statistics/base_statistics.hpp"
#include "duckdb/storage/table_storage_info.hpp"
#include "mysql_scanner.hpp"

namespace duckdb {

MySQLTableEntry::MySQLTableEntry(Catalog &catalog, SchemaCatalogEntry &schema, CreateTableInfo &info)
    : TableCatalogEntry(catalog, schema, info) {
}

MySQLTableEntry::MySQLTableEntry(Catalog &catalog, SchemaCatalogEntry &schema, MySQLTableInfo &info)
    : TableCatalogEntry(catalog, schema, *info.create_info) {
}

unique_ptr<BaseStatistics> MySQLTableEntry::GetStatistics(ClientContext &context, column_t column_id) {
	return nullptr;
}

void MySQLTableEntry::BindUpdateConstraints(LogicalGet &, LogicalProjection &, LogicalUpdate &, ClientContext &) {
}

TableFunction MySQLTableEntry::GetScanFunction(ClientContext &context, unique_ptr<FunctionData> &bind_data) {
	auto result = make_uniq<MySQLBindData>(*this);
	for (auto &col : columns.Logical()) {
		result->types.push_back(col.GetType());
		result->names.push_back(col.GetName());
	}

	bind_data = std::move(result);

	auto function = MySQLScanFunction();
	Value filter_pushdown;
	if (context.TryGetCurrentSetting("mysql_experimental_filter_pushdown", filter_pushdown)) {
		function.filter_pushdown = BooleanValue::Get(filter_pushdown);
	}
	return function;
}

TableStorageInfo MySQLTableEntry::GetStorageInfo(ClientContext &context) {
	auto &transaction = Transaction::Get(context, catalog).Cast<MySQLTransaction>();
	auto &db = transaction.GetConnection();
	TableStorageInfo result;
	result.cardinality = 0;
	result.index_info = db.GetIndexInfo(name);
	return result;
}

} // namespace duckdb
