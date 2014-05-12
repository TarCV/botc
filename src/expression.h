#ifndef BOTC_EXPRESSION_H
#define BOTC_EXPRESSION_H
#include "parser.h"

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
		ExpressionValue*		getResult();

	private:
		BotscriptParser*		m_parser;
		Lexer*					m_lexer;
		SymbolList				m_symbols;
		DataType				m_type;
		String					m_badTokenText;

		ExpressionValue*		evaluate(); // Process the expression and yield a result
		ExpressionSymbol*		parseSymbol();
		String					getTokenString();
		void					adjustOperators();
		void					verify(); // Ensure the expr is valid
		void					tryVerifyValue (bool* verified, SymbolList::Iterator it);
		ExpressionValue*		evaluateOperator (const ExpressionOperator* op,
												  const List<ExpressionValue*>& values);
		SymbolList::Iterator	findPrioritizedOperator();
};

// =============================================================================
//
class ExpressionSymbol
{
	public:
		ExpressionSymbol (ExpressionSymbolType type) :
			m_type (type) {}

	PROPERTY (private, ExpressionSymbolType, type, setType, STOCK_WRITE)
};

// =============================================================================
//
class ExpressionOperator final : public ExpressionSymbol
{
	PROPERTY (public, ExpressionOperatorType, id, setID, STOCK_WRITE)

	public:
		ExpressionOperator (ExpressionOperatorType id);
};

// =============================================================================
//
class ExpressionValue final : public ExpressionSymbol
{
	PROPERTY (public, int,			value,		setValue,		STOCK_WRITE)
	PROPERTY (public, DataBuffer*,	buffer,		setBuffer,		STOCK_WRITE)
	PROPERTY (public, DataType,		valueType,	setValueType,	STOCK_WRITE)

	public:
		ExpressionValue (DataType valuetype);
		~ExpressionValue();

		void					convertToBuffer();

		inline ExpressionValue* clone() const
		{
			return new ExpressionValue (*this);
		}

		inline bool isConstexpr() const
		{
			return buffer() == null;
		}
};

// =============================================================================
//
// This class represents a ":" in the expression. It serves as the colon for the
// ternary ?: operator. It's not an OPER_erand nor is an operator, nor can we just
// skip it so it is its own thing here.
//
class ExpressionColon final : public ExpressionSymbol
{
	public:
		ExpressionColon() :
			ExpressionSymbol (EXPRSYM_Colon) {}
};

#endif // BOTC_EXPRESSION_H
