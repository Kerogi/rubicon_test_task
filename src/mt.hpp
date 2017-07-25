#pragma once

#include <vector>
#include <string>
#include "records.hpp"

//simple facade to parallel fetch the query results from peer servers
query_results_t do_the_parallel_query(const std::vector<std::string>& urls, const std::string& query_string);