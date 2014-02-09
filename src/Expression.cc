#include "Expression.h"
#include "DataBuffer.h"
#include "Lexer.h"
#include "Variables.h"

struct OperatorInfo
{
	EToken		token;
	int			priority;
	int			numoperands;
	EDataHeader	header;
};

static const OperatorInfo gOperators[] =
{
	{ tkExclamationMark,	0,		1,	dhNegateLogical,	},
	{ tkMinus,				0,		1,	dhUnaryMinus,		},
	{ tkMultiply,			10,		2,	dhMultiply,			},
	{ tkDivide,				10,		2,	dhDivide,			},
	{ tkModulus,			10,		2,	dhModulus,			},
	{ tkPlus,				20,		2,	dhAdd,				},
	{ tkMinus,				20,		2,	dhSubtract,			},
	{ tkLeftShift,			30,		2,	dhLeftShift,		},
	{ tkRightShift,			30,		2,	dhRightShift,		},
	{ tkLesser,				40,		2,	dhLessThan,			},
	{ tkGreater,			40,		2,	dhGreaterThan,		},
	{ tkAtLeast,			40,		2,	dhAtLeast,			},
	{ tkAtMost,				40,		2,	dhAtMost,			},
	{ tkEquals,				50,		2,	dhEquals			},
	{ tkNotEquals,			50,		2,	dhNotEquals			},
	{ tkAmperstand,			60,		2,	dhAndBitwise		},
	{ tkCaret,				70,		2,	dhEorBitwise		},
	{ tkBar,				80,		2,	dhOrBitwise			},
	{ tkDoubleAmperstand,	90,		2,	dhAndLogical		},
	{ tkDoubleBar,			100,	2,	dhOrLogical			},
	{ tkQuestionMark,		110,	3,	(EDataHeader) 0		},
};

