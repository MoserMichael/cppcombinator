#include "gtest/gtest.h"

//enable execution trace with the next define
//#define  __PARSER_TRACE__
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
 
TEST(TestRules,testIntParser) {
 
	struct TokenIntParser : PTokInt<1> {};

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

	TokenIntParser::dumpJson(std::cout, (TokenIntParser::AstType *) result.get_ast() );
	std::cout << "\n";

	

	//printf("resul pos: start(line: %d col: %d) end(line: %d col: %d)\n", result.start_.line_, result.start_.column_, result.end_.line_, result.end_.column_ );
	
	EXPECT_TRUE(result.start_ == Position(1,2));
	EXPECT_TRUE(result.end_ ==  Position(1,6));
	EXPECT_TRUE(result.get_ast()->getRuleId() ==  1);

	type = (TokenIntParser::AstType *) result.get_ast();
	//printf("res: [%s]\n", type->entry_.c_str());
	EXPECT_TRUE(type->entry_ == "12934");


}


TEST(TestRules,testAnyCharParser) {
 

	struct TokenCharParser : PTokChar<1> {};

	auto result = test_string<TokenCharParser>((Char_t *) " \tfFd" );
	EXPECT_TRUE(result.success());
	TokenCharParser::dumpJson(std::cout, (TokenCharParser::AstType *) result.get_ast() );
	std::cout << "\n";


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
	
	struct TokenIfParser : PTok<0, CSTR2("if")> {};

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

	struct TokenIfParser : PTok<1, CSTR2("if")> {};

	struct TokenThenParser : PTok<2, CSTR4("then")> {};

	struct TokenElseParser : PTok<3, CSTR4("else")> {};

	struct TokenKeywdParser : PAny<4, TokenIfParser, TokenElseParser, TokenThenParser> {};

	CharParser chparser(text_stream);
	
	auto result = TokenKeywdParser::parse( chparser );
	EXPECT_TRUE(result.success());

	AstEntryBase *res = result.ast_.get();
	EXPECT_TRUE( res != nullptr );
	EXPECT_TRUE( res->ruleId_ == 4 );
	
	TokenKeywdParser::dumpJson(std::cout, (TokenKeywdParser::AstType *) result.get_ast() );
	std::cout << "\n";


	TokenKeywdParser::AstType *anyAst = (TokenKeywdParser::AstType *) res;

	EXPECT_TRUE(anyAst->entry_.index() == 1);
	EXPECT_TRUE( std::get<1>(anyAst->entry_).get()->ruleId_ == 3);
}

TEST(TestRules,testOptParser) {

	struct TokenIfParser : POpt<2, PTok<1, CSTR2("if")> > {};
	
	Parse_result result = test_string<TokenIfParser>((Char_t *) "\t  if\n");
	EXPECT_TRUE(result.success());

	TokenIfParser::AstType *ifAst = (TokenIfParser::AstType *) result.ast_.get();
	EXPECT_TRUE(ifAst->entry_.has_value());


	result = test_string<TokenIfParser>((Char_t *) "\t  else\n");
	EXPECT_TRUE(result.success());
	TokenIfParser::dumpJson(std::cout, (TokenIfParser::AstType *) result.get_ast() );
	std::cout << "\n";



	ifAst = (TokenIfParser::AstType *) result.ast_.get();
	EXPECT_FALSE(ifAst->entry_.has_value());


}

TEST(TestRules,testSeqParser) {

	Text_stream text_stream;
  
	bool isok = text_stream.open(-1,11);
	EXPECT_EQ(isok, true);

	const Char_t *str = " \tif then else"; 
	isok = text_stream.write_tail(str, strlen(str));

	struct TokenIfParser : PTok<1, CSTR2("if")> {};

	struct TokenThenParser : PTok<2, CSTR4("then")> {};

	struct TokenElseParser : PTok<3, CSTR4("else")> {};

	struct TokenKeywdParser : PSeq<4, TokenIfParser, TokenThenParser, TokenElseParser> {};

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

	struct TokenAParser : PTok<1, CSTR1("A")> {}; 

	struct TokenStarParser : PStar<4, TokenAParser> {};


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

	struct TokenIfParser : PTok<1, CSTR2("if")> {};
	struct TokenThenParser : PTok<2, CSTR4("then")> {};
	struct TokenElseParser : PTok<3, CSTR4("else")> {};


	struct TokenWithAndParser1 : PWithAndLookahead<TokenIfParser, TokenThenParser> {};
	struct TokenWithAndParser2 : PWithAndLookahead<TokenIfParser, TokenElseParser> {};

	CharParser chparser(text_stream);

	auto result2 = TokenWithAndParser2::parse( chparser );
	EXPECT_FALSE(result2.success());

	auto result1 = TokenWithAndParser1::parse( chparser );
	EXPECT_TRUE(result1.success());

	AstEntryBase *res = result1.ast_.get();
	EXPECT_TRUE( res != nullptr );

	//TokenWithAndParser1::AstType *ptype  = (TokenWithAndParser1::AstType *) res; 
	EXPECT_TRUE( res->ruleId_ == 1 );
}



}

