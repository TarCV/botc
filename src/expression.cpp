#include "expression.h"
#include "dataBuffer.h"
#include "lexer.h"

struct OperatorInfo
{
	Token	token;
	int			priority;
	int			numoperands;
	DataHeader	header;
};

static const OperatorInfo g_Operators[] =
{
	{Token::ExclamationMark,	0,		1,	DataHeader::NegateLogical,	},
	{Token::Minus,				0,		1,	DataHeader::UnaryMinus,		},
	{Token::Multiply,			10,		2,	DataHeader::Multiply,		},
	{Token::Divide,				10,		2,	DataHeader::Divide,			},
	{Token::Modulus,			10,		2,	DataHeader::Modulus,		},
	{Token::Plus,				20,		2,	DataHeader::Add,			},
	{Token::Minus,				20,		2,	DataHeader::Subtract,		},
	{Token::LeftShift,			30,		2,	DataHeader::LeftShift,		},
	{Token::RightShift,			30,		2,	DataHeader::RightShift,		},
	{Token::Lesser,				40,		2,	DataHeader::LessThan,		},
	{Token::Greater,			40,		2,	DataHeader::GreaterThan,	},
	{Token::AtLeast,			40,		2,	DataHeader::AtLeast,		},
	{Token::AtMost,				40,		2,	DataHeader::AtMost,			},
	{Token::Equals,				50,		2,	DataHeader::Equals			},
	{Token::NotEquals,			50,		2,	DataHeader::NotEquals		},
	{Token::Amperstand,			60,		2,	DataHeader::AndBitwise		},
	{Token::Caret,				70,		2,	DataHeader::EorBitwise		},
	{Token::Bar,				80,		2,	DataHeader::OrBitwise		},
	{Token::DoubleAmperstand,	90,		2,	DataHeader::AndLogical		},
	{Token::DoubleBar,			100,	2,	DataHeader::OrLogical		},
	{Token::QuestionMark,		110,	3,	DataHeader::NumDataHeaders	},
};

// -------------------------------------------------------------------------------------------------
//
Expression::Expression (BotscriptParser* parser, Lexer* lx, DataType reqtype) :
	m_parser (parser),
	m_lexer (lx),
	m_type (reqtype)
{
	ExpressionSymbol* sym;

	while ((sym = parseSymbol()) != null)
		m_symbols << sym;

	// If we were unable to get any expression symbols, something's wonky with
	// the script. Report an error. mBadTokenText is set to the token that
	// ParseSymbol ends at when it returns false.
	if (m_symbols.isEmpty())
		error ("unknown identifier '%1'", m_badTokenText);

	adjustOperators();
	verify();
	evaluate();
}

// -------------------------------------------------------------------------------------------------
//
Expression::~Expression()
{
	for (ExpressionSymbol* sym : m_symbols)
		delete sym;
}

