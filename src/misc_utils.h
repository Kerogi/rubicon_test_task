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

using dest_map_t = std::map<std::string, std::string>;

namespace quoted_string {
	inline std::ostream& operator<<(std::ostream& os, const std::string& s) {
		std::streamsize width = os.width();
		size_t max_text_length = (size_t)width;
		if (max_text_length == 0) max_text_length = 30;
		os.width(0);
		os << "'";
		if (s.length() <= max_text_length) {
			os << s.c_str();
		}
		else {
			os << s.substr(0, max_text_length - 3).c_str() << "...";
		}
		os << "'";
		os.width(width);
		return os;
	}
}

struct string_matcher {
	std::string expected_str;
};

inline
string_matcher match_string(const std::string& str) {
	return {str};
}

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

template<class ItemFirstT, class ItemSecondT>
inline
std::ostream& operator<<(std::ostream& os, const std::pair< ItemFirstT, ItemSecondT> & c) {
	using namespace quoted_string;
	return os << "(" << c.first << "; " << c.second << ")";
}

template<class ItemT>
inline
std::ostream& operator<<(std::ostream& os, const std::list< ItemT > & c) {
	using namespace quoted_string; 
	os << '(';
	if (!c.empty()) {
		bool first = true;
		for (const ItemT& item : c) {
			if (!first) os << ", ";
			os << item;
			first = false;
		}
	}
	os << ')';
	return os;
}


template<class ItemT>
std::ostream& operator<<(std::ostream& os, const std::vector< ItemT > & c) {
	using namespace quoted_string;
	os << '[';
	if (!c.empty()) {
		bool first = true;
		for (const ItemT& item : c) {
			if (!first) os << ", ";
			os << item;
			first = false;
		}
	}
	os << ']';
	return os;
}

template<class ItemT>
std::ostream& operator<<(std::ostream& os, const std::queue<ItemT> & c) {
	using namespace quoted_string;
	os << '<';
	if (!c.empty()) {
		bool first = true;
		for (const auto& i: c) {
			if (!first) os << ", ";
			os << i ;
			first = false;
		}
	}
	os << '>';
	return os;
}

template<class ItemT>
std::ostream& operator<<(std::ostream& os, const std::deque<ItemT> & c) {
	using namespace quoted_string;
	os << '<';
	if (!c.empty()) {
		bool first = true;
		for (const auto& i : c) {
			if (!first) os << ", ";
			os << i;
			first = false;
		}
	}
	os << '>';
	return os;
}



template<class KeyT, class ItemT, class ...Rest>
std::ostream& operator<<(std::ostream& os, const std::map< KeyT, ItemT,Rest...> & c) {
	using namespace quoted_string;
	os << '{';
	if (!c.empty()) {
		bool first = true;
		for (const auto& kv : c) {
			if (!first) os << ",\n";
			os << kv.first <<": "<< kv.second;
			first = false;
		}
	}
	os << '}';
	return os;
}


template<class KeyT, class ItemT>
std::ostream& operator<<(std::ostream& os, const std::multimap< KeyT, ItemT> & c) {
	using namespace quoted_string;
	os << '{';
	if (!c.empty()) {
		bool first = true;
		for (auto it = c.begin(); it != c.end(); ++it) {
			if (!first) os << ",\n";
			os << it->first << ": (";
			auto range = c.equal_range(it->first);
			//os << range;
			bool sub_first = true;
			for (auto v = range.first; v != range.second; ++v){
				if (!sub_first) os << ", ";
				os << *v;
				sub_first = false;
			}
			os << ")";
			first = false;
		}
	}
	os << '}';
	return os;
}

template<class KeyT>
std::ostream& operator<<(std::ostream& os, const std::set< KeyT> & c) {
	using namespace quoted_string;
	os << '{';
	if (!c.empty()) {
		bool first = true;
		for (const auto& k: c) {
			if (!first) os << ", ";
			os << k;
			first = false;
		}
	}
	os << '}';
	return os;
}
//iterator for cycle the sequence not from the start
// but exacly same times as from begin to end
// end in case we reach the end we jump to the begining
// end stops when we reach the iter were we start from (also takes in account cycles var)
// to properly stop we must compare it to the end it returned by facrory func
template< class IterT >
class cyclic_iterator : public std::iterator<
	std::input_iterator_tag,   // iterator_category
	typename IterT::value_type,
	typename IterT::difference_type,
	typename IterT::pointer,
	typename IterT::reference>
{
	IterT it;
	IterT begin;
	IterT end;
	size_t cycles;

	cyclic_iterator(size_t cycles, IterT begin, IterT start, IterT end)
		: it(start)
		, begin(begin)
		, end(end)
		, cycles(cycles)
	{}
public:
	cyclic_iterator() : it(), begin(), end(), cycles() {}
	cyclic_iterator(const cyclic_iterator&) = default;

	cyclic_iterator &operator++() {
		++it;
		if (it == end) {
			++cycles;
			it = begin;
		}
		return *this;
	}
	cyclic_iterator operator++(int) {
		cyclic_iterator prev = *this;
		++(*this);
		return prev;
	}

	friend bool operator==(cyclic_iterator const &lhs, cyclic_iterator const &rhs)
	{
		return lhs.it == rhs.it && lhs.cycles == rhs.cycles;
	}

	friend bool operator!=(cyclic_iterator const &lhs, cyclic_iterator const &rhs)
	{
		return lhs.it != rhs.it || lhs.cycles != rhs.cycles;
	}
	typename IterT::reference operator*() const {
		return *it;
	}
	template< class IterU >
	friend std::pair< cyclic_iterator<IterU>, cyclic_iterator<IterU> > cycle_range(size_t cycles, IterU begin, IterU start, IterU end);

	template< class IterU >
	friend std::pair< cyclic_iterator<IterU>, cyclic_iterator<IterU> > cycle_range(size_t cycles, IterU begin, IterU end);
};

template< class IterT >
std::pair< cyclic_iterator<IterT>, cyclic_iterator<IterT> > cycle_range(size_t cycles, IterT begin, IterT end)
{
	return std::make_pair(cyclic_iterator<IterT>(0, begin, begin, end), cyclic_iterator<IterT>(cycles, begin, begin, end));
}

template< class IterT >
std::pair< cyclic_iterator<IterT>, cyclic_iterator<IterT> > cycle_range(size_t cycles, IterT begin, IterT start, IterT end)
{
	return std::make_pair(cyclic_iterator<IterT>(0, begin, start, end), cyclic_iterator<IterT>(cycles, begin, start, end));
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


#define LBL(var) #var<<": "<<var