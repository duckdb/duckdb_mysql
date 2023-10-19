#include "duckdb.hpp"

#include "duckdb/main/extension_util.hpp"
#include "duckdb/parser/parsed_data/create_table_function_info.hpp"
#include "mysql_filter_pushdown.hpp"
#include "mysql_scanner.hpp"
#include "mysql_result.hpp"
#include "storage/mysql_transaction.hpp"
#include "storage/mysql_table_set.hpp"

namespace duckdb {

struct MySQLGlobalState;

struct MySQLLocalState : public LocalTableFunctionState {
	bool done = false;
	bool exec = false;
	string sql;
	vector<column_t> column_ids;
	TableFilterSet *filters;
	string col_names;
	MySQLConnection connection;

	void ScanChunk(ClientContext &context, const MySQLBindData &bind_data, MySQLGlobalState &gstate, DataChunk &output);
};

struct MySQLGlobalState : public GlobalTableFunctionState {
	explicit MySQLGlobalState(idx_t max_threads)
	    : page_idx(0), max_threads(max_threads) {
	}

	mutex lock;
	idx_t page_idx;
	idx_t max_threads;
	unique_ptr<ColumnDataCollection> collection;
	ColumnDataScanState scan_state;

	idx_t MaxThreads() const override {
		return max_threads;
	}
};

static unique_ptr<FunctionData> MySQLBind(ClientContext &context, TableFunctionBindInput &input,
                                             vector<LogicalType> &return_types, vector<string> &names) {
	throw InternalException("MySQLBind");
}

static void MySQLInitInternal(ClientContext &context, const MySQLBindData *bind_data_p,
                                 MySQLLocalState &lstate, idx_t task_min, idx_t task_max) {
	throw InternalException("MySQLInitInternal");
}

static idx_t MySQLMaxThreads(ClientContext &context, const FunctionData *bind_data_p) {
	D_ASSERT(bind_data_p);
	auto &bind_data = bind_data_p->Cast<MySQLBindData>();
	if (bind_data.requires_materialization) {
		return 1;
	}
	return bind_data.max_threads;
}

static unique_ptr<LocalTableFunctionState> GetLocalState(ClientContext &context, TableFunctionInitInput &input, MySQLGlobalState &gstate);

static unique_ptr<GlobalTableFunctionState> MySQLInitGlobalState(ClientContext &context,
                                                                    TableFunctionInitInput &input) {
	throw InternalException("MySQLInitGlobalState");
}

static bool MySQLParallelStateNext(ClientContext &context, const FunctionData *bind_data_p,
                                      MySQLLocalState &lstate, MySQLGlobalState &gstate) {
	throw InternalException("MySQLParallelStateNext");
}

static unique_ptr<LocalTableFunctionState> MySQLInitLocalState(ExecutionContext &context,
                                                                  TableFunctionInitInput &input,
                                                                  GlobalTableFunctionState *global_state) {
	throw InternalException("MySQLInitLocalState");
}

void MySQLLocalState::ScanChunk(ClientContext &context, const MySQLBindData &bind_data, MySQLGlobalState &gstate, DataChunk &output) {
	throw InternalException("ScanChunk");
}

static void MySQLScan(ClientContext &context, TableFunctionInput &data, DataChunk &output) {
	throw InternalException("MySQLScan");
}

static string MySQLScanToString(const FunctionData *bind_data_p) {
	D_ASSERT(bind_data_p);

	auto bind_data = (const MySQLBindData *)bind_data_p;
	return bind_data->table_name;
}

static void MySQLScanSerialize(Serializer &serializer, const optional_ptr<FunctionData> bind_data_p,
								 const TableFunction &function) {
	throw NotImplementedException("MySQLScanSerialize");
}

static unique_ptr<FunctionData> MySQLScanDeserialize(Deserializer &deserializer, TableFunction &function) {
	throw NotImplementedException("MySQLScanDeserialize");
}

MySQLScanFunction::MySQLScanFunction()
	: TableFunction("mysql_scan", {LogicalType::VARCHAR, LogicalType::VARCHAR, LogicalType::VARCHAR},
					MySQLScan, MySQLBind, MySQLInitGlobalState, MySQLInitLocalState) {
	to_string = MySQLScanToString;
	serialize = MySQLScanSerialize;
	deserialize = MySQLScanDeserialize;
	projection_pushdown = true;
}

}
