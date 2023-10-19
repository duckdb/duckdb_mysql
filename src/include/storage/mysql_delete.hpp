//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/mysql_delete.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/execution/physical_operator.hpp"

namespace duckdb {

class MySQLDelete : public PhysicalOperator {
public:
	MySQLDelete(LogicalOperator &op, TableCatalogEntry &table, idx_t row_id_index);

	//! The table to delete from
	TableCatalogEntry &table;
	idx_t row_id_index;

public:
	// Source interface
	SourceResultType GetData(ExecutionContext &context, DataChunk &chunk, OperatorSourceInput &input) const override;

	bool IsSource() const override {
		return true;
	}

public:
	// Sink interface
	unique_ptr<GlobalSinkState> GetGlobalSinkState(ClientContext &context) const override;
	SinkResultType Sink(ExecutionContext &context, DataChunk &chunk, OperatorSinkInput &input) const override;
	SinkFinalizeType Finalize(Pipeline &pipeline, Event &event, ClientContext &context,
	                          OperatorSinkFinalizeInput &input) const override;

	bool IsSink() const override {
		return true;
	}

	bool ParallelSink() const override {
		return false;
	}

	string GetName() const override;
	string ParamsToString() const override;
};

} // namespace duckdb
