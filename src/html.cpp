#include <sstream>
#include "html.hpp"
#include "http.hpp"

size_t format_html_body(std::ostream& os, const std::string& message, int code)
{
	std::stringstream ss_html_body;
  std::string code_name = code_names[code];
	ss_html_body <<
R"(<html>
  <head>
    <title>)"<<code<<" "<<code_name<<R"(</title>
  </head>
  <body>
    <h1>)"<<code_name<<R"(</h1>
    )"<<message<<R"(
  </body>
</html>
)";
	os << ss_html_body.str();
}

size_t format_html_query_notfound_body(std::ostream &os, const std::string &query_string)
{
	std::stringstream ss_html_body;
	ss_html_body <<
R"(<html>
  <head>
    <title>Nothing Found</title>
  </head>
  <body>
    <h1>:(</h1>
	)"
	<< "Nothing found for query <b>" << query_string << "</b>"
	<< R"(
  </body>
</html>
)";

	os << ss_html_body.str();
	return ss_html_body.str().length();
}

size_t format_html_query_results_body(std::ostream &os, const query_results_t &query_res)
{
  if(query_res.found_records.empty())
    return format_html_query_notfound_body(os, query_res.query_string);
	std::stringstream ss_html_body;
	ss_html_body <<
R"(<html>
  <head>
    <title>Results for query</title>
  </head>
  <body>
    <h3>Query: )" << query_res.query_string << R"(</h3>
    <ul>
)";
	for (const auto &res : query_res.found_records)
	{
		ss_html_body <<"    " << record_to_html(res) << "\n";
	}
	ss_html_body << R"(
    </ul>
  </body>
</html>
)";

	os << ss_html_body.str();
	return ss_html_body.str().length();
}