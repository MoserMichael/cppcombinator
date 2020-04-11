#pragma once

#include "parse_text.h"
#include <memory>
#include <list>
#include <tuple>
#include <variant>

namespace pparse {

typedef int RuleId;

class Position {
public:
	Position() : line_(0), column_(0) {
	}

	Position(int line, int column) : line_(line), column_(column) {
	}

	Position(Text_position &arg) : line_(arg.line_), column_(arg.column_) {
	}

	bool operator==(const Position  &arg) const {
		return arg.line_ == line_ && arg.column_ == column_;
	}


    int line_;
    int column_;
};


class AstVisitor;

class AstEntryBase {
public:
    AstEntryBase(RuleId ruleId) : ruleId_(ruleId) {
    }

    virtual ~AstEntryBase() {
    }
    
	RuleId getRuleId() const {
		return ruleId_;
	}

//	virtual void visit(AstVisitor &visitor); // = 0;
 
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

class ParserBase {
public:

		using Char_value = Text_stream::Next_char_value;

		template<class ParserBase>
		static Char_value  next_char(ParserBase &) {
				ERROR("Not implemented\n");
				return Char_value(false,' ');
		}
		
		template<class ParserBase>
		static Char_value  current_char(ParserBase &) {
				ERROR("Not implemented\n");
				return Char_value(false,' ');
		}

		template<class ParserBase>
		static Parse_result  parse(ParserBase &) {
				ERROR("Not implemented\n");
				return Parse_result{false};
		}

		template<class ParserBase>
		static inline Text_position current_pos(ParserBase &parser) {
				return parser.current_pos(); 
		}

		template<class ParserBase>
		static inline bool backtrack(ParserBase &parser, Text_position pos) {
				return  ParserBase::backtrack(parser, pos);
		}

		//virtual ~ParserBase() {}
};


class CharParser : public ParserBase {
public:

		using AstType = void;


		CharParser(Text_stream &stream) : text_(stream) {
		}

		static Char_value  current_char(CharParser &parser) {
				auto ch = parser.text_.current_char();
				return ch;
		}

		static Char_value  next_char(CharParser &parser) {
				auto ch = parser.text_.next_char();
				return ch;
		}


		static inline Text_position current_pos(CharParser &parser) {
				return parser.text_.pos_at_cursor();
		}

		static inline bool backtrack(CharParser &parser, Text_position pos) {
				return parser.text_.backtrack(pos);
		}


private:
		Text_stream &text_;
		Text_stream::Next_char_value ch_;
};

//
// Token parser 
//




#define CSTR1(str)  str[0]
#define CSTR2(str)  str[0], str[1]
#define CSTR3(str)  str[0], str[1], str[2]
#define CSTR4(str)  str[0], str[1], str[2], str[3]
#define CSTR5(str)  str[0], str[1], str[2], str[3], str[4]
#define CSTR6(str)  str[0], str[1], str[2], str[3], str[4], str[5]
#define CSTR7(str)  str[0], str[1], str[2], str[3], str[4], str[5], str[6]
#define CSTR8(str)  str[0], str[1], str[2], str[3], str[4], str[5], str[6], str[7]
#define CSTR9(str)  str[0], str[1], str[2], str[3], str[4], str[5], str[6], str[7], str[8]
#define CSTR10(str) str[0], str[1], str[2], str[3], str[4], str[5], str[6], str[7], str[8], str[9]
#define CSTR11(str) str[0], str[1], str[2], str[3], str[4], str[5], str[6], str[7], str[8], str[9], str[10]
#define CSTR12(str) str[0], str[1], str[2], str[3], str[4], str[5], str[6], str[7], str[8], str[9], str[10], str[11]



template<RuleId ruleId, Char_t ...Cs>
class PTok : public ParserBase  {
public:
		const RuleId RULE_ID = ruleId;

		class AstType : public AstEntryBase {
		public:
				AstType() : AstEntryBase(ruleId) {
				}
		};


