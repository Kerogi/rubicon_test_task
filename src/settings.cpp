#include "settings.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

dest_map_t g_destinations_db;

// <neighbors>
//     <peer>
//         <name>A</name>
//         <url>localhost:12301</url>
//     </peer>
//     <peer>
//         <name>B/name>
//         <url>localhost:12302</url>
//     </peer>
//     <peer>
//         <name>C/name>
//         <url>localhost:12303</url>
//     </peer>
// </neighbors>

bool load_destination(const std::string& filename, dest_map_t& dst)
{
    boost::property_tree::ptree tree;
    size_t before = dst.size();
    try{
        boost::property_tree::read_xml(filename, tree);

        for(auto& peer_kv: tree.get_child("neighbors")) {
            std::string name = peer_kv.second.get<std::string>("name");
            std::string url = peer_kv.second.get<std::string>("url");
            dst.insert(make_pair(name, url));
        }
    } catch(boost::property_tree::ptree_error const&  ex) {
        return false;
    }
    return before != dst.size();
}

// <restaurants>
//     <restaurant>
//         <name>A</name>
//         <type></type>
//         <avg_price></avg_price>
//     </restaurant>
//     <restaurant>
//         <name>A</name>
//         <type></type>
//         <avg_price></avg_price>
//     </restaurant>
//     <restaurant>
//         <name>A</name>
//         <type></type>
//         <avg_price></avg_price>
//     </restaurant>
// </restaurants>

bool load_records(const std::string& filename, records_t& dst)
{
    boost::property_tree::ptree tree;
    size_t before = dst.size();
    try{
        boost::property_tree::read_xml(filename, tree);

        for(auto rest_kv: tree.get_child("restaurants")) {
            std::string name = rest_kv.second.get<std::string>("name");
            std::string type = rest_kv.second.get<std::string>("type");
            double avg_price = rest_kv.second.get<double>("avg_price");
            data_record_t rec {type, name};
            dst.insert(make_pair(type, rec));
        }
    } catch(boost::property_tree::ptree_error const&  ex) {
        return false;
    }
    return before != dst.size();
}