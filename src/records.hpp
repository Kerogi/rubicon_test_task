#pragma once
#include <string>
#include <map>
#include <iostream>
#include <vector>
#include <boost/property_tree/ptree_fwd.hpp>

//simple dummy recort type
struct data_record_t {
    std::string key;
    std::string data;
};

//special key class for prefix match
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

//records map type with key that could find more than one record using prefix
// map = {
// 	{"a"    , "data"}, // a < [aa]
// 	{"aa"	, "data"}, // !([aa] < aa) & !(aa < [aa])   => [aa] = aa
// 	{"aaa"	, "data"}, // !([aa] < aaa) & !(aaa < [aa]) => [aa] = aaa
// 	{"aba"	, "data"}, // [aa] < aba
// 	{"b"	, "data"},
// 	{"c"	, "data"},
// };
// map.equal_range(partial_key(true, "aa")) 
// will give us iterators at "aa" and "aba"
// which is lower and upper bound respectively 
using records_t = std::multimap<partial_key, data_record_t>;

//the results of one query on some map
struct query_results_t {
    std::string query_string;
    std::vector<data_record_t> found_records;
};

//try's find all prefix matches in specified map
query_results_t query_records(const std::string& search_key, const records_t& records);

//serializations 
data_record_t record_from_html(const std::string& html);
std::string   record_to_html(const data_record_t& r);

data_record_t record_from_ptree(const boost::property_tree::ptree& pt);

// utils
std::ostream& operator<<(std::ostream& os, const data_record_t& dr);
std::ostream& operator<<(std::ostream& os, const query_results_t& qr);

//global variable oto imitate a records db
extern records_t g_records_db;