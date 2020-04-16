#pragma once

#include "parse_text.h"
#include <memory>
#include <list>
#include <tuple>
#include <variant>
#include "parsedef.h"
#include "dhelper.h"


namespace pparse {


struct ParserBase {

		using Char_value = Text_stream::Next_char_value;

		template<typename ParserBase>
		static Char_value  next_char(ParserBase &) {
				ERROR("Not implemented\n");
				return Char_value(false,' ');
		}
		
		template<typename ParserBase>
		static Char_value  current_char(ParserBase &) {
				ERROR("Not implemented\n");
				return Char_value(false,' ');
		}

		template<typename ParserBase>
		static Parse_result  parse(ParserBase &) {
				ERROR("Not implemented\n");
				return Parse_result{false};
		}

		template<typename ParserBase>
		static inline Text_position current_pos(ParserBase &parser) {
				return parser.current_pos(); 
		}

		template<typename ParserBase>
		static inline bool backtrack(ParserBase &parser, Text_position pos) {
				return  ParserBase::backtrack(parser, pos);
		}

		//virtual ~ParserBase() {}
};


struct CharParser : ParserBase {

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
// PEof - parse is ok if at end of input
//

struct PEof : ParserBase  {

		struct AstType : AstEntryBase {
				AstType() : AstEntryBase(-1) {
				}
		};


		template<typename ParserBase>
		static Parse_result  parse(ParserBase &base) {

				Text_position start_pos = ParserBase::current_pos(base);

				Char_value  nchar = ParserBase::current_char(base);
				while (nchar.first && isspace(nchar.second)) { 
						ParserBase::next_char(base);
						nchar = ParserBase::current_char(base);
				}

				Text_position end_pos = ParserBase::current_pos(base);

				if (!nchar.first) {
						return Parse_result{true, Position(end_pos), Position(end_pos) };
				}

				ParserBase::backtrack(base, start_pos);
				return Parse_result{false, Position(end_pos), Position(end_pos) };
		}
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
struct PTok : ParserBase  {
		
		const RuleId RULE_ID = ruleId;

		using ThisClass = PTok<ruleId, Cs...>;

		struct AstType : AstEntryBase {
				AstType() : AstEntryBase(ruleId) {
				}
		};


		template<typename ParserBase>
		static Parse_result  parse(ParserBase &base) {

				Text_position start_pos = ParserBase::current_pos(base);

				Char_value  nchar = ParserBase::current_char(base);
				while (nchar.first && isspace(nchar.second)) { 
						ParserBase::next_char(base);
						nchar = ParserBase::current_char(base);
				}	

				Text_position token_start_pos = ParserBase::current_pos(base);

#ifdef __PARSER_TRACE__
				std::string short_name = VisualizeTrace<ThisClass>::trace_start_parsing_token(token_start_pos);
#endif


				if (! parse_helper<ParserBase,Cs...>(base)) {
					ParserBase::backtrack(base, start_pos);
					Text_position end_pos = ParserBase::current_pos(base);

#ifdef __PARSER_TRACE__
					VisualizeTrace<ThisClass>::end_parsing(short_name, false, ParserBase::current_pos(base));
#endif

					
					return Parse_result{false, Position(token_start_pos), Position(token_start_pos) };
				}

#ifdef __PARSER_TRACE__
				VisualizeTrace<ThisClass>::end_parsing(short_name, true, ParserBase::current_pos(base));
#endif


				Text_position end_pos = ParserBase::current_pos(base);

				end_pos.column_ -= 1;

				if constexpr (ruleId == 0)
				{
					return Parse_result{true, Position(token_start_pos), Position(end_pos) };
				} 
				return Parse_result{true, Position(token_start_pos), Position(end_pos), std::make_unique<AstType>() };
		}


private:
		template<typename ParserBase, Char_t ch, Char_t ...Css>
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
// PTokFunc - token parser where token characters are structified by callback function
//

enum class Char_checker_result {
	error,
	proceed,
	acceptNow,
	acceptUnget,
};

typedef Char_checker_result (PTokVar_cb_t) (Char_t current_char, bool iseof, Char_t *matched_so_far);


template<RuleId ruleId, PTokVar_cb_t checker>
struct PTokVar : ParserBase  { 
		
