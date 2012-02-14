/*
 * a template class which implements a hashtable with integer keys
 */

#ifndef inthashtable_tpl_h
#define inthashtable_tpl_h

#include "hashtable_tpl.h"


/*
 * Define the key characteristica for hashing integer types
 */
template<class key_t>
class inthash_tpl {
public:
    static uint32 hash(const key_t key)
    {
		return (uint32)key;
    }

	static void dump(const key_t key)
    {
		printf("%d", (int)key);
    }

	static int comp(key_t key1, key_t key2)
    {
		return (int)key1 - (int)key2;
    }
};


/*
 * Ready to use class for hashing integer types. Note that key can be of any
 * integer type including enums.
 */
template<class key_t, class value_t>
class inthashtable_tpl : public hashtable_tpl<key_t, value_t, inthash_tpl<key_t> >
{
public:
	inthashtable_tpl() : hashtable_tpl<key_t, value_t, inthash_tpl<key_t> >() {}
private:
	inthashtable_tpl(const inthashtable_tpl&);
	inthashtable_tpl& operator=( inthashtable_tpl const&);
};

#endif
