#include "mysql_filter_pushdown.hpp"
#include "mysql_scanner.hpp"
#include "mysql_utils.hpp"
#include <iostream>

namespace duckdb {

string MySQLFilterPushdown::CreateExpression(string &column_name, vector<unique_ptr<TableFilter>> &filters, string op) {
	vector<string> filter_entries;
	for (auto &filter : filters) {
		filter_entries.push_back(TransformFilter(column_name, *filter));
	}
	return "(" + StringUtil::Join(filter_entries, " " + op + " ") + ")";
}

string MySQLFilterPushdown::TransformComparision(ExpressionType type) {
	switch (type) {
	case ExpressionType::COMPARE_EQUAL:
		return "=";
	case ExpressionType::COMPARE_NOTEQUAL:
		return "!=";
	case ExpressionType::COMPARE_LESSTHAN:
		return "<";
	case ExpressionType::COMPARE_GREATERTHAN:
		return ">";
	case ExpressionType::COMPARE_LESSTHANOREQUALTO:
		return "<=";
	case ExpressionType::COMPARE_GREATERTHANOREQUALTO:
		return ">=";
	case ExpressionType::COMPARE_IN:
		return "IN";	
	default:
		throw NotImplementedException("Unsupported expression type");
	}
}

string MySQLFilterPushdown::TransformFilter(string &column_name, TableFilter &filter) {
	switch (filter.filter_type) {
	case TableFilterType::IS_NULL:
		return column_name + " IS NULL";
	case TableFilterType::IS_NOT_NULL:
		return column_name + " IS NOT NULL";
	case TableFilterType::CONJUNCTION_AND: {
		auto &conjunction_filter = filter.Cast<ConjunctionAndFilter>();
		return CreateExpression(column_name, conjunction_filter.child_filters, "AND");
	}
	case TableFilterType::CONJUNCTION_OR: {
		auto &conjunction_filter = filter.Cast<ConjunctionAndFilter>();
		return CreateExpression(column_name, conjunction_filter.child_filters, "OR");
	}
	case TableFilterType::CONSTANT_COMPARISON: {
		auto &constant_filter = filter.Cast<ConstantFilter>();
		auto constant_string = constant_filter.constant.ToSQLString();
		auto operator_string = TransformComparision(constant_filter.comparison_type);
		return StringUtil::Format("%s %s %s", column_name, operator_string, constant_string);
	}
	default:
		throw InternalException("Unsupported table filter type");
	}
}

string MySQLFilterPushdown::TransformFilters(const vector<column_t> &column_ids, optional_ptr<TableFilterSet> filters,
                                             const vector<string> &names) {
	
	std::cout << "TransformFilters" << std::endl;
	if (!filters || filters->filters.empty()) {
		// no filters
		std::cout << "No filter" << std::endl;
		return string();
	}
	string result;
	for (auto &entry : filters->filters) {
		if (!result.empty()) {
			result += " AND ";
		}
		auto column_name = MySQLUtils::WriteIdentifier(names[column_ids[entry.first]]);
		auto &filter = *entry.second;
		std::cout << "filter: " << filter.ToString(column_name) << std::endl;
		result += TransformFilter(column_name, filter);
	}
	return result;
}

void MySQLFilterPushdown::ComplexFilterPushdown(ClientContext &context, LogicalGet &get, FunctionData *bind_data_p,
                                     vector<unique_ptr<Expression>> &filters) {
	
	auto &data = bind_data_p->Cast<MySQLBindData>();
	vector<unique_ptr<Expression>> filters_to_apply;
	vector<unique_ptr<Expression>> unsupported_filters;

	for (idx_t j = 0; j < filters.size(); j++) {
			auto &filter = filters[j];
			std::cout << "current filter : " << filter->ToString() << std::endl;
			unique_ptr<Expression> filter_copy = filter->Copy();
			if (filter->expression_class == ExpressionClass::BOUND_EXPRESSION ||
					filter->expression_class == ExpressionClass::BOUND_CONSTANT || 
					filter->expression_class == ExpressionClass::BOUND_CONJUNCTION ||
					filter->expression_class == ExpressionClass::BOUND_COMPARISON ||
					filter->expression_class == ExpressionClass::BOUND_OPERATOR) {
					filters_to_apply.emplace_back(std::move(filter_copy));
					std::cout << "filters_to_apply : " << filter->ToString() << std::endl;
			} else {
					unsupported_filters.emplace_back(std::move(filter_copy));
					std::cout << "unsupported_filters : " << filter->ToString() << " with class: " << static_cast<int>(filter->expression_class) << std::endl;
			}
		}
	
	data.filters_to_apply = std::move(filters_to_apply);	
	filters = std::move(unsupported_filters);
}

} // namespace duckdb
