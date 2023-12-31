//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/mysql_schema_set.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "storage/mysql_catalog_set.hpp"
#include "storage/mysql_schema_entry.hpp"

namespace duckdb {
struct CreateSchemaInfo;

class MySQLSchemaSet : public MySQLCatalogSet {
public:
	explicit MySQLSchemaSet(Catalog &catalog);

public:
	optional_ptr<CatalogEntry> CreateSchema(ClientContext &context, CreateSchemaInfo &info);

protected:
	void LoadEntries(ClientContext &context) override;
};

} // namespace duckdb
