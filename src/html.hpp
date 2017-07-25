
#pragma once
#include <string>
#include <ostream>
#include "records.hpp"

size_t format_html_query_results_body(std::ostream& os, const query_results_t& query_res);
size_t format_html_body(std::ostream& os, const std::string& message, int code = 200);

template<class FriteFuncT, class ...FuncArgsT>
std::stringbuf wrap_to_stringbuf(FriteFuncT&& func, FuncArgsT&&... func_args) {
    std::stringbuf obuf;
    std::ostream os(obuf);
    func(os,std::forward<FuncArgsT>(func_args)...);
    return obuf;
}