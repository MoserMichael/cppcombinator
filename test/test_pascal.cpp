#include "gtest/gtest.h"

//enable execution trace with the next define
//#define  __PARSER_TRACE__
#include "parse.h"

#include <sys/stat.h>
#include <fcntl.h>

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
 
[[maybe_unused]] inline Char_checker_result pparse_pascal_identifier(Char_t current_char, bool iseof, std::string &matched_so_far) {

	ssize_t slen = matched_so_far.size();
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

template<RuleId ruleId>
struct PascalIdentifier : PTokVar<ruleId, pparse_pascal_identifier>  { 
};


[[maybe_unused]] inline Char_checker_result pparse_pascal_string(Char_t current_char, bool iseof, std::string &matched_so_far) {
	
	ssize_t slen = matched_so_far.size();

	if (iseof) {
		return Char_checker_result::error; 
	}

	if (slen == 0) {
		if (current_char == '\'' ) {
			return Char_checker_result::proceed;
		} 
		return Char_checker_result::error; 
	
	} 
	if (current_char == '\'' ) {
		return Char_checker_result::acceptNow;
	}
	return Char_checker_result::proceed;
}

template<RuleId ruleId>
struct PascalStringConst : PTokVar<ruleId, pparse_pascal_string>  { 
};


TEST(TestPascal, test) {
	
   //*** https://condor.depaul.edu/ichu/csc447/notes/wk2/pascal.html ***
        
	struct Fraction :  PSeq<3, PTok<4,CSTR1(".")>, PTokInt<2> > {}; 
	
	struct Exponent  : PSeq<4, PTok<5,CSTR1("E")>, POpt<4, PAny<5, PTok<6, CSTR1("-")>, PTok<6, CSTR1("+")> > >, PTokInt<7> >  {};

	struct UnsignedNumber : PSeq<1, PTokInt<2>, POpt<2, Fraction>, POpt<3, Exponent> >  {};

	struct UnsignedConstant : PAny<1, PascalStringConst<1>, UnsignedNumber, PTok<7, CSTR3("NIL")>, PascalIdentifier<1> > {};

	struct SimpleType : PAny<1, PascalIdentifier<2>, PSeq<3, PTok<4,CSTR1("(")>, PascalIdentifier<4>, PTok<5, CSTR1(")") > > > {};

	struct ConstNum : PSeq<1, PAny<2, POpt<3, PAny<3, PTok<4,CSTR1("+")>, PTok<4,CSTR1("-")> > >, PAny<6, UnsignedNumber, UnsignedConstant>  >  > {};

	struct Constant : PAny<1, PascalStringConst<2>,  ConstNum > {}; 

	struct PointerType : PSeq<1, PTok<2, CSTR1("^")>, PascalIdentifier<3> > {};

	struct Type;

	struct SimpleTypeList;

	struct SimpleTypeList : PAny<1, PSeq<1, SimpleType,  PTok<2, CSTR1(",")> , SimpleTypeList >, SimpleType > {};

	struct ArrayType : PSeq<1, PTok<2, CSTR5("array")>,  PTok<2, CSTR1("[")>, SimpleTypeList, PTok<4,CSTR1("]")>, PTok<5,CSTR2("of")>, Type > {};
 
	struct FileType : PSeq<1 , PTok<2, CSTR4("file")>, PTok<3, CSTR2("of")>, Type > {};

	struct SetType : PSeq<1 , PTok<2, CSTR3("set")>, PTok<3, CSTR2("of")>, SimpleType > {};

/*	
	struct FieldList : 

	struct RecordType : PSeq<1 , PTok<2, CSTR6("record")>, FieldList, PTok<3, CSTR3("end")> > {};
*/

	struct Type : PAny<1, PascalIdentifier<2>,  PointerType, ArrayType, FileType, SetType > {};  

}

}
