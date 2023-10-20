#include "storage/mysql_index_set.hpp"
#include "storage/mysql_schema_entry.hpp"
#include "storage/mysql_transaction.hpp"
#include "duckdb/parser/parsed_data/create_schema_info.hpp"
#include "storage/mysql_index_entry.hpp"

namespace duckdb {

MySQLIndexSet::MySQLIndexSet(MySQLSchemaEntry &schema) : MySQLCatalogSet(schema.ParentCatalog()), schema(schema) {
}

void MySQLIndexSet::LoadEntries(ClientContext &context) {
	auto query = StringUtil::Replace(R"(
SELECT tablename, indexname
FROM pg_indexes
WHERE schemaname=${SCHEMA_NAME}
)",
	                                 "${SCHEMA_NAME}", KeywordHelper::WriteQuoted(schema.name));

	auto &transaction = MySQLTransaction::Get(context, catalog);
	auto result = transaction.Query(query);
	while (result->Next()) {
		auto table_name = result->GetString(0);
		auto index_name = result->GetString(1);
		CreateIndexInfo info;
		info.schema = schema.name;
		info.table = table_name;
		info.index_name = index_name;
		auto index_entry = make_uniq<MySQLIndexEntry>(catalog, schema, info, table_name);
		CreateEntry(std::move(index_entry));
	}
}

} // namespace duckdb