// -------------------------------------------------------------------------------------------------
//
// Try to parse an expression symbol (i.e. an operator or operand or a colon)
// from the lexer.
//
ExpressionSymbol* Expression::parseSymbol()
{
	int pos = m_lexer->position();
	ExpressionValue* op = null;

	if (m_lexer->next (Token::Colon))
		return new ExpressionColon;

	// Check for operator
	for (const OperatorInfo& op : g_Operators)
	{
		if (m_lexer->next (op.token))
			return new ExpressionOperator ((ExpressionOperatorType) (&op - &g_Operators[0]));
	}

	// Check sub-expression
	if (m_lexer->next (Token::ParenStart))
	{
		Expression expr (m_parser, m_lexer, m_type);
		m_lexer->mustGetNext (Token::ParenEnd);
		return expr.getResult()->clone();
	}

	op = new ExpressionValue (m_type);

	// Check function
	if (CommandInfo* comm = findCommandByName (m_lexer->peekNextString()))
	{
		m_lexer->skip();

		if (m_type != TYPE_Unknown and comm->returnvalue != m_type)
			error ("%1 returns an incompatible data type", comm->name);

		op->setBuffer (m_parser->parseCommand (comm));
		return op;
	}

	// Check for variables
	if (m_lexer->next (Token::DollarSign))
	{
		m_lexer->mustGetNext (Token::Symbol);
		Variable* var = m_parser->findVariable (getTokenString());

		if (var == null)
			error ("unknown variable %1", getTokenString());

		if (var->type != m_type)
		{
			error ("expression requires %1, variable $%2 is of type %3",
				dataTypeName (m_type), var->name, dataTypeName (var->type));
		}

		if (var->isarray)
		{
			m_lexer->mustGetNext (Token::BracketStart);
			Expression expr (m_parser, m_lexer, TYPE_Int);
			expr.getResult()->convertToBuffer();
			DataBuffer* buf = expr.getResult()->buffer()->clone();
			buf->writeDWord (DataHeader::PushGlobalArray);
			buf->writeDWord (var->index);
			op->setBuffer (buf);
			m_lexer->mustGetNext (Token::BracketEnd);
		}
		elif (var->writelevel == WRITE_Constexpr)
		{
			op->setValue (var->value);
		}
		else
		{
			DataBuffer* buf = new DataBuffer (8);

			if (var->isGlobal())
				buf->writeDWord (DataHeader::PushGlobalVar);
			else
				buf->writeDWord (DataHeader::PushLocalVar);

			buf->writeDWord (var->index);
			op->setBuffer (buf);
		}

		return op;
	}

	// Check for literal
	switch (m_type)
	{
		case TYPE_Void:
		case TYPE_Unknown:
		{
			error ("unknown identifier `%1` (expected keyword, function or variable)",
				getTokenString());
			break;
		}

		case TYPE_Bool:
		{
			if (m_lexer->next (Token::True) or m_lexer->next (Token::False))
			{
				Token tt = m_lexer->tokenType();
				op->setValue (tt == Token::True ? 1 : 0);
				return op;
			}
		}

		case TYPE_Int:
		{
			if (m_lexer->next (Token::Number))
			{
				op->setValue (getTokenString().toLong());
				return op;
			}
		}

		case TYPE_String:
		{
			if (m_lexer->next (Token::String))
			{
				op->setValue (getStringTableIndex (getTokenString()));
				return op;
			}
		}
	}

	m_badTokenText = m_lexer->token()->text;
	m_lexer->setPosition (pos);
	delete op;
	return null;
}

// -------------------------------------------------------------------------------------------------
//
// The symbol parsing process only does token-based checking for operators.
// Thus ALL minus operators are actually unary minuses simply because both
// have Token::Minus as their token and the unary minus is prior to the binary minus
// in the operator table. Now that we have all symbols present, we can
// correct cases like this.
//
void Expression::adjustOperators()
{
	for (auto it = m_symbols.begin() + 1; it != m_symbols.end(); ++it)
	{
		if ((*it)->type() != EXPRSYM_Operator)
			continue;

		ExpressionOperator* op = static_cast<ExpressionOperator*> (*it);

		// Unary minus with a value as the previous symbol cannot really be
		// unary; replace with binary minus.
		if (op->id() == OPER_UnaryMinus and (*(it - 1))->type() == EXPRSYM_Value)
			op->setID (OPER_Subtraction);
	}
}

// -------------------------------------------------------------------------------------------------
//
// Verifies a single value. Helper function for Expression::verify.
//
void Expression::tryVerifyValue (bool* verified, SymbolList::Iterator it)
{
	// If it's an unary operator we skip to its value. The actual operator will
	// be verified separately.
	if ((*it)->type() == EXPRSYM_Operator and
			g_Operators[static_cast<ExpressionOperator*> (*it)->id()].numoperands == 1)
	{
		++it;
	}

	int i = it - m_symbols.begin();

	// Ensure it's an actual value
	if ((*it)->type() != EXPRSYM_Value)
		error ("malformed expression (symbol #%1 is not a value)", i);

	verified[i] = true;
}

