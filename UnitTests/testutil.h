#pragma once

#include <stdexcept>
#include <string>

struct Counter
{
	Counter()
	{
		++ctors;
	}

	Counter(const Counter& other) :
		load(other.load) 
	{
		++copies;
	}
	Counter(Counter&& other) noexcept :
		load(other.load) 
	{
		++moves;
	}
	Counter& operator=(const Counter& other)
	{
		++copies;
		load = other.load;
		return *this;
	}
	Counter& operator=(Counter&& other) noexcept
	{
		++moves;
		load = other.load;
		return *this;
	}

	~Counter()
	{
		++dtors;
	}

	static void reset()
	{
		staticVal = 0;
		copies = 0;
		moves = 0;
		dtors = 0;
		ctors = 0;
	}

    void Val(int v) // NOLINT(readability-convert-member-functions-to-static)
	{
		if (dtors == ctors)
			throw std::logic_error("function call on a destroyed object");
		staticVal = v;
	}

    [[nodiscard]] static int Val()
    {
        return staticVal;
    }

    int load = 0;

	static inline int copies = 0;
	static inline int moves = 0;
	static inline int ctors = 0;
	static inline int dtors = 0;

private:
    static inline int staticVal = 0;
};

inline std::size_t freeFunction(int i, const std::string& s)
{
	return i + s.length();
}

struct TestObject
{
	int set(int i)
	{
		c.Val(i);
		return i;
	}

	[[nodiscard]] int get() // NOLINT(readability-convert-member-functions-to-static)
	{
		return Counter::Val();
	}

	[[nodiscard]] int getConst() const // NOLINT(readability-convert-member-functions-to-static)
	{
		return Counter::Val();
	}

	static std::size_t staticMemberFn(int i, const std::string& s)
	{
		return i + s.length();
	}

	Counter c;
};

struct TestObject2
{
	[[nodiscard]] int get() // NOLINT(readability-convert-member-functions-to-static)
	{
		return 0;
	}

	[[nodiscard]] int getConst() const // NOLINT(readability-convert-member-functions-to-static)
	{
		return 0;
	}
};

struct TestBase
{
	virtual ~TestBase() = default;

	virtual std::string fn()
	{
		return "base";
	}
};

struct TestDerived : TestBase
{
	std::string fn() override
	{
		return "derived";
	}
};


