
//===----------------------------------------------------------------------===//
//
//                         PelotonDB
//
// insert_plan.cpp
//
// Identification: /peloton/src/planner/insert_plan.cpp
//
// Copyright (c) 2015, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
 
#include "planner/insert_plan.h"
#include "planner/project_info.h"
#include "storage/data_table.h"
#include "storage/tuple.h"
#include "parser/peloton/insert_parse.h"
#include "catalog/bootstrapper.h"
#include "catalog/column.h"

namespace peloton{
namespace planner{
InsertPlan::InsertPlan(storage::DataTable *table, oid_t bulk_insert_count)
    : target_table_(table), bulk_insert_count(bulk_insert_count) {
  catalog::Bootstrapper::bootstrap();
  catalog::Bootstrapper::global_catalog->CreateDatabase(DEFAULT_DB_NAME);
}

// This constructor takes in a project info
InsertPlan::InsertPlan(
    storage::DataTable *table,
    std::unique_ptr<const planner::ProjectInfo> &&project_info,
    oid_t bulk_insert_count)
    : target_table_(table),
      project_info_(std::move(project_info)),
      bulk_insert_count(bulk_insert_count) {
	  catalog::Bootstrapper::bootstrap();
	  catalog::Bootstrapper::global_catalog->CreateDatabase(DEFAULT_DB_NAME);
}

// This constructor takes in a tuple
InsertPlan::InsertPlan(storage::DataTable *table,
                    std::unique_ptr<storage::Tuple> &&tuple,
                    oid_t bulk_insert_count)
    : target_table_(table),
      tuple_(std::move(tuple)),
      bulk_insert_count(bulk_insert_count) {
	  catalog::Bootstrapper::bootstrap();
	  catalog::Bootstrapper::global_catalog->CreateDatabase(DEFAULT_DB_NAME);
}

// This constructor takes a parse tree
InsertPlan::InsertPlan(parser::InsertParse *parse_tree, oid_t bulk_insert_count) : bulk_insert_count(bulk_insert_count) {
  catalog::Bootstrapper::bootstrap();
  catalog::Bootstrapper::global_catalog->CreateDatabase(DEFAULT_DB_NAME);

  std::vector<std::string> columns = parse_tree->getColumns();
  std::vector<Value> values = parse_tree->getValues();

  target_table_ = catalog::Bootstrapper::global_catalog->GetTableFromDatabase(DEFAULT_DB_NAME, parse_tree->GetTableName());
  PL_ASSERT(target_table_);
  catalog::Schema* table_schema = target_table_->GetSchema();

  if(columns.size() == 0){ // INSERT INTO table_name VALUES (val1, val2, ...);
	    PL_ASSERT(values.size() == table_schema->GetColumnCount());
	    std::unique_ptr<storage::Tuple> tuple(new storage::Tuple(table_schema, true));
	    int col_cntr = 0;
	    for(Value const& elem : values) {
	    	switch (elem.GetValueType()) {
	    	  case VALUE_TYPE_VARCHAR:
	    	  case VALUE_TYPE_VARBINARY:
	    		  tuple->SetValue(col_cntr++, elem, GetPlanPool());
	    		  break;

	    	    default: {
	    	    	tuple->SetValue(col_cntr++, elem, nullptr);
	    	    }
	    	  }
	    }
	    tuple_ = std::move(tuple);
  }
  else{  // INSERT INTO table_name (col1, col2, ...) VALUES (val1, val2, ...);
	    PL_ASSERT(columns.size() <= table_schema->GetColumnCount());  // columns has to be less than or equal that of schema
	    std::unique_ptr<storage::Tuple> tuple(new storage::Tuple(table_schema, true));
	    int col_cntr = 0;
	    auto table_columns = table_schema->GetColumns();
	    for(catalog::Column const& elem : table_columns){
	    	std::size_t pos = std::find(columns.begin(), columns.end(), elem.GetName()) - columns.begin();
			switch (elem.GetType()) {
			case VALUE_TYPE_VARCHAR:
			case VALUE_TYPE_VARBINARY: {
				if(pos >= columns.size()) {
					tuple->SetValue(col_cntr, ValueFactory::GetNullStringValue(), GetPlanPool());
				}
				else {
					tuple->SetValue(col_cntr, values[pos], GetPlanPool());
				}
				break;
			}

			default: {
				if(pos >= columns.size()) {
					tuple->SetValue(col_cntr, ValueFactory::GetNullStringValue(), GetPlanPool());
				}
				else {
					tuple->SetValue(col_cntr, values[pos], nullptr);
				}
			}
			}
			++col_cntr;
	    }
	    tuple_ = std::move(tuple);
  }
  LOG_INFO("Tuple to be inserted: %s", tuple_->GetInfo().c_str());
}

VarlenPool *InsertPlan::GetPlanPool() {
  // construct pool if needed
  if (pool_.get() == nullptr) pool_.reset(new VarlenPool(BACKEND_TYPE_MM));
  // return pool
  return pool_.get();
}

}
}


