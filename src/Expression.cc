#include "Expression.h"
#include "DataBuffer.h"
#include "Lexer.h"
#include "Variables.h"

struct OperatorInfo
{
	EToken		token;
	int			numoperands;
	int			priority;
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

/*
	// There isn't a dataheader for ternary operator. Instead, we use dhIfNotGoto
	// to create an "if-block" inside an expression.
	// Behold, big block of writing madness! :P
	ByteMark* mark1 = retbuf->AddMark (""); // start of "else" case
	ByteMark* mark2 = retbuf->AddMark (""); // end of expression
	retbuf->WriteDWord (dhIfNotGoto); // if the first operand (condition)
	retbuf->AddReference (mark1); // didn't eval true, jump into mark1
	retbuf->MergeAndDestroy (rb); // otherwise, perform second operand (true case)
	retbuf->WriteDWord (dhGoto); // afterwards, jump to the end, which is
	retbuf->AddReference (mark2); // marked by mark2.
	retbuf->AdjustMark (mark1); // move mark1 at the end of the true case
	retbuf->MergeAndDestroy (tb); // perform third operand (false case)
	retbuf->AdjustMark (mark2); // move the ending mark2 here
*/

// =============================================================================
//
Expression::Expression (BotscriptParser* parser, EType reqtype, Lexer* lx) :
	mParser (parser),
	mLexer (lx),
	mType (reqtype),
	mResult (null)
{
	ExpressionSymbol* sym;

	while ((sym = ParseSymbol()) != null)
		mSymbols << sym;

	if (mSymbols.IsEmpty())
		Error ("Expected expression");

	Verify();
	mResult = Evaluate();
}

// =============================================================================
//
Expression::~Expression()
{
	for (ExpressionSymbol* sym : mSymbols)
		delete sym;

	delete mResult;
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
		ScriptVariable* globalvar;
		mLexer->MustGetNext();

		if (mLexer->GetTokenType() == tkColon)
			return new ExpressionColon;

		// Check for operator
		for (const OperatorInfo* op : gOperators)
			if (mLexer->GetTokenType() == op->token)
				return new ExpressionOperator (op - gOperators);

		// Check sub-expression
		if (mLexer->GetTokenType() == tkParenStart)
		{
			mLexer->MustGetNext();
			Expression expr (mParser, mLexer, mType);
			mLexer->MustGetNext (tkParenEnd);
			return expr.GetResult();
		}

		op = new ExpressionValue;

		// Check function
		if (CommandInfo* comm = FindCommandByName (GetTokenString()))
		{
			if (mType != EUnknownType && comm->returnvalue != mType)
				Error ("%1 returns an incompatible data type", comm->name);

			op->SetBuffer (mParser->ParseCommand (comm));
			return op;
		}

		// Check constant
		if (ConstantInfo* constant = mParser->FindConstant (GetTokenString()))
		{
			if (mType != constant->type)
				Error ("constant `%1` is %2, expression requires %3\n",
					constant->name, GetTypeName (constant->type),
						GetTypeName (mType));

			switch (constant->type)
			{
				case EBoolType:
				case EIntType:
					op->SetValue (constant->val.ToLong());
					break;

				case EStringType:
					op->SetValue (GetStringTableIndex (constant->val));
					break;

				case EVoidType:
				case EUnknownType:
					break;
			}

			return op;
		}

		// Check global variable
		if ((globalvar = FindGlobalVariable (GetTokenString())))
		{
			DataBuffer* buf = new DataBuffer (8);
			buf->WriteDWord (dhPushGlobalVar);
			buf->WriteDWord (globalvar->index);
			op->SetBuffer (buf);
			return op;
		}

		EToken tt;

		// Check for literal
		switch (mType)
		{
			case EVoidType:
			case EUnknownType:
				Error ("unknown identifier `%1` (expected keyword, function or variable)", GetTokenString());
				break;

			case EBoolType:
				if ((tt = mLexer->GetTokenType()) == tkTrue || tt == tkFalse)
				{
					op->SetValue (tt == tkTrue ? 1 : 0);
					return op;
				}
			case EIntType:
				if (!mLexer->GetTokenType() != tkNumber)
					throw failed;

				op->SetValue (GetTokenString().ToLong());
				return op;

			case EStringType:
				if (!mLexer->GetTokenType() != tkString)
					throw failed;

				op->SetValue (GetStringTableIndex (GetTokenString()));
				return op;
		}

		assert (false);
		throw failed;
	}
	catch (ELocalException&)
	{
		// We use a local enum here since catch(...) would catch Error() calls.
		mLexer->SetPosition (pos);
		delete op;
		return false;
	}

	assert (false);
	return false;
}

// =============================================================================
//
ExpressionValue* Expression::Evaluate()
{
	
}

// =============================================================================
//
ExpressionValue* Expression::GetResult()
{
	return mResult;
}

// =============================================================================
//
String Expression::GetTokenString()
{
	return mLexer->GetToken()->text;
}

// =============================================================================
//
ExpressionOperator::ExpressionOperator (int id) :
	mID (id),
	mType (eOperator) {}

// =============================================================================
//
ExpressionValue::ExpressionValue(EType valuetype) :
	mBuffer (null),
	mType (eOperand),
	mValueType (valuetype) {}

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
