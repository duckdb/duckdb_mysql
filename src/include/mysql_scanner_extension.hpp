#ifndef DUCKDB_BUILD_LOADABLE_EXTENSION
#define DUCKDB_BUILD_LOADABLE_EXTENSION
#endif
#include "duckdb.hpp"

#include "duckdb/catalog/catalog.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"

using namespace duckdb;

class MysqlScannerExtension : public Extension {
public:
	std::string Name() override {
		return "mysql_scanner";
	}
	void Load(DuckDB &db) override;
};

extern "C" {
DUCKDB_EXTENSION_API void mysql_scanner_init(duckdb::DatabaseInstance &db);
DUCKDB_EXTENSION_API const char *mysql_scanner_version();
DUCKDB_EXTENSION_API void mysql_scanner_storage_init(DBConfig &config);
}
