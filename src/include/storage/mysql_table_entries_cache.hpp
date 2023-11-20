#pragma once

#include "duckdb.hpp"
#ifndef DUCKDB_AMALGAMATION
#include "duckdb/storage/object_cache.hpp"
#endif

namespace duckdb {

//! MysqlTableEntriesCache
class MysqlTableEntriesCache : public ObjectCacheEntry {
public:
	MysqlTableEntriesCache() : entries(nullptr) {
	}
	MysqlTableEntriesCache(unique_ptr<vector<string>> mysql_table_entries, time_t r_time)
	    : entries(std::move(mysql_table_entries)), read_time(r_time) {
	}

	~MysqlTableEntriesCache() override = default;

	//! Mysql table entries
	unique_ptr<vector<string>> entries;

	//! read time
	time_t read_time;

public:
	static string ObjectType() {
		return "mysql_table_entries";
	}

	string GetObjectType() override {
		return ObjectType();
	}
};
} // namespace duckdb