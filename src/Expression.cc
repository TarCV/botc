#include "Expression.h"
#include "DataBuffer.h"
#include "Lexer.h"

struct OperatorInfo
{
	ETokenType	token;
	int			priority;
	int			numoperands;
	DataHeader	header;
};

static const OperatorInfo g_Operators[] =
{
	{TK_ExclamationMark,	0,		1,	DH_NegateLogical,	},
	{TK_Minus,				0,		1,	DH_UnaryMinus,		},
	{TK_Multiply,			10,		2,	DH_Multiply,		},
	{TK_Divide,				10,		2,	DH_Divide,			},
	{TK_Modulus,			10,		2,	DH_Modulus,			},
	{TK_Plus,				20,		2,	DH_Add,				},
	{TK_Minus,				20,		2,	DH_Subtract,		},
	{TK_LeftShift,			30,		2,	DH_LeftShift,		},
	{TK_RightShift,			30,		2,	DH_RightShift,		},
	{TK_Lesser,				40,		2,	DH_LessThan,		},
	{TK_Greater,			40,		2,	DH_GreaterThan,		},
	{TK_AtLeast,			40,		2,	DH_AtLeast,			},
	{TK_AtMost,				40,		2,	DH_AtMost,			},
	{TK_Equals,				50,		2,	DH_Equals			},
	{TK_NotEquals,			50,		2,	DH_NotEquals		},
	{TK_Amperstand,			60,		2,	DH_AndBitwise		},
	{TK_Caret,				70,		2,	DH_EorBitwise		},
	{TK_Bar,				80,		2,	DH_OrBitwise		},
	{TK_DoubleAmperstand,	90,		2,	DH_AndLogical		},
	{TK_DoubleBar,			100,	2,	DH_OrLogical		},
	{TK_QuestionMark,		110,	3,	(DataHeader) 0		},
};

// =============================================================================
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

// =============================================================================
//
Expression::~Expression()
{
	for (ExpressionSymbol* sym : m_symbols)
		delete sym;
}

