#include "duckdb.hpp"

#include "duckdb/main/extension_util.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "mysql_scanner.hpp"
#include "mysql_result.hpp"
#include "storage/mysql_transaction.hpp"
#include "storage/mysql_table_set.hpp"
#include "mysql_filter_pushdown.hpp"

namespace duckdb {

struct MySQLGlobalState;

struct MySQLLocalState : public LocalTableFunctionState {};

struct MySQLGlobalState : public GlobalTableFunctionState {
	explicit MySQLGlobalState(unique_ptr<MySQLResult> result_p) : result(std::move(result_p)) {
	}

	unique_ptr<MySQLResult> result;
	DataChunk varchar_chunk;

	idx_t MaxThreads() const override {
		return 1;
	}
};

static unique_ptr<FunctionData> MySQLBind(ClientContext &context, TableFunctionBindInput &input,
                                          vector<LogicalType> &return_types, vector<string> &names) {
	throw InternalException("MySQLBind");
}

static unique_ptr<GlobalTableFunctionState> MySQLInitGlobalState(ClientContext &context,
                                                                 TableFunctionInitInput &input) {
	auto &bind_data = input.bind_data->Cast<MySQLBindData>();
	// generate the SELECT statement
	string select;
	select += "SELECT ";
	for (idx_t c = 0; c < input.column_ids.size(); c++) {
		if (c > 0) {
			select += ", ";
		}
		if (input.column_ids[c] == COLUMN_IDENTIFIER_ROW_ID) {
			select += "NULL";
		} else {
			auto &col = bind_data.table.GetColumn(LogicalIndex(input.column_ids[c]));
			auto col_name = col.GetName();
			select += MySQLUtils::WriteIdentifier(col_name);
		}
	}
	select += " FROM ";
	select += MySQLUtils::WriteIdentifier(bind_data.table.schema.name);
	select += ".";
	select += MySQLUtils::WriteIdentifier(bind_data.table.name);
	string filter_string = MySQLFilterPushdown::TransformFilters(input.column_ids, input.filters, bind_data.names);
	if (!filter_string.empty()) {
		select += " WHERE " + filter_string;
	}
	if (!bind_data.limit.empty()) {
		select += bind_data.limit;
	}
	// run the query
	auto &transaction = MySQLTransaction::Get(context, bind_data.table.catalog);
	auto &con = transaction.GetConnection();
	auto query_result = con.Query(select);
	auto result = make_uniq<MySQLGlobalState>(std::move(query_result));

	// generate the varchar chunk
	vector<LogicalType> varchar_types;
	for (idx_t c = 0; c < input.column_ids.size(); c++) {
		varchar_types.push_back(LogicalType::VARCHAR);
	}
	result->varchar_chunk.Initialize(Allocator::DefaultAllocator(), varchar_types);
	return std::move(result);
}

static unique_ptr<LocalTableFunctionState> MySQLInitLocalState(ExecutionContext &context, TableFunctionInitInput &input,
                                                               GlobalTableFunctionState *global_state) {
	return make_uniq<MySQLLocalState>();
}

void CastBoolFromMySQL(ClientContext &context, Vector &input, Vector &result, idx_t size) {
	auto input_data = FlatVector::GetData<string_t>(input);
	auto result_data = FlatVector::GetData<bool>(result);
	for (idx_t r = 0; r < size; r++) {
		if (FlatVector::IsNull(input, r)) {
			FlatVector::SetNull(result, r, true);
			continue;
		}
		auto str_data = input_data[r].GetData();
		auto str_size = input_data[r].GetSize();
		if (str_size != 1) {
			throw BinderException("Failed to cast MySQL boolean - expected 1 byte element but got element of size %s",
			                      str_size);
		}
		auto bool_char = *str_data;
		result_data[r] = bool_char == '\1' || bool_char == '1';
	}
}

static void MySQLScan(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	auto &gstate = data.global_state->Cast<MySQLGlobalState>();
	idx_t r;
	gstate.varchar_chunk.Reset();
	for (r = 0; r < STANDARD_VECTOR_SIZE; r++) {
		if (!gstate.result->Next()) {
			// exhausted result
			break;
		}
		for (idx_t c = 0; c < output.ColumnCount(); c++) {
			auto &vec = gstate.varchar_chunk.data[c];
			if (gstate.result->IsNull(c)) {
				FlatVector::SetNull(vec, r, true);
			} else {
				auto string_data = FlatVector::GetData<string_t>(vec);
				string_data[r] = StringVector::AddString(vec, gstate.result->GetStringT(c));
			}
		}
	}
	if (r == 0) {
		// done
		return;
	}
	D_ASSERT(output.ColumnCount() == gstate.varchar_chunk.ColumnCount());
	for (idx_t c = 0; c < output.ColumnCount(); c++) {
		switch (output.data[c].GetType().id()) {
		case LogicalTypeId::BLOB:
			// blobs are sent over the wire as-is
			output.data[c].Reinterpret(gstate.varchar_chunk.data[c]);
			break;
		case LogicalTypeId::BOOLEAN:
			// booleans can be sent either as numbers ('0' or '1') or as bits ('\0' or '\1')
			CastBoolFromMySQL(context, gstate.varchar_chunk.data[c], output.data[c], r);
			break;
		default: {
			string error;
			VectorOperations::TryCast(context, gstate.varchar_chunk.data[c], output.data[c], r, &error);
			break;
		}
		}
	}
	output.SetCardinality(r);
}

static string MySQLScanToString(const FunctionData *bind_data_p) {
	auto &bind_data = bind_data_p->Cast<MySQLBindData>();
	return bind_data.table.name;
}

static void MySQLScanSerialize(Serializer &serializer, const optional_ptr<FunctionData> bind_data_p,
                               const TableFunction &function) {
	throw NotImplementedException("MySQLScanSerialize");
}

static unique_ptr<FunctionData> MySQLScanDeserialize(Deserializer &deserializer, TableFunction &function) {
	throw NotImplementedException("MySQLScanDeserialize");
}

MySQLScanFunction::MySQLScanFunction()
    : TableFunction("mysql_scan", {LogicalType::VARCHAR, LogicalType::VARCHAR, LogicalType::VARCHAR}, MySQLScan,
                    MySQLBind, MySQLInitGlobalState, MySQLInitLocalState) {
	to_string = MySQLScanToString;
	serialize = MySQLScanSerialize;
	deserialize = MySQLScanDeserialize;
	projection_pushdown = true;
}

} // namespace duckdb
