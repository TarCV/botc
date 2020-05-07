/*
	Copyright 2012-2014 Teemu Piippo
	Copyright 2019-2020 TarCV
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice,
	   this list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its
	   contributors may be used to endorse or promote products derived from this
	   software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
	LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
	CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
	SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
	INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef BOTC_EXPRESSION_H
#define BOTC_EXPRESSION_H
#include "parser.h"

class DataBuffer;
class ExpressionSymbol;
class ExpressionValue;
class ExpressionOperator;

// =============================================================================
//
named_enum ExpressionOperatorType : char
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
	void					tryVerifyValue (List<bool>& verified, List< ExpressionSymbol* 
>::Iterator it);
	ExpressionValue*		evaluateOperator (const ExpressionOperator* op,
												const List<ExpressionValue*>& values);
	SymbolList::Iterator	findPrioritizedOperator();
};

// =============================================================================
//
class ExpressionSymbol
{
	PROPERTY (private, ExpressionSymbolType, type, setType, STOCK_WRITE)
public:
	ExpressionSymbol (ExpressionSymbolType type) :
		m_type (type) {}
};

// =============================================================================
//
class ExpressionOperator final : public ExpressionSymbol
{
	PROPERTY (public, ExpressionOperatorType, id, setID, STOCK_WRITE)
	PROPERTY(public, DataType, returnType, setReturnType, STOCK_WRITE)
public:
	ExpressionOperator (ExpressionOperatorType id, DataType returnType);
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
// ternary ?: operator. It's not an operand nor is an operator, nor can we just
// skip it so it is its own thing here.
//
class ExpressionColon final : public ExpressionSymbol
{
public:
	ExpressionColon() :
		ExpressionSymbol (EXPRSYM_Colon) {}
};

#endif // BOTC_EXPRESSION_H