		template<class ParserBase>
		static Parse_result  parse(ParserBase &base) {

				Text_position start_pos = ParserBase::current_pos(base);

				Char_value  nchar = ParserBase::current_char(base);
				while (nchar.first && isspace(nchar.second)) { 
						ParserBase::next_char(base);
						nchar = ParserBase::current_char(base);
				}	

				Text_position token_start_pos = ParserBase::current_pos(base);

				if (! parse_helper<ParserBase,Cs...>(base)) {
					ParserBase::backtrack(base, start_pos);
					Text_position end_pos = ParserBase::current_pos(base);
					return Parse_result{false, Position(token_start_pos), Position(end_pos) };
				}

				Text_position end_pos = ParserBase::current_pos(base);

				end_pos.column_ -= 1;

				if constexpr (ruleId == 0)
				{
					return Parse_result{true, Position(token_start_pos), Position(end_pos) };
				} 
				return Parse_result{true, Position(token_start_pos), Position(end_pos), std::make_unique<AstType>() };
		}


private:
		template<class ParserBase, Char_t ch, Char_t ...Css>
		static inline bool parse_helper( ParserBase &text ) {

				Text_stream::Text_stream::Next_char_value nchar =  ParserBase::next_char(text);

				if (!nchar.first || nchar.second != ch) {
					return false;
				}
				if constexpr (sizeof...(Css) > 0) {
					return parse_helper<ParserBase, Css...>( text );
				}

				return true;
		}

};


//
//  PAny - ordered choice parser combinator
//

template<RuleId ruleId, class ...Types>
class PAny : public ParserBase  {
public:

	const RuleId RULE_ID = ruleId;
		
	using VariantType = std::variant< std::unique_ptr<typename Types::AstType>...>;

	class AstType : public AstEntryBase {
	public:
			AstType() : AstEntryBase(ruleId) {
			}

			VariantType entry_;
	};


    template<class ParserBase>
	static Parse_result  parse(ParserBase &base) {

		Text_position start_pos = ParserBase::current_pos(base); 

		auto ast = std::make_unique<AstType>(); 

		Parse_result res = parse_helper<0,ParserBase,Types...>(base, ast.get()); 

		if (!res.success_) {
			ParserBase::backtrack(base, start_pos);
		} 

		if (res.success_) {
			res.ast_.reset( ast.release() );
		}
		// what happes with the ast entry?
		return res;
	}


private:
    template<size_t FieldIndex, class ParserBase, class PType, class ...PTypes>
    static inline Parse_result parse_helper(ParserBase &base, AstType *ast) {

		Parse_result res = PType::parse(base);			
		typedef std::unique_ptr<typename PType::AstType> PTypePtr; 
        
		if (res.success_) {
			typename PType::AstType *retAst;

			if (res.ast_.get() != nullptr) {
				typename PType::AstType *retAst = (typename PType::AstType *) res.ast_.release();
				ast->entry_ = VariantType{ std::in_place_index<FieldIndex>, PTypePtr(retAst) };
			}
			return res;
		}

		if constexpr (sizeof...(PTypes) > 0) {
			return parse_helper<FieldIndex + 1, ParserBase, PTypes...>(base, ast);
		}         
		return Parse_result{false};
    }

};


//
//  PSeq - sequence parser combinator
//


template<RuleId ruleId, class ...Types>
class PSeq : public ParserBase  {
public:

	const RuleId RULE_ID = ruleId;
	
	class AstType : public AstEntryBase {
	public:
			AstType() : AstEntryBase(ruleId) {
			}

			std::tuple<typename std::unique_ptr<typename Types::AstType>...> entry_;
	};


    template<class ParserBase>
	static Parse_result  parse(ParserBase &base) {

		Text_position start_pos = ParserBase::current_pos(base);

		auto ast = std::make_unique<AstType>(); 

		Parse_result res = parse_helper<0, ParserBase, Types...>(base, ast.get()); 

        if (!res.success_ ) {
			ParserBase::backtrack(base, start_pos);
        }

		if (res.success_) {
			res.ast_.reset( ast.release() );
		}

        return res;
    }


private:
    template<size_t FieldIndex, class ParserBase, class PType, class ...PTypes>
    static inline Parse_result parse_helper(ParserBase &base, AstType *ast ) {

		Parse_result res = PType::parse(base);						
    	typedef std::unique_ptr<typename PType::AstType> PTypePtr; 
     
		if (!res.success_) { 
			return res;
		}


		typename PType::AstType *retAst;

		if (res.ast_.get() != nullptr) {
			typename PType::AstType *retAst = (typename PType::AstType *) res.ast_.release();
			std::get<FieldIndex>( ast->entry_ ) = PTypePtr(retAst); 
		}
		
		if constexpr (sizeof...(PTypes) > 0) {
			return parse_helper<FieldIndex + 1, ParserBase, PTypes...>( base, ast );
		} 
		return Parse_result{true};
    }
};


//
//  PRepeat - repetition of element parser combinators
//

template<RuleId ruleId, class Type, int minOccurance = 0, int maxOccurance = 0>
class PRepeat : public ParserBase {
public:

