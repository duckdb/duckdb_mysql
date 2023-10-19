//===----------------------------------------------------------------------===//
//                         DuckDB
//
// mysql_scanner.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb.hpp"
#include "mysql_utils.hpp"
#include "mysql_connection.hpp"

namespace duckdb {
class MySQLTransaction;

struct MySQLBindData : public FunctionData {
	string schema_name;
	string table_name;
	idx_t pages_approx = 0;

	vector<MySQLType> mysql_types;
	vector<string> names;
	vector<LogicalType> types;

	string dsn;

	string snapshot;
	bool in_recovery;
	bool requires_materialization = false;
	bool read_only = true;
	idx_t max_threads = 1;

	MySQLConnection connection;
	optional_ptr<MySQLTransaction> transaction;

public:
	unique_ptr<FunctionData> Copy() const override {
		throw NotImplementedException("");
	}
	bool Equals(const FunctionData &other_p) const override {
		return false;
	}
};

class MySQLScanFunction : public TableFunction {
public:
	MySQLScanFunction();
};

class MySQLClearCacheFunction : public TableFunction {
public:
	MySQLClearCacheFunction();
};

} // namespace duckdb
