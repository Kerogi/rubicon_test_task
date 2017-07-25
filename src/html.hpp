
#pragma once
#include <string>
#include <ostream>
#include "records.hpp"

size_t html_not_implemented(std::ostream& os);
size_t html_forbiden(std::ostream& os);
size_t html_notfound_query(std::ostream& os, const std::string& query_string);
size_t html_query_results(std::ostream& os, const query_results_t& query_res);


template<class FriteFuncT, class ..FuncArgsT>
std::stringbuf wrap_to_stringbuf(FriteFuncT&& func, FuncArgsT&&... func_args) {
    std::stringbuf obuf;
    std::ostream os(obuf)
    func(os,std::forward<FuncArgsT>(func_args)...);
    return obuf;
}