	const RuleId RULE_ID = ruleId;
	
	typedef typename std::unique_ptr< typename Type::AstType> AstTypeEntry;
 
	class AstType : public AstEntryBase {
	public:
		AstType() : AstEntryBase(ruleId) {
		}

		std::list<AstTypeEntry> entry_;
	};


    template<class ParserBase>
	static Parse_result  parse(ParserBase &base) {

		bool ret;
		if constexpr (ruleId != 0) {
			auto ast = std::make_unique<AstType>(); 
			return parse_helper(base, &ast);
		} 			
		return parse_helper(base, nullptr);
	}

private:
    template<class ParserBase>
	static Parse_result  parse_helper(ParserBase &base, std::unique_ptr<AstType> *ast) {

		Text_position start_pos = ParserBase::current_pos(base);

		for(int i = 0; i < minOccurance; ++i) {
			Parse_result res = Type::parse(base);
			if (!res.success_) {
				ParserBase::backtrack(base, start_pos);
				Text_position end_pos = ParserBase::current_pos(base);
				return Parse_result{false, Position(start_pos), Position(end_pos) };
			}
			if (ast != nullptr) {
				typename Type::AstType *retAst = (typename Type::AstType *) res.ast_.release();
				ast->get()->entry_.push_back( AstTypeEntry( retAst ) ); 
			}
		}
		for(int i = minOccurance; ; ++i ) {

			Parse_result res = Type::parse(base);
			if (!res.success_) {
				break;
			}
			if (maxOccurance != 0 && i >= maxOccurance) {
				break;
			}
			if (ast != nullptr) {
				typename Type::AstType *retAst = (typename Type::AstType *) res.ast_.release();
				ast->get()->entry_.push_back( AstTypeEntry( retAst ) ); 
			}
		}
		Text_position end_pos = ParserBase::current_pos(base);

		if (ast != nullptr) {
			return Parse_result{true, Position(start_pos), Position(end_pos), std::unique_ptr<AstEntryBase>(ast->release()) };
		}

		return Parse_result{true, Position(start_pos), Position(end_pos) };
	}
	
};

//
//  PStar - zero or more parser combinators
//


template<RuleId ruleId, class Type>
class PStar : public PRepeat<ruleId, Type, 0, 0> {
};

//
//  PPlus - zero or more parser combinator
//

template<RuleId ruleId, class Type>
class PPlus : public PRepeat<ruleId, Type, 0, 0> {
};

//
// PWithAndLookaheadImpl - implementation class for lookahead parsers (don't use directly)
//

template<bool AndOrNotLookahead, class Type, class LookaheadType>
class PWithAndLookaheadImpl : public ParserBase {
public:
	const RuleId RULE_ID = Type::RULE_ID;
		
	class AstType : public Type::AstType {
	};

    template<class ParserBase>
	static Parse_result  parse(ParserBase &base) {

		Text_position start_pos = ParserBase::current_pos(base);
		Parse_result res = Type::parse(base);
		if (!res.success_) {
			ParserBase::backtrack(base, start_pos);
			Text_position end_pos = ParserBase::current_pos(base);
			return Parse_result{false, Position(start_pos), Position(end_pos) };
		}

		Text_position lookahead_start_pos = ParserBase::current_pos(base);
		Parse_result resLookahead = LookaheadType::parse(base);

		bool isfail;

		if constexpr(AndOrNotLookahead) {
			isfail = !resLookahead.success_;
		} else {
			isfail = resLookahead.success_;
		}

		if (isfail) {
			ParserBase::backtrack(base, start_pos);
			Text_position end_pos = ParserBase::current_pos(base);
			return Parse_result{false, Position(start_pos), Position(end_pos) };
		}
		ParserBase::backtrack(base, lookahead_start_pos);

		return res;
  	}
};


//
// PWithAndLookahead -  
//

template<class Type, class LookaheadType>
class PWithAndLookahead : public PWithAndLookaheadImpl<true, Type, LookaheadType> {
};

//
// PWithNotLookahead -  
//

template<class Type, class LookaheadType>
class PWithNotLookahead: public PWithAndLookaheadImpl<false, Type, LookaheadType> {
};

//
//class AstVisitor {
//	virtual ~AstVisitor() {}
//
//	virtual void visitID(AstEntryBase & ) = 0;
//
//};
//
}; // namespace pparse

#if 0

class PRegEx : public ParserBase {

};

#endif
