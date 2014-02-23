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

static const OperatorInfo gOperators[] =
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
	mParser (parser),
	mLexer (lx),
	mType (reqtype)
{
	ExpressionSymbol* sym;

	while ((sym = ParseSymbol()) != null)
		mSymbols << sym;

	// If we were unable to get any expression symbols, something's wonky with
	// the script. Report an error. mBadTokenText is set to the token that
	// ParseSymbol ends at when it returns false.
	if (mSymbols.IsEmpty())
		Error ("unknown identifier '%1'", mBadTokenText);

	AdjustOperators();
	Verify();
	Evaluate();
}

// =============================================================================
//
Expression::~Expression()
{
	for (ExpressionSymbol* sym : mSymbols)
		delete sym;
}

// =============================================================================
//
// Try to parse an expression symbol (i.e. an OPER_erator or OPER_erand or a colon)
// from the lexer.
//
ExpressionSymbol* Expression::ParseSymbol()
{
	int pos = mLexer->Position();
	ExpressionValue* op = null;

	if (mLexer->Next (TK_Colon))
		return new ExpressionColon;

	// Check for OPER_erator
	for (const OperatorInfo& op : gOperators)
		if (mLexer->Next (op.token))
			return new ExpressionOperator ((ExpressionOperatorType) (&op - &gOperators[0]));

	// Check sub-expression
	if (mLexer->Next (TK_ParenStart))
	{
		Expression expr (mParser, mLexer, mType);
		mLexer->MustGetNext (TK_ParenEnd);
		return expr.Result()->Clone();
	}

	op = new ExpressionValue (mType);

	// Check function
	if (CommandInfo* comm = FindCommandByName (mLexer->PeekNextString()))
	{
		mLexer->Skip();

		if (mType != TYPE_Unknown && comm->returnvalue != mType)
			Error ("%1 returns an incompatible data type", comm->name);

		op->SetBuffer (mParser->ParseCommand (comm));
		return op;
	}

	// Check for variables
	if (mLexer->Next (TK_DollarSign))
	{
		mLexer->MustGetNext (TK_Symbol);
		Variable* var = mParser->FindVariable (TokenString());

		if (var == null)
			Error ("unknown variable %1", TokenString());

		if (var->type != mType)
			Error ("expression requires %1, variable $%2 is of type %3",
				DataTypeName (mType), var->name, DataTypeName (var->type));

		if (var->isarray)
		{
			mLexer->MustGetNext (TK_BracketStart);
			Expression expr (mParser, mLexer, TYPE_Int);
			expr.Result()->ConvertToBuffer();
			DataBuffer* buf = expr.Result()->Buffer()->Clone();
			buf->WriteDWord (DH_PushGlobalArray);
			buf->WriteDWord (var->index);
			op->SetBuffer (buf);
			mLexer->MustGetNext (TK_BracketEnd);
		}
		elif (var->writelevel == WRITE_Constexpr)
			op->SetValue (var->value);
		else
		{
			DataBuffer* buf = new DataBuffer (8);

			if (var->IsGlobal())
				buf->WriteDWord (DH_PushGlobalVar);
			else
				buf->WriteDWord (DH_PushLocalVar);

			buf->WriteDWord (var->index);
			op->SetBuffer (buf);
		}

		return op;
	}

	// Check for literal
	switch (mType)
	{
		case TYPE_Void:
		case TYPE_Unknown:
		{
			Error ("unknown identifier `%1` (expected keyword, function or variable)", TokenString());
			break;
		}

		case TYPE_Bool:
		{
			if (mLexer->Next (TK_True) || mLexer->Next (TK_False))
			{
				ETokenType tt = mLexer->TokenType();
				op->SetValue (tt == TK_True ? 1 : 0);
				return op;
			}
		}

		case TYPE_Int:
		{
			if (mLexer->Next (TK_Number))
			{
				op->SetValue (TokenString().ToLong());
				return op;
			}
		}

		case TYPE_String:
		{
			if (mLexer->Next (TK_String))
			{
				op->SetValue (StringTableIndex (TokenString()));
				return op;
			}
		}
	}

	mBadTokenText = mLexer->Token()->text;
	mLexer->SetPosition (pos);
	delete op;
	return null;
}

// =============================================================================
//
// The symbol parsing process only does token-based checking for OPER_erators. Thus
// ALL minus OPER_erators are actually unary minuses simply because both have
//TK_Minus as their token and the unary minus is prior to the binary minus in
// the OPER_erator table. Now that we have all symbols present, we can correct
// cases like this.
//
void Expression::AdjustOperators()
{
	for (auto it = mSymbols.begin() + 1; it != mSymbols.end(); ++it)
	{
		if ((*it)->Type() != EXPRSYM_Operator)
			continue;

		ExpressionOperator* op = static_cast<ExpressionOperator*> (*it);

		// Unary minus with a value as the previous symbol cannot really be
		// unary; replace with binary minus.
		if (op->ID() == OPER_UnaryMinus && (*(it - 1))->Type() == EXPRSYM_Value)
			op->SetID (OPER_Subtraction);
	}
}

