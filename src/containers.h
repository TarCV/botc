#ifndef LIBCOBALT_CONTAINERS_H
#define LIBCOBALT_CONTAINERS_H

#include <cassert>
#include <algorithm>
#include <deque>
#include <initializer_list>

template<class T> class list
{
    public:
        typedef typename ::std::deque<T> list_type;
        typedef typename list_type::iterator it;
        typedef typename list_type::const_iterator c_it;
        typedef typename list_type::reverse_iterator r_it;
        typedef typename list_type::const_reverse_iterator cr_it;
        typedef T element_type;
        typedef list<T> self_type;

        list() {}
        list (std::initializer_list<element_type> vals)
        {
            m_data = vals;
        }

        list (const list_type& a) : m_data (a) {}

        it begin()
        {
            return m_data.begin();
        }

        c_it begin() const
        {
            return m_data.cbegin();
        }

        it end()
        {
            return m_data.end();
        }

        c_it end() const
        {
            return m_data.cend();
        }

        r_it rbegin()
        {
            return m_data.rbegin();
        }

        cr_it crbegin() const
        {
            return m_data.crbegin();
        }

        r_it rend()
        {
            return m_data.rend();
        }

        cr_it crend() const
        {
            return m_data.crend();
        }

        void erase (int pos)
        {
            assert (pos < size());
            m_data.erase (m_data.begin() + pos);
        }

        element_type& push_front (const element_type& value)
        {
            m_data.push_front (value);
            return m_data[0];
        }

        element_type& push_back (const element_type& value)
        {
            m_data.push_back (value);
            return m_data[m_data.size() - 1];
        }

        void push_back (const self_type& vals)
        {
            for (const T & val : vals)
                push_back (val);
        }

        bool pop (T& val)
        {
            if (size() == 0)
                return false;

            val = m_data[size() - 1];
            erase (size() - 1);
            return true;
        }

        T& operator<< (const T& value)
        {
            return push_back (value);
        }

        void operator<< (const self_type& vals)
        {
            push_back (vals);
        }

        bool operator>> (T& value)
        {
            return pop (value);
        }

        self_type reverse() const
        {
            self_type rev;

            for (const T & val : *this)
                val >> rev;

            return rev;
        }

        void clear()
        {
            m_data.clear();
        }

        void insert (int pos, const element_type& value)
        {
            m_data.insert (m_data.begin() + pos, value);
        }

        void makeUnique()
        {
            // Remove duplicate entries. For this to be effective, the vector must be
            // sorted first.
            sort();
            it pos = std::unique (begin(), end());
            resize (std::distance (begin(), pos));
        }

        int size() const
        {
            return m_data.size();
        }

        element_type& operator[] (int n)
        {
            assert (n < size());
            return m_data[n];
        }

        const element_type& operator[] (int n) const
        {
            assert (n < size());
            return m_data[n];
        }

        void resize (std::ptrdiff_t size)
        {
            m_data.resize (size);
        }

        void sort()
        {
            std::sort (begin(), end());
        }

        int find (const element_type& needle)
        {
            int i = 0;

            for (const element_type & hay : *this)
            {
                if (hay == needle)
                    return i;

                i++;
            }

            return -1;
        }

        void remove (const element_type& it)
        {
            int idx;

            if ( (idx = find (it)) != -1)
                erase (idx);
        }

        inline bool is_empty() const
        {
            return size() == 0;
        }

        self_type mid (int a, int b) const
        {
            assert (a >= 0 && b >= 0 && a < size() && b < size() && a <= b);
            self_type result;

            for (int i = a; i <= b; ++i)
                result << operator[] (i);

            return result;
        }

        inline const list_type& std_deque() const
        {
            return m_data;
        }

    private:
        list_type m_data;
};

template<class T> list<T>& operator>> (const T& value, list<T>& haystack)
{
    haystack.push_front (value);
    return haystack;
}

#endif // LIBCOBALT_CONTAINERS_H
