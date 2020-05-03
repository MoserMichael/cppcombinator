#include "gtest/gtest.h"

//enable execution trace with the next define
//#define  __PARSER_TRACE__
#include "parse.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>


namespace {

using namespace pparse;

bool runJq() {
	int rt = system("which jq");
	if (rt == 0) {
		int rcode = system("jq . <test.json");
		return rcode == 0;
	} else {
	    rt = system("cat test.json");
        if (rt != 0) {
            printf("can't run cat\n");;
        }
	}
	return true;
}


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
	EXPECT_TRUE(res.get_ast() != nullptr);

	return res;

}



TEST(TestGrammar, testRecursion) {

	struct Int : PTokInt<1> {};

	struct Expr;

	struct Mult : PAny<2, PTok<3,CSTR1("*")>, PTok<4,CSTR1("/")> > {};

	struct Add : PAny<4, PTok<5,CSTR1("+")>, PTok<6,CSTR1("-")> > {};

	struct NestedExpr : PSeq<7, PTok<8, CSTR1("(")>, Expr, PTok<9, CSTR1(")")> > {};

	struct NegativeInt : PSeq<10, PTok<11, CSTR1("-")>, Int> {};

	struct SimpleExpr : PAny<12, Int, NegativeInt, NestedExpr> {}; // 

	struct MultExpr;

	struct MultExpr: PAny<13, PSeq<14, SimpleExpr, Mult, MultExpr >, SimpleExpr> {};

	struct Expr;

	struct Expr: PAny<15, PSeq<16, MultExpr, Add, Expr >, MultExpr> {};
	
	struct ExprEof : PRequireEof<Expr> {};

	Parse_result result = test_string<ExprEof>((Char_t *) "-1");
	EXPECT_TRUE(result.success());
//
//	if (result.get_ast() != nullptr ) {
//
//		
//		ExprEof::dumpJson(std::cout, (ExprEof::AstType *) result.get_ast() );
//	}
//
	result = test_string<ExprEof>((Char_t *) "1 * 3");
	EXPECT_TRUE(result.success());

//	if (result.get_ast() != nullptr ) {
//		ExprEof::dumpJson(std::cout, (ExprEof::AstType *) result.get_ast() );
//	}
//

	result = test_string<ExprEof>((Char_t *) "2 + 2 + 2");
	EXPECT_TRUE(result.success());

	if (result.get_ast() != nullptr ) {
		std::ofstream out("test.json");
//		ExprEof::dumpJson(out, (ExprEof::AstType *) result.get_ast() );
		out.close();
//		int rcode = system("jq . <test.json");
//		EXPECT_TRUE(rcode == 0);

	}


	result = test_string<ExprEof>((Char_t *) "2 + 2 + 2 + 1 * 3");
	EXPECT_TRUE(result.success());

	if (result.get_ast() != nullptr ) {
		std::ofstream out("test.json");
		ExprEof::dumpJson(out, (ExprEof::AstType *) result.get_ast() );
		out.close();
		bool rt = runJq();
		EXPECT_TRUE(rt);
	}


	result = test_string<ExprEof>((Char_t *) "(2*3) + 5");
	EXPECT_TRUE(result.success());

	if (result.get_ast() != nullptr ) {
		std::ofstream out("test.json");
		ExprEof::dumpJson(out, (ExprEof::AstType *) result.get_ast() );
		out.close();
		bool rt = runJq();
		EXPECT_TRUE(rt);
	}

}

inline Char_checker_result pparse_regex_char(Char_t current_char, bool iseof, std::string &matched_so_far) {
	return !iseof && 
			matched_so_far.size() == 0  && 
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

	struct Regex : PStar<1, PAny< 1, RegexSetWithOp, RegexNestedWithOp, RegexSetItemWithOp> > {};

	struct RegexEof :  PRequireEof<Regex> {};

	Parse_result result = test_string<RegexEof>((Char_t *) "ab");
	EXPECT_TRUE(result.success());

	result = test_string<RegexEof>((Char_t *) "abc123");
	EXPECT_TRUE(result.success());

	result = test_string<RegexEof>((Char_t *) "[a-z]" );
	EXPECT_TRUE(result.success());

	result = test_string<RegexEof>((Char_t *) "abc[a-z]+" );
	EXPECT_TRUE(result.success());

	result = test_string<RegexEof>((Char_t *) "abc([0-9]+)?" );
	EXPECT_TRUE(result.success());

}

}