// =============================================================================
//
// Verifies a single value. Helper function for Expression::Verify.
//
void Expression::TryVerifyValue (bool* verified, SymbolList::Iterator it)
{
	// If it's an unary OPER_erator we skip to its value. The actual OPER_erator will
	// be verified separately.
	if ((*it)->Type() == EXPRSYM_Operator &&
		gOperators[static_cast<ExpressionOperator*> (*it)->ID()].numoperands == 1)
	{
		++it;
	}

	int i = it - mSymbols.begin();

	// Ensure it's an actual value
	if ((*it)->Type() != EXPRSYM_Value)
		Error ("malformed expression (symbol #%1 is not a value)", i);

	verified[i] = true;
}

// =============================================================================
//
// Ensures the expression is valid and well-formed and not OMGWTFBBQ. Throws an
// error if this is not the case.
//
void Expression::Verify()
{
	if (mSymbols.Size() == 1)
	{
		if (mSymbols[0]->Type() != EXPRSYM_Value)
			Error ("bad expression");

		return;
	}

	if (mType == TYPE_String)
		Error ("Cannot perform OPER_erations on strings");

	bool* verified = new bool[mSymbols.Size()];
	memset (verified, 0, mSymbols.Size() * sizeof (decltype (*verified)));
	const auto last = mSymbols.end() - 1;
	const auto first = mSymbols.begin();

	for (auto it = mSymbols.begin(); it != mSymbols.end(); ++it)
	{
		int i = (it - first);

		if ((*it)->Type() != EXPRSYM_Operator)
			continue;

		ExpressionOperator* op = static_cast<ExpressionOperator*> (*it);
		int numoperands = gOperators[op->ID()].numoperands;

		switch (numoperands)
		{
			case 1:
			{
				// Ensure that:
				// -	unary OPER_erator is not the last symbol
				// -	unary OPER_erator is succeeded by a value symbol
				// -	neither symbol overlaps with something already verified
				TryVerifyValue (verified, it + 1);

				if (it == last || verified[i] == true)
					Error ("malformed expression");

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
					Error ("malformed expression");

				TryVerifyValue (verified, it + 1);
				TryVerifyValue (verified, it - 1);
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
				TryVerifyValue (verified, it - 1);
				TryVerifyValue (verified, it + 1);
				TryVerifyValue (verified, it + 3);

				if (it == first ||
					it >= mSymbols.end() - 3 ||
					verified[i] == true ||
					verified[i + 2] == true ||
					(*(it + 2))->Type() != EXPRSYM_Colon)
				{
					Error ("malformed expression");
				}

				verified[i] = true;
				verified[i + 2] = true;
				break;
			}

			default:
				Error ("WTF OPER_erator with %1 OPER_erands", numoperands);
		}
	}

	for (int i = 0; i < mSymbols.Size(); ++i)
		if (verified[i] == false)
			Error ("malformed expression: expr symbol #%1 is was left unverified", i);

	delete verified;
}