// =============================================================================
//
Expression::Expression (BotscriptParser* parser, Lexer* lx, EType reqtype) :
	mParser (parser),
	mLexer (lx),
	mType (reqtype)
{
	ExpressionSymbol* sym;

	while ((sym = ParseSymbol()) != null)
		mSymbols << sym;

	if (mSymbols.IsEmpty())
		Error ("Expected expression");

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
// Try to parse an expression symbol (i.e. an operator or operand or a colon)
// from the lexer.
//
ExpressionSymbol* Expression::ParseSymbol()
{
	int pos = mLexer->GetPosition();
	ExpressionValue* op = null;
	enum ELocalException { failed };

	try
	{
		mLexer->MustGetNext (tkAny);

		if (mLexer->GetTokenType() == tkColon)
			return new ExpressionColon;

		// Check for operator
		for (const OperatorInfo& op : gOperators)
			if (mLexer->GetTokenType() == op.token)
				return new ExpressionOperator ((EOperator) (&op - &gOperators[0]));

		// Check sub-expression
		if (mLexer->GetTokenType() == tkParenStart)
		{
			Expression expr (mParser, mLexer, mType);
			mLexer->MustGetNext (tkParenEnd);
			return expr.GetResult()->Clone();
		}

		op = new ExpressionValue (mType);

		// Check function
		if (CommandInfo* comm = FindCommandByName (GetTokenString()))
		{
			if (mType != EUnknownType && comm->returnvalue != mType)
				Error ("%1 returns an incompatible data type", comm->name);

			op->SetBuffer (mParser->ParseCommand (comm));
			return op;
		}

		// Check global variable
		// TODO: handle locals too when they're implemented
		if (mLexer->GetTokenType() == tkDollarSign)
		{
			mLexer->MustGetNext (tkSymbol);
			ScriptVariable* globalvar = FindGlobalVariable (GetTokenString());

			if (globalvar == null)
				Error ("unknown variable %1", GetTokenString());

			if (globalvar->writelevel == ScriptVariable::WRITE_Constexpr)
				op->SetValue (globalvar->value);
			else
			{
				DataBuffer* buf = new DataBuffer (8);
				buf->WriteDWord (dhPushGlobalVar);
				buf->WriteDWord (globalvar->index);
				op->SetBuffer (buf);
			}

			return op;
		}

		EToken tt;

		// Check for literal
		switch (mType)
		{
			case EVoidType:
			case EUnknownType:
			{
				Error ("unknown identifier `%1` (expected keyword, function or variable)", GetTokenString());
				break;
			}

			case EBoolType:
			{
				if ((tt = mLexer->GetTokenType()) == tkTrue || tt == tkFalse)
				{
					op->SetValue (tt == tkTrue ? 1 : 0);
					return op;
				}
			}
			case EIntType:
			{
				if (mLexer->GetTokenType() != tkNumber)
					throw failed;

				op->SetValue (GetTokenString().ToLong());
				return op;
			}

			case EStringType:
			{
				if (mLexer->GetTokenType() != tkString)
					throw failed;

				op->SetValue (GetStringTableIndex (GetTokenString()));
				return op;
			}
		}

		assert (false);
		throw failed;
	}
	catch (ELocalException&)
	{
		// We use a local enum here since catch(...) would catch Error() calls.
		mLexer->SetPosition (pos);
		delete op;
		return null;
	}

	assert (false);
	return null;
}

// =============================================================================
//
// The symbol parsing process only does token-based checking for operators. Thus
// ALL minus operators are actually unary minuses simply because both have
// tkMinus as their token and the unary minus is prior to the binary minus in
// the operator table. Now that we have all symbols present, we can correct
// cases like this.
//
void Expression::AdjustOperators()
{
	for (auto it = mSymbols.begin() + 1; it != mSymbols.end(); ++it)
	{
		if ((*it)->GetType() != eOperatorSymbol)
			continue;

		ExpressionOperator* op = static_cast<ExpressionOperator*> (*it);

		// Unary minus with a value as the previous symbol cannot really be
		// unary; replace with binary minus.
		if (op->GetID() == opUnaryMinus && (*(it - 1))->GetType() == eValueSymbol)
			op->SetID (opSubtraction);
	}
}

// =============================================================================
//
// Verifies a single value. Helper function for Expression::Verify.
//
void Expression::TryVerifyValue (bool* verified, SymbolList::Iterator it)
{
	// If it's an unary operator we skip to its value. The actual operator will
	// be verified separately.
	if ((*it)->GetType() == eOperatorSymbol &&
		gOperators[static_cast<ExpressionOperator*> (*it)->GetID()].numoperands == 1)
	{
		++it;
	}

	int i = it - mSymbols.begin();

	// Ensure it's an actual value
	if ((*it)->GetType() != eValueSymbol)
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
		if (mSymbols[0]->GetType() != eValueSymbol)
			Error ("bad expression");

		return;
	}

	if (mType == EStringType)
		Error ("Cannot perform operations on strings");

	bool* verified = new bool[mSymbols.Size()];
	memset (verified, 0, mSymbols.Size() * sizeof (decltype (*verified)));
	const auto last = mSymbols.end() - 1;
	const auto first = mSymbols.begin();

	for (auto it = mSymbols.begin(); it != mSymbols.end(); ++it)
	{
		int i = (it - first);

		if ((*it)->GetType() != eOperatorSymbol)
			continue;

		ExpressionOperator* op = static_cast<ExpressionOperator*> (*it);
		int numoperands = gOperators[op->GetID()].numoperands;

		switch (numoperands)
		{
			case 1:
			{
				// Ensure that:
				// -	unary operator is not the last symbol
				// -	unary operator is succeeded by a value symbol
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
				// -	binary operator is not the first or last symbol
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
				TryVerifyValue (verified, it - 1);
				TryVerifyValue (verified, it + 1);
				TryVerifyValue (verified, it + 3);

				if (it == first ||
					it >= mSymbols.end() - 3 ||
					verified[i] == true ||
					verified[i + 2] == true ||
					(*(it + 2))->GetType() != eColonSymbol)
				{
					Error ("malformed expression");
				}

				verified[i] = true;
				verified[i + 2] = true;
				break;
			}

			default:
				Error ("WTF operator with %1 operands", numoperands);
		}
	}

	for (int i = 0; i < mSymbols.Size(); ++i)
		if (verified[i] == false)
			Error ("malformed expression: expr symbol #%1 is was left unverified", i);

	delete verified;
}


