#include <sstream>
#include "html.hpp"

size_t html_not_implemented(std::ostream &os)
{
    std::string html_not_implemented_s(
R"(<html>
  <head>
    <title>501 Not Implemented</title>
  </head>
  <body>
    <h1>Not Implemented</h1>
    The requested URL, file type or operation is not implemented on nweb server.
  </body>
</html>
)");
    os << html_not_implemented_s;
    return html_not_implemented_s.length();
}

size_t html_forbiden(std::ostream &os)
{
    std::string html_forbiden_s(
R"(<html>
  <head>
    <title>403 Forbidden</title>
  </head>
  <body>
    <h1>Forbidden</h1>
    The requested URL, file type or operation is not allowed on nweb server.
  </body>
</html>
)");
    os << html_forbiden_s;
    return html_forbiden_s.length();
}

size_t html_notfound_query(std::ostream &os, const std::string &query_string)
{
    std::stringstream ss_html_body;
    ss_html_body <<
R"(<html>
    <head>
      <title>404 Not Found</title>
    </head>
    <body>
      <h1>Not Found</h1>
    )"
    << "Nothing found for query <b>" << query_string << "</b>"
    << R"(
    </body>
</html>
)";

    os << ss_html_body.str();
    return ss_html_body.str().length();
}

size_t html_query_results(std::ostream &os, const query_results_t &query_res)
{
    std::stringstream ss_html_body;
    ss_html_body <<
        R"(<html>
    <head>
      <title>Results for query</title>
    </head>
    <body>
      <h3>Query: )"
               << query_res.query_string << R"(</h3>
    <ul>)";
    for (const auto &res : query_res.found_records)
    {
        ss_html_body  << record_to_html(res) << "\n";
    }
    ss_html_body << R"(</ul>
    </body>
  </html>
  )";

    os << ss_html_body.str();
    return ss_html_body.str().length();
}