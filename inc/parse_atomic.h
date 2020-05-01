#pragma once

namespace pparse {

//
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
struct PTokChar : PTokVar<ruleId,parse_print_char>  { 
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
	return isalnum(current_char) || current_char == '_' ? Char_checker_result::proceed : Char_checker_result::acceptUnget;
}

//
// PTokIdentifier
//
template<RuleId ruleId>
struct PTokIdentifierCStyle : PTokVar<ruleId, pparse_identifier, PTokVarCheckTokenClash>  { 
};



// 
// Always acceot or reject any input
//
template<bool acceptOrReject>
struct  PAlways { 
        static inline const RuleId RULE_ID = 0;

        using ThisClass = PAlways<acceptOrReject>;

		struct AstType : AstEntryBase {
				AstType() : AstEntryBase(RULE_ID) {
				}
        };

		template<typename ParserBase>
		static Parse_result  parse(ParserBase &base) {
			Text_position token_start_pos = ParserBase::current_pos(base);
		    return Parse_result{acceptOrReject, Position(token_start_pos), Position(token_start_pos), std::make_unique<AstType>( Position(token_start_pos), Position(token_start_pos) ) };
		}

#ifdef __PARSER_ANALYSE__
		template<typename HelperType>
		static bool verify_no_cycles(HelperType *,CycleDetectorHelper &helper, std::ostream &out) {
			return true;
		}

		template<typename HelperType>
		static bool can_accept_empty_input(HelperType *) {
			return  true;
		}

#endif		


		template<typename Stream>
		static void dumpJson(Stream &out,  const AstType *ast) {
		}

     	template<typename ParserBase>
        static void init_collision_checker(ParserBase &base) {
        }

};


} // namespace pparse


