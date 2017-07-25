#include "records.hpp"
#include "misc_utils.h"
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <iomanip>

records_t g_records_db;

std::ostream& operator<<(std::ostream& os, const data_record_t& dr) {
    return os << "{ type: '"<<dr.type<<"', name: '"<<dr.name<<"', price: '"<<dr.avg_price<<"' }";
}

std::ostream& operator<<(std::ostream& os, const query_results_t& qr) {
	if(qr.found_records.empty()){
		os << "{ nothing found for '"<<qr.query_string<<"'}";
	} else {
		os << "{ for '"<<qr.query_string<<"' were found:\n";
		for(const auto& dr: qr.found_records) {
			os << '\t' << dr << '\n';
		}
		os << "}";
	}
	return os;
}


query_results_t query_records(const std::string& search_key, const records_t& records) {
	query_results_t res {search_key, {} };
	auto it_range = records.equal_range(partial_key(true, search_key));
	records_t::const_iterator it;
	for ( it = it_range.first; it != it_range.second; ++it) {
		res.found_records.push_back(it->second);
	}
	return res;
}

//deserialize record from html entry string
data_record_t record_from_html(const std::string& html) {
	std::istringstream  ss(html);
	data_record_t r;
	ss>>match_string("<li><i>")>>r.type>>match_string("</i>  <b>")
		>>r.name>>match_string("</b> Average Meal Price: <u>")
		>>r.avg_price>>match_string("</u></li>");
	return r;
}

//serialise record to html entry string
std::string record_to_html(const data_record_t& r) {
	std::stringstream ss;
	ss<<"<li><i>"<<r.type<<"</i>  <b>"<<r.name<<"</b> Average Meal Price: <u>"<<std::setprecision(2)<<std::fixed<<r.avg_price<<"</u></li>";
	return ss.str();
}

boost::property_tree::ptree record_to_ptree(const data_record_t& rec) {
	boost::property_tree::ptree pt_record;
    pt_record.put("type", rec.type);
    pt_record.put("name", rec.name);
    pt_record.put("avg_price", rec.avg_price);
	return pt_record;
}       

data_record_t record_from_ptree(const boost::property_tree::ptree& pt) {
  	std::string name = pt.get<std::string>("name");
    std::string type = pt.get<std::string>("type");
    double avg_price = pt.get<double>("avg_price");
	return data_record_t { type, name, avg_price};
}       