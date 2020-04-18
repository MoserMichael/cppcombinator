#pragma once

#include "parse_text.h"
#include <memory>
#include <list>
#include <tuple>
#include <variant>

namespace pparse {

typedef int RuleId;

struct Position {
	Position() : line_(0), column_(0) {
	}

	Position(int line, int column) : line_(line), column_(column) {
	}

	Position(Text_position &arg) : line_(arg.line_), column_(arg.column_) {
	}

	bool operator==(const Position  &arg) const {
		return arg.line_ == line_ && arg.column_ == column_;
	}

	bool operator<(const Position  &arg) const {
		return line_ < arg.line_  || (line_ == arg.line_ && column_ < arg.column_);
	}

	int line() const {
		return line_;
	}

	int column() const {
		return column_;
	}


    int line_;
    int column_;
};


struct AstEntryBase {
    AstEntryBase(RuleId ruleId) : ruleId_(ruleId) {
    }

    virtual ~AstEntryBase() {
    }
    
	RuleId getRuleId() const {
		return ruleId_;
	}

 
	RuleId ruleId_;
};


struct Parse_result {

	bool success() const {
		return success_;
	}

	AstEntryBase *get_ast() const {
		return ast_.get();
	}

	Position get_start_pos() const {
		return start_;
	}

	Position get_end_pos() const {
		return end_;
	}

	bool success_;
	Position start_;
	Position end_;
	std::unique_ptr<AstEntryBase>  ast_;
};

} // namespace pparse


