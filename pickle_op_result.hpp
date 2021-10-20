#ifndef _PICKLE_OP_RESULT_H
#define _PICKLE_OP_RESULT_H

// #include <string>

#include "lab1_cpp/compfuncs.hpp"

namespace result_pickle
{
	using namespace os::lab1::compfuncs;

	template<typename T>
	std::string fail_dumps(const T &res){
		switch (res.index()) {
		case 0:
			return "HARD_FAIL";
		case 1:
			return "SOFT_FAIL";
		}
		return "";
	}


	std::string dumps(typename op_group_traits<INT_SUM>::result_type &res){
		if(res.index() != 2)
			return fail_dumps(res);
		
		int res_value = std::get<2>(res);
		return std::to_string(res_value);
	}

	typename op_group_traits<INT_SUM>::result_type loads(const std::string &str){
		std::cout << "Str:" << str << "  " << str.length() << std::endl;
		if (str.find("HARD_FAIL") != std::string::npos) 
			return comp_result<typename op_group_traits<INT_SUM>::value_type>(hard_fail());
		if (str.find("SOFT_FAIL") != std::string::npos) 
			return comp_result<typename op_group_traits<INT_SUM>::value_type>(soft_fail());

		return comp_result<typename op_group_traits<INT_SUM>::value_type>(std::stoi(str));
	}
	
}

// atoi(hello)


#endif // _PICKLE_OP_RESULT_H
