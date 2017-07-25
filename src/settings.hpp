#pragma once
#include <map>
#include <string>
#include <iostream>
#include "records.hpp"

using dest_map_t = std::map<std::string, std::string>;

bool load_destination(const std::string& filename, dest_map_t& dst);
bool load_records(const std::string& filename, records_t& dst);

//global DB of peer nodes
extern dest_map_t g_destinations_db;