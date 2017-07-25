#include "records.hpp"
#include "misc_utils.h"
#include <sstream>
#include <boost/property_tree/ptree.hpp>

query_results_t query_records(const std::string& search_key, const records_t& records) {
    query_results_t res {search_key, {} };
    auto it_range = records.equal_range(partial_key(true, search_key));
    records_t::const_iterator it;
    for ( it = it_range.first; it != it_range.second; ++it) {
      res.found_records.push_back(it->second);
    }
    return res;
}


data_record_t record_from_html(const std::string& html) {
    std::istringstream  ss(html);
    data_record_t r;
    ss>>match_string("<li key=\"")>>r.key>>match_string("\">")>>r.data>>match_string("</li>");
    return r;
}


std::string record_to_html(const data_record_t& r) {
    std::stringstream ss;
    ss<<"<li key=\""<<r.key<<"\">"<<r.data<<"</li>";
    return ss.str();
}


data_record_t record_from_ptree(const boost::property_tree::ptree& pt) {
    std::string key = pt.get<std::string>("<xmlattr>.key");
    std::string data = pt.data();

    return data_record_t { key, data};
}