// =============================================================================
//
// Try to parse an expression symbol (i.e. an OPER_erator or OPER_erand or a colon)
// from the lexer.
//
ExpressionSymbol* Expression::parseSymbol()
{
	int pos = m_lexer->position();
	ExpressionValue* op = null;

	if (m_lexer->next (TK_Colon))
		return new ExpressionColon;

	// Check for OPER_erator
	for (const OperatorInfo& op : g_Operators)
		if (m_lexer->next (op.token))
			return new ExpressionOperator ((ExpressionOperatorType) (&op - &g_Operators[0]));

	// Check sub-expression
	if (m_lexer->next (TK_ParenStart))
	{
		Expression expr (m_parser, m_lexer, m_type);
		m_lexer->mustGetNext (TK_ParenEnd);
		return expr.getResult()->clone();
	}

	op = new ExpressionValue (m_type);

	// Check function
	if (CommandInfo* comm = findCommandByName (m_lexer->peekNextString()))
	{
		m_lexer->skip();

		if (m_type != TYPE_Unknown && comm->returnvalue != m_type)
			error ("%1 returns an incompatible data type", comm->name);

		op->setBuffer (m_parser->parseCommand (comm));
		return op;
	}

	// Check for variables
	if (m_lexer->next (TK_DollarSign))
	{
		m_lexer->mustGetNext (TK_Symbol);
		Variable* var = m_parser->findVariable (getTokenString());

		if (var == null)
			error ("unknown variable %1", getTokenString());

		if (var->type != m_type)
			error ("expression requires %1, variable $%2 is of type %3",
				dataTypeName (m_type), var->name, dataTypeName (var->type));

		if (var->isarray)
		{
			m_lexer->mustGetNext (TK_BracketStart);
			Expression expr (m_parser, m_lexer, TYPE_Int);
			expr.getResult()->convertToBuffer();
			DataBuffer* buf = expr.getResult()->buffer()->clone();
			buf->writeDWord (DH_PushGlobalArray);
			buf->writeDWord (var->index);
			op->setBuffer (buf);
			m_lexer->mustGetNext (TK_BracketEnd);
		}
		elif (var->writelevel == WRITE_Constexpr)
			op->setValue (var->value);
		else
		{
			DataBuffer* buf = new DataBuffer (8);

			if (var->IsGlobal())
				buf->writeDWord (DH_PushGlobalVar);
			else
				buf->writeDWord (DH_PushLocalVar);

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
			error ("unknown identifier `%1` (expected keyword, function or variable)", getTokenString());
			break;
		}

		case TYPE_Bool:
		{
			if (m_lexer->next (TK_True) || m_lexer->next (TK_False))
			{
				ETokenType tt = m_lexer->tokenType();
				op->setValue (tt == TK_True ? 1 : 0);
				return op;
			}
		}

		case TYPE_Int:
		{
			if (m_lexer->next (TK_Number))
			{
				op->setValue (getTokenString().toLong());
				return op;
			}
		}

		case TYPE_String:
		{
			if (m_lexer->next (TK_String))
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

// =============================================================================
//
// The symbol parsing process only does token-based checking for OPER_erators.
// Thus ALL minus OPER_erators are actually unary minuses simply because both
// have TK_Minus as their token and the unary minus is prior to the binary minus
// in the OPER_erator table. Now that we have all symbols present, we can
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
		if (op->id() == OPER_UnaryMinus && (*(it - 1))->type() == EXPRSYM_Value)
			op->setID (OPER_Subtraction);
	}
}

// =============================================================================
//
// Verifies a single value. Helper function for Expression::verify.
//
void Expression::tryVerifyValue (bool* verified, SymbolList::Iterator it)
{
	// If it's an unary OPER_erator we skip to its value. The actual OPER_erator will
	// be verified separately.
	if ((*it)->type() == EXPRSYM_Operator &&
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

// =============================================================================
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
		error ("Cannot perform OPER_erations on strings");

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
				// -	unary OPER_erator is not the last symbol
				// -	unary OPER_erator is succeeded by a value symbol
				// -	neither symbol overlaps with something already verified
				tryVerifyValue (verified, it + 1);

				if (it == last || verified[i] == true)
					error ("malformed expression");

				verified[i] = true;
				break;
			}

			case 2:
			{
				// Ensure that:
				// -	binary OPER_erator is not the first or last symbol
				// -	is preceded and succeeded by values
				// -	none of the three tokens are already verified
				//
				// Basically similar logic as above.
				if (it == first || it == last || verified[i] == true)
					error ("malformed expression");

				tryVerifyValue (verified, it + 1);
				tryVerifyValue (verified, it - 1);
				verified[i] = true;
				break;
			}

			case 3:
			{
				// Ternary OPER_erator case. This goes a bit nuts.
				// This time we have the following:
				//
				// (VALUE) ? (VALUE) : (VALUE)
				//         ^
				// --------/ we are here
				//
				// Check that the:
				// -	questionmark OPER_erator is not misplaced (first or last)
				// -	the value behind the OPER_erator (-1) is valid
				// -	the value after the OPER_erator (+1) is valid
				// -	the value after the colon (+3) is valid
				// -	none of the five tokens are verified
				//
				tryVerifyValue (verified, it - 1);
				tryVerifyValue (verified, it + 1);
				tryVerifyValue (verified, it + 3);

				if (it == first ||
					it >= m_symbols.end() - 3 ||
					verified[i] == true ||
					verified[i + 2] == true ||
					(*(it + 2))->type() != EXPRSYM_Colon)
				{
					error ("malformed expression");
				}

				verified[i] = true;
				verified[i + 2] = true;
				break;
			}

			default:
				error ("WTF OPER_erator with %1 OPER_erands", numoperands);
		}
	}

	for (int i = 0; i < m_symbols.size(); ++i)
		if (verified[i] == false)
			error ("malformed expression: expr symbol #%1 is was left unverified", i);

	delete verified;
}


// =============================================================================
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

