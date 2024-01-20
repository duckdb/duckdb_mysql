#include "storage/mysql_optimizer.hpp"
#include "duckdb/planner/operator/logical_get.hpp"
#include "duckdb/planner/operator/logical_limit.hpp"
#include "mysql_scanner.hpp"

namespace duckdb {

static bool IsMySQLScan(const string &function_name) {
	return function_name == "mysql_scan";
}

void OptimizeMySQLScan(unique_ptr<LogicalOperator> &op) {
	if (op->type == LogicalOperatorType::LOGICAL_LIMIT) {
		auto &limit = op->Cast<LogicalLimit>();
		reference<LogicalOperator> child = *op->children[0];
		while (child.get().type == LogicalOperatorType::LOGICAL_PROJECTION) {
			child = *child.get().children[0];
		}
		if (child.get().type != LogicalOperatorType::LOGICAL_GET) {
			return;
		}
		auto &get = child.get().Cast<LogicalGet>();
		if (!IsMySQLScan(get.function.name)) {
			return;
		}
		if (limit.limit || limit.offset) {
			// not a constant limit
			return;
		}
		auto &bind_data = get.bind_data->Cast<MySQLBindData>();
		if (limit.limit_val > 0) {
			bind_data.limit += " LIMIT " + to_string(limit.limit_val);
		}
		if (limit.offset_val > 0) {
			bind_data.limit += " OFFSET " + to_string(limit.offset_val);
		}
		// remove the limit
		op = std::move(op->children[0]);
		return;
	}
	// recurse into children
	for (auto &child : op->children) {
		OptimizeMySQLScan(child);
	}
}

void MySQLOptimizer::Optimize(ClientContext &context, OptimizerExtensionInfo *info, unique_ptr<LogicalOperator> &plan) {
	OptimizeMySQLScan(plan);
}

} // namespace duckdb
