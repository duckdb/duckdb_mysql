#include "duckdb.hpp"

#include "mysql_storage.hpp"
#include "storage/mysql_catalog.hpp"
#include "duckdb/parser/parsed_data/attach_info.hpp"
#include "storage/mysql_transaction_manager.hpp"
#include "duckdb/main/secret/secret_manager.hpp"

namespace duckdb {

string EscapeConnectionString(const string &input) {
	string result = "\"";
	for (auto c : input) {
		if (c == '\\') {
			result += "\\\\";
		} else if (c == '"') {
			result += "\\\"";
		} else {
			result += c;
		}
	}
	result += "\"";
	return result;
}

string AddConnectionOption(const KeyValueSecret &kv_secret, const string &name) {
	Value input_val = kv_secret.TryGetValue(name);
	if (input_val.IsNull()) {
		// not provided
		return string();
	}
	string result;
	result += name;
	result += "=";
	result += EscapeConnectionString(input_val.ToString());
	result += " ";
	return result;
}

unique_ptr<SecretEntry> GetSecret(ClientContext &context, const string &secret_name) {
	auto &secret_manager = SecretManager::Get(context);
	auto transaction = CatalogTransaction::GetSystemCatalogTransaction(context);
	// FIXME: this should be adjusted once the `GetSecretByName` API supports this
	// use case
	auto secret_entry = secret_manager.GetSecretByName(transaction, secret_name, "memory");
	if (secret_entry) {
		return secret_entry;
	}
	secret_entry = secret_manager.GetSecretByName(transaction, secret_name, "local_file");
	if (secret_entry) {
		return secret_entry;
	}
	return nullptr;
}

static unique_ptr<Catalog> MySQLAttach(StorageExtensionInfo *storage_info, ClientContext &context, AttachedDatabase &db,
                                       const string &name, AttachInfo &info, AccessMode access_mode) {
	// check if we have a secret provided
	string secret_name;
	for (auto &entry : info.options) {
		auto lower_name = StringUtil::Lower(entry.first);
		if (lower_name == "type" || lower_name == "read_only") {
			// already handled
		} else if (lower_name == "secret") {
			secret_name = entry.second.ToString();
		} else {
			throw BinderException("Unrecognized option for MySQL attach: %s", entry.first);
		}
	}

	// if no secret is specified we default to the unnamed mysql secret, if it
	// exists
	bool explicit_secret = !secret_name.empty();
	if (!explicit_secret) {
		// look up settings from the default unnamed mysql secret if none is
		// provided
		secret_name = "__default_mysql";
	}

	string connection_string = info.path;
	auto secret_entry = GetSecret(context, secret_name);
	if (secret_entry) {
		// secret found - read data
		const auto &kv_secret = dynamic_cast<const KeyValueSecret &>(*secret_entry->secret);
		string new_connection_info;

		new_connection_info += AddConnectionOption(kv_secret, "user");
		new_connection_info += AddConnectionOption(kv_secret, "password");
		new_connection_info += AddConnectionOption(kv_secret, "host");
		new_connection_info += AddConnectionOption(kv_secret, "port");
		new_connection_info += AddConnectionOption(kv_secret, "database");
		new_connection_info += AddConnectionOption(kv_secret, "socket");

		connection_string = new_connection_info + connection_string;
	} else if (explicit_secret) {
		// secret not found and one was explicitly provided - throw an error
		throw BinderException("Secret with name \"%s\" not found", secret_name);
	}

	return make_uniq<MySQLCatalog>(db, connection_string, access_mode);
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
