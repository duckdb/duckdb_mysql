#include "duckdb.hpp"

#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "mysql_scanner.hpp"
#include "duckdb/main/database_manager.hpp"
#include "duckdb/main/attached_database.hpp"
#include "storage/mysql_catalog.hpp"

namespace duckdb {

struct CacheSchemaFunctionData : public TableFunctionData {
	bool finished = false;
};

static unique_ptr<FunctionData> CacheSchemaBind(ClientContext &context, TableFunctionBindInput &input,
                                               vector<LogicalType> &return_types, vector<string> &names) {

	auto result = make_uniq<CacheSchemaFunctionData>();
	return_types.push_back(LogicalType::BOOLEAN);
	names.emplace_back("Success");
	return std::move(result);
}

static void CacheSchemaFunction(ClientContext &context, TableFunctionInput &data_p, DataChunk &output) {
	auto &data = data_p.bind_data->CastNoConst<CacheSchemaFunctionData>();
	if (data.finished) {
		return;
	}
	auto databases = DatabaseManager::Get(context).GetDatabases(context);
	for (auto &db_ref : databases) {
		auto &db = db_ref.get();
		auto &catalog = db.GetCatalog();
		if (catalog.GetCatalogType() != "mysql") {
			continue;
		}
		catalog.Cast<MySQLCatalog>().CacheSchema(context);
	}
	data.finished = true;
}

MySQLCacheSchemaFunction::MySQLCacheSchemaFunction()
    : TableFunction("mysql_cache_schema", {}, CacheSchemaFunction, CacheSchemaBind) {
}
} // namespace duckdb
