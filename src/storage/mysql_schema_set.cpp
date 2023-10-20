#include "storage/mysql_schema_set.hpp"
#include "storage/mysql_transaction.hpp"
#include "duckdb/parser/parsed_data/create_schema_info.hpp"

namespace duckdb {

MySQLSchemaSet::MySQLSchemaSet(Catalog &catalog) : MySQLCatalogSet(catalog) {
}

void MySQLSchemaSet::LoadEntries(ClientContext &context) {
	auto query = R"(
SELECT schema_name
FROM information_schema.schemata;
)";

	auto &transaction = MySQLTransaction::Get(context, catalog);
	auto result = transaction.Query(query);
	while (result->Next()) {
		auto schema_name = result->GetString(0);
		auto schema = make_uniq<MySQLSchemaEntry>(catalog, schema_name);
		CreateEntry(std::move(schema));
	}
}

optional_ptr<CatalogEntry> MySQLSchemaSet::CreateSchema(ClientContext &context, CreateSchemaInfo &info) {
	auto &transaction = MySQLTransaction::Get(context, catalog);

	string create_sql = "CREATE SCHEMA " + KeywordHelper::WriteQuoted(info.schema, '`');
	transaction.Query(create_sql);
	auto schema_entry = make_uniq<MySQLSchemaEntry>(catalog, info.schema);
	return CreateEntry(std::move(schema_entry));
}

} // namespace duckdb
