#include "gtest/gtest.h"

#define __PARSER_ANALYSE__
#include "parse.h"

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
 
TEST(TestAnalyze, testCycle) {

	struct Int : PTokInt<1> {};

	struct Expr;

	struct Mult : PAny<2, PTok<3,CSTR1("*")>, PTok<4,CSTR1("/")> > {};

	struct Add : PAny<5, PTok<6,CSTR1("+")>, PTok<7,CSTR1("-")> > {};

	struct NestedExpr : PSeq<8, /*PTok<0, CSTR1("(")>,*/ Expr, PTok<0, CSTR1(")")> > {};

	struct NegativeInt : PSeq<1, PTok<11, CSTR1("-")>, Int> {};

	struct SimpleExpr : PAny<6, Int, NegativeInt, NestedExpr> {}; // 

	struct MultExpr;

	struct MultExpr: PAny<7, PSeq<6, SimpleExpr, Mult, MultExpr >, SimpleExpr> {};

	struct Expr: PAny<7, PSeq<6, MultExpr, Add, Expr >, MultExpr> {};
	
		CycleDetectorHelper helper;

		helper.push_and_check(std::cout, get_tinfo((MultExpr *) nullptr), -1);

		bool is_valid = MultExpr::verify_no_cycles((void *) nullptr, helper, std::cout);

		helper.pop();

		EXPECT_FALSE(is_valid);

}

TEST(TestAnalyze, testNoCycle) {

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
	
		CycleDetectorHelper helper;

		helper.push_and_check(std::cout, get_tinfo((MultExpr *) nullptr), -1);

		bool is_valid = MultExpr::verify_no_cycles((void *) nullptr, helper, std::cout);

		helper.pop();

		EXPECT_TRUE(is_valid);

}

}//namespace


