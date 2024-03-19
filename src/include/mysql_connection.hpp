//===----------------------------------------------------------------------===//
//                         DuckDB
//
// mysql_connection.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "mysql_utils.hpp"
#include "mysql_result.hpp"

namespace duckdb {
class MySQLBinaryWriter;
class MySQLTextWriter;
struct MySQLBinaryReader;
class MySQLSchemaEntry;
class MySQLTableEntry;
class MySQLStatement;
class MySQLResult;
struct IndexInfo;

struct OwnedMySQLConnection {
	explicit OwnedMySQLConnection(MYSQL *conn = nullptr) : connection(conn) {
	}
	~OwnedMySQLConnection() {
		if (!connection) {
			return;
		}
		mysql_close(connection);
		connection = nullptr;
	}

	MYSQL *connection;
};

class MySQLConnection {
public:
	explicit MySQLConnection(shared_ptr<OwnedMySQLConnection> connection = nullptr);
	~MySQLConnection();
	// disable copy constructors
	MySQLConnection(const MySQLConnection &other) = delete;
	MySQLConnection &operator=(const MySQLConnection &) = delete;
	//! enable move constructors
	MySQLConnection(MySQLConnection &&other) noexcept;
	MySQLConnection &operator=(MySQLConnection &&) noexcept;

public:
	static MySQLConnection Open(const string &connection_string);
	void Execute(const string &query);
	unique_ptr<MySQLResult> Query(const string &query, optional_ptr<ClientContext> context = nullptr);

	vector<IndexInfo> GetIndexInfo(const string &table_name);

	bool IsOpen();
	void Close();

	shared_ptr<OwnedMySQLConnection> GetConnection() {
		return connection;
	}
	string GetDSN() {
		return dsn;
	}

	MYSQL *GetConn() {
		if (!connection || !connection->connection) {
			throw InternalException("MySQLConnection::GetConn - no connection available");
		}
		return connection->connection;
	}

	static void DebugSetPrintQueries(bool print);
	static bool DebugPrintQueries();

private:
	MYSQL_RES *MySQLExecute(const string &query);

	shared_ptr<OwnedMySQLConnection> connection;
	string dsn;
};

} // namespace duckdb
