#pragma once

#include <ostream>
#include <list>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <queue>
#include <deque>
#include <iterator>

struct string_matcher {
	std::string expected_str;
};

inline
std::istream& operator>>(std::istream& is, const string_matcher& ms) {
	std::string actual_str;
	actual_str.reserve(ms.expected_str.size());
	auto prev_pos = is.tellg();
	auto pos = is.rdbuf()->sgetn(&actual_str.front(), ms.expected_str.size());
	if(actual_str != ms.expected_str) {
		is.seekg(prev_pos);
		is.setstate(std::ios::failbit);
	}
	return is;
}

inline
string_matcher match_string(const std::string& str) {
	return {str};
}

template <typename ContOfStringT>
inline
ContOfStringT& split(
  ContOfStringT& result,
  const typename ContOfStringT::value_type& s,
  const typename ContOfStringT::value_type& delimiters,
  size_t  start_pos = 0,
  bool    empties = true )
{
  result.clear();
  size_t current;
  size_t next = start_pos-1;
  do
  {
    if (!empties)
    {
      next = s.find_first_not_of( delimiters, next + 1 );
      if (next == ContOfStringT::value_type::npos) break;
      next -= 1;
    }
    current = next + 1;
    next = s.find_first_of( delimiters, current );
    result.push_back( s.substr( current, next - current ) );
  }
  while (next != ContOfStringT::value_type::npos);
  return result;
}
