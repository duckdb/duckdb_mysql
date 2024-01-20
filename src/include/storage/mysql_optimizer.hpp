//===----------------------------------------------------------------------===//
//                         DuckDB
//
// storage/mysql_optimizer.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/main/config.hpp"

namespace duckdb {
class MySQLOptimizer {
public:
	static void Optimize(ClientContext &context, OptimizerExtensionInfo *info, unique_ptr<LogicalOperator> &plan);
};

} // namespace duckdb