// =============================================================================
//
// Which operator to evaluate?
//
Expression::SymbolList::Iterator Expression::FindPrioritizedOperator()
{
	SymbolList::Iterator	best = mSymbols.end();
	int						bestpriority = INT_MAX;

	for (SymbolList::Iterator it = mSymbols.begin(); it != mSymbols.end(); ++it)
	{
		if ((*it)->GetType() != eOperatorSymbol)
			continue;

		ExpressionOperator* op = static_cast<ExpressionOperator*> (*it);
		const OperatorInfo* info = &gOperators[op->GetID()];

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
// Process the given operator and values into a new value.
//
ExpressionValue* Expression::EvaluateOperator (const ExpressionOperator* op,
											   const List<ExpressionValue*>& values)
{
	const OperatorInfo* info = &gOperators[op->GetID()];
	bool isconstexpr = true;

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

		if (op->GetID() == opTernary)
		{
			// There isn't a dataheader for ternary operator. Instead, we use dhIfNotGoto
			// to create an "if-block" inside an expression.
			// Behold, big block of writing madness! :P
			//
			DataBuffer* buf = newval->GetBuffer();
			DataBuffer* b0 = values[0]->GetBuffer();
			DataBuffer* b1 = values[1]->GetBuffer();
			DataBuffer* b2 = values[2]->GetBuffer();
			ByteMark* mark1 = buf->AddMark (""); // start of "else" case
			ByteMark* mark2 = buf->AddMark (""); // end of expression
			buf->MergeAndDestroy (b0);
			buf->WriteDWord (dhIfNotGoto); // if the first operand (condition)
			buf->AddReference (mark1); // didn't eval true, jump into mark1
			buf->MergeAndDestroy (b1); // otherwise, perform second operand (true case)
			buf->WriteDWord (dhGoto); // afterwards, jump to the end, which is
			buf->AddReference (mark2); // marked by mark2.
			buf->AdjustMark (mark1); // move mark1 at the end of the true case
			buf->MergeAndDestroy (b2); // perform third operand (false case)
			buf->AdjustMark (mark2); // move the ending mark2 here

			for (int i = 0; i < 3; ++i)
				values[i]->SetBuffer (null);
		}
		else
		{
			// Generic case: write all arguments and apply the operator's
			// data header.
			for (ExpressionValue* val : values)
			{
				newval->GetBuffer()->MergeAndDestroy (val->GetBuffer());

				// Null the pointer out so that the value's destructor will not
				// attempt to double-free it.
				val->SetBuffer (null);
			}

			newval->GetBuffer()->WriteDWord (info->header);
		}
	}
	else
	{
		// We have a constant expression. We know all the values involved and
		// can thus compute the result of this expression on compile-time.
		List<int> nums;
		int a;

		for (ExpressionValue* val : values)
			nums << val->GetValue();

		switch (op->GetID())
		{
			case opAddition:			a = nums[0] + nums[1];					break;
			case opSubtraction:			a = nums[0] - nums[1];					break;
			case opMultiplication:		a = nums[0] * nums[1];					break;
			case opUnaryMinus:			a = -nums[0];							break;
			case opNegateLogical:		a = !nums[0];							break;
			case opLeftShift:			a = nums[0] << nums[1];					break;
			case opRightShift:			a = nums[0] >> nums[1];					break;
			case opCompareLesser:		a = (nums[0] < nums[1]) ? 1 : 0;		break;
			case opCompareGreater:		a = (nums[0] > nums[1]) ? 1 : 0;		break;
			case opCompareAtLeast:		a = (nums[0] <= nums[1]) ? 1 : 0;		break;
			case opCompareAtMost:		a = (nums[0] >= nums[1]) ? 1 : 0;		break;
			case opCompareEquals:		a = (nums[0] == nums[1]) ? 1 : 0;		break;
			case opCompareNotEquals:	a = (nums[0] != nums[1]) ? 1 : 0;		break;
			case opBitwiseAnd:			a = nums[0] & nums[1];					break;
			case opBitwiseOr:			a = nums[0] | nums[1];					break;
			case opBitwiseXOr:			a = nums[0] ^ nums[1];					break;
			case opLogicalAnd:			a = (nums[0] && nums[1]) ? 1 : 0;		break;
			case opLogicalOr:			a = (nums[0] || nums[1]) ? 1 : 0;		break;
			case opTernary:				a = (nums[0] != 0) ? nums[1] : nums[2];	break;

			case opDivision:
				if (nums[1] == 0)
					Error ("division by zero in constant expression");

				a = nums[0] / nums[1];
				break;

			case opModulus:
				if (nums[1] == 0)
					Error ("modulus by zero in constant expression");

				a = nums[0] % nums[1];
				break;
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
		List<SymbolList::Iterator> operands;
		ExpressionOperator* op = static_cast<ExpressionOperator*> (*it);
		const OperatorInfo* info = &gOperators[op->GetID()];
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
				assert (false);
		}

		List<ExpressionValue*> values;

		for (auto it : operands)
			values << static_cast<ExpressionValue*> (*it);

		// Note: @op and all of @values are invalid after this call.
		ExpressionValue* newvalue = EvaluateOperator (op, values);

		for (int i = upper; i >= lower; --i)
			mSymbols.RemoveAt (i);

		mSymbols.Insert (lower, newvalue);
	}

	assert (mSymbols.Size() == 1 && mSymbols.First()->GetType() == eValueSymbol);
	ExpressionValue* val = static_cast<ExpressionValue*> (mSymbols.First());
	return val;
}

// =============================================================================
//
ExpressionValue* Expression::GetResult()
{
	return static_cast<ExpressionValue*> (mSymbols.First());
}

// =============================================================================
//
String Expression::GetTokenString()
{
	return mLexer->GetToken()->text;
}

// =============================================================================
//
ExpressionOperator::ExpressionOperator (EOperator id) :
	ExpressionSymbol (Expression::eOperatorSymbol),
	mID (id) {}

// =============================================================================
//
ExpressionValue::ExpressionValue (EType valuetype) :
	ExpressionSymbol (Expression::eValueSymbol),
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
		case EBoolType:
		case EIntType:
			GetBuffer()->WriteDWord (dhPushNumber);
			GetBuffer()->WriteDWord (abs (mValue));

			if (mValue < 0)
				GetBuffer()->WriteDWord (dhUnaryMinus);
			break;

		case EStringType:
			GetBuffer()->WriteDWord (dhPushStringIndex);
			GetBuffer()->WriteDWord (mValue);
			break;

		case EVoidType:
		case EUnknownType:
			assert (false);
			break;
	}
}