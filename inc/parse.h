#pragma once

#include <memory>
#include <list>
#include <tuple>
#include <variant>
#include <optional>
#include <iostream>
#include <sstream>
#include <cstring>
#include "parse_text.h"
#include "parsedef.h"
#include "tokencollisionhelper.h"
#include "dhelper.h"
#include "vhelper.h"
#include "analyse.h"
#include "json.h"

namespace pparse {

struct ParserBase {

		using AstType = void;

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
		static inline Text_position current_pos_and_inc_nesting(ParserBase &parser) {
				ERROR("Not implemented\n");
				return parser.current_pos_and_inc_nesting();
		}

		template<typename ParserBase>
		static inline Text_position current_pos(ParserBase &parser) {
				ERROR("Not implemented\n");
				return parser.current_pos(); 
		}

		template<typename ParserBase>
		static inline bool backtrack(ParserBase &parser, Text_position pos) {
				ERROR("Not implemented\n");
				return parser.backtrack(parser, pos);
		}

//		static inline Text_position dec_position_nesting(ParserBase &parser) {
//				ERROR("Not implemented\n");
//				return  parser.dec_position_nesting(parser);
//		}
//


#ifdef __PARSER_ANALYSE__
		template<typename HelperType>
		static bool verify_no_cycles(HelperType *,CycleDetectorHelper &helper, std::ostream &out) {
			ERROR("shouldn't get here");	
			return true;
		}

		template<typename HelperType>
		static bool can_accept_empty_input(HelperType *) {
			ERROR("shouldn't get here");	
			return false;
		}

     	template<typename ParserBase>
        static void init_collision_checker(ParserBase &base) {
	        ERROR("shouldn't get here");	
        }
#endif

		template<typename Stream>
		static void dumpJson(Stream &out, const AstType *ast) {
				ERROR("Not implemented\n");
		}

        void init_collission_checker() {
            TokenCollisionChecker *ptr = new TokenCollisionChecker();
            colission_checker_.reset( ptr );
        }



        std::unique_ptr<TokenCollisionChecker> colission_checker_;


        //must not be a polymorphic type, is accessed from static stuff only. don't have virtual functions here.
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

		static inline Text_position current_pos_and_inc_nesting(CharParser&parser) {
				return parser.text_.pos_at_cursor_and_inc_nesting(); 
		}


		static inline Text_position dec_position_nesting(CharParser &parser) {
				return parser.text_.pos_at_cursor();
		}

		static inline bool backtrack(CharParser &parser, Text_position pos) {
				return parser.text_.backtrack(pos);
		}



#ifdef __PARSER_ANALYSE_
		template<typename HelperType>
		static bool verify_no_cycles(HelperType *,CycleDetectorHelper &helper, std::ostream &out) {
			ERROR("shouldn't get here");	
			return true;
		}

		template<typename HelperType>
		static bool can_accept_empty_input(HelperType *) {
			ERROR("shouldn't get here");	
			return false;
		}
#endif

		template<typename Stream>
		static void dumpJson(Stream &out,  const AstType *) {
		}

     	template<typename ParserBase>
        static void init_collision_checker(ParserBase &base) {
        }

private:
		Text_stream &text_;
		Text_stream::Next_char_value ch_;
};



// PTopLevelParser  - top level parser, initialises the collision checker;
// The collision checker is used to check that variables that have the string value of a token are not counted as a variable;
// the parse method initialises the collision checker and attaches it to the base parser, and after using it it is deleted.
//
template<typename Type> 
struct PTopLevelParser : ParserBase {

		using AstType = typename Type::AstType;
        using ThisClass = PTopLevelParser<Type>;

		template<typename ParserBase>
		static Parse_result  parse(ParserBase &base) {
         
                PTopLevelParser<Type>::init_collision_checker(base);
                //Type::init_collision_checker(base);
                return Type::parse(base);
        }

       	template<typename ParserBase>
        static void init_collision_checker(ParserBase &base) {

            base.init_collission_checker();
            if (base.colission_checker_->insert_type_info(&typeid(ThisClass))) { 
                Type::init_collision_checker(base);
                base.colission_checker_->remove_type_info(&typeid(ThisClass));
            }
        }
};

//
// PEof - parse is ok if at end of input
//

template<typename Type>
struct PRequireEof : PTopLevelParser<Type>  {