// =============================================================================
//
// Process the given OPER_erator and values into a new value.
//
ExpressionValue* Expression::evaluateOperator (const ExpressionOperator* op,
											   const List<ExpressionValue*>& values)
{
	const OperatorInfo* info = &g_Operators[op->id()];
	bool isconstexpr = true;
	assert (values.size() == info->numoperands);

	for (ExpressionValue* val : values)
	{
		if (val->isConstexpr() == false)
		{
			isconstexpr = false;
			break;
		}
	}

	// If not all of the values are constant expressions, none of them shall be.
	if (isconstexpr == false)
		for (ExpressionValue* val : values)
			val->convertToBuffer();

	ExpressionValue* newval = new ExpressionValue (m_type);

	if (isconstexpr == false)
	{
		// This is not a constant expression so we'll have to use databuffers
		// to convey the expression to bytecode. Actual value cannot be evaluated
		// until Zandronum processes it at run-time.
		newval->setBuffer (new DataBuffer);

		if (op->id() == OPER_Ternary)
		{
			// There isn't a dataheader for ternary OPER_erator. Instead, we use DH_IfNotGoto
			// to create an "if-block" inside an expression.
			// Behold, big block of writing madness! :P
			//
			DataBuffer* buf = newval->buffer();
			DataBuffer* b0 = values[0]->buffer();
			DataBuffer* b1 = values[1]->buffer();
			DataBuffer* b2 = values[2]->buffer();
			ByteMark* mark1 = buf->addMark (""); // start of "else" case
			ByteMark* mark2 = buf->addMark (""); // end of expression
			buf->mergeAndDestroy (b0);
			buf->writeDWord (DH_IfNotGoto); // if the first OPER_erand (condition)
			buf->addReference (mark1); // didn't eval true, jump into mark1
			buf->mergeAndDestroy (b1); // otherwise, perform second OPER_erand (true case)
			buf->writeDWord (DH_Goto); // afterwards, jump to the end, which is
			buf->addReference (mark2); // marked by mark2.
			buf->adjustMark (mark1); // move mark1 at the end of the true case
			buf->mergeAndDestroy (b2); // perform third OPER_erand (false case)
			buf->adjustMark (mark2); // move the ending mark2 here

			for (int i = 0; i < 3; ++i)
				values[i]->setBuffer (null);
		}
		else
		{
			// Generic case: write all arguments and apply the OPER_erator's
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
			case OPER_LogicalAnd:			a = (nums[0] && nums[1]) ? 1 : 0;		break;
			case OPER_LogicalOr:			a = (nums[0] || nums[1]) ? 1 : 0;		break;
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

// =============================================================================
//
ExpressionValue* Expression::evaluate()
{
	SymbolList::Iterator it;

	while ((it = findPrioritizedOperator()) != m_symbols.end())
	{
		int i = it - m_symbols.begin();
		List<SymbolList::Iterator> OPER_erands;
		ExpressionOperator* op = static_cast<ExpressionOperator*> (*it);
		const OperatorInfo* info = &g_Operators[op->id()];
		int lower, upper; // Boundaries of area to replace

		switch (info->numoperands)
		{
			case 1:
			{
				lower = i;
				upper = i + 1;
				OPER_erands << it + 1;
				break;
			}

			case 2:
			{
				lower = i - 1;
				upper = i + 1;
				OPER_erands << it - 1
				         << it + 1;
				break;
			}

			case 3:
			{
				lower = i - 1;
				upper = i + 3;
				OPER_erands << it - 1
				         << it + 1
				         << it + 3;
				break;
			}

			default:
				assert (false);
		}

		List<ExpressionValue*> values;

		for (auto it : OPER_erands)
			values << static_cast<ExpressionValue*> (*it);

		// Note: @op and all of @values are invalid after this call.
		ExpressionValue* newvalue = evaluateOperator (op, values);

		for (int i = upper; i >= lower; --i)
			m_symbols.removeAt (i);

		m_symbols.insert (lower, newvalue);
	}

	assert (m_symbols.size() == 1 && m_symbols.first()->type() == EXPRSYM_Value);
	ExpressionValue* val = static_cast<ExpressionValue*> (m_symbols.first());
	return val;
}

// =============================================================================
//
ExpressionValue* Expression::getResult()
{
	return static_cast<ExpressionValue*> (m_symbols.first());
}

// =============================================================================
//
String Expression::getTokenString()
{
	return m_lexer->token()->text;
}

// =============================================================================
//
ExpressionOperator::ExpressionOperator (ExpressionOperatorType id) :
	ExpressionSymbol (EXPRSYM_Operator),
	m_id (id) {}

// =============================================================================
//
ExpressionValue::ExpressionValue (DataType valuetype) :
	ExpressionSymbol (EXPRSYM_Value),
	m_buffer (null),
	m_valueType (valuetype) {}

// =============================================================================
//
ExpressionValue::~ExpressionValue()
{
	delete m_buffer;
}

// =============================================================================
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
			buffer()->writeDWord (DH_PushNumber);
			buffer()->writeDWord (abs (value()));

			if (value() < 0)
				buffer()->writeDWord (DH_UnaryMinus);
			break;

		case TYPE_String:
			buffer()->writeDWord (DH_PushStringIndex);
			buffer()->writeDWord (value());
			break;

		case TYPE_Void:
		case TYPE_Unknown:
			assert (false);
			break;
	}
}
