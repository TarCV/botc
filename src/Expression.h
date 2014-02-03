#ifndef BOTC_EXPRESSION_H
#define BOTC_EXPRESSION_H
#include "Parser.h"

class DataBuffer;
class ExpressionSymbol;
class ExpressionValue;

// =============================================================================
//
class Expression final
{
	public:
		Expression (BotscriptParser* parser, EType reqtype, Lexer* lx);
		~Expression();
		ExpressionValue*		GetResult();

	private:
		Lexer*					mLexer;
		List<ExpressionSymbol*>	mSymbols;
		EType					mType;
		ExpressionValue*		mResult;
		BotscriptParser*		mParser;

		ExpressionValue*		Evaluate(); // Process the expression and yield a result
		ExpressionSymbol*		ParseSymbol();
		String					GetTokenString();
		void					Verify(); // Ensure the expr is valid
};

// =============================================================================
//
class ExpressionSymbol
{
	public:
		enum EExpressionSymbolType
		{
			eOperator,
			eOperand,
			eColon,
		};

	PROPERTY (private,	EExpressionSymbolType,	Type,	NO_OPS,	STOCK_WRITE)
};

// =============================================================================
//
class ExpressionOperator final : public ExpressionSymbol
{
	PROPERTY (public,	int,	ID,	NO_OPS,	STOCK_WRITE)

	public:
		ExpressionOperator (int id);
};

// =============================================================================
//
class ExpressionValue final : public ExpressionSymbol
{
	PROPERTY (public,	int,			Value,		BOOL_OPS,	STOCK_WRITE)
	PROPERTY (public,	DataBuffer*,	Buffer,		NO_OPS,		STOCK_WRITE)
	PROPERTY (public,	EType,			ValueType,	NO_OPS,		STOCK_WRITE)

	public:
		ExpressionValue (EType valuetype);

		void ConvertToBuffer();

		inline bool IsConstexpr() const
		{
			return GetBuffer() == null;
		}
};

// =============================================================================
//
// This class represents a ":" in the expression. It serves as the colon for the
// ternary ?: operator. It's not an operand nor is an operator, nor can we just
// skip it so it is its own thing here.
//
class ExpressionColon final : public ExpressionSymbol
{
	public:
		ExpressionColon() :
			mType (eColon) {}
};

#endif // BOTC_EXPRESSION_H