// -------------------------------------------------------------------------------------------------
//
// Ensures the expression is valid and well-formed and not OMGWTFBBQ. Throws an
// error if this is not the case.
//
void Expression::verify()
{
	if (m_symbols.size() == 1)
	{
		if (m_symbols[0]->type() != EXPRSYM_Value)
			error ("bad expression");

		return;
	}

	if (m_type == TYPE_String)
		error ("Cannot perform operations on strings");

	bool* verified = new bool[m_symbols.size()];
	memset (verified, 0, m_symbols.size() * sizeof (decltype (*verified)));
	const auto last = m_symbols.end() - 1;
	const auto first = m_symbols.begin();

	for (auto it = m_symbols.begin(); it != m_symbols.end(); ++it)
	{
		int i = (it - first);

		if ((*it)->type() != EXPRSYM_Operator)
			continue;

		ExpressionOperator* op = static_cast<ExpressionOperator*> (*it);
		int numoperands = g_Operators[op->id()].numoperands;

		switch (numoperands)
		{
			case 1:
			{
				// Ensure that:
				// -	unary operator is not the last symbol
				// -	unary operator is succeeded by a value symbol
				// -	neither symbol overlaps with something already verified
				tryVerifyValue (verified, it + 1);

				if (it == last or verified[i] == true)
					error ("ill-formed expression");

				verified[i] = true;
				break;
			}

			case 2:
			{
				// Ensure that:
				// -	binary operator is not the first or last symbol
				// -	is preceded and succeeded by values
				// -	none of the three tokens are already verified
				//
				// Basically similar logic as above.
				if (it == first or it == last or verified[i] == true)
					error ("ill-formed expression");

				tryVerifyValue (verified, it + 1);
				tryVerifyValue (verified, it - 1);
				verified[i] = true;
				break;
			}

			case 3:
			{
				// Ternary operator case. This goes a bit nuts.
				// This time we have the following:
				//
				// (VALUE) ? (VALUE) : (VALUE)
				//         ^
				// --------/ we are here
				//
				// Check that the:
				// -	questionmark operator is not misplaced (first or last)
				// -	the value behind the operator (-1) is valid
				// -	the value after the operator (+1) is valid
				// -	the value after the colon (+3) is valid
				// -	none of the five tokens are verified
				//
				tryVerifyValue (verified, it - 1);
				tryVerifyValue (verified, it + 1);
				tryVerifyValue (verified, it + 3);

				if (it == first
					or it >= m_symbols.end() - 3
					or verified[i] == true
					or verified[i + 2] == true
					or (*(it + 2))->type() != EXPRSYM_Colon)
				{
					error ("ill-formed expression");
				}

				verified[i] = true;
				verified[i + 2] = true;
				break;
			}

			default:
				error ("WTF operator with %1 operands", numoperands);
		}
	}

	for (int i = 0; i < m_symbols.size(); ++i)
	{
		if (verified[i] == false)
			error ("malformed expression: expr symbol #%1 is was left unverified", i);
	}

	delete verified;
}


// -------------------------------------------------------------------------------------------------
//
// Which operator to evaluate?
//
Expression::SymbolList::Iterator Expression::findPrioritizedOperator()
{
	SymbolList::Iterator	best = m_symbols.end();
	int						bestpriority = __INT_MAX__;

	for (SymbolList::Iterator it = m_symbols.begin(); it != m_symbols.end(); ++it)
	{
		if ((*it)->type() != EXPRSYM_Operator)
			continue;

		ExpressionOperator* op = static_cast<ExpressionOperator*> (*it);
		const OperatorInfo* info = &g_Operators[op->id()];

		if (info->priority < bestpriority)
		{
			best = it;
			bestpriority = info->priority;
		}
	}

	return best;
}

