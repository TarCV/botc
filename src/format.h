#ifndef BOTC_FORMAT_H
#define BOTC_FORMAT_H

#include "str.h"
#include "containers.h"

class format_arg
{
	public:
		format_arg (const string& a)
		{
			m_string = a;
		}

		format_arg (int a)
		{
			m_string.sprintf ("%d", a);
		}

		format_arg (long a)
		{
			m_string.sprintf ("%ld", a);
		}

		format_arg (uint a)
		{
			m_string.sprintf ("%u", a);
		}

		format_arg (ulong a)
		{
			m_string.sprintf ("%lu", a);
		}

		format_arg (const char* a)
		{
			m_string = a;
		}

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

/*
		template<class T, class R> format_arg (const std::map<T, R>& a)
		{
			if (a.size() == 0)
			{
				m_string = "{}";
				return;
			}

			m_string = "{ ";

			for (std::pair<T, R> it : a)
			{
				if (m_string != "{ ")
					m_string += ", ";

				m_string += format_arg (it.first).as_string() + "=" +
							format_arg (it.second).as_string();
			}

			m_string += " }";
		}
*/

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

string format_args (const list<format_arg>& args);
void print_args (FILE* fp, const list<format_arg>& args);
void do_fatal (const list<format_arg>& args);

#ifndef IN_IDE_PARSER
#define format(...) format_args({ __VA_ARGS__ })
#define fprint(A, ...) print_args( A, { __VA_ARGS__ })
#define print(...) print_args( stdout, { __VA_ARGS__ })
#define fatal(...) do_fatal({ __VA_ARGS__ })
#else
string format (void, ...);
void fprint (FILE* fp, ...);
void print (void, ...);
void fatal (void, ...);
#endif

#ifndef IN_IDE_PARSER
# ifdef DEBUG
#  define devf(...) fprint( stderr, __VA_ARGS__ )
#  define dvalof( A ) fprint( stderr, "value of '%1' = %2\n", #A, A )
# else
#  define devf(...)
#  define dvalof( A )
# endif // DEBUG
#else
void devf (void, ...);
void dvalof (void a);
#endif // IN_IDE_PARSER

#endif // BOTC_FORMAT_H
