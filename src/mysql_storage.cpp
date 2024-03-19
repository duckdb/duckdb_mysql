#include "duckdb.hpp"

#include "mysql_storage.hpp"
#include "storage/mysql_catalog.hpp"
#include "duckdb/parser/parsed_data/attach_info.hpp"
#include "storage/mysql_transaction_manager.hpp"

namespace duckdb {

static unique_ptr<Catalog> MySQLAttach(StorageExtensionInfo *storage_info, ClientContext &context, AttachedDatabase &db,
                                       const string &name, AttachInfo &info, AccessMode access_mode) {
	return make_uniq<MySQLCatalog>(db, info.path, access_mode);
}

static unique_ptr<TransactionManager> MySQLCreateTransactionManager(StorageExtensionInfo *storage_info,
                                                                    AttachedDatabase &db, Catalog &catalog) {
	auto &mysql_catalog = catalog.Cast<MySQLCatalog>();
	return make_uniq<MySQLTransactionManager>(db, mysql_catalog);
}

MySQLStorageExtension::MySQLStorageExtension() {
	attach = MySQLAttach;
	create_transaction_manager = MySQLCreateTransactionManager;
}

} // namespace duckdb
