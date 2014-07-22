/*
	Copyright 2014 Teemu Piippo
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

#pragma once
#include "main.h"
#include <typeinfo>
#include <type_traits>
#include <cstring>

// _________________________________________________________________________________________________
//
class CommandLineOption
{
private:
	char _shortform;
	String _longform;
	String _description;
	std::type_info const* _type;
	void* _ptr;

public:
	template<typename T>
	CommandLineOption (T& data, char shortform, const char* longform, const char* description) :
		_shortform (shortform),
		_longform (longform),
		_description (description),
		_type (&typeid (T)),
		_ptr (&data)
	{
		static_assert (std::is_same<T, int>::value
			or std::is_same<T, bool>::value
			or std::is_same<T, String>::value
			or std::is_same<T, double>::value
			or std::is_enum<T>::value,
			"value to CommandLineOption must be either int, bool, String or double");

		if (shortform == '\0' and not std::strlen (longform))
			error ("commandline option left without short-form or long-form name");
	}

	virtual ~CommandLineOption() {}

	inline String describe() const
	{
		return std::strlen (_longform)
			? (String ("--") + _longform)
			: String ({'-', _shortform, '\0'});
	}

	inline String const& longform() const
	{
		return _longform;
	}

	inline String const& description() const
	{
		return _description;
	}

	inline char shortform() const
	{
		return _shortform;
	}

	inline void* pointer() const
	{
		return _ptr;
	}

	template<typename T>
	inline bool isOfType() const
	{
		return &typeid(T) == _type;
	}

	virtual void handleValue (String const& a);
};

template<typename T>
class EnumeratedCommandLineOption : public CommandLineOption
{
	StringList _values;

public:
	EnumeratedCommandLineOption (T& data, char shortform, const char* longform,
		const char* description) :
		CommandLineOption (data, shortform, longform, description)
	{
		for (int i = 0; i < int (T::NumValues); ++i)
		{
			String value = MakeFormatArgument (T (i)).toLowercase();
			int pos;

			if ((pos = value.firstIndexOf ("::")) != -1)
				value = value.mid (pos + 2);

			_values << value;
		}
	}

	virtual void handleValue (String const& value) override
	{
		String const lowvalue (value.toLowercase());

		for (int i = 0; i < _values.size(); ++i)
		{
			if (_values[i].toLowercase() == lowvalue)
			{
				*reinterpret_cast<T*> (pointer()) = T (i);
				return;
			}
		}

		error ("bad value passed to %1 (%2), valid values are: %3", describe(), value, _values);
	}
};

// _________________________________________________________________________________________________
//
class CommandLine
{
	List<CommandLineOption*> _options;
	DELETE_COPY (CommandLine)
	
public:
	CommandLine() = default;
	~CommandLine();
	template<typename Enum>
	void addEnumeratedOption (Enum& e, char shortform, const char* longform,
		const char* description);
	void addOption (CommandLineOption* option);
	StringList process (int argc, char* argv[]);

	template<typename... Args>
	void addOption (Args... args)
	{
		addOption (new CommandLineOption (args...));
	}
};

// _________________________________________________________________________________________________
//
template<typename Enum>
void CommandLine::addEnumeratedOption (Enum& e, char shortform, const char* longform,
	const char* description)
{
	static_assert(std::is_enum<Enum>::value, "addEnumeratedOption requires a named enumerator");
	auto enumoption = new EnumeratedCommandLineOption<Enum> (e, shortform, longform,
		description);
	addOption (static_cast<CommandLineOption*> (enumoption));
}
