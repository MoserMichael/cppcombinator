#include "gtest/gtest.h"
#include "parse.h"
#include <sys/stat.h>
#include <fcntl.h>

namespace {

TEST(TextStream,readCharsOneBufWithoutResize) {
	FILE *fp = fopen(__FILE__,"r");
	EXPECT_TRUE(fp != NULL);

	fseek(fp, 0, SEEK_END);
	long file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	pparse::Text_stream stream(false);
	
	bool isok = stream.open(__FILE__, (uint32_t) file_size+2);
	EXPECT_EQ(isok, true);

	for(long pos=0; ; ++pos) {
			int ch = fgetc(fp);
			if (ch == EOF) {
				ASSERT_TRUE( pos == file_size );
			}

			pparse::Text_stream::Next_char_value nch = stream.next_char();
			if (!nch.first) {
				printf("pos %ld size %ld\n",pos, file_size);
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

	pparse::Text_stream stream;
	
	bool isok = stream.open(__FILE__, (uint32_t) file_size+2);
	EXPECT_EQ(isok, true);

	for(long pos=0; ; ++pos) {
			int ch = fgetc(fp);
			if (ch == EOF) {
				ASSERT_TRUE( pos == file_size );
			}

			pparse::Text_stream::Next_char_value nch = stream.next_char();
			if (!nch.first) {
				printf("pos %ld size %ld\n",pos, file_size);
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

	pparse::Text_stream stream;
	
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

			pparse::Text_stream::Next_char_value nch = stream.next_char();
			if (!nch.first) {
				printf("pos %ld size %ld tch %x ch %x\n", pos, file_size, nch.second, ch);
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
	pparse::Text_stream stream;
  
	bool isok = stream.open(-1,11);
	EXPECT_EQ(isok, true);

	isok = stream.write_tail((pparse::Char_t*)"12345", 5);
	EXPECT_EQ(isok, true);

	long pos=0;
	for(; ; ++pos) {
			pparse::Text_stream::Next_char_value nch = stream.next_char();
			if (!nch.first) {
				break;
			}
			ASSERT_TRUE(nch.second == '0' + (pos+1) %  10);
	}
	ASSERT_TRUE(pos==5);
 
	isok = stream.move_on( stream.pos_at_cursor() );
	EXPECT_EQ(isok, true);

	isok = stream.write_tail((pparse::Char_t*)"6789012345", 10);
	EXPECT_EQ(isok, true);

	for(; ; ++pos) {
			pparse::Text_stream::Next_char_value nch = stream.next_char();
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
 
TEST(Parser,testTokenParser) {
 
	pparse::Text_stream text_stream;
  
	bool isok = text_stream.open(-1,11);
	EXPECT_EQ(isok, true);

	isok = text_stream.write_tail((pparse::Char_t*)" \tif " , 5);

	pparse::CharParser chparser(text_stream);
	
	using TokenIfParser = pparse::PTok<0, CSTR2("if")>;

	auto result = TokenIfParser::parse( chparser );
	EXPECT_TRUE(result.success());

	//printf("resul pos: start(line: %d col: %d) end(line: %d col: %d)\n", result.start_.line_, result.start_.column_, result.end_.line_, result.end_.column_ );
	
	EXPECT_TRUE(result.start_ == pparse::Position(1,2));
	EXPECT_TRUE(result.end_ ==  pparse::Position(1,3));
}

TEST(Parser,testAnyParser) {

	pparse::Text_stream text_stream;
  
	bool isok = text_stream.open(-1,11);
	EXPECT_EQ(isok, true);

	isok = text_stream.write_tail((pparse::Char_t*)" \telse" , 7);

	using TokenIfParser = pparse::PTok<1, CSTR2("if")>;

	using TokenThenParser = pparse::PTok<2, CSTR4("then")>;

	using TokenElseParser = pparse::PTok<3, CSTR4("else")>;

	using TokenKeywdParser = pparse::PAny<4, TokenIfParser, TokenElseParser, TokenThenParser>;

	pparse::CharParser chparser(text_stream);
	
	auto result = TokenKeywdParser::parse( chparser );
	EXPECT_TRUE(result.success());

	pparse::AstEntryBase *res = result.ast_.get();
	EXPECT_TRUE( res != nullptr );
	EXPECT_TRUE( res->ruleId_ == 4 );

	TokenKeywdParser::AstType *anyAst = (TokenKeywdParser::AstType *) res;

	EXPECT_TRUE(anyAst->entry_.index() == 1);
	EXPECT_TRUE( std::get<1>(anyAst->entry_).get()->ruleId_ == 3);
}

TEST(Parser,testSeqParser) {

	pparse::Text_stream text_stream;
  
	bool isok = text_stream.open(-1,11);
	EXPECT_EQ(isok, true);

	const pparse::Char_t *str = " \tif then else"; 
	isok = text_stream.write_tail(str, strlen(str));

	using TokenIfParser = pparse::PTok<1, CSTR2("if")>;

	using TokenThenParser = pparse::PTok<2, CSTR4("then")>;

	using TokenElseParser = pparse::PTok<3, CSTR4("else")>;

	using TokenKeywdParser = pparse::PSeq<4, TokenIfParser, TokenThenParser, TokenElseParser>;

	pparse::CharParser chparser(text_stream);
	
	auto result = TokenKeywdParser::parse( chparser );
	EXPECT_TRUE(result.success());

	pparse::AstEntryBase *res = result.ast_.get();
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

TEST(Parser,testStarParser) {

	pparse::Text_stream text_stream;
	
	bool isok = text_stream.open(-1,11);
	EXPECT_EQ(isok, true);

	const pparse::Char_t *str = " \tAA A\nA A\tAA"; 
	isok = text_stream.write_tail(str, strlen(str));

	using TokenAParser = pparse::PTok<1, CSTR1("A")>;

	using TokenStarParser = pparse::PStar<4, TokenAParser>;


	pparse::CharParser chparser(text_stream);
	
	auto result = TokenStarParser::parse( chparser );
	EXPECT_TRUE(result.success());

	pparse::AstEntryBase *res = result.ast_.get();
	EXPECT_TRUE( res != nullptr );

	EXPECT_TRUE( res->ruleId_ == 4 );

	TokenStarParser::AstType *star = (TokenStarParser::AstType *) res;
	
	for(auto &entry : star->entry_) {

		EXPECT_TRUE( entry.get()->ruleId_ == 1 );
	}

	EXPECT_TRUE( star->entry_.size() == 7 );
}

TEST(Parser,testWithAndParser) {

	pparse::Text_stream text_stream;
	
	bool isok = text_stream.open(-1,11);
	EXPECT_EQ(isok, true);

	const pparse::Char_t *str = " \t if then"; 
	isok = text_stream.write_tail(str, strlen(str));

	using TokenIfParser = pparse::PTok<1, CSTR2("if")>;
	using TokenThenParser = pparse::PTok<2, CSTR4("then")>;
	using TokenElseParser = pparse::PTok<3, CSTR4("else")>;


	using TokenWithAndParser1 = pparse::PWithAndLookahead<TokenIfParser, TokenThenParser>;
	using TokenWithAndParser2 = pparse::PWithAndLookahead<TokenIfParser, TokenElseParser>;

	pparse::CharParser chparser(text_stream);

	auto result2 = TokenWithAndParser2::parse( chparser );
	EXPECT_FALSE(result2.success());

	auto result1 = TokenWithAndParser1::parse( chparser );
	EXPECT_TRUE(result1.success());

	pparse::AstEntryBase *res = result1.ast_.get();
	EXPECT_TRUE( res != nullptr );

	TokenWithAndParser1::AstType *ptype  = (TokenWithAndParser1::AstType *) res; 
	EXPECT_TRUE( res->ruleId_ == 1 );
}

} // namespace


int main(int argc, char *argv[]) {

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS() ? 0 : 1;
}
