#ifndef BOTC_EXPRESSION_H
#define BOTC_EXPRESSION_H
#include "Parser.h"

class DataBuffer;
class ExpressionSymbol;
class ExpressionValue;
class ExpressionOperator;

// =============================================================================
//
named_enum ExpressionOperatorType
{
	OPER_NegateLogical,
	OPER_UnaryMinus,
	OPER_Multiplication,
	OPER_Division,
	OPER_Modulus,
	OPER_Addition,
	OPER_Subtraction,
	OPER_LeftShift,
	OPER_RightShift,
	OPER_CompareLesser,
	OPER_CompareGreater,
	OPER_CompareAtLeast,
	OPER_CompareAtMost,
	OPER_CompareEquals,
	OPER_CompareNotEquals,
	OPER_BitwiseAnd,
	OPER_BitwiseXOr,
	OPER_BitwiseOr,
	OPER_LogicalAnd,
	OPER_LogicalOr,
	OPER_Ternary,
};

// =============================================================================
//
enum ExpressionSymbolType
{
	EXPRSYM_Operator,
	EXPRSYM_Value,
	EXPRSYM_Colon,
};

class Expression final
{
	public:
		using SymbolList = List<ExpressionSymbol*>;

		Expression (BotscriptParser* parser, Lexer* lx, DataType reqtype);
		~Expression();
		ExpressionValue*		GetResult();

	private:
		BotscriptParser*		mParser;
		Lexer*					mLexer;
		SymbolList				mSymbols;
		DataType				mType;
		String					mBadTokenText;

		ExpressionValue*		Evaluate(); // Process the expression and yield a result
		ExpressionSymbol*		ParseSymbol();
		String					GetTokenString();
		void					AdjustOperators();
		void					Verify(); // Ensure the expr is valid
		void					TryVerifyValue (bool* verified, SymbolList::Iterator it);
		ExpressionValue*		EvaluateOperator (const ExpressionOperator* op,
												  const List<ExpressionValue*>& values);
		SymbolList::Iterator	FindPrioritizedOperator();
};

// =============================================================================
//
class ExpressionSymbol
{
	public:
		ExpressionSymbol (ExpressionSymbolType type) :
			mType (type) {}

	PROPERTY (private, ExpressionSymbolType, Type, NO_OPS, STOCK_WRITE)
};

// =============================================================================
//
class ExpressionOperator final : public ExpressionSymbol
{
	PROPERTY (public, ExpressionOperatorType, ID, NO_OPS, STOCK_WRITE)

	public:
		ExpressionOperator (ExpressionOperatorType id);
};

// =============================================================================
//
class ExpressionValue final : public ExpressionSymbol
{
	PROPERTY (public, int,			Value,		NUM_OPS,	STOCK_WRITE)
	PROPERTY (public, DataBuffer*,	Buffer,		NO_OPS,		STOCK_WRITE)
	PROPERTY (public, DataType,		ValueType,	NO_OPS,		STOCK_WRITE)

	public:
		ExpressionValue (DataType valuetype);
		~ExpressionValue();

		void				ConvertToBuffer();

		inline ExpressionValue* Clone() const
		{
			return new ExpressionValue (*this);
		}

		inline bool IsConstexpr() const
		{
			return GetBuffer() == null;
		}
};

// =============================================================================
//
// This class represents a ":" in the expression. It serves as the colon for the
// ternary ?: OPER_erator. It's not an OPER_erand nor is an OPER_erator, nor can we just
// skip it so it is its own thing here.
//
class ExpressionColon final : public ExpressionSymbol
{
	public:
		ExpressionColon() :
			ExpressionSymbol (EXPRSYM_Colon) {}
};

#endif // BOTC_EXPRESSION_H
