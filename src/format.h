/*
	Copyright (c) 2014, Santeri Piippo
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

		* Redistributions of source code must retain the above copyright
		  notice, this list of conditions and the following disclaimer.

		* Redistributions in binary form must reproduce the above copyright
		  notice, this list of conditions and the following disclaimer in the
		  documentation and/or other materials provided with the distribution.

		* Neither the name of the <organization> nor the
		  names of its contributors may be used to endorse or promote products
		  derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef BOTC_FORMAT_H
#define BOTC_FORMAT_H

#include "str.h"
#include "containers.h"

class format_arg
{
	public:
		format_arg (const string& a) : m_string (a) {}
		format_arg (char a) : m_string (a) {}
		format_arg (int a) : m_string (string::from_number (a)) {}
		format_arg (long a) : m_string (string::from_number (a)) {}
		format_arg (const char* a) : m_string (a) {}

		format_arg (void* a)
		{
			m_string.sprintf ("%p", a);
		}

		format_arg (const void* a)
		{
			m_string.sprintf ("%p", a);
		}

		template<class T> format_arg (const list<T>& list)
		{
			if (list.is_empty())
			{
				m_string = "{}";
				return;
			}

			m_string = "{ ";

			for (const T & a : list)
			{
				if (&a != &list[0])
					m_string += ", ";

				m_string += format_arg (a).as_string();
			}

			m_string += " }";
		}

		inline const string& as_string() const
		{
			return m_string;
		}

	private:
		string m_string;
};

template<class T> string custom_format (T a, const char* fmtstr)
{
	string out;
	out.sprintf (fmtstr, a);
	return out;
}

inline string hex (ulong a)
{
	return custom_format (a, "0x%X");
}

inline string charnum (char a)
{
	return custom_format (a, "%d");
}

class script_error : public std::exception
{
	public:
		script_error (const string& msg) : m_msg (msg) {}

		inline const char* what() const throw()
		{
			return m_msg.c_str();
		}

	private:
		string m_msg;
};

string format_args (const list<format_arg>& args);
void print_args (FILE* fp, const list<format_arg>& args);
void do_fatal (const list<format_arg>& args);
void do_error (string msg);

#ifndef IN_IDE_PARSER
# define format(...) format_args({ __VA_ARGS__ })
# define fprint(A, ...) print_args( A, { __VA_ARGS__ })
# define print(...) print_args( stdout, { __VA_ARGS__ })
# define error(...) do_error (format (__VA_ARGS__))
#else
string format (void, ...);
void fprint (FILE* fp, ...);
void print (void, ...);
void error (void, ...);
#endif

#ifndef IN_IDE_PARSER
# ifdef DEBUG
#  define devf(...) fprint (stderr, __VA_ARGS__)
#  define dvalof( A ) fprint (stderr, "value of '%1' = %2\n", #A, A)
# else
#  define devf(...)
#  define dvalof( A )
# endif // DEBUG
#else
// print something in debug builds
void devf (void, ...);

// print the value of @a
void dvalof (void a);
#endif // IN_IDE_PARSER

#endif // BOTC_FORMAT_H
