#ifndef BOTC_EXPRESSION_H
#define BOTC_EXPRESSION_H
#include "Parser.h"

class DataBuffer;
class ExpressionSymbol;
class ExpressionValue;

// =============================================================================
//
enum EOperator
{
	opNegateLogical,
	opUnaryMinus,
	opMultiplication,
	opDivision,
	opModulus,
	opAddition,
	opSubtraction,
	opLeftShift,
	opRightShift,
	opCompareLesser,
	opCompareGreater,
	opCompareAtLeast,
	opCompareAtMost,
	opCompareEquals,
	opCompareNotEquals,
	opBitwiseAnd,
	opBitwiseXOr,
	opBitwiseOr,
	opLogicalAnd,
	opLogicalOr,
	opTernary,
};

// =============================================================================
//
class Expression final
{
	public:
		enum ESymbolType
		{
			eOperatorSymbol,
			eValueSymbol,
			eColonSymbol,
		};

		using SymbolList = List<ExpressionSymbol*>;

		Expression (BotscriptParser* parser, Lexer* lx, EType reqtype);
		~Expression();
		ExpressionValue*		GetResult();

	private:
		BotscriptParser*		mParser;
		Lexer*					mLexer;
		SymbolList				mSymbols;
		EType					mType;
		ExpressionValue*		mResult;

		ExpressionValue*		Evaluate(); // Process the expression and yield a result
		ExpressionSymbol*		ParseSymbol();
		String					GetTokenString();
		void					AdjustOperators();
		void					Verify(); // Ensure the expr is valid
		void					TryVerifyValue (bool* verified, SymbolList::Iterator it);
};

// =============================================================================
//
class ExpressionSymbol
{
	public:
		ExpressionSymbol (Expression::ESymbolType type) :
			mType (type) {}

	PROPERTY (private, Expression::ESymbolType, Type, NO_OPS, STOCK_WRITE)
};

// =============================================================================
//
class ExpressionOperator final : public ExpressionSymbol
{
	PROPERTY (public, EOperator, ID, NO_OPS, STOCK_WRITE)

	public:
		ExpressionOperator (EOperator id);
};

// =============================================================================
//
class ExpressionValue final : public ExpressionSymbol
{
	PROPERTY (public, int,			Value,		NUM_OPS,	STOCK_WRITE)
	PROPERTY (public, DataBuffer*,	Buffer,		NO_OPS,		STOCK_WRITE)
	PROPERTY (public, EType,		ValueType,	NO_OPS,		STOCK_WRITE)

	public:
		ExpressionValue (EType valuetype);
		~ExpressionValue();

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
			ExpressionSymbol (Expression::eColonSymbol) {}
};

#endif // BOTC_EXPRESSION_H