//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/mysql_table_set.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "storage/mysql_catalog_set.hpp"
#include "storage/mysql_table_entry.hpp"

namespace duckdb {
struct CreateTableInfo;
class MySQLConnection;
class MySQLResult;
class MySQLSchemaEntry;

class MySQLTableSet : public MySQLCatalogSet {
public:
	explicit MySQLTableSet(MySQLSchemaEntry &schema);

public:
	optional_ptr<CatalogEntry> CreateTable(ClientContext &context, BoundCreateTableInfo &info);

	static unique_ptr<MySQLTableInfo> GetTableInfo(MySQLTransaction &transaction, MySQLSchemaEntry &schema,
	                                                  const string &table_name);
	static unique_ptr<MySQLTableInfo> GetTableInfo(MySQLConnection &connection, const string &schema_name,
	                                                  const string &table_name);
	optional_ptr<CatalogEntry> RefreshTable(ClientContext &context, const string &table_name);

	void AlterTable(ClientContext &context, AlterTableInfo &info);

protected:
	void LoadEntries(ClientContext &context) override;

	void AlterTable(ClientContext &context, RenameTableInfo &info);
	void AlterTable(ClientContext &context, RenameColumnInfo &info);
	void AlterTable(ClientContext &context, AddColumnInfo &info);
	void AlterTable(ClientContext &context, RemoveColumnInfo &info);

	static void AddColumn(MySQLResult &result, MySQLTableInfo &table_info, idx_t column_offset = 0);

protected:
	MySQLSchemaEntry &schema;
};

} // namespace duckdb