		static inline const RuleId RULE_ID = Type::RULE_ID;

		using AstType = typename Type::AstType;

		template<typename ParserBase>
		static Parse_result  parse(ParserBase &base) {

				Parse_result res = PTopLevelParser<Type>::parse(base);
				if (!res.success_) {
					return res;
				}

                typename ParserBase::Char_value  nchar = ParserBase::current_char(base);
				while (nchar.first && isspace(nchar.second)) { 
						ParserBase::next_char(base);
						nchar = ParserBase::current_char(base);
				}

				Text_position end_pos = ParserBase::current_pos(base);

				if (!nchar.first) {
						return res;
				}
				return Parse_result{false, Position(end_pos), Position(end_pos) };
		}

#ifdef __PARSER_ANALYSE__
		template<typename HelperType>
		static bool verify_no_cycles(HelperType *,CycleDetectorHelper &helper, std::ostream &out) {
			bool ret;
			
			helper.push_and_check(out, get_tinfo((Type *) nullptr), -1);

			ret = Type::verify_no_cycles((HelperType *) nullptr, helper, out) ;

			helper.pop();

			return ret;			
		}

		template<typename HelperType>
		static bool can_accept_empty_input(HelperType *) {
			//return Type::can_accept_empty_input<HelperType>();
			return false;
		}

	
#endif
		template<typename Stream>
		static void dumpJson(Stream &out,  const AstType *ast) {
			Type::dumpJson(out, ast);		
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
		
		static inline const RuleId RULE_ID = ruleId;

		using ThisClass = PTok<ruleId, Cs...>;

		struct AstType : AstEntryBase {
				AstType(Position start, Position end) : AstEntryBase(ruleId) {
						start_ = start;
						end_ = end;
				}
		};


		template<typename ParserBase>
		static Parse_result  parse(ParserBase &base) {

				Text_position start_pos = ParserBase::current_pos_and_inc_nesting(base);

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

#ifdef __PARSER_TRACE__
					VisualizeTrace<ThisClass>::end_parsing(short_name, false, ParserBase::current_pos(base));
#endif

					
					return Parse_result{false, Position(token_start_pos), Position(token_start_pos) };
				} else {
						ParserBase::dec_position_nesting(base);
				}


#ifdef __PARSER_TRACE__
				VisualizeTrace<ThisClass>::end_parsing(short_name, true, ParserBase::current_pos(base));
#endif


				Text_position end_pos = ParserBase::current_pos(base);

				end_pos.column_ -= 1;

				return Parse_result{true, Position(token_start_pos), Position(end_pos), std::make_unique<AstType>( Position(token_start_pos), Position(end_pos) ) };
		}

#ifdef __PARSER_ANALYSE__
		template<typename HelperType>
		static  bool verify_no_cycles(HelperType *,CycleDetectorHelper &helper, std::ostream &out) {
			return true;
		}

		template<typename HelperType>
		static bool can_accept_empty_input(HelperType *) {
			return false;
		}

		
#endif		

		template<typename Stream>
		static void dumpJson(Stream &out,  const AstType *ast) {

			Json<Stream>::dumpRule(out, RULE_ID, "PTok" );

			Json<Stream>::jsonStartNested(out, "value");
			out << "\"";
			dump_helper<Stream,Cs...>(out);
			out << "\"";

			Json<Stream>::jsonEndTag(out, true);
		}