// -------------------------------------------------------------------------------------------------
//
// Process the given operator and values into a new value.
//
ExpressionValue* Expression::evaluateOperator (const ExpressionOperator* op,
											   const List<ExpressionValue*>& values)
{
	const OperatorInfo* info = &g_Operators[op->id()];
	bool isconstexpr = true;
	ASSERT_EQ (values.size(), info->numoperands)

	// See whether the values are constexpr
	for (ExpressionValue* val : values)
	{
		if (not val->isConstexpr())
		{
			isconstexpr = false;
			break;
		}
	}

	// If not all of the values are constexpr, none of them shall be.
	if (not isconstexpr)
	{
		for (ExpressionValue* val : values)
			val->convertToBuffer();
	}

	ExpressionValue* newval = new ExpressionValue (m_type);

	if (isconstexpr == false)
	{
		// This is not a constant expression so we'll have to use databuffers
		// to convey the expression to bytecode. Actual value cannot be evaluated
		// until Zandronum processes it at run-time.
		newval->setBuffer (new DataBuffer);

		if (op->id() == OPER_Ternary)
		{
			// There isn't a dataheader for ternary operator. Instead, we use DataHeader::IfNotGoto
			// to create an "if-block" inside an expression. Behold, big block of writing madness!
			DataBuffer* buf = newval->buffer();
			DataBuffer* b0 = values[0]->buffer();
			DataBuffer* b1 = values[1]->buffer();
			DataBuffer* b2 = values[2]->buffer();
			ByteMark* mark1 = buf->addMark (""); // start of "else" case
			ByteMark* mark2 = buf->addMark (""); // end of expression
			buf->mergeAndDestroy (b0);
			buf->writeDWord (DataHeader::IfNotGoto); // if the first operand (condition)
			buf->addReference (mark1); // didn't eval true, jump into mark1
			buf->mergeAndDestroy (b1); // otherwise, perform second operand (true case)
			buf->writeDWord (DataHeader::Goto); // afterwards, jump to the end, which is
			buf->addReference (mark2); // marked by mark2.
			buf->adjustMark (mark1); // move mark1 at the end of the true case
			buf->mergeAndDestroy (b2); // perform third operand (false case)
			buf->adjustMark (mark2); // move the ending mark2 here

			for (int i = 0; i < 3; ++i)
				values[i]->setBuffer (null);
		}
		else
		{
			ASSERT_NE (info->header, DataHeader::NumDataHeaders);

			// Generic case: write all arguments and apply the operator's
			// data header.
			for (ExpressionValue* val : values)
			{
				newval->buffer()->mergeAndDestroy (val->buffer());

				// Null the pointer out so that the value's destructor will not
				// attempt to double-free it.
				val->setBuffer (null);
			}

			newval->buffer()->writeDWord (info->header);
		}
	}
	else
	{
		// We have a constant expression. We know all the values involved and
		// can thus compute the result of this expression on compile-time.
		List<int> nums;
		int a;

		for (ExpressionValue* val : values)
			nums << val->value();

		switch (op->id())
		{
			case OPER_Addition:				a = nums[0] + nums[1];					break;
			case OPER_Subtraction:			a = nums[0] - nums[1];					break;
			case OPER_Multiplication:		a = nums[0] * nums[1];					break;
			case OPER_UnaryMinus:			a = -nums[0];							break;
			case OPER_NegateLogical:		a = !nums[0];							break;
			case OPER_LeftShift:			a = nums[0] << nums[1];					break;
			case OPER_RightShift:			a = nums[0] >> nums[1];					break;
			case OPER_CompareLesser:		a = (nums[0] < nums[1]) ? 1 : 0;		break;
			case OPER_CompareGreater:		a = (nums[0] > nums[1]) ? 1 : 0;		break;
			case OPER_CompareAtLeast:		a = (nums[0] <= nums[1]) ? 1 : 0;		break;
			case OPER_CompareAtMost:		a = (nums[0] >= nums[1]) ? 1 : 0;		break;
			case OPER_CompareEquals:		a = (nums[0] == nums[1]) ? 1 : 0;		break;
			case OPER_CompareNotEquals:		a = (nums[0] != nums[1]) ? 1 : 0;		break;
			case OPER_BitwiseAnd:			a = nums[0] & nums[1];					break;
			case OPER_BitwiseOr:			a = nums[0] | nums[1];					break;
			case OPER_BitwiseXOr:			a = nums[0] ^ nums[1];					break;
			case OPER_LogicalAnd:			a = (nums[0] and nums[1]) ? 1 : 0;		break;
			case OPER_LogicalOr:			a = (nums[0] or nums[1]) ? 1 : 0;		break;
			case OPER_Ternary:				a = (nums[0] != 0) ? nums[1] : nums[2];	break;

			case OPER_Division:
			{
				if (nums[1] == 0)
					error ("division by zero in constant expression");

				a = nums[0] / nums[1];
				break;
			}

			case OPER_Modulus:
			{
				if (nums[1] == 0)
					error ("modulus by zero in constant expression");

				a = nums[0] % nums[1];
				break;
			}
		}

		newval->setValue (a);
	}

	// The new value has been generated. We don't need the old stuff anymore.
	for (ExpressionValue* val : values)
		delete val;

	delete op;
	return newval;
}

