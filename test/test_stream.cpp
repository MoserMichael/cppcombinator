#include "gtest/gtest.h"

//enable execution trace with the next define
//#define  __PARSER_TRACE__
#include "parse.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

namespace {


using namespace pparse;


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
 


} // namespace



