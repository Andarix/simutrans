#ifndef PLAINSTRING_H
#define PLAINSTRING_H

#include <cstring>

#include "../simtypes.h"


class plainstring
{
	public:
		plainstring() : str_() {}

		plainstring(char const* const s) : str_(copy_string(s)) {}

		~plainstring() { delete [] str_; }

		plainstring& operator =(char const* const o)
		{
			char* const s = copy_string(o);
			delete [] str_;
			str_ = s;
			return *this;
		}

		plainstring& operator =(plainstring const& o)
		{
			char* const s = copy_string(o.str_);
			delete [] str_;
			str_ = s;
			return *this;
		}

		char const* c_str() const { return str_; }

		operator char const*() const { return str_; }
		operator char*()             { return str_; }

		bool operator ==(char const* const o) const { return str_ && o ? std::strcmp(str_, o) == 0 : str_ == o; }
		bool operator !=(char const* const o) const { return !(*this == o); }

	private:
		static char* copy_string(char const* const s)
		{
			if (s) {
				size_t const n = std::strlen(s) + 1;
				return static_cast<char*>(std::memcpy(new char[n], s, n));
			} else {
				return 0;
			}
		}

		char* str_;
};

void free(plainstring const&) DELETED;

#endif
