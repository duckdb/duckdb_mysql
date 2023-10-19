//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/mysql_index_set.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "storage/mysql_catalog_set.hpp"
#include "storage/mysql_index_entry.hpp"

namespace duckdb {
class MySQLSchemaEntry;

class MySQLIndexSet : public MySQLCatalogSet {
public:
	MySQLIndexSet(MySQLSchemaEntry &schema);

protected:
	void LoadEntries(ClientContext &context) override;

protected:
	MySQLSchemaEntry &schema;
};

} // namespace duckdb
