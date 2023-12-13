#include "storage/mysql_schema_set.hpp"
#include "storage/mysql_transaction.hpp"
#include "duckdb/parser/parsed_data/create_schema_info.hpp"
#include "duckdb/storage/object_cache.hpp"
#include "storage/mysql_schema_entries_cache.hpp"
#include <iostream>

namespace duckdb {

const std::string MYSQL_SCHEMA_ENTRIES_CACHE_KEY = "mysql_schema_entries_cache";

MySQLSchemaSet::MySQLSchemaSet(Catalog &catalog) : MySQLCatalogSet(catalog) {
}

void MySQLSchemaSet::LoadEntries(ClientContext &context) {
	auto db = context.db;
	auto& object_cache = db->GetObjectCache();
	auto schema_cache = object_cache.Get<MysqlSchemaEntriesCache>(MYSQL_SCHEMA_ENTRIES_CACHE_KEY);
	if(schema_cache != nullptr) {
		//std::cout << "load from cache" << std::endl;
		auto mysql_schema_entries = std::move(schema_cache->entries);
		for(auto& schema_name : *mysql_schema_entries) {
			auto schema = make_uniq<MySQLSchemaEntry>(catalog, schema_name);
			CreateEntry(std::move(schema));
		}
	} else {
		//std::cout << "load from mysql" << std::endl;
		auto schemas = LoadSchemasFromMysqlInformationSchema(context);
		for(auto& schema : schemas) {
			auto schema_entry = make_uniq<MySQLSchemaEntry>(catalog, schema);
			CreateEntry(std::move(schema_entry));
		}
		PutInCache(context, schemas);
	}
}

void MySQLSchemaSet::PutInCache(ClientContext &context, vector<string> &schemas) {
	auto db = context.db;
	auto& object_cache = db->GetObjectCache();
	auto mysql_schema_entries = make_shared<MysqlSchemaEntriesCache>(make_uniq<vector<string>>(schemas), time(nullptr));
	object_cache.Put(MYSQL_SCHEMA_ENTRIES_CACHE_KEY, mysql_schema_entries);
}

vector<string> MySQLSchemaSet::LoadSchemasFromMysqlInformationSchema(ClientContext &context) {
	auto query = R"(
SELECT schema_name
FROM information_schema.schemata;
)";
	
	auto &transaction = MySQLTransaction::Get(context, catalog);
	auto result = transaction.Query(query);
	
	auto schemas = vector<string>();
	while (result->Next()) {
		auto schema_name = result->GetString(0);
		schemas.push_back(schema_name);
	}
	return schemas;
}

optional_ptr<CatalogEntry> MySQLSchemaSet::CreateSchema(ClientContext &context, CreateSchemaInfo &info) {
	auto &transaction = MySQLTransaction::Get(context, catalog);

	string create_sql = "CREATE SCHEMA " + MySQLUtils::WriteIdentifier(info.schema);
	transaction.Query(create_sql);
	auto schema_entry = make_uniq<MySQLSchemaEntry>(catalog, info.schema);
	return CreateEntry(std::move(schema_entry));
}

} // namespace duckdb