		const RuleId RULE_ID = ruleId;

		struct AstType : AstEntryBase {
				AstType() : AstEntryBase(ruleId) {
				}

				std::string entry_;
		};


		template<typename ParserBase>
		static Parse_result  parse(ParserBase &base) {
			
				Text_position start_pos = ParserBase::current_pos(base);

				Char_value  nchar = ParserBase::current_char(base);
				while (nchar.first && isspace(nchar.second)) { 
						ParserBase::next_char(base);
						nchar = ParserBase::current_char(base);
				}	

				Text_position token_start_pos = ParserBase::current_pos(base);

				auto ast = std::make_unique<AstType>();
				while( nchar.first ) {

						nchar = ParserBase::current_char(base);
						Char_checker_result res = checker(nchar.second, !nchar.first, (Char_t *) ast.get()->entry_.c_str());
						switch(res) {

							case Char_checker_result::proceed:
									ast.get()->entry_ += (char) nchar.second;
									break;

							case Char_checker_result::error:
									token_start_pos = ParserBase::current_pos(base);	
									return Parse_result{false, token_start_pos, token_start_pos };	

							case Char_checker_result::acceptNow: {
									ast.get()->entry_ += (char) nchar.second;
									Text_position end_pos = ParserBase::current_pos(base);
									ParserBase::next_char(base);
									return Parse_result{true, token_start_pos, end_pos, std::unique_ptr<AstEntryBase>(ast.release()) };	
							}

							case Char_checker_result::acceptUnget: {
									Text_position end_pos = ParserBase::current_pos(base);
									end_pos.column_ -= 1;

									return Parse_result{true, token_start_pos, end_pos, std::unique_ptr<AstEntryBase>(ast.release()) };	
							}

						}

						if (!nchar.first) {
							ERROR("eof not handled by PTokVar callback\n");
							break;
						}

						nchar = ParserBase::next_char(base);
				}
		
				Text_position end_pos = ParserBase::current_pos(base);

				end_pos.column_ -= 1;

				return Parse_result{false, token_start_pos, token_start_pos};
		}
};

inline Char_checker_result pparse_is_digit(Char_t current_char, bool iseof, Char_t *matched_so_far) {

	if (!iseof && isdigit((char) current_char)) {
		return Char_checker_result::proceed;
	}
	if (strlen(matched_so_far) == 0) {
		return Char_checker_result::error;
	}
	return Char_checker_result::acceptUnget;
}

// 
// PTokInt - sequence of digites
//
template<RuleId ruleId>
struct PTokInt : PTokVar<ruleId, pparse_is_digit>  { 
};


inline Char_checker_result parse_print_char(Char_t current_char, bool iseof, Char_t *matched_so_far) {
	return !iseof && strlen(matched_so_far) == 0 && isprint((char) current_char) ? Char_checker_result::acceptNow : Char_checker_result::error;
}

// 
// PTokPrintChar - single printable character
//
template<RuleId ruleId>
struct PTokChar : PTokVar<ruleId,parse_print_char>  { 
};



//inline bool pparse_regex_char(Char_t current_char, Char_t *matched_so_far) {
//	return isprint((char) current_char) && strchr(current_char,"+-*()") == 0;
//}
//

// 
// PTokInt - sequence of digites
//
//template<RuleId ruleId>
//struct PTokRegexChar : PTokVar<ruleId, pparse_regex_char>  { 
//};
//


//
//  PAny - ordered choice parser combinator
//

template<RuleId ruleId, typename ...Types>
struct PAny : ParserBase  {

	const RuleId RULE_ID = ruleId;
		
    using ThisClass = PAny<ruleId, Types...>;

	using VariantType = std::variant< std::unique_ptr<typename Types::AstType>...>;

	struct AstType : AstEntryBase {
			AstType() : AstEntryBase(ruleId) {
			}

