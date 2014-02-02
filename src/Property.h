/*
	Copyright 2012-2014 Santeri Piippo
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	1. Redistributions of source code must retain the above copyright
	   notice, this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	3. The name of the author may not be used to endorse or promote products
	   derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
	IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
	IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
	NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
	THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef BOTC_PROPERTY_H
#define BOTC_PROPERTY_H

// =============================================================================
//
// Identifier names
//
#define PROPERTY_SET_ACCESSOR(NAME)			Set##NAME
#define PROPERTY_GET_ACCESSOR(NAME)			Get##NAME
#define PROPERTY_IS_ACCESSOR(NAME)			Is##NAME // for bool types
#define PROPERTY_MEMBER_NAME(NAME)			m##NAME

// Names of operations
#define PROPERTY_APPEND_OPERATION(NAME)		AppendTo##NAME
#define PROPERTY_PREPEND_OPERATION(NAME)	PrependTo##NAME
#define PROPERTY_REPLACE_OPERATION(NAME)	ReplaceIn##NAME
#define PROPERTY_INCREASE_OPERATION(NAME)	Increase##NAME
#define PROPERTY_DECREASE_OPERATION(NAME)	Decrease##NAME
#define PROPERTY_TOGGLE_OPERATION(NAME)		Toggle##NAME
#define PROPERTY_PUSH_OPERATION(NAME)		PushTo##NAME
#define PROPERTY_REMOVE_OPERATION(NAME)		RemoveFrom##NAME
#define PROPERTY_CLEAR_OPERATION(NAME)		Clear##NAME
#define PROPERTY_COUNT_OPERATION(NAME)		Count##NAME

// Operation definitions
// These are the methods of the list type that are called in the operations.
#define PROPERTY_APPEND_METHOD_NAME			Append		// String::Append
#define PROPERTY_PREPEND_METHOD_NAME		Prepend		// String::Prepend
#define PROPERTY_REPLACE_METHOD_NAME		Replace		// String::Replace
#define PROPERTY_PUSH_METHOD_NAME			Append		// List<T>::Append
#define PROPERTY_REMOVE_METHOD_NAME			Remove		// List<T>::Remove
#define PROPERTY_CLEAR_METHOD_NAME			Clear		// List<T>::Clear

// =============================================================================
//
// Main PROPERTY macro
//
#define PROPERTY(ACCESS, TYPE, NAME, OPS, WRITETYPE)							\
	private:																	\
		TYPE PROPERTY_MEMBER_NAME(NAME);										\
																				\
	public:																		\
		inline TYPE const& PROPERTY_GET_READ_METHOD (NAME, OPS) const			\
		{																		\
			return PROPERTY_MEMBER_NAME(NAME); 									\
		}																		\
																				\
	ACCESS:																		\
		PROPERTY_MAKE_WRITE (TYPE, NAME, WRITETYPE)								\
		PROPERTY_DEFINE_OPERATIONS (TYPE, NAME, OPS)

// =============================================================================
//
// PROPERTY_GET_READ_METHOD
//
// This macro uses the OPS argument to construct the name of the actual
// macro which returns the name of the get accessor. This is so that the
// bool properties get is<NAME>() accessors while non-bools get get<NAME>()
//
#define PROPERTY_GET_READ_METHOD(NAME, OPS)										\
	PROPERTY_GET_READ_METHOD_##OPS (NAME)

#define PROPERTY_GET_READ_METHOD_BOOL_OPS(NAME) PROPERTY_IS_ACCESSOR (NAME)()
#define PROPERTY_GET_READ_METHOD_NO_OPS(NAME) PROPERTY_GET_ACCESSOR (NAME)()
#define PROPERTY_GET_READ_METHOD_STR_OPS(NAME) PROPERTY_GET_ACCESSOR (NAME)()
#define PROPERTY_GET_READ_METHOD_NUM_OPS(NAME) PROPERTY_GET_ACCESSOR (NAME)()
#define PROPERTY_GET_READ_METHOD_LIST_OPS(NAME) PROPERTY_GET_ACCESSOR (NAME)()

// =============================================================================
//
// PROPERTY_MAKE_WRITE
//
// This macro uses the WRITETYPE argument to construct the set accessor of the
// property. If WRITETYPE is STOCK_WRITE, an inline method is defined to just
// set the new value of the property. If WRITETYPE is CUSTOM_WRITE, the accessor
// is merely declared and is left for the user to define.
//
#define PROPERTY_MAKE_WRITE(TYPE, NAME, WRITETYPE)								\
	PROPERTY_MAKE_WRITE_##WRITETYPE (TYPE, NAME)

#define PROPERTY_MAKE_WRITE_STOCK_WRITE(TYPE, NAME)								\
		inline void PROPERTY_SET_ACCESSOR(NAME) (TYPE const& a)					\
		{																		\
			PROPERTY_MEMBER_NAME(NAME) = a;										\
		}

#define PROPERTY_MAKE_WRITE_CUSTOM_WRITE(TYPE, NAME)							\
		void PROPERTY_SET_ACCESSOR(NAME) (TYPE const& NAME);					\

// =============================================================================
//
// PROPERTY_DEFINE_OPERATIONS
//
// This macro may expand into methods defining additional operations for the
// method. 

#define PROPERTY_DEFINE_OPERATIONS(TYPE, NAME, OPS)								\
	DEFINE_PROPERTY_##OPS (TYPE, NAME)

// =============================================================================
//
// DEFINE_PROPERTY_NO_OPS
//
// Obviously NO_OPS expands into no operations.
//
#define DEFINE_PROPERTY_NO_OPS(TYPE, NAME)

// =============================================================================
//
// DEFINE_PROPERTY_STR_OPS
//
#define DEFINE_PROPERTY_STR_OPS(TYPE, NAME)										\
		void PROPERTY_APPEND_OPERATION(NAME) (const TYPE& a)					\
		{																		\
			TYPE tmp (PROPERTY_MEMBER_NAME(NAME));								\
			tmp.PROPERTY_APPEND_METHOD_NAME (a);								\
			PROPERTY_SET_ACCESSOR(NAME) (tmp);									\
		}																		\
																				\
		void PROPERTY_PREPEND_OPERATION(NAME) (const TYPE& a)					\
		{																		\
			TYPE tmp (PROPERTY_MEMBER_NAME(NAME));								\
			tmp.PROPERTY_PREPEND_METHOD_NAME (a);								\
			PROPERTY_SET_ACCESSOR(NAME) (tmp);									\
		}																		\
																				\
		void PROPERTY_REPLACE_OPERATION(NAME) (const TYPE& a, const TYPE& b)	\
		{																		\
			TYPE tmp (PROPERTY_MEMBER_NAME(NAME));								\
			tmp.PROPERTY_REPLACE_METHOD_NAME (a, b);							\
			PROPERTY_SET_ACCESSOR(NAME) (tmp);									\
		}

// =============================================================================
//
// DEFINE_PROPERTY_NUM_OPS
//
#define DEFINE_PROPERTY_NUM_OPS(TYPE, NAME)										\
		inline void PROPERTY_INCREASE_OPERATION(NAME) (TYPE a = 1)				\
		{																		\
			PROPERTY_SET_ACCESSOR(NAME) (PROPERTY_MEMBER_NAME(NAME) + a);		\
		}																		\
																				\
		inline void PROPERTY_DECREASE_OPERATION(NAME) (TYPE a = 1)				\
		{																		\
			PROPERTY_SET_ACCESSOR(NAME) (PROPERTY_MEMBER_NAME(NAME) - a);		\
		}

// =============================================================================
//
// DEFINE_PROPERTY_BOOL_OPS
//
#define DEFINE_PROPERTY_BOOL_OPS(TYPE, NAME)									\
		inline void PROPERTY_TOGGLE_OPERATION(NAME)()							\
		{																		\
			PROPERTY_SET_ACCESSOR(NAME) (!PROPERTY_MEMBER_NAME(NAME));			\
		}

// =============================================================================
//
// DEFINE_PROPERTY_LIST_OPS
//
#define DEFINE_PROPERTY_LIST_OPS(TYPE, NAME)									\
		void PROPERTY_PUSH_OPERATION(NAME) (const TYPE::ValueType& a)			\
		{																		\
			PROPERTY_MEMBER_NAME(NAME).PROPERTY_PUSH_METHOD_NAME (a);			\
		}																		\
																				\
		void PROPERTY_REMOVE_OPERATION(NAME) (const TYPE::ValueType& a)			\
		{																		\
			PROPERTY_MEMBER_NAME(NAME).PROPERTY_REMOVE_METHOD_NAME (a);			\
		}																		\
																				\
		inline void PROPERTY_CLEAR_OPERATION(NAME)()							\
		{																		\
			PROPERTY_MEMBER_NAME(NAME).PROPERTY_CLEAR_METHOD_NAME();			\
		}																		\
																				\
	public:																		\
		inline int PROPERTY_COUNT_OPERATION(NAME)() const						\
		{																		\
			return PROPERTY_GET_ACCESSOR (NAME)().Size();						\
		}

#endif // BOTC_PROPERTY_H