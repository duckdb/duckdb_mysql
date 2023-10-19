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
    : TableCatalogEntry(catalog, schema, *info.create_info)  {
}

unique_ptr<BaseStatistics> MySQLTableEntry::GetStatistics(ClientContext &context, column_t column_id) {
	return nullptr;
}

void MySQLTableEntry::BindUpdateConstraints(LogicalGet &, LogicalProjection &, LogicalUpdate &, ClientContext &) {
}

TableFunction MySQLTableEntry::GetScanFunction(ClientContext &context, unique_ptr<FunctionData> &bind_data) {
	auto &transaction = Transaction::Get(context, catalog).Cast<MySQLTransaction>();
	auto &conn = transaction.GetConnection();

	throw InternalException("MySQLTableEntry::GetScanFunction");
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