			VariantType entry_;
	};


    template<typename ParserBase>
	static Parse_result  parse(ParserBase &base) {

		Text_position start_pos = ParserBase::current_pos(base); 
		Position error_pos;

#ifdef __PARSER_TRACE__
		std::string short_name = VisualizeTrace<ThisClass>::trace_start_parsing(start_pos);
#endif

		auto ast = std::make_unique<AstType>(); 

		Parse_result res = parse_helper<0,ParserBase,Types...>(base, ast.get(), error_pos); 

		if (!res.success_) {
			ParserBase::backtrack(base, start_pos);
		} 

#ifdef __PARSER_TRACE__
		VisualizeTrace<ThisClass>::end_parsing_choice(short_name, res.success_, ParserBase::current_pos(base), ast.get()->entry_.index());
#endif

		if (res.success_) {
			res.ast_.reset( ast.release() );
		}

		
		return res;
	}


private:
    template<size_t FieldIndex, typename ParserBase, typename PType, typename ...PTypes>
    static inline Parse_result parse_helper(ParserBase &base, AstType *ast, Position error_pos) {

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

		if (!res.success_) {
			if (error_pos < res.start_) {
				error_pos = res.start_;
			}
		}

		if constexpr (sizeof...(PTypes) > 0) {
			return parse_helper<FieldIndex + 1, ParserBase, PTypes...>(base, ast, error_pos);
		}         
		return Parse_result{false, error_pos, error_pos};
    }

};

//
//  POpt - optional rule parser combinator
//

template<RuleId ruleId, typename PType>
struct POpt : ParserBase  {

	const RuleId RULE_ID = ruleId;
		
    using ThisClass = POpt<ruleId, PType>;
		
	using PTypePtr = typename std::unique_ptr<typename PType::AstType>;  

	using OptionType = std::optional< PTypePtr >;

	struct AstType : AstEntryBase {
			AstType() : AstEntryBase(ruleId) {
			}

			OptionType entry_;
	};


    template<typename ParserBase>
	static Parse_result  parse(ParserBase &base) {

		Text_position start_pos = ParserBase::current_pos(base); 

#ifdef __PARSER_TRACE__
		std::string short_name = VisualizeTrace<ThisClass>::trace_start_parsing(start_pos);
#endif


		auto ast = std::make_unique<AstType>(); 

		Parse_result res = PType::parse(base);			

		if (res.success_) {
			typename PType::AstType *ptr = (typename PType::AstType *) res.ast_.release();
			AstType *rval = ast.get();
			rval->entry_ = OptionType(PTypePtr(ptr));
		}
		if (!res.success_) {
			ParserBase::backtrack(base, start_pos);
			res.end_ = res.start_ = start_pos;
		} 

		res.success_ = true;
		res.ast_.reset( ast.release() );

#ifdef __PARSER_TRACE__
		VisualizeTrace<ThisClass>::end_parsing(short_name, res.success_, ParserBase::current_pos(base));
#endif


		return res;
	}
};

//
//  PSeq - sequence parser combinator
//


template<RuleId ruleId, typename ...Types>
struct PSeq : ParserBase  {

    using ThisClass = PSeq<ruleId, Types...>;
	
	const RuleId RULE_ID = ruleId;
	
	struct AstType : AstEntryBase {
			AstType() : AstEntryBase(ruleId) {
			}

			std::tuple<typename std::unique_ptr<typename Types::AstType>...> entry_;
	};


