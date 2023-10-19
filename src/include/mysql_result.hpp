//===----------------------------------------------------------------------===//
//                         DuckDB
//
// mysql_result.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "mysql_utils.hpp"

namespace duckdb {

class MySQLResult {
public:
	MySQLResult(MYSQL_RES *res_p, idx_t field_count) : res(res_p), field_count(field_count) {
	}
	MySQLResult(idx_t affected_rows) : affected_rows(affected_rows) {
	}
	~MySQLResult() {
		if (res) {
			mysql_free_result(res);
		}
	}

public:
	string GetString(idx_t row) {
		D_ASSERT(res);
		return string(GetNonNullValue(row));
	}
	int32_t GetInt32(idx_t row) {
		return atoi(GetNonNullValue(row));
	}
	int64_t GetInt64(idx_t row) {
		return atoll(GetNonNullValue(row));
	}
	bool GetBool(idx_t row) {
		return strcmp(GetNonNullValue(row), "t");
	}
	bool IsNull(idx_t row) {
		return !GetValueInternal(row);
	}
	bool Next() {
		if (!res) {
			throw InternalException("MySQLResult::Next called without result");
		}
		mysql_row = mysql_fetch_row(res);
		return mysql_row;
	}
	idx_t AffectedRows() {
		if (affected_rows == idx_t(-1)) {
			throw InternalException("MySQLResult::AffectedRows called for result that didn't affect any rows");
		}
		return affected_rows;
	}

private:
	MYSQL_RES *res = nullptr;
	idx_t affected_rows = idx_t(-1);
	MYSQL_ROW mysql_row = nullptr;
	idx_t field_count = 0;

	char *GetNonNullValue(idx_t row) {
		auto val = GetValueInternal(row);
		if (!val) {
			throw InternalException("MySQLResult::GetNonNullValue called for a NULL value");
		}
		return val;
	}

	char *GetValueInternal(idx_t row) {
		if (!mysql_row) {
			throw InternalException("MySQLResult::GetValueInternal called without row");
		}
		if (row >= field_count) {
			throw InternalException("MySQLResult::GetValueInternal row out of range of field count");
		}
		return mysql_row[row];
	}
};

} // namespace duckdb
