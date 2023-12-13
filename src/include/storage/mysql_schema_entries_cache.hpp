#pragma once

#include "duckdb.hpp"
#ifndef DUCKDB_AMALGAMATION
#include "duckdb/storage/object_cache.hpp"
#endif
//#include "mysql_schema_entry.hpp"

namespace duckdb {

//! MysqlSchemaEntriesCache
class MysqlSchemaEntriesCache : public ObjectCacheEntry {
public:
	MysqlSchemaEntriesCache() : entries(nullptr) {
	}
	MysqlSchemaEntriesCache(unique_ptr<vector<string>> mysql_schema_entries, time_t r_time)
	    : entries(std::move(mysql_schema_entries)), read_time(r_time) {
	}

	~MysqlSchemaEntriesCache() override = default;

	//! Mysql schema entries
	unique_ptr<vector<string>> entries;

	//! read time
	time_t read_time;

public:
	static string ObjectType() {
		return "mysql_schema_entries";
	}

	string GetObjectType() override {
		return ObjectType();
	}
};
} // namespace duckdb