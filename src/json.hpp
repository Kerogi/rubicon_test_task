#pragma once
#include <iostream>
#include "records.hpp"

//simple html formatters for query results
size_t format_json_query_results_body(std::ostream& os, const query_results_t& query_res);