    template<typename ParserBase>
	static Parse_result  parse(ParserBase &base) {


		Text_position start_pos = ParserBase::current_pos(base);
		Position start_seq;

#ifdef __PARSER_TRACE__
		std::string short_name = VisualizeTrace<ThisClass>::trace_start_parsing(start_pos);
#endif

		auto ast = std::make_unique<AstType>(); 

		Parse_result res = parse_helper<0, ParserBase, Types...>(base, ast.get(), start_seq); 

        if (!res.success_ ) {
			ParserBase::backtrack(base, start_pos);
        }

		if (res.success_) {
			res.ast_.reset( ast.release() );
		}


#ifdef __PARSER_TRACE__
		VisualizeTrace<ThisClass>::end_parsing(short_name, res.success_, ParserBase::current_pos(base));
#endif


        return res;
    }


private:
    template<size_t FieldIndex, typename ParserBase, typename PType, typename ...PTypes>
    static inline Parse_result parse_helper(ParserBase &base, AstType *ast, Position start_seq ) {

		Parse_result res = PType::parse(base);						
    	typedef std::unique_ptr<typename PType::AstType> PTypePtr; 
     
		if (!res.success_) { 
			return res;
		}

		if constexpr (FieldIndex == 0) {
			start_seq = res.start_;
		}

		typename PType::AstType *retAst;

		if (res.ast_.get() != nullptr) {
			typename PType::AstType *retAst = (typename PType::AstType *) res.ast_.release();
			std::get<FieldIndex>( ast->entry_ ) = PTypePtr(retAst); 
		}
		
		if constexpr (sizeof...(PTypes) > 0) {
			return parse_helper<FieldIndex + 1, ParserBase, PTypes...>( base, ast, start_seq );
		} 

		return Parse_result{true, start_seq, res.end_};
    }
};


//
//  PRepeat - repetition of element parser combinators
//

template<RuleId ruleId, typename Type, int minOccurance = 0, int maxOccurance = 0>
struct PRepeat : ParserBase {

	const RuleId RULE_ID = ruleId;
	
	typedef typename std::unique_ptr< typename Type::AstType> AstTypeEntry;
 
	struct AstType : AstEntryBase {
		AstType() : AstEntryBase(ruleId) {
		}

		std::list<AstTypeEntry> entry_;
	};


    template<typename ParserBase>
	static Parse_result  parse(ParserBase &base) {

		bool ret;
		if constexpr (ruleId != 0) {
			auto ast = std::make_unique<AstType>(); 
			return parse_helper(base, &ast);
		} 			
		return parse_helper(base, nullptr);
	}

private:
    template<typename ParserBase>
	static Parse_result  parse_helper(ParserBase &base, std::unique_ptr<AstType> *ast) {

		Text_position start_pos = ParserBase::current_pos(base);

		for(int i = 0; i < minOccurance; ++i) {
			Parse_result res = Type::parse(base);
			if (!res.success_) {
				ParserBase::backtrack(base, start_pos);
				Text_position end_pos = ParserBase::current_pos(base);
				return res;
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


template<RuleId ruleId, typename Type>
struct PStar : PRepeat<ruleId, Type, 0, 0> {
};

//
//  PPlus - zero or more parser combinator
//

template<RuleId ruleId, typename Type>
struct PPlus : PRepeat<ruleId, Type, 0, 0> {
};


//
// PWithAndLookaheadImpl - implementation struct for lookahead parsers (don't use directly)
//

template<bool AndOrNotLookahead, typename Type, typename LookaheadType>
struct PWithAndLookaheadImpl : ParserBase {
	const RuleId RULE_ID = Type::RULE_ID;
		
	struct AstType : Type::AstType {
	};

    template<typename ParserBase>
	static Parse_result  parse(ParserBase &base) {


		Text_position start_pos = ParserBase::current_pos(base);
		Parse_result res = Type::parse(base);
		if (!res.success_) {
			ParserBase::backtrack(base, start_pos);
			return res;
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
			return Parse_result{false, resLookahead.start_, resLookahead.end_ };
		}
		ParserBase::backtrack(base, lookahead_start_pos);

		return res;
  	}
};


//
// PWithAndLookahead -  
//

template<typename Type, typename LookaheadType>
struct PWithAndLookahead :  PWithAndLookaheadImpl<true, Type, LookaheadType> {
};

//
// PWithNotLookahead -  
//

template<typename Type, typename LookaheadType>
struct PWithNotLookahead: PWithAndLookaheadImpl<false, Type, LookaheadType> {
};

}; // namespace pparse