// -------------------------------------------------------------------------------------------------
//
ExpressionValue* Expression::evaluate()
{
	SymbolList::Iterator it;

	while ((it = findPrioritizedOperator()) != m_symbols.end())
	{
		int i = it - m_symbols.begin();
		List<SymbolList::Iterator> operands;
		ExpressionOperator* op = static_cast<ExpressionOperator*> (*it);
		const OperatorInfo* info = &g_Operators[op->id()];
		int lower, upper; // Boundaries of area to replace

		switch (info->numoperands)
		{
			case 1:
			{
				lower = i;
				upper = i + 1;
				operands << it + 1;
				break;
			}

			case 2:
			{
				lower = i - 1;
				upper = i + 1;
				operands << it - 1
				         << it + 1;
				break;
			}

			case 3:
			{
				lower = i - 1;
				upper = i + 3;
				operands << it - 1
				         << it + 1
				         << it + 3;
				break;
			}

			default:
				error ("WTF bad expression with %1 operands", info->numoperands);
		}

		List<ExpressionValue*> values;

		for (auto it : operands)
			values << static_cast<ExpressionValue*> (*it);

		// Note: @op and all of @values are invalid after this call.
		ExpressionValue* newvalue = evaluateOperator (op, values);

		for (int i = upper; i >= lower; --i)
			m_symbols.removeAt (i);

		m_symbols.insert (lower, newvalue);
	}

	ASSERT_EQ (m_symbols.size(), 1)
	ASSERT_EQ (m_symbols.first()->type(), EXPRSYM_Value)
	ExpressionValue* val = static_cast<ExpressionValue*> (m_symbols.first());
	return val;
}

// -------------------------------------------------------------------------------------------------
//
ExpressionValue* Expression::getResult()
{
	return static_cast<ExpressionValue*> (m_symbols.first());
}

// -------------------------------------------------------------------------------------------------
//
String Expression::getTokenString()
{
	return m_lexer->token()->text;
}

// -------------------------------------------------------------------------------------------------
//
ExpressionOperator::ExpressionOperator (ExpressionOperatorType id) :
	ExpressionSymbol (EXPRSYM_Operator),
	m_id (id) {}

// -------------------------------------------------------------------------------------------------
//
ExpressionValue::ExpressionValue (DataType valuetype) :
	ExpressionSymbol (EXPRSYM_Value),
	m_buffer (null),
	m_valueType (valuetype) {}

// -------------------------------------------------------------------------------------------------
//
ExpressionValue::~ExpressionValue()
{
	delete m_buffer;
}

// -------------------------------------------------------------------------------------------------
//
void ExpressionValue::convertToBuffer()
{
	if (isConstexpr() == false)
		return;

	setBuffer (new DataBuffer);

	switch (m_valueType)
	{
		case TYPE_Bool:
		case TYPE_Int:
			buffer()->writeDWord (DataHeader::PushNumber);
			buffer()->writeDWord (abs (value()));

			if (value() < 0)
				buffer()->writeDWord (DataHeader::UnaryMinus);
			break;

		case TYPE_String:
			buffer()->writeDWord (DataHeader::PushStringIndex);
			buffer()->writeDWord (value());
			break;

		case TYPE_Void:
		case TYPE_Unknown:
			error ("WTF: tried to convert bad expression value type %1 to buffer", m_valueType);
	}
}
