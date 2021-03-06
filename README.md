
# cppcombinator

- [cppcombinator](#cppcombinator)
- [Introduction](#introduction)
- [Bounded buffer](#bounded-buffer)
- [Parser reference](#parser-reference)
  * [Parser combinators](#parser-combinators)
    + [Ordered choice](#ordered-choice)
    + [Sequence](#sequence)
    + [Optional](#optional)
    + [Require end of file after parsing the input type.](#require-end-of-file-after-parsing-the-input-type)
    + [Parse repetition of term](#parse-repetition-of-term)
    + [Zero or more](#zero-or-more)
    + [One or more](#one-or-more)
    + [parse type T with And Predicate LookaheadType](#parse-type-t-with-and-predicate-lookaheadtype)
    + [parse type T with Not Predicate LookaheadType](#parse-type-t-with-not-predicate-lookaheadtype)
  * [Atomic parsers](#atomic-parsers)
    + [Fixed string token parser](#fixed-string-token-parser)
    + [Extension parser](#extension-parser)
    + [Parse unsigned integer (sequence of digits)](#parse-unsigned-integer--sequence-of-digits-)
    + [parse a single printable character](#parse-a-single-printable-character)
    + [parse a cstyle identifier](#parse-a-cstyle-identifier)
- [Validation of grammar](#validation-of-grammar)
- [Dumping of abstract syntax tree to json.](#dumping-of-abstract-syntax-tree-to-json)
- [Tracing the generated parser](#tracing)
- [What i learned from all this](#what-i-learned-from-all-this)

<small><i><a href='http://ecotrust-canada.github.io/markdown-toc/'>Table of contents generated with markdown-toc</a></i></small>



# Introduction

This library is a header only C++17 template library.

The library provides template classes for building top-down [PEG parser](https://en.wikipedia.org/wiki/Parsing_expression_grammar), the parsers build an in-memory parse tree,
Each template instantiation stands for a grammar rule, which translates directly into a node in the parse tree.


# Example usage 

To use this library #include "parse.h" - the classes of this library are in namespace pparse.

```
    #include "parse.h"

	using namespace pparse;

```

Lets dive into an example grammar:

Each grammar rule is represented by the instantation of some template, lets look at this example grammar:
The library provides template classes that can express a terminal or non terminal gramar rule.


Terminal rules in this example:


```
TokInt<1>
```

terminal rule that consumes a positive integer (regex \d+). Note that the integer constant 1 will identify the node in the parse tree, this id is always the first argument to a template rule

```
PTok<3, CSTR1("*")> 
```
terminal rule that consumes a fixed string * - the string is one character in length, hence CSTR1("*").


Non terminal rules in this example:

```
PAny<2, PTok<3,CSTR1("*")>, PTok<4,CSTR1("/")> >
```

PAny will match any one of the grammar rules provided as template arguments, in this examples these are either a fixed string * (expressed by PTok&lt;3,CSTR1("*")&gt;) or the fixed string / (expressed by PTok&lt4,CSTR1("/")&gt;) ; 


```
PSeq<10, PTok<11, CSTR1("-")>, TokInt<1> > 
```

PSeq will match the sequence of grammar rules provided as template arguments. in this example this is the fixed string - following by a positive integer. 

```
PRequireEof<Expr> 
```

PRequireEof will parse the argument type Expr, but will require it to be followed by whitespaces until the end of input.


And example grammar for matching an arithmetic expression; note that you can give each production rule a name by deriving it from the template epression that stands for the right hand side of the production rule.

```
	struct Int : PTokInt<1> {}; // terminal rule: consumes an integer (\d+)

	struct Expr;

	struct Mult : PAny<2, PTok<3,CSTR1("*")>, PTok<4,CSTR1("/")> > {};q

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

Each grammar rule (like struct Expr : PAny&lt;12, Int, NegativeInt, NestedExpr&gt;) has a nested AstType (Expr::AstType) ; this type defines the record in the abstract syntax tree
that is produced by the parser on successfull parsing of the input text. This ast type uses c++17 helper objects for its representation, for example the PAny&lt;ruleId,Parser1,Parser2...&gt; parser uses std::variant to stand for any one of the argument types Parser1, Parser2 ... while PSeq&lt;ruleId,Parser1,Parser2&gt; uses std::tuple for the sequence of the argument parsers.

Please note that the first argument of each rule is an integer constant that identifies the parser type - the rule id is returned from each ast type with getRuleId();
Note that for a real gramar you should proabably define an enumeration and use the enumeration values instead of integer constants.

To call the parser you first need to create an input stream object.

```
	bool isok = text_stream.open("input_file.txt");

or

	fd = open("input_file",O_RDONLY);
	bool isok = text_stream.open(fd);

or you can create it with invalid file descriptor and feed it input by yourself:

	bool isok = text_stream.open(-1);
	isok = text_stream.write_tail(test_string, strlen(test_string) );


```

The input stream is implemented as a circular buffer, by default a lookahead buffer of 4k is allocated.

Then to call the parser:

```
	// create a base character parser
	CharParser chparser(text_stream);
	
	// call the parser with the main term of the grammar
	Parse_result res = ExprEof::parse(chparser);
```	

Note that the parse call is instantiated with the top level rule of your grammar.
the result object has res.success() - this should be true if the input text has been parsed successfully. It not then res.get_start_pos() returns the line and column of the error. If parsing succeed then res.get_ast() returns the Ast type that stands for the parsed output, res.get_start_pos() and res.get_end_pos() are the location of the beginning and and of the parsed structure.

Note that a CharParser is wrapping the text stream object: this way it is possible to craft a base parser for a particular grammar that consumes comments, in that case comments will not have to be dealt with by the grammar.

# Bounded buffer

This parser library has one optimization for the read ahead buffer: if one is parsing a top level rule then all input is discarded when the top level rule has been parsed:
In the previous example the following rule is a top level rule PAny<15, PSeq<16, MultExpr, Add, Expr >, MultExpr>   - it stands for a repetition of a choice of either one of: MultExpr or the nested sequence PSeq<16, MultExpr, Add, Expr >. For each instances after the first repetition has been parsed we can discard all input up to that point.
This should keep the lookahead buffer bounded for most cases. (however the lookahead buffer will be reallocated if you really need a larger buffer).

# Parser rule reference

a reference of all parsing rules provided by this library:


## Parser combinators

Given any existing parsing types, a new parsing expression can be constructed using the following operators:


### Ordered choice

```
template<RuleId ruleId, typename ...Types>
struct PAny 
```

This type allows for recursive definitions, that means the argument types do not have to be defined when defining a PAny rule.

### Sequence

```
template<RuleId ruleId, typename ...Types>
struct PSeq 
```

The nested AST type (PSeq::AstType) looks as follows

```
	struct AstType : AstEntryBase {
			AstType() : AstEntryBase(ruleId) {
			}

			std::tuple<typename std::unique_ptr<typename Types::AstType>...> entry_;
	};
```


This type allows for recursive definitions, that means the argument types do not have to be defined when defining a PAny rule.

### Optional

```
template<RuleId ruleId, typename PType>
struct POpt 
```
The nested AST type (POpt::AstType) looks as follows

```
	using PTypePtr = typename std::unique_ptr<typename PType::AstType>;  

	using OptionType = std::optional< PTypePtr >;

	struct AstType : AstEntryBase {
			AstType() : AstEntryBase(ruleId) {
			}

			OptionType entry_;
	};

```


### Require end of file after parsing the input type.

```
template<typename Type>
PRequireEof
```

The AST of the type Type is forwarded by this parser.

Note that this is the only parser that doesn't backtrack if parsing of Type fails.
This is done on purpose - if the nested parser is a repetition parser (like zero or more or one or more), then each instance of the nested parser will advance the input and discard the buffer upon parsing an instance of the term (see Bounded Buffer section)

This rule does not generate a node in the parse tree, it therefore does not need an identifying constant as the first template argument


Please note that the parser for this type must be declared after the declaration of the argument type Type. Forward declarations are not possible as the Ast type of the argument type is used as is.

### Parse repetition of term

```
template<RuleId ruleId, typename Type, int minOccurance = 0, int maxOccurance = 0>
struct PRepeat
```

The AST of the type Type is forwarded by this parser.

### Zero or more

```
template<RuleId ruleId, typename Type>
struct PStar
```

The AST of the type Type is forwarded by this parser.

### One or more

```
template<RuleId ruleId, typename Type>
struct PPlus 
```

The AST of the type Type is forwarded by this parser.



### parse type T with And Predicate LookaheadType

```
template<typename Type, typename LookaheadType>
struct PWithAndLookahead `
```

parsing is successfull if T parses, on condition that it is followed by input parsed by LookaheadType.
The AST of the type Type is forwarded by this parser. The AST object generated upon parsing by LookaheadType is discarded.


### parse type T with Not Predicate LookaheadType


```
template<typename Type, typename LookaheadType>
struct PWithNotLookahead
```

parsing is successfull if T parses, on condition that it is not followed by input parsed by LookaheadType.
The AST of the type Type is forwarded by this parser. The AST object generated upon parsing by LookaheadType is discarded. 


## Atomic parsers

The following parser types consume terminal symbols

### Fixed string token parser

```
template<RuleId ruleId, Char_t ...Cs>
struct PTok : ParserBase  {
```

Note that this parser receives a sequence of characters, as currrently there is no way to instantiate a template with a string constant (that feature was added in c++20).
However thre are macros that expand an argument string into a character sequence:

Example usage:
```
PTok<1,CSTR2("if")>
PTok<3,CSTR4("else)>
PTok<4,CSTR5("while)>
```

### Extension parser

```
enum class Char_checker_result {
	error,
	proceed,
	acceptNow,
	acceptUnget,
};

typedef Char_checker_result (PTokVar_cb_t) (Char_t current_char, bool iseof, Char_t *matched_so_far);

template<RuleId ruleId, PTokVar_cb_t checker, bool canAcceptEmptyInput = false>
struct PTokVar : ParserBase  { 	
```

The nested AST type of this parser includes the parsed expression is string entry_.

```
		struct AstType : AstEntryBase {
				AstType() : AstEntryBase(ruleId) {
				}

				std::string entry_;
		};
```


### Parse unsigned integer (sequence of digits)

```
template<RuleId ruleId>
struct PTokInt : PTokVar<ruleId, pparse_is_digit>  { 
}
```

### parse a single printable character

```
template<RuleId ruleId>
struct PTokChar 
```

### parse a cstyle identifier

```
template<RuleId ruleId>
struct PTokIdentifierCStyle 
```

Parses an identifier that maches the following regex [a-zA-Z][a-zA-Z0-9\_]*
Special care is taken that tokens that were accepted by PTok are not accepted as identifiers. 
Please note that this trick works only if the top level parser is of type PRequireEof or of type PTopLevelParser.

# Validation of grammar

You can validate the generated grammar for left recursion errors.
To enable this feature you need to place #define __PARSER_ANALYSE__ before #include "parse.h"

Then call the analysis step with the following call
```
	bool no_cycles = ParserChecker<ExprEof>::check(std::cout);
	EXPECT_TRUE(no_cycles);
```
Note that the check call is instantiated with the top level rule of your grammar.
It even tells you what is causing the left recursion, the error is written on the stream that is passed as argument to the check function

# Dumping of abstract syntax tree to json.

You can dump the ast into json:

```
	ExprEof::dumpJson(std::cout, (ExprEof::AstType *) result.get_ast() );
```
please note that this feature requires RTTI support enabled.



# Tracing the generated parser

You can trace the running of the parser, this can be quite usefull for debugging purposes.
To do you need to place #define  __PARSER_TRACE__ right before #include "parse.h"

The execution trace is dumpedto standard output and looks like this:
Please note that this feature requires RTTI support enabled.

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

A surprising thing is that [member templates](https://en.cppreference.com/w/cpp/language/member_template) let you do recursive definition, that's because of the lazy natue of template instantiation in c++. You never stop learning with c++. 

Also the parser visualization feature of this projects shows that PEG grammars really do an enormous amount of backtracking; If you need a very performant parser then you should probably stick with lex and yacc - here you get something without backtracking, however the process of writing a grammar in lex and yacc is a really complicated business. Writing a PEG grammar is much easier, and more flexible in terms of the grammar that is accepted.

Also i learned a few things from looking at the [PEGTL](https://github.com/taocpp/PEGTL) project. Thanks and acknowledgements.

# License

This code is licensed under both MIT and GPLv2 licenses. Pick any license that you like.
