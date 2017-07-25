#pragma once
#include <vector>
#include <string>
#include "records.hpp"

query_results_t do_the_parallel_query(const std::vector<std::string>& urls, const std::string& query_string);