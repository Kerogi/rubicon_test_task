
#pragma once
#include <string>
#include <ostream>
#include "records.hpp"


//simple html formatters for query results
size_t format_html_query_results_body(std::ostream& os, const query_results_t& query_res);

//simple html formatter which just creates page with
// tittle set to responce code
// one header with same test
// and message sting in page body
size_t format_html_body(std::ostream& os, const std::string& message, int code = 200);
