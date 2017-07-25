#include "json.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

size_t format_json_query_results_body(std::ostream& os, const query_results_t& query_res)
{
    
    try{
        boost::property_tree::ptree tree;
        tree.put("query_results.query_string", query_res.query_string);
        
        for(const data_record_t& rec: query_res.found_records) {
            tree.add_child("query_results.found_records.record", record_to_ptree(rec));
        }
        std::ostringstream ss_json_body;
        boost::property_tree::write_json(ss_json_body, tree);
        os << ss_json_body.str();
        return ss_json_body.str().length();
    } catch(boost::property_tree::ptree_error const&  ex) {
        std::ostringstream ss_error;
        ss_error <<"{\"error\":\""<<ex.what()<<"\"}";
	    os <<ss_error.str();
        return ss_error.str().size();
    }
   
   
}
