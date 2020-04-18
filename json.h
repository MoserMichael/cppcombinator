#pragma once
#include <iostream>
#include "parsedef.h"

namespace pparse {


template<typename stream>
struct Json {

		static void jsonStartTag(stream &out) {
			out << "{ ";
		}

		static void jsonAddField(stream &out, const char *fieldName, const std::string &fieldValue, bool isLast = false) {
			out << "\"" << fieldName << "\": \"" << fieldValue.c_str() << "\"";
			if (!isLast) {
				out << ",";
			}
		}
		template<typename FType>
		static void jsonAddField(stream &out, const char *fieldName, FType fieldValue, bool isLast = false) {
			out << "\"" << fieldName << "\": \"" << fieldValue << "\"";
			if (!isLast) {
				out << ",";
			}
		}

		static void jsonStartNested(stream &out, const char *fieldName, bool is_array = false) {
			out << "\"" << fieldName << "\": ";
			if (is_array) {
				out << "[ ";
			}
		}

		static void jsonEndNested(stream &out, bool isLast, bool is_array = false) {
			if (is_array) {
				out << "] ";
			}
			if (!isLast) {
				out << ",";
			}
		}

		static void jsonEndTag(stream &out, bool isLast = false) {
			out << "} ";
			if (!isLast) {
				out << ",";
			}
		}

		static void dumpRule(stream &out, RuleId ruleId, const char *typeName, bool isLast = false) {
			jsonStartTag(out);
			jsonAddField(out, "ruleId", ruleId);
			jsonAddField(out, "typeName", typeName, isLast );
		}

};

}
