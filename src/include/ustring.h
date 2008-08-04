/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2008 Larry Gritz.
// 
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// 
// (This is the MIT open source license.)
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
/// \class ustring
///
/// A ustring is an alternative to char* or std::string for storing
/// strings, in which the character sequence is unique (allowing many
/// speed advantages for assignment, equality testing, and inequality
/// testing).
///
/// The implementation is that behind the scenes there is a hash set of
/// allocated strings, so the characters of each string are unique.  A
/// ustring itself is a pointer to the characters of one of these canonical
/// strings.  Therefore, assignment and equality testing is just a single
/// 32- or 64-bit int operation, the only mutex is when a ustring is
/// created from raw characters, and the only malloc is the first time
/// each canonical ustring is created.
///
/// The internal table also contains a std::string version and the length
/// of the string, so converting a ustring to a std::string (via
/// ustring::string()) or querying the number of characters (via
/// ustring::size() or ustring::length()) is extremely inexpensive, and does
/// not involve creation/allocation of a new std::string or a call to
/// strlen.
///
/// We try very hard to completely mimic the API of std::string,
/// including all the constructors, comparisons, iterations, etc.  Of
/// course, the charaters of a ustring are non-modifiable, so we do not
/// replicate any of the non-const methods of std::string.  But in most
/// other ways it looks and acts like a std::string and so most templated
/// algorthms that would work on a "const std::string &" will also work
/// on a ustring.
/// 
/// Usage guidelines:
///
/// Compared to standard strings, ustrings have several advantages:
/// 
///   - Each individual ustring is very small -- in fact, we guarantee that
///     a ustring is the same size and memory layout as an ordinary char*.
///   - Storage is frugal, since there is only one allocated copy of each
///     unique character sequence, throughout the lifetime of the program.
///   - Assignment from one ustring to another is just copy of the pointer;
///     no allocation, no character copying, no reference counting.
///   - Equality testing (do the strings contain the same characters) is
///     a single operation, the comparison of the pointer.
///   - Memory allocation only occurs when a new ustring is construted from
///     raw characters the FIRST time -- subsequent constructions of the
///     same string just finds it in the canonial string set, but doesn't
///     need to allocate new storage.  Destruction of a ustring is trivial,
///     there is no de-allocation because the canonical version stays in
///     the set.  Also, therefore, no user code mistake can lead to
///     memory leaks.
///
/// But there are some problems, too.  Canonical strings are never freed
/// from the table.  So in some sense all the strings "leak", but they
/// only leak one copy for each unique string that the program ever comes
/// across.  Also, creation of unique strings from raw characters is more
/// expensive than for standard strings, due to hashing, table queries,
/// and other overhead.
///
/// On the whole, ustrings are a really great string representation
///   - if you tend to have (relatively) few unique strings, but many
///     copies of those strings;
///   - if the creation of strings from raw characters is relatively
///     rare compared to copying existing strings;
///   - if you tend to make the same strings over and over again, and
///     if it's relatively rare that a single unique character sequence
///     is used only once in the entire lifetime of the program;
///   - if your most common string operations are assignment and equality
///     testing and you want them to be as fast as possible;
///   - if you are doing relatively little character-by-character assembly
///     of strings, string concatenation, or other "string manipulation"
///     (other than equality testing).
///
/// ustrings are not so hot
///   - if your program tends to have very few copies of each character
///     sequence over the entire lifetime of the program;
///   - if your program tends to generate a huge variety of unique
///     strings over its lifetime, each of which is used only a short
///     time and then discarded, never to be needed again;
///   - if you don't need to do a lot of string assignment or equality
///     testing, but lots of more complex string manipulation.
///
/////////////////////////////////////////////////////////////////////////////


#ifndef USTRING_H
#define USTRING_H

#include <string>
#include <iostream>
#include "export.h"

#ifndef NULL
#define NULL 0
#endif


// FIXME: want a namespace 
// namespace blah {


class DLLPUBLIC ustring {
public:
    typedef char value_type;
    typedef value_type * pointer;
    typedef value_type & reference;
    typedef const value_type & const_reference;
    typedef size_t size_type;
    static const size_type npos = static_cast<size_type>(-1);
    typedef std::string::const_iterator const_iterator;
    typedef std::string::const_reverse_iterator const_reverse_iterator;

    /// Default ctr for ustring -- make an empty string.
    ///
    ustring (void) : m_chars(NULL) { }

    /// Construct a ustring from a null-terminated C string (char *).
    ///
    explicit ustring (const char *str) {
        m_chars = str ? _make_unique(str)->c_str() : NULL;
    }

