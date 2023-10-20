#include "mysql_utils.hpp"
#include "storage/mysql_schema_entry.hpp"
#include "storage/mysql_transaction.hpp"

namespace duckdb {

MySQLConnectionParameters MySQLUtils::ParseConnectionParameters(const string &dsn) {
	MySQLConnectionParameters result;
	// parse options
	auto parameters = StringUtil::Split(dsn, ' ');
	for (auto &param : parameters) {
		StringUtil::Trim(param);
		if (param.empty()) {
			continue;
		}
		auto splits = StringUtil::Split(param, '=');
		if (splits.size() != 2) {
			throw InvalidInputException("Invalid dsn \"%s\" - expected key=value pairs separated by spaces", dsn);
		}
		auto key = StringUtil::Lower(splits[0]);
		auto &value = splits[1];
		if (key == "host") {
			result.host = value;
		} else if (key == "user") {
			result.user = value;
		} else if (key == "passwd" || key == "password") {
			result.passwd = value;
		} else if (key == "db" || key == "database") {
			result.db = value;
		} else if (key == "port") {
			constexpr const static int PORT_MIN = 0;
			constexpr const static int PORT_MAX = 65353;
			int port_val = std::stoi(value);
			if (port_val < PORT_MIN || port_val > PORT_MAX) {
				throw InvalidInputException("Invalid port %d - port must be between %d and %d", port_val, PORT_MIN,
				                            PORT_MAX);
			}
			result.port = uint32_t(port_val);
		} else if (key == "socket" || key == "unix_socket") {
			result.unix_socket = value;
		} else {
			throw InvalidInputException("Unrecognized configuration parameter \"%s\" - expected options are host, "
			                            "user, passwd, db, port, socket",
			                            key);
		}
	}
	return result;
	for (idx_t pos = 0; pos < dsn.size(); pos++)
		while (pos < dsn.size()) {
			if (StringUtil::CharacterIsSpace(dsn[pos])) {
				continue;
			}
		}
}

MYSQL *MySQLUtils::Connect(const string &dsn) {
	MYSQL *mysql = mysql_init(NULL);
	if (!mysql) {
		throw IOException("Failure in mysql_init");
	}
	MYSQL *result;
	auto config = ParseConnectionParameters(dsn);
	const char *host = config.host.size() == 0 ? nullptr : config.host.c_str();
	const char *user = config.user.size() == 0 ? nullptr : config.user.c_str();
	const char *passwd = config.passwd.size() == 0 ? nullptr : config.passwd.c_str();
	const char *db = config.db.size() == 0 ? nullptr : config.db.c_str();
	const char *unix_socket = config.unix_socket.size() == 0 ? nullptr : config.unix_socket.c_str();
	result = mysql_real_connect(mysql, host, user, passwd, db, config.port, unix_socket, config.client_flag);
	if (!result) {
		throw IOException("Failed to connect to MySQL database with parameters \"%s\": %s", dsn, mysql_error(mysql));
	}
	D_ASSERT(mysql == result);
	return result;
}

string MySQLUtils::TypeToString(const LogicalType &input) {
	return input.ToString();
}

LogicalType MySQLUtils::TypeToLogicalType(const MySQLTypeData &type_info) {
	if (type_info.type_name == "tinyint") {
		return LogicalType::TINYINT;
	} else if (type_info.type_name == "smallint") {
		return LogicalType::SMALLINT;
	} else if (type_info.type_name == "int") {
		return LogicalType::INTEGER;
	} else if (type_info.type_name == "bigint") {
		return LogicalType::BIGINT;
	} else if (type_info.type_name == "float") {
		return LogicalType::FLOAT;
	} else if (type_info.type_name == "double") {
		return LogicalType::DOUBLE;
	} else if (type_info.type_name == "time") {
		return LogicalType::TIME;
	} else if (type_info.type_name == "timestamp" || type_info.type_name == "datetime") {
		return LogicalType::TIMESTAMP;
	} else if (type_info.type_name == "decimal") {
		// FIXME
		return LogicalType::DOUBLE;
	} else if (type_info.type_name == "json") {
		// FIXME
		return LogicalType::VARCHAR;
	} else if (type_info.type_name == "varchar" || type_info.type_name == "mediumtext" ||
	           type_info.type_name == "longtext" || type_info.type_name == "text" || type_info.type_name == "enum" ||
	           type_info.type_name == "char") {
		return LogicalType::VARCHAR;
	}
	// fallback for unknown types
	return LogicalType::VARCHAR;
}

LogicalType MySQLUtils::ToMySQLType(const LogicalType &input) {
	return input;
}

} // namespace duckdb
