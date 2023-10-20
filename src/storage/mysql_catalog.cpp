#include "storage/mysql_catalog.hpp"
#include "storage/mysql_schema_entry.hpp"
#include "storage/mysql_transaction.hpp"
#include "mysql_connection.hpp"
#include "duckdb/storage/database_size.hpp"
#include "duckdb/parser/parsed_data/drop_info.hpp"
#include "duckdb/parser/parsed_data/create_schema_info.hpp"
#include "duckdb/main/attached_database.hpp"

namespace duckdb {

MySQLCatalog::MySQLCatalog(AttachedDatabase &db_p, const string &path, AccessMode access_mode)
    : Catalog(db_p), path(path), access_mode(access_mode), schemas(*this) {
	default_schema = MySQLUtils::ParseConnectionParameters(path).db;
}

MySQLCatalog::~MySQLCatalog() = default;

void MySQLCatalog::Initialize(bool load_builtin) {
}

optional_ptr<CatalogEntry> MySQLCatalog::CreateSchema(CatalogTransaction transaction, CreateSchemaInfo &info) {
	if (info.on_conflict == OnCreateConflict::REPLACE_ON_CONFLICT) {
		DropInfo try_drop;
		try_drop.type = CatalogType::SCHEMA_ENTRY;
		try_drop.name = info.schema;
		try_drop.if_not_found = OnEntryNotFound::RETURN_NULL;
		try_drop.cascade = false;
		schemas.DropEntry(transaction.GetContext(), try_drop);
	}
	return schemas.CreateSchema(transaction.GetContext(), info);
}

void MySQLCatalog::DropSchema(ClientContext &context, DropInfo &info) {
	return schemas.DropEntry(context, info);
}

void MySQLCatalog::ScanSchemas(ClientContext &context, std::function<void(SchemaCatalogEntry &)> callback) {
	schemas.Scan(context, [&](CatalogEntry &schema) { callback(schema.Cast<MySQLSchemaEntry>()); });
}

optional_ptr<SchemaCatalogEntry> MySQLCatalog::GetSchema(CatalogTransaction transaction, const string &schema_name,
                                                         OnEntryNotFound if_not_found,
                                                         QueryErrorContext error_context) {
	if (schema_name == DEFAULT_SCHEMA) {
		if (default_schema.empty()) {
			throw InvalidInputException(
			    "Attempting to fetch the default schema - but no database was provided in the connection string");
		}
		return GetSchema(transaction, default_schema, if_not_found, error_context);
	}
	auto &mysql_transaction = MySQLTransaction::Get(transaction.GetContext(), *this);
	auto entry = schemas.GetEntry(transaction.GetContext(), schema_name);
	if (!entry && if_not_found != OnEntryNotFound::RETURN_NULL) {
		throw BinderException("Schema with name \"%s\" not found", schema_name);
	}
	return reinterpret_cast<SchemaCatalogEntry *>(entry.get());
}

bool MySQLCatalog::InMemory() {
	return false;
}

string MySQLCatalog::GetDBPath() {
	return path;
}

DatabaseSize MySQLCatalog::GetDatabaseSize(ClientContext &context) {
	throw InternalException("FIXME: GetDatabaseSize");
}

void MySQLCatalog::ClearCache() {
	schemas.ClearEntries();
}

} // namespace duckdb
