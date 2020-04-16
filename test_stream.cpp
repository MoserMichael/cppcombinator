#include "gtest/gtest.h"

#define  __PARSER_TRACE__
#include "parse.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

namespace {


using namespace pparse;

template<typename Parser>
Parse_result test_string(Char_t *test_string) {
	Text_stream text_stream;
	
	bool isok = text_stream.open(-1);
	EXPECT_EQ(isok, true);

	isok = text_stream.write_tail(test_string, strlen(test_string) );
	EXPECT_EQ(isok, true);


	CharParser chparser(text_stream);
	
	Parse_result res = Parser::parse(chparser);
	
	if (!res.success()) {
		printf("Error: parser error at: line: %d column: %d text: %s\n", res.get_start_pos().line(), res.get_start_pos().column(), test_string);
	}

	return res;

}


TEST(TextStream,readCharsOneBufWithoutResize) {
	FILE *fp = fopen(__FILE__,"r");
	EXPECT_TRUE(fp != NULL);

	fseek(fp, 0, SEEK_END);
	long file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	Text_stream stream(false);
	
	bool isok = stream.open(__FILE__, (uint32_t) file_size+2);
	EXPECT_EQ(isok, true);

	for(long pos=0; ; ++pos) {
			int ch = fgetc(fp);
			if (ch == EOF) {
				ASSERT_TRUE( pos == file_size );
			}

			Text_stream::Next_char_value nch = stream.next_char();
			if (!nch.first) {
				//printf("pos %ld size %ld\n",pos, file_size);
				ASSERT_TRUE( pos == file_size );
				ASSERT_TRUE( stream.error() == 0 );
				break;
			}

			if (nch.second != ch) {
				printf("pos %ld size %ld tch %x ch %x\n", pos, file_size, nch.second, ch);
			}
			ASSERT_TRUE(nch.second == ch);

	}

	EXPECT_TRUE(errno == 0);
	EXPECT_TRUE(stream.error() == 0);


	isok = stream.close();
	EXPECT_EQ(isok, true);

	fclose(fp);
}

TEST(TextStream,readCharsOneBufWithResize) {
	FILE *fp = fopen(__FILE__,"r");
	EXPECT_TRUE(fp != NULL);

	fseek(fp, 0, SEEK_END);
	long file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	Text_stream stream;
	
	bool isok = stream.open(__FILE__, (uint32_t) file_size+2);
	EXPECT_EQ(isok, true);

	for(long pos=0; ; ++pos) {
			int ch = fgetc(fp);
			if (ch == EOF) {
				ASSERT_TRUE( pos == file_size );
			}

			Text_stream::Next_char_value nch = stream.next_char();
			if (!nch.first) {
				//printf("pos %ld size %ld\n",pos, file_size);
				ASSERT_TRUE( pos == file_size );
				ASSERT_TRUE( stream.error() == 0 );
				break;
			}

			if (nch.second != ch) {
				printf("pos %ld size %ld tch %x ch %x\n", pos, file_size, nch.second, ch);
			}
			ASSERT_TRUE(nch.second == ch);

	}

	EXPECT_TRUE(errno == 0);
	EXPECT_TRUE(stream.error() == 0);


	isok = stream.close();
	EXPECT_EQ(isok, true);

	fclose(fp);
}


TEST(TextStream,readCharsLimitedBuffer) {
	FILE *fp = fopen(__FILE__,"r");
	EXPECT_TRUE(fp != NULL);

	fseek(fp, 0, SEEK_END);
	long file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	Text_stream stream;
	
	bool isok = stream.open(__FILE__, 11);
	EXPECT_EQ(isok, true);

	for(long pos=0; ; ++pos) {
			int ch = fgetc(fp);

			if (ch == EOF) {
				ASSERT_TRUE( pos == file_size );
			}

			if (pos != 0 && pos % 10 == 0) {
				isok = stream.move_on( stream.pos_at_cursor() );
				ASSERT_TRUE( isok );
			}

			Text_stream::Next_char_value nch = stream.next_char();
			if (!nch.first) {
				//printf("pos %ld size %ld tch %x ch %x\n", pos, file_size, nch.second, ch);
				ASSERT_TRUE( pos == file_size );
				ASSERT_TRUE( stream.error() == 0 );
				break;
			}

			//printf("%c[%c]",(char) nch.second, (char) ch);

			if (nch.second != ch) {
				printf("pos %ld size %ld tch %x ch %x\n", pos, file_size, nch.second, ch);
			}
			ASSERT_TRUE(nch.second == ch);

	}

	EXPECT_TRUE(errno == 0);
	EXPECT_TRUE(stream.error() == 0);


	isok = stream.close();
	EXPECT_EQ(isok, true);

	fclose(fp);
}

TEST(TextStream,writeTest) {
	Text_stream stream;
  
	bool isok = stream.open(-1,11);
	EXPECT_EQ(isok, true);

	isok = stream.write_tail((Char_t*)"12345", 5);
	EXPECT_EQ(isok, true);

	long pos=0;
	for(; ; ++pos) {
			Text_stream::Next_char_value nch = stream.next_char();
			if (!nch.first) {
				break;
			}
			ASSERT_TRUE(nch.second == '0' + (pos+1) %  10);
	}
	ASSERT_TRUE(pos==5);
 
	isok = stream.move_on( stream.pos_at_cursor() );
	EXPECT_EQ(isok, true);

	isok = stream.write_tail((Char_t*)"6789012345", 10);
	EXPECT_EQ(isok, true);

	for(; ; ++pos) {
			Text_stream::Next_char_value nch = stream.next_char();
			if (!nch.first) {
				break;
			}
			if (nch.second != '0' + (pos+1) %  10) {
				printf("! pos %ld\n", pos);
			}
			ASSERT_TRUE(nch.second == '0' + (pos+1) %  10);
	}
	ASSERT_TRUE(pos==15);

 
}
 
TEST(TestRules,testIntParser) {
 
	using TokenIntParser = PTokInt<1>;

	auto result = test_string<TokenIntParser>((Char_t *) " \t12934 " );
	EXPECT_TRUE(result.success());

	//printf("resul pos: start(line: %d col: %d) end(line: %d col: %d)\n", result.start_.line_, result.start_.column_, result.end_.line_, result.end_.column_ );
	
	EXPECT_TRUE(result.start_ == Position(1,2));
	EXPECT_TRUE(result.end_ ==  Position(1,6));
	EXPECT_TRUE(result.get_ast()->getRuleId() ==  1);

	TokenIntParser::AstType *type = (TokenIntParser::AstType *) result.get_ast();
	//printf("res: [%s]\n", type->entry_.c_str());
	EXPECT_TRUE(type->entry_ == "12934");


	result = test_string<TokenIntParser>((Char_t *) " \t12934" );
	EXPECT_TRUE(result.success());

	//printf("resul pos: start(line: %d col: %d) end(line: %d col: %d)\n", result.start_.line_, result.start_.column_, result.end_.line_, result.end_.column_ );
	
	EXPECT_TRUE(result.start_ == Position(1,2));
	EXPECT_TRUE(result.end_ ==  Position(1,6));
	EXPECT_TRUE(result.get_ast()->getRuleId() ==  1);

	type = (TokenIntParser::AstType *) result.get_ast();
	//printf("res: [%s]\n", type->entry_.c_str());
	EXPECT_TRUE(type->entry_ == "12934");


}


TEST(TestRules,testAnyCharParser) {
 

	using TokenCharParser = PTokChar<1>;

	auto result = test_string<TokenCharParser>((Char_t *) " \tfFd" );
	EXPECT_TRUE(result.success());


	//printf("resul pos: start(line: %d col: %d) end(line: %d col: %d)\n", result.start_.line_, result.start_.column_, result.end_.line_, result.end_.column_ );
	
	EXPECT_TRUE(result.start_ == Position(1,2));
	EXPECT_TRUE(result.end_ ==  Position(1,2));
	EXPECT_TRUE(result.get_ast()->getRuleId() ==  1);

	TokenCharParser::AstType *type = (TokenCharParser::AstType *) result.get_ast();

	//printf("res: %s\n", type->entry_.c_str());
	
	EXPECT_TRUE(type->entry_ == "f");

}


TEST(TestRules,testTokenParser) {
 
	Text_stream text_stream;
  
	bool isok = text_stream.open(-1,11);
	EXPECT_EQ(isok, true);

	isok = text_stream.write_tail((Char_t*)" \tif " , 5);

	CharParser chparser(text_stream);
	
	using TokenIfParser = PTok<0, CSTR2("if")>;

	auto result = TokenIfParser::parse( chparser );
	EXPECT_TRUE(result.success());

	//printf("resul pos: start(line: %d col: %d) end(line: %d col: %d)\n", result.start_.line_, result.start_.column_, result.end_.line_, result.end_.column_ );
	
	EXPECT_TRUE(result.start_ == Position(1,2));
	EXPECT_TRUE(result.end_ ==  Position(1,3));
}

TEST(TestRules,testAnyParser) {

	Text_stream text_stream;
  
	bool isok = text_stream.open(-1,11);
	EXPECT_EQ(isok, true);

	isok = text_stream.write_tail((Char_t*)" \telse" , 7);

	using TokenIfParser = PTok<1, CSTR2("if")>;

	using TokenThenParser = PTok<2, CSTR4("then")>;

	using TokenElseParser = PTok<3, CSTR4("else")>;

	using TokenKeywdParser = PAny<4, TokenIfParser, TokenElseParser, TokenThenParser>;

	CharParser chparser(text_stream);
	
	auto result = TokenKeywdParser::parse( chparser );
	EXPECT_TRUE(result.success());

	AstEntryBase *res = result.ast_.get();
	EXPECT_TRUE( res != nullptr );
	EXPECT_TRUE( res->ruleId_ == 4 );

	TokenKeywdParser::AstType *anyAst = (TokenKeywdParser::AstType *) res;

	EXPECT_TRUE(anyAst->entry_.index() == 1);
	EXPECT_TRUE( std::get<1>(anyAst->entry_).get()->ruleId_ == 3);
}

TEST(TestRules,testOptParser) {

	using TokenIfParser = POpt<2, PTok<1, CSTR2("if")> >;
	
	Parse_result result = test_string<TokenIfParser>((Char_t *) "\t  if\n");
	EXPECT_TRUE(result.success());

	TokenIfParser::AstType *ifAst = (TokenIfParser::AstType *) result.ast_.get();
	EXPECT_TRUE(ifAst->entry_.has_value());


	result = test_string<TokenIfParser>((Char_t *) "\t  else\n");
	EXPECT_TRUE(result.success());

	ifAst = (TokenIfParser::AstType *) result.ast_.get();
	EXPECT_FALSE(ifAst->entry_.has_value());


}

TEST(TestRules,testSeqParser) {

	Text_stream text_stream;
  
	bool isok = text_stream.open(-1,11);
	EXPECT_EQ(isok, true);

	const Char_t *str = " \tif then else"; 
	isok = text_stream.write_tail(str, strlen(str));

	using TokenIfParser = PTok<1, CSTR2("if")>;

	using TokenThenParser = PTok<2, CSTR4("then")>;

	using TokenElseParser = PTok<3, CSTR4("else")>;

	using TokenKeywdParser = PSeq<4, TokenIfParser, TokenThenParser, TokenElseParser>;

	CharParser chparser(text_stream);
	
	auto result = TokenKeywdParser::parse( chparser );
	EXPECT_TRUE(result.success());

	AstEntryBase *res = result.ast_.get();
	EXPECT_TRUE( res != nullptr );
	EXPECT_TRUE( res->ruleId_ == 4 );

	TokenKeywdParser::AstType *seqAst = (TokenKeywdParser::AstType *) res;

	EXPECT_TRUE(std::get<0>(seqAst->entry_)->ruleId_ == 1);
	EXPECT_TRUE(std::get<1>(seqAst->entry_)->ruleId_ == 2);
	EXPECT_TRUE(std::get<2>(seqAst->entry_)->ruleId_ == 3);


	//printf("index %d ruleid %d\n", anyAst->entry_.index(), std::get<1>(anyAst->entry_).ruleId_ );
	
	//EXPECT_TRUE(anyAst->entry_.index() == 1);
	//EXPECT_TRUE( std::get<1>(anyAst->entry_).ruleId_ == 2);
}

TEST(TestRules,testStarParser) {

	Text_stream text_stream;
	
	bool isok = text_stream.open(-1,11);
	EXPECT_EQ(isok, true);

	const Char_t *str = " \tAA A\nA A\tAA"; 
	isok = text_stream.write_tail(str, strlen(str));

	using TokenAParser = PTok<1, CSTR1("A")>;

	using TokenStarParser = PStar<4, TokenAParser>;


	CharParser chparser(text_stream);
	
	auto result = TokenStarParser::parse( chparser );
	EXPECT_TRUE(result.success());

	AstEntryBase *res = result.ast_.get();
	EXPECT_TRUE( res != nullptr );

	EXPECT_TRUE( res->ruleId_ == 4 );

			TokenStarParser::AstType *star = (TokenStarParser::AstType *) res;
	
	for(auto &entry : star->entry_) {

		EXPECT_TRUE( entry.get()->ruleId_ == 1 );
	}

	EXPECT_TRUE( star->entry_.size() == 7 );
}

TEST(TestRules,testWithAndParser) {

	Text_stream text_stream;
	
	bool isok = text_stream.open(-1,11);
	EXPECT_EQ(isok, true);

	const Char_t *str = " \t if then"; 
	isok = text_stream.write_tail(str, strlen(str));

	using TokenIfParser = PTok<1, CSTR2("if")>;
	using TokenThenParser = PTok<2, CSTR4("then")>;
	using TokenElseParser = PTok<3, CSTR4("else")>;


	using TokenWithAndParser1 = PWithAndLookahead<TokenIfParser, TokenThenParser>;
	using TokenWithAndParser2 = PWithAndLookahead<TokenIfParser, TokenElseParser>;

	CharParser chparser(text_stream);

	auto result2 = TokenWithAndParser2::parse( chparser );
	EXPECT_FALSE(result2.success());

	auto result1 = TokenWithAndParser1::parse( chparser );
	EXPECT_TRUE(result1.success());

	AstEntryBase *res = result1.ast_.get();
	EXPECT_TRUE( res != nullptr );

	TokenWithAndParser1::AstType *ptype  = (TokenWithAndParser1::AstType *) res; 
	EXPECT_TRUE( res->ruleId_ == 1 );
}



TEST(TestGrammar, testRecursion) {

	struct Int : PTokInt<1> {};

	struct Expr;

	struct Mult : PAny<2, PTok<3,CSTR1("*")>, PTok<4,CSTR1("/")> > {};

	struct Add : PAny<5, PTok<6,CSTR1("+")>, PTok<7,CSTR1("-")> > {};

	struct NestedExpr : PSeq<8, PTok<0, CSTR1("(")>, Expr, PTok<0, CSTR1(")")> > {};

	struct NegativeInt : PSeq<1, PTok<11, CSTR1("-")>, Int> {};

	struct SimpleExpr : PAny<6, Int, NegativeInt, NestedExpr> {}; // 

	struct MultExpr;

	struct MultExpr: PAny<7, PSeq<6, SimpleExpr, Mult, MultExpr >, SimpleExpr> {};

	struct Expr;

	struct Expr: PAny<7, PSeq<6, MultExpr, Add, Expr >, MultExpr> {};
	
	struct ExprEof : PSeq<8, Expr, PEof> {};

	Parse_result result = test_string<ExprEof>((Char_t *) "-1");
	EXPECT_TRUE(result.success());

	result = test_string<ExprEof>((Char_t *) "1 * 3");
	EXPECT_TRUE(result.success());

	result = test_string<ExprEof>((Char_t *) "2 + 2 + 2");
	EXPECT_TRUE(result.success());

	result = test_string<ExprEof>((Char_t *) "2 + 2 + 2 + 1 * 3");
	EXPECT_TRUE(result.success());

	result = test_string<ExprEof>((Char_t *) "(2*3) + 5");
	EXPECT_TRUE(result.success());
}

inline Char_checker_result pparse_regex_char(Char_t current_char, bool iseof, Char_t *matched_so_far) {
	return !iseof && 
			strlen(matched_so_far) == 0  && 
			isprint((char) current_char) && 
			strchr("+-*()[].",current_char) == 0 ? Char_checker_result::acceptNow : Char_checker_result::error;
}

template<RuleId ruleId>
struct PTokRegexChar : PTokVar<ruleId, pparse_regex_char>  { 
};

TEST(TestGrammar, testRegEx) {

	struct Regex;

	struct RegexAnyChar : PTok<1, CSTR1(".") > {};

	struct RegexEscapeChar : PSeq<1, PTok<0,CSTR1("\\")>, PTokChar<1>> {};

	struct RegexCharRangeChar : PAny<1, RegexEscapeChar, PTokRegexChar<1> >  {};

	struct RegexCharRange : PSeq<1, RegexCharRangeChar, PTok<1, CSTR1("-")>, RegexCharRangeChar> {};

	struct RegexSetItems : PPlus<1, PAny<1, RegexCharRange, RegexAnyChar> > {};

	struct RegexSet : PSeq<1, PTok<0, CSTR1("[")>, RegexSetItems, PTok<0, CSTR1("]")> > {};

	struct RegexOp : PAny<1, PTok<1, CSTR1("*")>, PTok<2, CSTR1("?")>, PTok<3, CSTR1("+") > > {};

	struct RegexSetWithOp : PSeq<1, RegexSet , POpt<1, RegexOp>> {};

	struct RegexNestedWithOp : PSeq<1, PTok<1,CSTR1("(")>, Regex, PTok<1, CSTR1(")")>, POpt<1, RegexOp> > {};

	struct RegexSetItemWithOp : PSeq<1, PAny<1, RegexCharRange, RegexAnyChar, RegexCharRangeChar>, POpt<1, RegexOp> > {};

	struct Regex : PSeq<1, PAny< 1, RegexSetWithOp, RegexNestedWithOp, RegexSetItemWithOp> > {};

	struct RegexEof : PSeq<8, Regex, PEof> {};

	Parse_result result = test_string<RegexEof>((Char_t *) "ab");
	EXPECT_TRUE(result.success());

	result = test_string<RegexEof>((Char_t *) "abc123");
	EXPECT_TRUE(result.success());

	result = test_string<RegexEof>((Char_t *) "[a-z]" );
	EXPECT_TRUE(result.success());

#if 0
	result = test_string<RegexEof>((Char_t *) "123a[a-z123]+" );
	EXPECT_TRUE(result.success());

	result = test_string<RegexEof>((Char_t *) "abc([0-9]+)?" );
	EXPECT_TRUE(result.success());
#endif

}


} // namespace


int main(int argc, char *argv[]) {

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS() ? 0 : 1;
}
