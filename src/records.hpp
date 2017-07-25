#pragma once
#include <string>
#include <map>
#include <iostream>
#include <vector>
#include <boost/property_tree/ptree_fwd.hpp>

struct data_record_t {
  std::string key;
  std::string data;
};

struct partial_key {
	std::string key;
	bool partial_match = false;
	partial_key(const char * s) :key(s), partial_match(false) {};

	partial_key(const std::string& s) :key(s), partial_match(false) {};
	explicit partial_key(bool partial, const std::string& s) :key(s), partial_match(partial) {};

	friend bool operator<(const partial_key& lhs, const partial_key& rhs) {
		if (lhs.partial_match != rhs.partial_match) {
			int partials_left = (rhs.partial_match)?rhs.key.length(): lhs.key.length();
		
			for ( size_t il = 0, ir = 0; il < lhs.key.length() && ir < rhs.key.length();	++il, ++ir, --partials_left) {
				bool res = ((lhs.key[il] < rhs.key[ir]));
				if (res) return true;
			}

			return (lhs.partial_match)? false : partials_left > 0;
		}
		return lhs.key < rhs.key;
	}

	friend std::ostream& operator<<(std::ostream& os, const partial_key& rhs) {
		if(rhs.partial_match)
			return os << '[' << rhs.key << ']';
		return os << rhs.key;
	}
};

using records_t = std::map<partial_key, data_record_t>;


struct query_results_t {
  std::string query_string;
  std::vector<data_record_t> found_records;
};

inline std::ostream& operator<<(std::ostream& os, const data_record_t& dr) {
  return os << "{ key: '"<<dr.key<<"', data: '"<<dr.data<<"' }";
}

inline std::ostream& operator<<(std::ostream& os, const query_results_t& qr) {
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

query_results_t query_records(const std::string& search_key, const records_t& records);

data_record_t record_from_html(const std::string& html);

std::string record_to_html(const data_record_t& r);

data_record_t record_from_ptree(const boost::property_tree::ptree& pt);