// =============================================================================
//
// Which OPER_erator to evaluate?
//
Expression::SymbolList::Iterator Expression::FindPrioritizedOperator()
{
	SymbolList::Iterator	best = mSymbols.end();
	int						bestpriority = __INT_MAX__;

	for (SymbolList::Iterator it = mSymbols.begin(); it != mSymbols.end(); ++it)
	{
		if ((*it)->Type() != EXPRSYM_Operator)
			continue;

		ExpressionOperator* op = static_cast<ExpressionOperator*> (*it);
		const OperatorInfo* info = &gOperators[op->ID()];

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
ExpressionValue* Expression::EvaluateOperator (const ExpressionOperator* op,
											   const List<ExpressionValue*>& values)
{
	const OperatorInfo* info = &gOperators[op->ID()];
	bool isconstexpr = true;
	assert (values.Size() == info->numoperands);

	for (ExpressionValue* val : values)
	{
		if (val->IsConstexpr() == false)
		{
			isconstexpr = false;
			break;
		}
	}

	// If not all of the values are constant expressions, none of them shall be.
	if (isconstexpr == false)
		for (ExpressionValue* val : values)
			val->ConvertToBuffer();

	ExpressionValue* newval = new ExpressionValue (mType);

	if (isconstexpr == false)
	{
		// This is not a constant expression so we'll have to use databuffers
		// to convey the expression to bytecode. Actual value cannot be evaluated
		// until Zandronum processes it at run-time.
		newval->SetBuffer (new DataBuffer);

		if (op->ID() == OPER_Ternary)
		{
			// There isn't a dataheader for ternary OPER_erator. Instead, we use DH_IfNotGoto
			// to create an "if-block" inside an expression.
			// Behold, big block of writing madness! :P
			//
			DataBuffer* buf = newval->Buffer();
			DataBuffer* b0 = values[0]->Buffer();
			DataBuffer* b1 = values[1]->Buffer();
			DataBuffer* b2 = values[2]->Buffer();
			ByteMark* mark1 = buf->AddMark (""); // start of "else" case
			ByteMark* mark2 = buf->AddMark (""); // end of expression
			buf->MergeAndDestroy (b0);
			buf->WriteDWord (DH_IfNotGoto); // if the first OPER_erand (condition)
			buf->AddReference (mark1); // didn't eval true, jump into mark1
			buf->MergeAndDestroy (b1); // otherwise, perform second OPER_erand (true case)
			buf->WriteDWord (DH_Goto); // afterwards, jump to the end, which is
			buf->AddReference (mark2); // marked by mark2.
			buf->AdjustMark (mark1); // move mark1 at the end of the true case
			buf->MergeAndDestroy (b2); // perform third OPER_erand (false case)
			buf->AdjustMark (mark2); // move the ending mark2 here

			for (int i = 0; i < 3; ++i)
				values[i]->SetBuffer (null);
		}
		else
		{
			// Generic case: write all arguments and apply the OPER_erator's
			// data header.
			for (ExpressionValue* val : values)
			{
				newval->Buffer()->MergeAndDestroy (val->Buffer());

				// Null the pointer out so that the value's destructor will not
				// attempt to double-free it.
				val->SetBuffer (null);
			}

			newval->Buffer()->WriteDWord (info->header);
		}
	}
	else
	{
		// We have a constant expression. We know all the values involved and
		// can thus compute the result of this expression on compile-time.
		List<int> nums;
		int a;

		for (ExpressionValue* val : values)
			nums << val->Value();

		switch (op->ID())
		{
			case OPER_Addition:			a = nums[0] + nums[1];					break;
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
			case OPER_CompareNotEquals:	a = (nums[0] != nums[1]) ? 1 : 0;		break;
			case OPER_BitwiseAnd:			a = nums[0] & nums[1];					break;
			case OPER_BitwiseOr:			a = nums[0] | nums[1];					break;
			case OPER_BitwiseXOr:			a = nums[0] ^ nums[1];					break;
			case OPER_LogicalAnd:			a = (nums[0] && nums[1]) ? 1 : 0;		break;
			case OPER_LogicalOr:			a = (nums[0] || nums[1]) ? 1 : 0;		break;
			case OPER_Ternary:				a = (nums[0] != 0) ? nums[1] : nums[2];	break;

			case OPER_Division:
			{
				if (nums[1] == 0)
					Error ("division by zero in constant expression");

				a = nums[0] / nums[1];
				break;
			}

			case OPER_Modulus:
			{
				if (nums[1] == 0)
					Error ("modulus by zero in constant expression");

				a = nums[0] % nums[1];
				break;
			}
		}

		newval->SetValue (a);
	}

	// The new value has been generated. We don't need the old stuff anymore.
	for (ExpressionValue* val : values)
		delete val;

	delete op;
	return newval;
}

// =============================================================================
//
ExpressionValue* Expression::Evaluate()
{
	SymbolList::Iterator it;

	while ((it = FindPrioritizedOperator()) != mSymbols.end())
	{
		int i = it - mSymbols.begin();
		List<SymbolList::Iterator> OPER_erands;
		ExpressionOperator* op = static_cast<ExpressionOperator*> (*it);
		const OperatorInfo* info = &gOperators[op->ID()];
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
		ExpressionValue* newvalue = EvaluateOperator (op, values);

		for (int i = upper; i >= lower; --i)
			mSymbols.RemoveAt (i);

		mSymbols.Insert (lower, newvalue);
	}

	assert (mSymbols.Size() == 1 && mSymbols.First()->Type() == EXPRSYM_Value);
	ExpressionValue* val = static_cast<ExpressionValue*> (mSymbols.First());
	return val;
}

// =============================================================================
//
ExpressionValue* Expression::Result()
{
	return static_cast<ExpressionValue*> (mSymbols.First());
}

// =============================================================================
//
String Expression::TokenString()
{
	return mLexer->Token()->text;
}

// =============================================================================
//
ExpressionOperator::ExpressionOperator (ExpressionOperatorType id) :
	ExpressionSymbol (EXPRSYM_Operator),
	mID (id) {}

// =============================================================================
//
ExpressionValue::ExpressionValue (DataType valuetype) :
	ExpressionSymbol (EXPRSYM_Value),
	mBuffer (null),
	mValueType (valuetype) {}

// =============================================================================
//
ExpressionValue::~ExpressionValue()
{
	delete mBuffer;
}

// =============================================================================
//
void ExpressionValue::ConvertToBuffer()
{
	if (IsConstexpr() == false)
		return;

	SetBuffer (new DataBuffer);

	switch (mValueType)
	{
		case TYPE_Bool:
		case TYPE_Int:
			Buffer()->WriteDWord (DH_PushNumber);
			Buffer()->WriteDWord (abs (mValue));

			if (mValue < 0)
				Buffer()->WriteDWord (DH_UnaryMinus);
			break;

		case TYPE_String:
			Buffer()->WriteDWord (DH_PushStringIndex);
			Buffer()->WriteDWord (mValue);
			break;

		case TYPE_Void:
		case TYPE_Unknown:
			assert (false);
			break;
	}
}