     	template<typename ParserBase>
        static void init_collision_checker(ParserBase &base) {
            std::string sval;
            gather_token_value<Cs...>(sval);
            base.colission_checker_->insert(sval.c_str(), sval.size() );
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

		template<typename Stream, Char_t ch, Char_t ...Css>
		static inline bool dump_helper( Stream &stream) {

				stream << ch;

				if constexpr (sizeof...(Css) > 0) {
					return dump_helper<Stream, Css...>( stream );
				}

				return true;
		} 

		template<Char_t ch, Char_t ...Css>
		static inline bool gather_token_value(std::string &strval) {

				strval += ch;

				if constexpr (sizeof...(Css) > 0) {
					return dump_helper<Css...>( strval );
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


const int PTokVarCanAcceptEmptyInput = 1;
const int PTokVarCheckTokenClash = 2;

template<RuleId ruleId, PTokVar_cb_t checker, int TokVarFlags = 0>
struct PTokVar : ParserBase  { 
		
		using ThisClass = PTokVar<ruleId, checker, TokVarFlags>;

		static inline const RuleId RULE_ID = ruleId;

		struct AstType : AstEntryBase {
				AstType() : AstEntryBase(ruleId) {
				}

				std::string entry_;
		};


		template<typename ParserBase>
		static Parse_result  parse(ParserBase &base) {
			
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

									AstEntryBase *ret = ast.release();
									ret->start_ = token_start_pos;
									ret->end_ = end_pos;
 
                                    if (has_collision(base, ast.get()->entry_)) {
				                        return Parse_result{false, token_start_pos, token_start_pos};
                                    }
            						return Parse_result{true, token_start_pos, end_pos, std::unique_ptr<AstEntryBase>(ret) };	

							}

							case Char_checker_result::acceptUnget: {
									Text_position end_pos = ParserBase::current_pos(base);
									end_pos.column_ -= 1;

									AstEntryBase *ret = ast.release();
									ret->start_ = token_start_pos;
									ret->end_ = end_pos;
		    					    
                                    if (has_collision(base, ast.get()->entry_)) {
				                        return Parse_result{false, token_start_pos, token_start_pos};
                                    }
                            		return Parse_result{true, token_start_pos, end_pos, std::unique_ptr<AstEntryBase>(ret) };	
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

#ifdef __PARSER_ANALYSE__
		template<typename HelperType>
		static bool verify_no_cycles(HelperType *,CycleDetectorHelper &helper, std::ostream &out) {
			return true;
		}

		template<typename HelperType>
		static bool can_accept_empty_input(HelperType *) {
			return  TokVarFlags & PTokVarCanAcceptEmptyInput;
		}

#endif		

		template<typename Stream>
		static void dumpJson(Stream &out,  const AstType *ast) {
			Json<Stream>::dumpRule(out, RULE_ID,"PTokVar" );

			//? extract the name of the callback function from type signature ?
			Json<Stream>::jsonAddField(out, "token", ast->entry_.c_str(), true);

			Json<Stream>::jsonEndTag(out, true);
		}

     	template<typename ParserBase>
        static void init_collision_checker(ParserBase &base) {
        }

        inline static bool has_collision(ParserBase &base, std::string &entry) {

            if constexpr ((TokVarFlags & PTokVarCheckTokenClash) != 0) {
                if (base.colission_checker_ != nullptr) {

                    const Char_t *tok =  (const Char_t *) entry.c_str();
                    int len = entry.size();

                    return base.colission_checker_->has_token(tok, len);
                }
            }
            return false;
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
struct PTokChar : PTokVar<ruleId,parse_print_char, PTokVarCheckTokenClash>  { 
};

inline Char_checker_result pparse_identifier(Char_t current_char, bool iseof, Char_t *matched_so_far) {

	ssize_t slen = strlen(matched_so_far);
	if (iseof) {
		if (slen >0) {
			return Char_checker_result::acceptNow;
		}
	}

	if (slen == 0) {
		return isalpha(current_char) ? Char_checker_result::proceed : Char_checker_result::error;
	}
	return isalnum(current_char) ? Char_checker_result::proceed : Char_checker_result::acceptUnget;
}

//
// PTokIdentifier
//
template<RuleId ruleId>
struct PTokIdentifier : PTokVar<ruleId, pparse_identifier>  { 
};



//
//  PAny - ordered choice parser combinator
//

template<RuleId ruleId, typename ...Types>
struct PAny : ParserBase  {

	static inline const RuleId RULE_ID = ruleId;
		
    using ThisClass = PAny<ruleId, Types...>;

	//typedef VariantType  typename std::variant< typename std::unique_ptr<typename Types::AstType>...>;

	struct AstType : AstEntryBase {
			AstType() : AstEntryBase(ruleId) {
			}

			std::variant< typename std::unique_ptr<typename Types::AstType>...> entry_;
	};


    template<typename ParserBase>
	static Parse_result  parse(ParserBase &base) {

		Position error_pos;

#ifdef __PARSER_TRACE__
		Text_position start_pos = ParserBase::current_pos(base); 
		std::string short_name = VisualizeTrace<ThisClass>::trace_start_parsing(start_pos);
#endif

		auto ast = std::make_unique<AstType>(); 

		Parse_result res = parse_helper<0,ParserBase,Types...>(base, ast.get(), error_pos); 

#ifdef __PARSER_TRACE__
		VisualizeTrace<ThisClass>::end_parsing_choice(short_name, res.success_, ParserBase::current_pos(base), ast.get()->entry_.index());
#endif

		if (res.success_) {
			res.ast_.reset( ast.release() );
		}

		
		return res;
	}

#ifdef __PARSER_ANALYSE__

		template<typename HelperType>
		static bool verify_no_cycles(HelperType *,CycleDetectorHelper &helper, std::ostream &out) {
			return verify_no_cycles_helper<HelperType, 0, Types...>(helper, out);
		}

		template<typename HelperType>
		static bool can_accept_empty_input(HelperType *) {
			return can_accept_empty_input_helper<HelperType, Types...>();
		}

#endif		

		template<typename Stream>
		static void dumpJson(Stream &out,  const AstType *ast) {
			Json<Stream>::dumpRule(out, RULE_ID,"PAny" );
			
			dump_helper<0, Stream, Types...>(out, ast);

			Json<Stream>::jsonEndTag(out, true);
		}

     	template<typename ParserBase>
        static void init_collision_checker(ParserBase &base) {

            if (base.colission_checker_ != nullptr && base.colission_checker_->insert_type_info(&typeid(ThisClass))) { 
                init_collision_checker_helper<ParserBase, Types...>(base);
                base.colission_checker_->remove_type_info(&typeid(ThisClass));
            }
        }



private:
    template<size_t FieldIndex, typename ParserBase, typename PType, typename ...PTypes>
    static inline Parse_result parse_helper(ParserBase &base, AstType *ast, Position error_pos) {

		Parse_result res = PType::parse(base);			
		typedef std::unique_ptr<typename PType::AstType> PTypePtr; 
        
		if (res.success_) {
			if (res.ast_.get() != nullptr) {
				typename PType::AstType *retAst = (typename PType::AstType *) res.ast_.release();
				typedef typename std::variant< typename std::unique_ptr<typename Types::AstType>...> VariantType;


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


#ifdef __PARSER_ANALYSE__

		template<typename HelperType, size_t FieldIndex, typename PType, typename ...PTypes>
		static inline bool verify_no_cycles_helper(CycleDetectorHelper &helper, std::ostream &out) {
			
			if (!helper.push_and_check(out, get_tinfo((PType *) nullptr), FieldIndex)) {
				return false;
			}

			bool ret = 	PType::verify_no_cycles((HelperType *) nullptr, helper, out);
			
			helper.pop();


			if constexpr(sizeof ...(PTypes) > 0) {

				ret = ret && verify_no_cycles_helper<HelperType, FieldIndex+1,PTypes...>(helper, out);
			}
					
			return ret;
		}

		template<typename HelperType, typename PType, typename ...PTypes>
		static bool can_accept_empty_input_helper() {

			if (PType::can_accept_empty_input((HelperType *) nullptr)) {
				return true;
			}

			if constexpr(sizeof ...(PTypes) > 0) {
				return can_accept_empty_input_helper<HelperType, PTypes...>();
			}
			return false;
		}
#endif		

		template<size_t FieldIndex, typename Stream, typename PType, typename ...PTypes>
		static inline bool dump_helper( Stream &stream, const AstType *ast ) {

				std::stringstream sout;

				if (ast->entry_.index() == FieldIndex) {
					sout << FieldIndex;
					std::string sval(sout.str());


					Json<Stream>::jsonAddField(stream, "optionIndex", sval.c_str());
					Json<Stream>::jsonStartNested(stream, "data"); 

					typename PType::AstType *retAst = (typename PType::AstType *) std::get<FieldIndex>( ast->entry_ ).get();
					PType::dumpJson(stream, retAst );

					Json<Stream>::jsonEndNested(stream,true);			

				}
  
				if constexpr (sizeof...(PTypes) > 0) {
					return dump_helper<FieldIndex+1, Stream, PTypes...>( stream, ast );
				} 

				return true;
		} 

		template<typename ParserBase, typename PType, typename ...PTypes>
        static void init_collision_checker_helper(ParserBase &base) {
	
                PType::init_collision_checker(base); 

                if constexpr (sizeof...(PTypes) > 0) {
					return init_collision_checker_helper<ParserBase, PTypes...>( base );
                }
        }
 
};

//
//  POpt - optional rule parser combinator
//

template<RuleId ruleId, typename PType>
struct POpt : ParserBase  {

	static inline const RuleId RULE_ID = ruleId;
		
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

#ifdef __PARSER_TRACE__
		Text_position start_pos = ParserBase::current_pos(base); 
		std::string short_name = VisualizeTrace<ThisClass>::trace_start_parsing(start_pos);
#endif


		auto ast = std::make_unique<AstType>(); 

		Parse_result res = PType::parse(base);			

		if (res.success_) {
			typename PType::AstType *ptr = (typename PType::AstType *) res.ast_.release();
			AstType *rval = ast.get();
			rval->entry_ = OptionType(PTypePtr(ptr));
			rval->start_ = res.start_;
			rval->end_ = res.end_;
		}
		res.success_ = true;
		res.ast_.reset( ast.release() );

#ifdef __PARSER_TRACE__
		VisualizeTrace<ThisClass>::end_parsing(short_name, res.success_, ParserBase::current_pos(base));
#endif


		return res;
	}

#ifdef __PARSER_ANALYSE__
		template<typename HelperType>
		static bool verify_no_cycles(HelperType *,CycleDetectorHelper &helper, std::ostream &out) {

			helper.push_and_check(out, get_tinfo((PType *) nullptr), -1);

			bool ret = PType::verify_no_cycles(helper, out) ;

			helper.pop();

			return ret;
		}

		template<typename HelperType>
		static bool can_accept_empty_input(HelperType *) {
			return true;
		}

#endif		

		template<typename Stream>
		static void dumpJson(Stream &out,  const AstType *ast) {
			Json<Stream>::dumpRule(out, RULE_ID,"POpt" );
		
			Json<Stream>::jsonStartNested(out, "Type");
			typename PType::AstType *ptr = (typename PType::AstType *) ast;
			PType::dumpJson(out, ptr); 
			Json<Stream>::jsonEndNested(out,true);
		
			Json<Stream>::jsonEndTag(out, true);
		}

     	template<typename ParserBase>
        static void init_collision_checker(ParserBase &base) {
            if (base.colission_checker_ != nullptr && base.colission_checker_->insert_type_info(&typeid(ThisClass))) { 
                PType::init_collision_checker(base);
                base.colission_checker_->remove_type_info(&typeid(ThisClass));
            }
        }



private:

};

//
//  PSeq - sequence parser combinator
//


template<RuleId ruleId, typename ...Types>
struct PSeq : ParserBase  {

    using ThisClass = PSeq<ruleId, Types...>;
	
	static inline const RuleId RULE_ID = ruleId;
	
	struct AstType : AstEntryBase {
			AstType() : AstEntryBase(ruleId) {
			}

			std::tuple<typename std::unique_ptr<typename Types::AstType>...> entry_;
	};


    template<typename ParserBase>
	static Parse_result  parse(ParserBase &base) {


		Text_position start_pos = ParserBase::current_pos_and_inc_nesting(base);
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
			ParserBase::dec_position_nesting(base);
		}


#ifdef __PARSER_TRACE__
		VisualizeTrace<ThisClass>::end_parsing(short_name, res.success_, ParserBase::current_pos(base));
#endif


        return res;
    }

#ifdef __PARSER_ANALYSE__
		template<typename HelperType>
		static bool verify_no_cycles(HelperType *,CycleDetectorHelper &helper, std::ostream &out) {
			return verify_no_cycles_helper<HelperType, 0, Types...>(helper, out);
		}

		template<typename HelperType>
		static bool can_accept_empty_input(HelperType *) {
			return can_accept_empty_input_helper<HelperType, Types...>();
		}

#endif		
		template<typename Stream>
		static void dumpJson(Stream &out,  const AstType *ast) {
			Json<Stream>::dumpRule(out, RULE_ID,"PSeq" );
			
			Json<Stream>::jsonStartNested(out, "Type", true);
			dump_helper<0, Stream, Types...>(out, ast);
			Json<Stream>::jsonEndNested(out, true, true);

			Json<Stream>::jsonEndTag(out, true);
		}

     	template<typename ParserBase>
        static void init_collision_checker(ParserBase &base) {
            if (base.colission_checker_ != nullptr && base.colission_checker_->insert_type_info(&typeid(ThisClass))) { 
                init_collision_checker_helper<ParserBase, Types...>(base);
                base.colission_checker_->remove_type_info(&typeid(ThisClass));
            }
        }



private:

		template<size_t FieldIndex, typename Stream,  typename PType, typename ...PTypes>
		static inline bool dump_helper( Stream &stream, const AstType *ast ) {

				PType::dumpJson(stream, std::get<FieldIndex>( ast->entry_ ).get() );
	
				if constexpr (sizeof...(PTypes) > 0) {

					Json<Stream>::jsonEndNested(stream,false);

					return dump_helper<FieldIndex+1, Stream, PTypes...>( stream, ast );
				} 

				return true;
		} 
   
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

		if (res.ast_.get() != nullptr) {
			typename PType::AstType *retAst = (typename PType::AstType *) res.ast_.release();
			std::get<FieldIndex>( ast->entry_ ) = PTypePtr(retAst); 
		}
		
		if constexpr (sizeof...(PTypes) > 0) {
			return parse_helper<FieldIndex + 1, ParserBase, PTypes...>( base, ast, start_seq );
		} 

		ast->start_ = start_seq;
		ast->end_ = res.end_;
		return Parse_result{true, start_seq, res.end_};
    }


#ifdef __PARSER_ANALYSE__

		template<typename HelperType, size_t FieldIndex, typename PType, typename ...PTypes>
		static bool verify_no_cycles_helper(CycleDetectorHelper &helper, std::ostream &out) {

			if (!helper.push_and_check(out, get_tinfo((PType *) nullptr), FieldIndex)) {
				return false;
			}

			bool ret = 	PType::verify_no_cycles((HelperType *) nullptr, helper, out);

			helper.pop();

			if (PType::can_accept_empty_input((HelperType *) nullptr)) {

				if constexpr(sizeof ...(PTypes) > 0) {
					ret = ret && verify_no_cycles_helper<HelperType, FieldIndex+1,PTypes...>(helper, out);
				}
			}
					
			return ret;
		}

		template<typename HelperType, typename PType, typename ...PTypes>
		static inline bool can_accept_empty_input_helper() {

			if (PType::can_accept_empty_input((HelperType *) nullptr)) {
				if constexpr(sizeof ...(PTypes) > 0) {
					return can_accept_empty_input_helper<HelperType, PTypes...>();
				}
			}
			return true;
		}
#endif		

		template<typename ParserBase, typename PType, typename ...PTypes>
        static void init_collision_checker_helper(ParserBase &base) {
	
                PType::init_collision_checker(base); 

                if constexpr (sizeof...(PTypes) > 0) {
					return init_collision_checker_helper<ParserBase, PTypes...>( base );
				} 
        }
 
	
};


//
//  PRepeat - repetition of element parser combinators
//

template<RuleId ruleId, typename Type, int minOccurance = 0, int maxOccurance = 0>
struct PRepeat : ParserBase {

    using ThisClass = PRepeat<ruleId, Type>;

	static inline const RuleId RULE_ID = ruleId;
	
	typedef typename std::unique_ptr< typename Type::AstType> AstTypeEntry;
 
	struct AstType : AstEntryBase {
		AstType() : AstEntryBase(ruleId) {
		}

		std::list<AstTypeEntry> entry_;
	};


    template<typename ParserBase>
	static Parse_result  parse(ParserBase &base) {

		auto ast = std::make_unique<AstType>(); 
		return parse_helper(base, &ast);
	}

	template<typename Stream>
	static void dumpJson(Stream &out,  const AstType *ast) {
			Json<Stream>::dumpRule(out, RULE_ID,"PRepeat" );
			Json<Stream>::jsonAddField(out, "minOccurance", minOccurance ); 
			Json<Stream>::jsonAddField(out, "maxOccurance", minOccurance ); 
			
			Json<Stream>::jsonStartNested(out, "content");

			for(auto it=ast->entry_.begin(); it != ast->entry_.end(); ++it) {
				Type::dumpJson(out, &it->entry_);	
			}
			Json<Stream>::jsonEndNested(out,true);

			Json<Stream>::jsonEndTag(out, true);
	}

     	template<typename ParserBase>
        static void init_collision_checker(ParserBase &base) {

           if (base.colission_checker_ != nullptr && base.colission_checker_->insert_type_info(&typeid(ThisClass))) { 
               Type::init_collision_checker(base);
                base.colission_checker_->remove_type_info(&typeid(ThisClass));
            }
        }


private:
    template<typename ParserBase>
	static Parse_result  parse_helper(ParserBase &base, std::unique_ptr<AstType> *ast) {

		Text_position start_pos = ParserBase::current_pos_and_inc_nesting(base);

#ifdef __PARSER_TRACE__
		std::string short_name = VisualizeTrace<ThisClass>::trace_start_parsing(start_pos);
#endif



		for(int i = 0; i < minOccurance; ++i) {
			Parse_result res = Type::parse(base);
			if (!res.success_) {
				ParserBase::backtrack(base, start_pos);

#ifdef __PARSER_TRACE__
				VisualizeTrace<ThisClass>::end_parsing(short_name, res.success_, ParserBase::current_pos(base));
#endif


				return res;
			} 
			if (ast != nullptr) {
				typename Type::AstType *retAst = (typename Type::AstType *) res.ast_.release();
				ast->get()->entry_.push_back( AstTypeEntry( retAst ) ); 
			}
		}

		ParserBase::dec_position_nesting(base);

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

#ifdef __PARSER_TRACE__
		VisualizeTrace<ThisClass>::end_parsing(short_name, true, ParserBase::current_pos(base));
#endif

		if (ast != nullptr) {
			AstType * ret = ast->release(); 
			ret->start_ = Position(start_pos);
			ret->end_ = Position(end_pos);
			return Parse_result{true, Position(start_pos), Position(end_pos), std::unique_ptr<AstEntryBase>(ret) };
		}
		return Parse_result{true, Position(start_pos), Position(end_pos) };
	}
	
#ifdef __PARSER_ANALYSE__
		template<typename HelperType>
		static bool verify_no_cycles(HelperType *,CycleDetectorHelper &helper, std::ostream &out) {

			helper.push_and_check(out, get_tinfo((Type *) nullptr), -1);

			bool ret = Type::verify_no_cycles((HelperType *) nullptr, helper, out) ;

			helper.pop();

			return ret;			
		}

		template<typename HelperType>
		static bool can_accept_empty_input(HelperType *arg) {
			if constexpr (minOccurance  == 0) {
				return true;
			}
			return Type::can_accept_empty_input(arg);
		}
#endif		
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

    using ThisClass = PWithAndLookaheadImpl<AndOrNotLookahead, Type, LookaheadType>;
		
	static inline const RuleId RULE_ID = Type::RULE_ID;
		
	using AstType = typename Type::AstType;

    template<typename ParserBase>
	static Parse_result  parse(ParserBase &base) {


		Text_position start_pos = ParserBase::current_pos_and_inc_nesting(base);
		Parse_result res = Type::parse(base);
		if (!res.success_) {
			ParserBase::dec_position_nesting(base);
//			ParserBase::backtrack(base, start_pos);
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
			return Parse_result{false, resLookahead.start_, resLookahead.end_ };
		}
		ParserBase::backtrack(base, lookahead_start_pos);

		return res;
  	}

#ifdef __PARSER_ANALYSE__
		template<typename HelperType>
		static bool verify_no_cycles(HelperType *,CycleDetectorHelper &helper, std::ostream &out) {
			bool ret;
			
			helper.push_and_check(out, get_tinfo((Type *) nullptr), -1);

			ret = Type::verify_no_cycles((HelperType *) nullptr, helper, out) ;

			helper.pop();

			if (ret) {
				
				LookaheadType info;

				helper.push_and_check(out, get_tinfo((LookaheadType *) nullptr), -1);

				ret = Type::verify_no_cycles((HelperType *) nullptr, helper, out) ;

				helper.pop();
			}

			return ret;			
		}

		template<typename HelperType>
		static bool can_accept_empty_input(HelperType *arg) {
			return Type::can_accept_empty_input(arg);
		}

	
#endif

		template<typename Stream>
		static void dumpJson(Stream &out,  const AstType *ast) {
			Type::dumpJson(out, ast);		
		}

     	template<typename ParserBase>
        static void init_collision_checker(ParserBase &base) {

           if (base.colission_checker_ != nullptr && base.colission_checker_->insert_type_info(&typeid(ThisClass))) { 
                Type::init_collision_checker(base);
                LookaheadType::init_collision_checker(base);
                base.colission_checker_->remove_type_info(&typeid(ThisClass));
            }
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

} // namespace pparse