    /// Construct a ustring from at most n characters of str, starting at
    /// position pos.
    ustring (const char *str, size_type pos, size_type n)
        : m_chars (_make_unique(std::string(str,pos,n).c_str())->c_str()) { }

    /// Construct a ustring from at most n characters beginning at str.
    ///
    ustring (const char *str, size_type n)
        : m_chars (_make_unique(std::string(str,n).c_str())->c_str()) { }

    /// Construct a ustring from n copies of character c.
    ///
    ustring (size_type n, char c)
        : m_chars (_make_unique(std::string(n,c).c_str())->c_str()) { }

    /// Construct a ustring from a C++ std::string.
    ///
    explicit ustring (const std::string &str) { *this = ustring(str.c_str()); }

    /// Construct a ustring from an indexed substring of a std::string.
    ///
    ustring (const std::string &str, size_type pos, size_type n=npos)
        : m_chars (_make_unique(std::string(str, pos, n).c_str())->c_str()) { }

    /// Copy construct a ustring from another ustring.
    ///
    ustring (const ustring &str) : m_chars(str.m_chars) { }

    /// Construct a ustring from an indexed substring of a ustring.
    ///
    ustring (const ustring &str, size_type pos, size_type n=npos)
        : m_chars (_make_unique(std::string(str.c_str(),pos,n).c_str())->c_str()) { }

    /// ustring destructor.
    ///
    ~ustring () { }

    /// Assign a ustring to *this.
    ///
    const ustring & assign (const ustring &str) {
        m_chars = str.m_chars;
        return *this;
    }

    /// Assign a substring of a ustring to *this.
    ///
    const ustring & assign (const ustring &str, size_type pos, size_type n=npos)
        { *this = ustring(str,pos,n); return *this; }

    /// Assign a std::string to *this.
    ///
    const ustring & assign (const std::string &str) {
        assign (str.c_str());
        return *this;
    } 

    /// Assign a substring of a std::string to *this.
    ///
    const ustring & assign (const std::string &str, size_type pos, size_type n=npos)
        { *this = ustring(str,pos,n); return *this; }

    /// Assign a null-terminated C string (char*) to *this.
    ///
    const ustring & assign (const char *str) {
        m_chars = str ? _make_unique(str)->c_str() : NULL;
        return *this;
    }

    /// Assign the first n characters of str to *this.
    ///
    const ustring & assign (const char *str, size_type n)
        { *this = ustring(str,n); return *this; }

    /// Assign n copies of c to *this.
    ///
    const ustring & assign (size_type n, char c)
        { *this = ustring(n,c); return *this; }

    /// Assign a ustring to another ustring.
    ///
    const ustring & operator= (const ustring &str) { return assign(str); }

    /// Assign a null-terminated C string (char *) to a ustring.
    ///
    const ustring & operator= (const char *str) { assign(str); }

    /// Assign a C++ std::string to a ustring.
    ///
    const ustring & operator= (const std::string &str) { assign(str); }

    /// Assign a single char to a ustring.
    ///
    const ustring & operator= (char c) {
        char s[2];
        s[0] = c; s[1] = 0;
        *this = ustring (s);
        return *this;
    }

    /// Return a C string representation of a ustring.
    ///
    const char *c_str () const {
        return m_chars;
    }

    /// Return a C string representation of a ustring.
    ///
    const char *data () const { return c_str(); }

    /// Return a C++ std::string representation of a ustring.
    ///
    const std::string & string () const {
        const TableRep *rep = (const TableRep *)(m_chars - chars_offset);
        return rep->str;
    }

    /// Reset to an empty string.
    ///
    void clear (void) {
        m_chars = NULL;
    }

    /// Return the number of characters in the string.
    ///
    size_t length (void) const {
        if (! m_chars)
            return 0;
        const TableRep *rep = (const TableRep *)(m_chars - chars_offset);
        return rep->length;
    }

    /// Return the number of characters in the string.
    ///
    size_t size (void) const { return length(); }

    /// Is the string empty -- i.e., is it the NULL pointer or does it
    /// point to an empty string?
    bool empty (void) const { return (size() == 0); }

    /// Cast to int, which is interpreted as testing whether it's not an
    /// empty string.  This allows you to write "if (t)" with the same
    /// semantics as if it were a char*.
    operator int (void) { return !empty(); }

    /// Return a const_iterator that references the first character of
    /// the string.
    const_iterator begin () const { return string().begin(); }

    /// Return a const_iterator that references the end of a traversal
    /// of the characters of the string.
    const_iterator end () const { return string().end(); }

    /// Return a const_reverse_iterator that references the last
    /// character of the string.
    const_reverse_iterator rbegin () const { return string().rbegin(); }

