
# cppcombinator

This library is a parser and AST (abstract syntax tree) generator written in C++17.
The library is a header only c++17 library. 

The generated parser is a [PEG grammar](https://en.wikipedia.org/wiki/Parsing_expression_grammar)

It has some advanced features such as grammar validation and tracing of the parser in action. 


# Introduction

To use this library #include "parse.h" - the classes of this library are in namespace pparse.

Each parser and parser combinator is an instantation of some template, lets look at this example grammar:

```
	using namespace pparse;

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
```

Each grammar rule (like struct Expr : PAny&lt;12, Int, NetagiveInt, NestedExpr&gt;) as its all nested AstType (Expr::AstType) ; this type defines the record in the abstract syntax tree
that is produced by the parser on successfull parsing of the input text. This ast type uses c++17 helper objects for its representation, for example the PAny&lt;ruleId,Parser1,Parser2...&gt; parser uses std::variant to stand for any one of the argument types Parser1, Parser2 ... while PSeq&lt;ruleId,Parser1,Parser2&gt; uses std::tuple for the sequence of the argument parsers.

Please note that the first argument of each rule is an integer constant that identifies the parser type - the rule id is returned from each ast type with getRuleId();

To call the parser you first need to create an input stream object.

```
	bool isok = text_stream.open("input_file.txt");
	EXPECT_EQ(isok, true);a

```

The input stream is implemented as a circular buffer, by default a lookahead buffer of 4k is allocated.

Then to call the parser:

```
	// create a base character parser
	CharParser chparser(text_stream);
	
	// call the parser with the main term of the grammar
	Parse_result res = ExprEof::parse(chparser);
```	

the result object has res.success() - this should be true if the input text has been parsed successfully. It not then res.get_start_pos() returns the line and column of the error. If parsing succeed then res.get_ast() returns the Ast type that stands for the parsed output, res.get_start_pos() and res.get_end_pos() are the location of the beginning and and of the parsed structure.


# Bounded buffer

This parser library has one optimization for the read ahea buffer: if one is parsing a top level rule then all input is discarded when the top level rule has been parsed:
In the previous example the following rule is a top level rule PAny<15, PSeq<16, MultExpr, Add, Expr >, MultExpr>   - it stands for a repetition of a choice of either one of: MultExpr or the nested sequence PSeq<16, MultExpr, Add, Expr >. For each instances after the first repetition has been parsed we can discard all input up to that point.
This should keep the lookahead buffer bounded for most cases. (however the lookahead buffer will be reallocated if you really need a larger buffer).

# Rule reference

TBD

# Validation of grammar

You can validate the generated grammar for left recursion errors.
To enable this feature you need to place #define __PARSER_ANALYSE__ before #include "parse.h"

Then call the analysis step with the following call
```
	bool no_cycles = ParserChecker<Expr>::check(std::cout);
	EXPECT_TRUE(no_cycles);
```
It even tells you what is causing the left recursion, the error is written on the stream that is passed as argument to the check function

# Dumping of abstract syntax tree to json.

You can dump the ast into json:

```
	ExprEof::dumpJson(std::cout, (ExprEof::AstType *) result.get_ast() );
```


# Tracing

You can trace the running of the parser, this can be quite usefull for debugging purposes.
To do you need to place #define  __PARSER_TRACE__ right before #include "parse.h"

The execution trace is dumpedto standard output and looks like this:
(please note that this feature requires RTTI support enabled)

```
(000)start parsing: PAny<7, pparse::PSeq<6>  MultExpr> at: (1:0 offset: 0)
(001) start parsing: PSeq<6, MultExpr, Add, Expr> at: (1:0 offset: 0)
(002)  start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:0 offset: 0)
(003)   start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:0 offset: 0)
(004)    start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:0 offset: 0)
(005)     start parsing: PSeq<1, pparse::PTok<11>  Int> at: (1:0 offset: 0)
(005)     end parsing: PSeq<1, pparse::PTok<11>  Int> SUCCESS at: (1:2 offset: 2)
(004)    end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 1 at: (1:2 offset: 2)
(003)   end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:0 offset: 0)
(003)   start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:0 offset: 0)
(004)    start parsing: PSeq<1, pparse::PTok<11>  Int> at: (1:0 offset: 0)
(004)    end parsing: PSeq<1, pparse::PTok<11>  Int> SUCCESS at: (1:2 offset: 2)
(003)   end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 1 at: (1:2 offset: 2)
(002)  end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:2 offset: 2)
(001) end parsing: PSeq<6, MultExpr, Add, Expr> FAIL at: (1:0 offset: 0)
(001) start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:0 offset: 0)
(002)  start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:0 offset: 0)
(003)   start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:0 offset: 0)
(004)    start parsing: PSeq<1, pparse::PTok<11>  Int> at: (1:0 offset: 0)
(004)    end parsing: PSeq<1, pparse::PTok<11>  Int> SUCCESS at: (1:2 offset: 2)
(003)   end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 1 at: (1:2 offset: 2)
(002)  end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:0 offset: 0)
(002)  start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:0 offset: 0)
(003)   start parsing: PSeq<1, pparse::PTok<11>  Int> at: (1:0 offset: 0)
(003)   end parsing: PSeq<1, pparse::PTok<11>  Int> SUCCESS at: (1:2 offset: 2)
(002)  end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 1 at: (1:2 offset: 2)
(001) end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:2 offset: 2)
(000)end parsing: PAny<7, pparse::PSeq<6>  MultExpr> SUCCESS choice-index: 1 at: (1:2 offset: 2)
(000)start parsing: PAny<7, pparse::PSeq<6>  MultExpr> at: (1:0 offset: 0)
(001) start parsing: PSeq<6, MultExpr, Add, Expr> at: (1:0 offset: 0)
(002)  start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:0 offset: 0)
(003)   start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:0 offset: 0)
(004)    start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:0 offset: 0)
(004)    end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:1 offset: 1)
(004)    start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:3 offset: 3)
(005)     start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:3 offset: 3)
(006)      start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:3 offset: 3)
(006)      end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:5 offset: 5)
(005)     end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:3 offset: 3)
(005)     start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:3 offset: 3)
(005)     end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:5 offset: 5)
(004)    end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:5 offset: 5)
(003)   end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> SUCCESS at: (1:5 offset: 5)
(002)  end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 0 at: (1:5 offset: 5)
(001) end parsing: PSeq<6, MultExpr, Add, Expr> FAIL at: (1:0 offset: 0)
(001) start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:0 offset: 0)
(002)  start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:0 offset: 0)
(003)   start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:0 offset: 0)
(003)   end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:1 offset: 1)
(003)   start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:3 offset: 3)
(004)    start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:3 offset: 3)
(005)     start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:3 offset: 3)
(005)     end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:5 offset: 5)
(004)    end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:3 offset: 3)
(004)    start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:3 offset: 3)
(004)    end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:5 offset: 5)
(003)   end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:5 offset: 5)
(002)  end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> SUCCESS at: (1:5 offset: 5)
(001) end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 0 at: (1:5 offset: 5)
(000)end parsing: PAny<7, pparse::PSeq<6>  MultExpr> SUCCESS choice-index: 1 at: (1:5 offset: 5)
(000)start parsing: PAny<7, pparse::PSeq<6>  MultExpr> at: (1:0 offset: 0)
(001) start parsing: PSeq<6, MultExpr, Add, Expr> at: (1:0 offset: 0)
(002)  start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:0 offset: 0)
(003)   start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:0 offset: 0)
(004)    start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:0 offset: 0)
(004)    end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:1 offset: 1)
(003)   end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:0 offset: 0)
(003)   start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:0 offset: 0)
(003)   end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:1 offset: 1)
(002)  end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:1 offset: 1)
(002)  start parsing: PAny<7, pparse::PSeq<6>  MultExpr> at: (1:3 offset: 3)
(003)   start parsing: PSeq<6, MultExpr, Add, Expr> at: (1:3 offset: 3)
(004)    start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:3 offset: 3)
(005)     start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:3 offset: 3)
(006)      start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:3 offset: 3)
(006)      end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:5 offset: 5)
(005)     end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:3 offset: 3)
(005)     start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:3 offset: 3)
(005)     end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:5 offset: 5)
(004)    end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:5 offset: 5)
(004)    start parsing: PAny<7, pparse::PSeq<6>  MultExpr> at: (1:7 offset: 7)
(005)     start parsing: PSeq<6, MultExpr, Add, Expr> at: (1:7 offset: 7)
(006)      start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:7 offset: 7)
(007)       start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:7 offset: 7)
(008)        start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:7 offset: 7)
(008)        end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:9 offset: 9)
(007)       end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:7 offset: 7)
(007)       start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:7 offset: 7)
(007)       end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:9 offset: 9)
(006)      end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:9 offset: 9)
(005)     end parsing: PSeq<6, MultExpr, Add, Expr> FAIL at: (1:7 offset: 7)
(005)     start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:7 offset: 7)
(006)      start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:7 offset: 7)
(007)       start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:7 offset: 7)
(007)       end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:9 offset: 9)
(006)      end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:7 offset: 7)
(006)      start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:7 offset: 7)
(006)      end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:9 offset: 9)
(005)     end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:9 offset: 9)
(004)    end parsing: PAny<7, pparse::PSeq<6>  MultExpr> SUCCESS choice-index: 1 at: (1:9 offset: 9)
(003)   end parsing: PSeq<6, MultExpr, Add, Expr> SUCCESS at: (1:9 offset: 9)
(002)  end parsing: PAny<7, pparse::PSeq<6>  MultExpr> SUCCESS choice-index: 0 at: (1:9 offset: 9)
(001) end parsing: PSeq<6, MultExpr, Add, Expr> SUCCESS at: (1:9 offset: 9)
(000)end parsing: PAny<7, pparse::PSeq<6>  MultExpr> SUCCESS choice-index: 0 at: (1:9 offset: 9)
(000)start parsing: PAny<7, pparse::PSeq<6>  MultExpr> at: (1:0 offset: 0)
(001) start parsing: PSeq<6, MultExpr, Add, Expr> at: (1:0 offset: 0)
(002)  start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:0 offset: 0)
(003)   start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:0 offset: 0)
(004)    start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:0 offset: 0)
(004)    end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:1 offset: 1)
(003)   end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:0 offset: 0)
(003)   start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:0 offset: 0)
(003)   end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:1 offset: 1)
(002)  end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:1 offset: 1)
(002)  start parsing: PAny<7, pparse::PSeq<6>  MultExpr> at: (1:3 offset: 3)
(003)   start parsing: PSeq<6, MultExpr, Add, Expr> at: (1:3 offset: 3)
(004)    start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:3 offset: 3)
(005)     start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:3 offset: 3)
(006)      start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:3 offset: 3)
(006)      end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:5 offset: 5)
(005)     end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:3 offset: 3)
(005)     start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:3 offset: 3)
(005)     end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:5 offset: 5)
(004)    end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:5 offset: 5)
(004)    start parsing: PAny<7, pparse::PSeq<6>  MultExpr> at: (1:7 offset: 7)
(005)     start parsing: PSeq<6, MultExpr, Add, Expr> at: (1:7 offset: 7)
(006)      start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:7 offset: 7)
(007)       start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:7 offset: 7)
(008)        start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:7 offset: 7)
(008)        end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:9 offset: 9)
(007)       end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:7 offset: 7)
(007)       start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:7 offset: 7)
(007)       end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:9 offset: 9)
(006)      end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:9 offset: 9)
(006)      start parsing: PAny<7, pparse::PSeq<6>  MultExpr> at: (1:11 offset: 11)
(007)       start parsing: PSeq<6, MultExpr, Add, Expr> at: (1:11 offset: 11)
(008)        start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:11 offset: 11)
(009)         start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:11 offset: 11)
(010)          start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:11 offset: 11)
(010)          end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:13 offset: 13)
(010)          start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:15 offset: 15)
(011)           start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:15 offset: 15)
(012)            start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:15 offset: 15)
(012)            end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:17 offset: 17)
(011)           end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:15 offset: 15)
(011)           start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:15 offset: 15)
(011)           end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:17 offset: 17)
(010)          end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:17 offset: 17)
(009)         end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> SUCCESS at: (1:17 offset: 17)
(008)        end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 0 at: (1:17 offset: 17)
(007)       end parsing: PSeq<6, MultExpr, Add, Expr> FAIL at: (1:11 offset: 11)
(007)       start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:11 offset: 11)
(008)        start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:11 offset: 11)
(009)         start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:11 offset: 11)
(009)         end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:13 offset: 13)
(009)         start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:15 offset: 15)
(010)          start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:15 offset: 15)
(011)           start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:15 offset: 15)
(011)           end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:17 offset: 17)
(010)          end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:15 offset: 15)
(010)          start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:15 offset: 15)
(010)          end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:17 offset: 17)
(009)         end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:17 offset: 17)
(008)        end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> SUCCESS at: (1:17 offset: 17)
(007)       end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 0 at: (1:17 offset: 17)
(006)      end parsing: PAny<7, pparse::PSeq<6>  MultExpr> SUCCESS choice-index: 1 at: (1:17 offset: 17)
(005)     end parsing: PSeq<6, MultExpr, Add, Expr> SUCCESS at: (1:17 offset: 17)
(004)    end parsing: PAny<7, pparse::PSeq<6>  MultExpr> SUCCESS choice-index: 0 at: (1:17 offset: 17)
(003)   end parsing: PSeq<6, MultExpr, Add, Expr> SUCCESS at: (1:17 offset: 17)
(002)  end parsing: PAny<7, pparse::PSeq<6>  MultExpr> SUCCESS choice-index: 0 at: (1:17 offset: 17)
(001) end parsing: PSeq<6, MultExpr, Add, Expr> SUCCESS at: (1:17 offset: 17)
(000)end parsing: PAny<7, pparse::PSeq<6>  MultExpr> SUCCESS choice-index: 0 at: (1:17 offset: 17)
(000)start parsing: PAny<7, pparse::PSeq<6>  MultExpr> at: (1:0 offset: 0)
(001) start parsing: PSeq<6, MultExpr, Add, Expr> at: (1:0 offset: 0)
(002)  start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:0 offset: 0)
(003)   start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:0 offset: 0)
(004)    start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:0 offset: 0)
(005)     start parsing: PSeq<1, pparse::PTok<11>  Int> at: (1:0 offset: 0)
(005)     end parsing: PSeq<1, pparse::PTok<11>  Int> FAIL at: (1:0 offset: 0)
(005)     start parsing: PSeq<8, pparse::PTok<0>  Expr, pparse::PTok<0>  > at: (1:0 offset: 0)
(006)      start parsing: PAny<7, pparse::PSeq<6>  MultExpr> at: (1:1 offset: 1)
(007)       start parsing: PSeq<6, MultExpr, Add, Expr> at: (1:1 offset: 1)
(008)        start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:1 offset: 1)
(009)         start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:1 offset: 1)
(010)          start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:1 offset: 1)
(010)          end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:2 offset: 2)
(010)          start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:3 offset: 3)
(011)           start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:3 offset: 3)
(012)            start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:3 offset: 3)
(012)            end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:4 offset: 4)
(011)           end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:3 offset: 3)
(011)           start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:3 offset: 3)
(011)           end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:4 offset: 4)
(010)          end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:4 offset: 4)
(009)         end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> SUCCESS at: (1:4 offset: 4)
(008)        end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 0 at: (1:4 offset: 4)
(007)       end parsing: PSeq<6, MultExpr, Add, Expr> FAIL at: (1:1 offset: 1)
(007)       start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:1 offset: 1)
(008)        start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:1 offset: 1)
(009)         start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:1 offset: 1)
(009)         end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:2 offset: 2)
(009)         start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:3 offset: 3)
(010)          start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:3 offset: 3)
(011)           start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:3 offset: 3)
(011)           end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:4 offset: 4)
(010)          end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:3 offset: 3)
(010)          start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:3 offset: 3)
(010)          end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:4 offset: 4)
(009)         end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:4 offset: 4)
(008)        end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> SUCCESS at: (1:4 offset: 4)
(007)       end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 0 at: (1:4 offset: 4)
(006)      end parsing: PAny<7, pparse::PSeq<6>  MultExpr> SUCCESS choice-index: 1 at: (1:4 offset: 4)
(005)     end parsing: PSeq<8, pparse::PTok<0>  Expr, pparse::PTok<0>  > SUCCESS at: (1:5 offset: 5)
(004)    end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 2 at: (1:5 offset: 5)
(003)   end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:0 offset: 0)
(003)   start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:0 offset: 0)
(004)    start parsing: PSeq<1, pparse::PTok<11>  Int> at: (1:0 offset: 0)
(004)    end parsing: PSeq<1, pparse::PTok<11>  Int> FAIL at: (1:0 offset: 0)
(004)    start parsing: PSeq<8, pparse::PTok<0>  Expr, pparse::PTok<0>  > at: (1:0 offset: 0)
(005)     start parsing: PAny<7, pparse::PSeq<6>  MultExpr> at: (1:1 offset: 1)
(006)      start parsing: PSeq<6, MultExpr, Add, Expr> at: (1:1 offset: 1)
(007)       start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:1 offset: 1)
(008)        start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:1 offset: 1)
(009)         start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:1 offset: 1)
(009)         end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:2 offset: 2)
(009)         start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:3 offset: 3)
(010)          start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:3 offset: 3)
(011)           start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:3 offset: 3)
(011)           end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:4 offset: 4)
(010)          end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:3 offset: 3)
(010)          start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:3 offset: 3)
(010)          end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:4 offset: 4)
(009)         end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:4 offset: 4)
(008)        end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> SUCCESS at: (1:4 offset: 4)
(007)       end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 0 at: (1:4 offset: 4)
(006)      end parsing: PSeq<6, MultExpr, Add, Expr> FAIL at: (1:1 offset: 1)
(006)      start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:1 offset: 1)
(007)       start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:1 offset: 1)
(008)        start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:1 offset: 1)
(008)        end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:2 offset: 2)
(008)        start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:3 offset: 3)
(009)         start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:3 offset: 3)
(010)          start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:3 offset: 3)
(010)          end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:4 offset: 4)
(009)         end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:3 offset: 3)
(009)         start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:3 offset: 3)
(009)         end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:4 offset: 4)
(008)        end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:4 offset: 4)
(007)       end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> SUCCESS at: (1:4 offset: 4)
(006)      end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 0 at: (1:4 offset: 4)
(005)     end parsing: PAny<7, pparse::PSeq<6>  MultExpr> SUCCESS choice-index: 1 at: (1:4 offset: 4)
(004)    end parsing: PSeq<8, pparse::PTok<0>  Expr, pparse::PTok<0>  > SUCCESS at: (1:5 offset: 5)
(003)   end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 2 at: (1:5 offset: 5)
(002)  end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:5 offset: 5)
(002)  start parsing: PAny<7, pparse::PSeq<6>  MultExpr> at: (1:7 offset: 7)
(003)   start parsing: PSeq<6, MultExpr, Add, Expr> at: (1:7 offset: 7)
(004)    start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:7 offset: 7)
(005)     start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:7 offset: 7)
(006)      start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:7 offset: 7)
(006)      end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:9 offset: 9)
(005)     end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:7 offset: 7)
(005)     start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:7 offset: 7)
(005)     end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:9 offset: 9)
(004)    end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:9 offset: 9)
(003)   end parsing: PSeq<6, MultExpr, Add, Expr> FAIL at: (1:7 offset: 7)
(003)   start parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> at: (1:7 offset: 7)
(004)    start parsing: PSeq<6, SimpleExpr, Mult, MultExpr> at: (1:7 offset: 7)
(005)     start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:7 offset: 7)
(005)     end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:9 offset: 9)
(004)    end parsing: PSeq<6, SimpleExpr, Mult, MultExpr> FAIL at: (1:7 offset: 7)
(004)    start parsing: PAny<6, Int, NegativeInt, NestedExpr> at: (1:7 offset: 7)
(004)    end parsing: PAny<6, Int, NegativeInt, NestedExpr> SUCCESS choice-index: 0 at: (1:9 offset: 9)
(003)   end parsing: PAny<7, pparse::PSeq<6>  SimpleExpr> SUCCESS choice-index: 1 at: (1:9 offset: 9)
(002)  end parsing: PAny<7, pparse::PSeq<6>  MultExpr> SUCCESS choice-index: 1 at: (1:9 offset: 9)
(001) end parsing: PSeq<6, MultExpr, Add, Expr> SUCCESS at: (1:9 offset: 9)
(000)end parsing: PAny<7, pparse::PSeq<6>  MultExpr> SUCCESS choice-index: 0 at: (1:9 offset: 9)

```


# What i learned from all this

i learned that most of additions in c++17 are tooled toward template metaprogramming: things like std::tuple and std::variant are very useful in this context.
i think template metaprogramming is now a doable thing ever since c++17; it takes some time to get used to the concept - this project has helped me to learn this approach well, i think.
