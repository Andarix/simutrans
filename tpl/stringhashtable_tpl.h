/*
 * a template class which implements a hashtable with string keys
 */

#ifndef stringhashtable_tpl_h
#define stringhashtable_tpl_h

#include "hashtable_tpl.h"
#include <string.h>

/*
 * Define the key characteristica for hashing "const char *".
 */
class stringhash_t {
public:
	static uint32 hash(const char *key)
	{
		uint32 hash = 0;
#if 1
		for(  sint8 i=16;  i*key[0]!=0;  i--  ) {
			hash += (uint8)(*key++);
		}
#else
		while (*key != '\0') {
			hash = hash * 33 + *key++;
		}
#endif
		return hash;
	}

	static const char *null()
	{
		return NULL;
	}

	static void dump(const char *key)
	{
		printf("%s", key);
	}

	static int comp(const char *key1, const char *key2)
	{
		return strcmp(key1, key2);
	}
};


/*
 * Ready to use class for hashing strings.
 */
template <class value_t>
class stringhashtable_tpl : public hashtable_tpl<const char *, value_t, stringhash_t>
{
};


template<class value_t>
class stringhashtable_iterator_tpl : public hashtable_iterator_tpl<const char *, value_t, stringhash_t>
{
public:
	stringhashtable_iterator_tpl(const hashtable_tpl<const char *, value_t, stringhash_t> *hashtable) :
		hashtable_iterator_tpl<const char *, value_t, stringhash_t>(hashtable) {}

	stringhashtable_iterator_tpl(const hashtable_tpl<const char *, value_t, stringhash_t> &hashtable) :
		hashtable_iterator_tpl<const char *, value_t, stringhash_t>(hashtable) {}
};

#endif