    /// Return a const_reverse_iterator that references the end of
    /// a reverse traversal of the characters of the string.
    const_reverse_iterator rend () const { return string().rend(); }

    /// Return a reference to the character at the given position.
    /// Note that it's up to the caller to be sure pos is within the
    /// size of the string.
    const_reference operator[] (size_type pos) const { return c_str()[pos]; }

    /// Dump into character array s the characters of this ustring,
    /// beginning with position pos and copying at most n characters.
    size_type copy (char* s, size_type n, size_type pos = 0) const {
        char *c = strncpy (s, c_str()+pos, n);
        return (size_type)(c-s);
    }

    // FIXME: implement find, rfind, find_first_of, find_last_of,
    // find_first_not_of, find_last_not_of, substr, compare.

    /// Return 0 if *this is lexicographically equal to str, -1 if 
    /// *this is lexicographically earlier than str, 1 if *this is
    /// lexicographically after str.

    int compare (const ustring& str) const {
        return c_str() == str.c_str() ? 0 : strcmp (c_str(), str.c_str());
    }

    /// Return 0 if *this is lexicographically equal to str, -1 if 
    /// *this is lexicographically earlier than str, 1 if *this is
    /// lexicographically after str.
    int compare (const std::string& str) const {
        return strcmp (c_str(), str.c_str());
    }

    /// Return 0 if a is lexicographically equal to b, -1 if a is
    /// lexicographically earlier than b, 1 if a is lexicographically
    /// after b.
    friend int compare (const std::string& a, const ustring &b) {
        return strcmp (a.c_str(), b.c_str());
    }

    /// Test two ustrings for equality -- are they comprised of the same
    /// sequence of characters.  Note that because ustrings are unique,
    /// this is a trivial pointer comparison, not a char-by-char loop as
    /// would be the case with a char* or a std::string.
    bool operator== (const ustring &str) const { return c_str() == str.c_str(); }

    /// Test two ustrings for inequality -- are they comprised of different
    /// sequences of characters.  Note that because ustrings are unique,
    /// this is a trivial pointer comparison, not a char-by-char loop as
    /// would be the case with a char* or a std::string.
    bool operator!= (const ustring &str) const { return c_str() != str.c_str(); }

    /// Test a ustring (*this) for lexicographic equality with std::string
    /// x.
    bool operator== (const std::string &x) const { return compare(x) == 0; }

    /// Test for lexicographic equality between std::string a and ustring
    /// b.
    friend bool operator== (const std::string &a, const ustring &b) {
        return b.compare(a) == 0;
    }

    /// Test a ustring (*this) for lexicographic inequality with
    /// std::string x.
    bool operator!= (const std::string &x) const { return compare(x) != 0; }

    /// Test for lexicographic inequality between std::string a and
    /// ustring b.
    friend bool operator!= (const std::string &a, const ustring &b) {
        return b.compare(a) != 0;
    }


    /// Construct a ustring in a printf-like fashion.  In other words,
    /// something like:
    ///    ustring s = ustring::format ("blah %d %g", (int)foo, (float)bar);
    ///
    static ustring format (const char *fmt, ...);

    /// Generic stream output of a ustring.
    ///
    friend std::ostream & operator<< (std::ostream &out, const ustring &str) {
        if (str.c_str())
            out << str.c_str();
        return out;
    }

private:

    /// Individual ustring internal representation -- the unique characters.
    ///
    const char *m_chars;

public:
    /// Representation within the hidden string table -- DON'T EVER CREATE
    /// ONE OF THESE YOURSELF!
    struct TableRep {
        std::string str;     // String representation
        size_t length;       // Length of the string
        char chars[0];       // The characters
        TableRep (const char *s) : str(s), length(str.size()) {
            strcpy (chars, s);
        }
        const char *c_str () const { return chars; }
    };
    /// Constant defining how far beyond the beginning of a TableRep are
    /// the canonical characters.
    static const off_t chars_offset = offsetof(TableRep, chars);

private:
    /// Important internal guts of ustring -- given a null-terminated
    /// string, return a pointer to the unique internal table
    /// representation of the string (creating a new table entry if we
    /// haven't seen this sequence of characters before).
    static const TableRep * _make_unique (const char *str);
};



class ustringHash
#ifdef WINNT
    : public hash_compare<ustring>
#endif
{
public:
    size_t operator() (const ustring &s) const { return (size_t)s.c_str(); }
#ifdef WINNT
    bool operator() (const ustring &a, const ustring &b) {
        return strcmp (a.c_str(), b.c_str()) < 0;
    }
#endif
};




// };  // end namespace blah

#endif // USTRING_H