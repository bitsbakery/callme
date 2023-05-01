#include "pch.h"

#include <array>
#include <memory>

#include "Stopwatch.h"
#include "CallMe.Event.h"

#if defined(_MSC_VER) && !defined(CMAKE)
import pretty;
#else
#include "pretty.h"
#endif

using namespace CallMe;
using namespace CallMe::internal;
using namespace std::literals;
using std::cout;
using std::endl;

constexpr auto nIters = 10'000'000;

constexpr auto nEventSubscriptions = 10;
constexpr auto nEventIters = nIters / nEventSubscriptions;

int I1 = 1;
auto I2 = "qwertyuiop[]asdfghjkl;'zxcvbnm,./"s;
int O1;
std::size_t O2;
constexpr auto NotAvailable = "N/A";

#ifdef _MSC_VER
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif

//this signature is common for all test targets (free functions, member functions, functors),
//except for event benchmark
using FreeSignature = void(std::string&, volatile int*, volatile std::size_t*);

struct TargetObject
{
	void InlineMethod(std::string& i2, // NOLINT(readability-make-member-function-const)
					   volatile int* o1,
					   volatile std::size_t* o2)
	{
		*o1 = _i;
		*o2 = i2.size();
	}

	void NOINLINE NonInlinedMethod(std::string& i2, // NOLINT(readability-make-member-function-const)
								   volatile int* o1,
								   volatile std::size_t* o2)
	{
		*o1 = _i;
		*o2 = i2.size();
	}

	int _i = 1;
};

using TargetMethod = void (TargetObject::*)(
	std::string& i2,
	volatile int* o1,
	volatile std::size_t* o2);

void InlineFreeFunction(std::string& i2, volatile int* o1, volatile std::size_t* o2)
{
	*o1 = I1;
	*o2 = i2.size();
}

NOINLINE void NoninlinedFreeFunction(std::string& i2, volatile int* o1, volatile std::size_t* o2)
{
	*o1 = I1;
	*o2 = i2.size();
}

using OwningDelegateT = OwningDelegate<FreeSignature>;
using DelegateT = Delegate<FreeSignature>;
using bitwizDelegate = cpp::delegate<void(std::string&, int*, std::size_t*)>;

static std::string toString(DurationT t)
{
	const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t);
	return std::to_string(duration.count()) + " us";
}

struct ResultHandler
{
	explicit ResultHandler(pretty::Table& table,
						   pretty::RowN row,
						   pretty::ColN col) :
		_table(table),
		_col(col),
		_row(row)
	{
	}

	void submit(std::string&& s)
	{
		if (_row > _table.numRows() - 1)
			_table.addRows(1);
		_table.setText(_row, _col, std::move(s));
		_row.checkedInc();
	}

	void submit(DurationT t)
	{
		submit(toString(t));
	}

	void RowCol(pretty::RowN r, pretty::ColN c)
	{
		_row = r; _col = c;
	}

private:
	pretty::Table& _table;
	pretty::ColN _col;
	pretty::RowN _row;
};

void DelegateDependentFunction(DelegateT& d)
{
	d(I2, &O1, &O2);
}

void NOINLINE NoninlinedDelegateDependentFunction(DelegateT& d)
{
	d(I2, &O1, &O2);
}

template<auto function>
DurationT BenchmarkDelegateDependentFunction(DelegateT& delegate)
{
	Stopwatch time;
	time.start();
	for (auto i = nIters; i; --i)
	{
		function(delegate);
	}
	time.stop();

	return time.elapsed();
}

DurationT BenchmarkInvocableDynamic(auto& invocable)
{
	Stopwatch time;
	time.start();
	for (auto i = nIters; i; --i)
	{
		invocable(I2, &O1, &O2);
	}
	time.stop();

	return time.elapsed();
}

template<auto invocable>
DurationT BenchmarkInvocableStatic()
{
	Stopwatch time;
	time.start();
	for (auto i = nIters; i; --i)
	{
		invocable(I2, &O1, &O2);
	}
	time.stop();

	return time.elapsed();
}

struct DirectCallBenchmark
{
	static std::string Name() { return "direct call"; }

    static DurationT BenchmarkFunctor(AnyFunctor auto& functor)
	{
		return BenchmarkInvocableDynamic(functor);
	}

	template<TargetMethod method>
    static DurationT BenchmarkMethod(TargetObject& t)
	{
		Stopwatch time;
		time.start();
		for (auto i = nIters; i; --i)
		{
			if constexpr(method == &TargetObject::NonInlinedMethod)
				t.NonInlinedMethod(I2, &O1, &O2);
			else if constexpr(method == &TargetObject::InlineMethod)
				t.InlineMethod(I2, &O1, &O2);
		}
		time.stop();

		return time.elapsed();
	}

	template<auto function>
	static DurationT BenchmarkFunction()
	{
		return BenchmarkInvocableStatic<function>();
	}
};

struct OwningDelegateBenchmark
{
    static std::string Name() { return "OwningDelegate"; }

	static DurationT BenchmarkFunctor(NonLiteFunctor auto& functor)
	{
		//allocate a new functor on the heap and
		//copy-construct from functor
		auto functorCopy = new auto(functor);

		auto delegate = fromFunctorOwned(functorCopy);
		return BenchmarkInvocableDynamic(delegate);
	}

	template<TargetMethod method>
    static DurationT BenchmarkMethod(TargetObject& t)
	{
		//allocate a new object on the heap and
		//copy-construct from t
		auto objectCopy = new TargetObject(t);

		auto delegate = fromMethodOwned<method>(objectCopy);
		return BenchmarkInvocableDynamic(delegate);
	}
};

struct ObjectWithFunctorDelegate
{
	DelegateT delegate;

	explicit ObjectWithFunctorDelegate(AnyFunctor auto& functor) :
		delegate( fromFunctor(functor) )
	{
	}

	DurationT BenchmarkFunctor()
	{
		return BenchmarkInvocableDynamic(delegate);
	}
};

template<MemberFunction auto method>
struct ObjectWithMethodDelegate
{
	DelegateT delegate;

	ObjectWithMethodDelegate(TargetObject&t) : 
		delegate( fromMethod<method>(t) )
	{
	}

	DurationT BenchmarkMethod()
	{
		return BenchmarkInvocableDynamic(delegate);
	}
};

template<FreeFunction auto function>
struct ObjectWithFunctionDelegate
{
	DelegateT delegate;

	ObjectWithFunctionDelegate() : 
		delegate( fromFunction<function>() )
	{
	}

	DurationT BenchmarkFunction()
	{
		return BenchmarkInvocableDynamic(delegate);
	}
};

struct DelegateInObjectOnHeapBenchmark
{
    static std::string Name() { return "Delegate in object on heap"; }

    static DurationT BenchmarkFunctor(auto& functor)
	{
		auto obj = std::make_unique<ObjectWithFunctorDelegate>(functor);
		return obj->BenchmarkFunctor();
	}

	template<TargetMethod method>
    static DurationT BenchmarkMethod(TargetObject& t)
	{
		auto obj = std::make_unique<ObjectWithMethodDelegate<method>>(t);
		return obj->BenchmarkMethod();
	}

	template<FreeFunction auto function>
	static DurationT BenchmarkFunction()
	{
		auto obj = std::make_unique<ObjectWithFunctionDelegate<function>>();
		return obj->BenchmarkFunction();
	}
};

struct DelegateInVectorBenchmark
{
    static std::string Name() { return "Delegate in vector"; }

    static DurationT BenchmarkFunctor(AnyFunctor auto& functor)
	{
		std::vector<DelegateT> v;
		v.push_back(fromFunctor(functor));
		return BenchmarkInvocableDynamic(v.back());
	}

	template<TargetMethod method>
    static DurationT BenchmarkMethod(TargetObject& t)
	{
		std::vector<DelegateT> v;
		v.push_back(fromMethod<method>(t));
		return BenchmarkInvocableDynamic(v.back());
	}

	template<FreeFunction auto function>
	static DurationT BenchmarkFunction()
	{
		std::vector<DelegateT> v;
		v.push_back(fromFunction<function>());
		return BenchmarkInvocableDynamic(v.back());
	}
};

struct DelegateOnHeapBenchmark
{
    static std::string Name() { return "Delegate on heap"; }

    static DurationT BenchmarkFunctor(AnyFunctor auto& functor)
	{
		auto delegate = std::make_unique<DelegateT>(functor);
		return BenchmarkInvocableDynamic(*delegate);
	}

	template<TargetMethod method>
    static DurationT BenchmarkMethod(TargetObject& t)
	{
		auto delegate = std::make_unique<DelegateT>(t, tag<method>());
		return BenchmarkInvocableDynamic(*delegate);
	}

	template<FreeFunction auto function>
	static DurationT BenchmarkFunction()
	{
		auto delegate = std::make_unique<DelegateT>(tag<function>());
		return BenchmarkInvocableDynamic(*delegate);
	}
};

struct DelegateMoveCtorBenchmark
{
	static std::string Name() { return "Delegate(Delegate&&)"; }

	static DurationT BenchmarkFunctor(AnyFunctor auto& functor)
	{
		DelegateT delegateA = fromFunctor(functor);
		delegateA(I2, &O1, &O2);//use A
		DelegateT delegateB(std::move(delegateA));//move A to B
		return BenchmarkInvocableDynamic(delegateB);
	}

	template<TargetMethod method>
	static DurationT BenchmarkMethod(TargetObject& t)
	{
		DelegateT delegateA = fromMethod<method>(t);
		delegateA(I2, &O1, &O2);//use A
		DelegateT delegateB(std::move(delegateA));//move A to B
		return BenchmarkInvocableDynamic(delegateB);
	}

	template<FreeFunction auto function>
	static DurationT BenchmarkFunction()
	{
		DelegateT delegateA = fromFunction<function>();
		delegateA(I2, &O1, &O2);//use A
		DelegateT delegateB(std::move(delegateA));//move A to B
		return BenchmarkInvocableDynamic(delegateB);
	}
};

struct OwningDelegateMoveCtorBenchmark
{
    static std::string Name() { return "ODelegate(ODelegate&&)"; }

    static DurationT BenchmarkFunctor(NonLiteFunctor auto& functor)
	{
		OwningDelegateT delegateA = fromFunctorOwned(new auto(functor));
		delegateA(I2, &O1, &O2);//use A
		OwningDelegateT delegateB(std::move(delegateA));//move A to B
		return BenchmarkInvocableDynamic(delegateB);
	}

	template<TargetMethod method>
    static DurationT BenchmarkMethod(TargetObject& t)
	{
		OwningDelegateT delegateA = fromMethodOwned<method>(new TargetObject(t));
		delegateA(I2, &O1, &O2);//use A
		OwningDelegateT delegateB(std::move(delegateA));//move A to B
		return BenchmarkInvocableDynamic(delegateB);
	}
};

struct OwningDelegateMoveAssignedBenchmark
{
    static std::string Name() { return "ODelegate(nonempty)=move()"; }

    static DurationT BenchmarkFunctor(NonLiteFunctor auto& functor)
	{
		OwningDelegateT delegateA = fromFunctorOwned(new auto(functor));
		delegateA(I2, &O1, &O2);//use A

		OwningDelegateT delegateB = fromFunctorOwned(new auto(functor));
		delegateB(I2, &O1, &O2);//use B

		delegateB = std::move(delegateA);//move A to B

		return BenchmarkInvocableDynamic(delegateB);
	}

	template<TargetMethod method>
    static DurationT BenchmarkMethod(TargetObject& t)
	{
		OwningDelegateT delegateA = fromMethodOwned<method>(new TargetObject(t));
		delegateA(I2, &O1, &O2);//use A

		OwningDelegateT delegateB = fromMethodOwned<method>(new TargetObject(t));
		delegateB(I2, &O1, &O2);//use B

		delegateB = std::move(delegateA);//move A to B

		return BenchmarkInvocableDynamic(delegateB);
	}
};

struct DelegateMoveAssignedBenchmark
{
	static std::string Name() { return "Delegate(nonempty)=move()"; }

	static DurationT BenchmarkFunctor(AnyFunctor auto& functor)
	{
		DelegateT delegateA = fromFunctor(functor);
		delegateA(I2, &O1, &O2);//use A

		DelegateT delegateB = fromFunctor(functor);
		delegateB(I2, &O1, &O2);//use B

		delegateB = std::move(delegateA);//move A to B

		return BenchmarkInvocableDynamic(delegateB);
	}

	template<TargetMethod method>
	static DurationT BenchmarkMethod(TargetObject& t)
	{
		DelegateT delegateA = fromMethod<method>(t);
		delegateA(I2, &O1, &O2);//use A

		DelegateT delegateB = fromMethod<method>(t);
		delegateB(I2, &O1, &O2);//use B

		delegateB = std::move(delegateA);//move A to B

		return BenchmarkInvocableDynamic(delegateB);
	}

	template<auto function>
	static DurationT BenchmarkFunction()
	{
		DelegateT delegateA = fromFunction<function>();
		delegateA(I2, &O1, &O2);//use A

		DelegateT delegateB = fromFunction<function>();
		delegateB(I2, &O1, &O2);//use B

		delegateB = std::move(delegateA);//move A to B

		return BenchmarkInvocableDynamic(delegateB);
	}
};

struct EmptyOwningDelegateMoveAssignedBenchmark
{
    static std::string Name() { return "ODelegate(empty)=move()"; }

    static DurationT BenchmarkFunctor(NonLiteFunctor auto& functor)
	{
		OwningDelegateT delegateA;
		delegateA(I2, &O1, &O2);//use A, NOP

		OwningDelegateT delegateB = fromFunctorOwned(new auto(functor));
		delegateB(I2, &O1, &O2);//use B

		delegateA = std::move(delegateB);//move B to A

		return BenchmarkInvocableDynamic(delegateA);
	}

	template<TargetMethod method>
    static DurationT BenchmarkMethod(TargetObject& t)
	{
		OwningDelegateT delegateA;
		delegateA(I2, &O1, &O2);//use A, NOP

		OwningDelegateT delegateB = fromMethodOwned<method>(new TargetObject(t));
		delegateB(I2, &O1, &O2);//use B

		delegateA = std::move(delegateB);//move B to A

		return BenchmarkInvocableDynamic(delegateA);
	}
};

struct EmptyDelegateMoveAssignedBenchmark
{
    static std::string Name() { return "Delegate(empty)=move()"; }

    static DurationT BenchmarkFunctor(AnyFunctor auto& functor)
	{
		DelegateT delegateA;
		delegateA(I2, &O1, &O2);//use A, NOP

		DelegateT delegateB = fromFunctor(functor);
		delegateB(I2, &O1, &O2);//use B

		delegateA = std::move(delegateB);//move B to A

		return BenchmarkInvocableDynamic(delegateA);
	}

	template<TargetMethod method>
    static DurationT BenchmarkMethod(TargetObject& t)
	{
		DelegateT delegateA;
		delegateA(I2, &O1, &O2);//use A, NOP

		DelegateT delegateB = fromMethod<method>(t);
		delegateB(I2, &O1, &O2);//use B
		
		delegateA = std::move(delegateB);//move B to A

		return BenchmarkInvocableDynamic(delegateA);
	}

	template<FreeFunction auto function>
	static DurationT BenchmarkFunction()
	{
		DelegateT delegateA;
		delegateA(I2, &O1, &O2);//use A, NOP

		DelegateT delegateB = fromFunction<function>();
		delegateB(I2, &O1, &O2);//use B

		delegateA = std::move(delegateB);//move B to A

		return BenchmarkInvocableDynamic(delegateA);
	}
};

struct DelegateBenchmark
{
    static std::string Name() { return "Delegate"; }

    static DurationT BenchmarkFunctor(AnyFunctor auto& functor)
	{
		auto delegate = fromFunctor(functor);
		return BenchmarkInvocableDynamic(delegate);
	}

	template<TargetMethod method>
    static DurationT BenchmarkMethod(TargetObject& t)
	{
		auto delegate = fromMethod<method>(t);
		return BenchmarkInvocableDynamic(delegate);
	}

	template<FreeFunction auto function>
	static DurationT BenchmarkFunction()
    {
		auto delegate = fromFunction<function>();
		return BenchmarkInvocableDynamic(delegate);
    }
};

struct InlineDelegateDependentFunctionBenchmark
{
    static std::string Name() { return "inline f(Delegate& d)"; }

    static DurationT BenchmarkFunctor(AnyFunctor auto& functor)
	{
		auto delegate = fromFunctor(functor);
		return BenchmarkDelegateDependentFunction<DelegateDependentFunction>(delegate);
	}

	template<TargetMethod method>
    static DurationT BenchmarkMethod(TargetObject& t)
	{
		auto delegate = fromMethod<method>(t);
		return BenchmarkDelegateDependentFunction<DelegateDependentFunction>(delegate);
	}

	template<FreeFunction auto function>
	static DurationT BenchmarkFunction()
	{
		auto delegate = fromFunction<function>();
		return BenchmarkDelegateDependentFunction<DelegateDependentFunction>(delegate);
	}
};

struct NoninlinedDelegateDependentFunctionBenchmark
{
    static std::string Name() { return "noninlined f(Delegate& d)"; }

    static DurationT BenchmarkFunctor(AnyFunctor auto& functor)
	{
		auto delegate = fromFunctor(functor);
		return BenchmarkDelegateDependentFunction<NoninlinedDelegateDependentFunction>(delegate);
	}

	template<TargetMethod method>
    static DurationT BenchmarkMethod(TargetObject& t)
	{
		auto delegate = fromMethod<method>(t);
		return BenchmarkDelegateDependentFunction<NoninlinedDelegateDependentFunction>(delegate);
	}

	template<FreeFunction auto function>
	static DurationT BenchmarkFunction()
	{
		auto delegate = fromFunction<function>();
		return BenchmarkDelegateDependentFunction<NoninlinedDelegateDependentFunction>(delegate);
	}
};

struct BitwizeshiftDelegateBenchmark
{
    static std::string Name() { return "Bitwizeshift"; }

    static DurationT BenchmarkFunctor(AnyFunctor auto& functor)
	{
		bitwizDelegate delegate = cpp::bind(&functor);
		return BenchmarkInvocableDynamic(delegate);
	}

	template<TargetMethod method>
    static DurationT BenchmarkMethod(TargetObject& t)
	{
		bitwizDelegate delegate = cpp::bind<method>(&t);
		return BenchmarkInvocableDynamic(delegate);
	}

	template<FreeFunction auto function>
	static DurationT BenchmarkFunction()
	{
		bitwizDelegate delegate = cpp::bind<function>();
		return BenchmarkInvocableDynamic(delegate);
	}
};

template<typename Benchmark>
concept HasBenchmarkFunction = requires(Benchmark&b)
{
	{b.template BenchmarkFunction<&InlineFreeFunction>()};
};

template<typename Benchmark, typename Functor>
concept HasBenchmarkFunctor = requires(Benchmark&b, Functor& f)
{
	{b.BenchmarkFunctor(f)};
};

template<typename benchmarkSet>
struct Benchmarks
{
	benchmarkSet benchmarks;

	constexpr static auto numTests = 
		std::tuple_size_v<decltype(benchmarks)>;

	template<AnyFunctor Functor, int I = 0>
	void BenchmarkFunctor(Functor& f, ResultHandler& h)
	{
		if constexpr (I < numTests)
		{
			auto& benchmark = std::get<I>(benchmarks);
			if constexpr (HasBenchmarkFunctor<decltype(benchmark), Functor>)
			{
				const DurationT res = benchmark.BenchmarkFunctor(f);
				h.submit(res);
			}
			else
			{
				h.submit(NotAvailable);
			}
			BenchmarkFunctor<Functor, I + 1>(f, h);
		}
	}

	template<TargetMethod method, int I = 0>
	void BenchmarkMethod(TargetObject& t, ResultHandler& h)
	{
		if constexpr (I < numTests)
		{
			auto& benchmark = std::get<I>(benchmarks);
			const DurationT res = benchmark.template BenchmarkMethod<method>(t);
			h.submit(res);

			BenchmarkMethod<method, I + 1>(t, h);
		}
	}

	template<FreeFunction auto function, int I = 0>
	void BenchmarkFunction(ResultHandler& h)
	{
		if constexpr (I < numTests)
		{
			auto& benchmark = std::get<I>(benchmarks);
			if constexpr (HasBenchmarkFunction<decltype(benchmark)>)
			{
				const DurationT res = benchmark.template BenchmarkFunction<function>();
				h.submit(res);
			}
			else
			{
				h.submit(NotAvailable);
			}
			BenchmarkFunction<function, I + 1>(h);
		}
	}

	template<int I = 0>
	void GatherNames(ResultHandler& h)
	{
		if constexpr (I < numTests)
		{
			std::string name = std::get<I>(benchmarks).Name();
			h.submit(std::move(name));

			GatherNames<I + 1>(h);
		}
	}
};

struct EventBenchmark
{
	template<TargetMethod method>
	static DurationT BenchmarkDirectCall()
	{
		std::vector<TargetObject> targets(nEventSubscriptions);

		Stopwatch time;
		time.start();
		for (auto i = nEventIters; i; --i)
		{
			//notify all subscribers
			for (auto s = 0; s != nEventSubscriptions; ++s)
				(targets[s].*method)(I2, &O1, &O2);
		}
		time.stop();

		return time.elapsed();
	}

	template<TargetMethod method>
	static DurationT BenchmarkArrayOfPointers()
	{
		struct Subscription
		{
			TargetObject o;
			TargetMethod m {nullptr};
		};
		Subscription targets[nEventSubscriptions];

		for (auto s = 0; s != nEventSubscriptions; ++s)
			targets[s].m = method;

		Stopwatch time;
		time.start();
		for (auto i = nEventIters; i; --i)
		{
			//notify all subscribers
			for (auto s = 0; s != nEventSubscriptions; ++s)
			{
				Subscription* sub = targets + s;
				(sub->o.*sub->m)(I2, &O1, &O2);
			}
		}
		time.stop();
		return time.elapsed();
	}

	template<TargetMethod method>
	static DurationT BenchmarkEventRaise()
	{
		CallMe::Event<FreeSignature, nEventSubscriptions> event(nEventSubscriptions);

		std::vector<TargetObject> targets(nEventSubscriptions);

		std::vector<Subscription> subscriptions;
		subscriptions.reserve(nEventSubscriptions);

		for (auto& t : targets)
		{
			event.subscribe(fromMethod<method>(t), subscriptions);
		}
		assert(subscriptions.size() == targets.size());

		Stopwatch time;
		time.start();
		for (auto i = nEventIters; i; --i)
		{
			event.raise(I2, &O1, &O2);
		}
		time.stop();

		return time.elapsed();
	}
};

struct ArgumentPassingBenchmark
{
	static NOINLINE void FunctionWithRefArgs(std::string&& a, std::string& b)
	{
		b = std::move(a);
	}

	template<typename Callable>
	static DurationT Benchmark(Callable& callable)
	{
		Stopwatch time;
		std::string a = "1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./";
		std::string b;
		time.start();

		MSVC_SUPPRESS_WARNING_WITH_PUSH(26800) //use of a moved object
		for (auto i = nIters; i; --i)
		{
			callable(std::move(a), b);
			callable(std::move(b), a);
		}
		time.stop();
		MSVC_SUPPRESS_WARNING_POP

		return time.elapsed();
	}

	static DurationT BenchmarkDirectCall()
	{
		return Benchmark(FunctionWithRefArgs);
	}

	static DurationT BenchmarkDelegate()
	{
		Delegate d{tag<FunctionWithRefArgs>()};
		return Benchmark(d);
	}

	static DurationT BenchmarkEvent()
	{
		using signature = void(std::string&& a, std::string& b);
		CallMe::Event<signature, 1> e;
		auto sub = e.subscribe(fromFunction<FunctionWithRefArgs>());
		return Benchmark(e);
	}
};

int main()
{
	int i1 = 1;

	//when compiled with optimizations (Release mode),
	//MSVC++ inlines this lambda
	auto λInline = [&i1](std::string& i2,
						volatile int* o1,
						volatile std::size_t* o2) 
	{
		*o1 = i1;
		*o2 = i2.size();
	};
	auto λNoninlined = [&i1](std::string& i2,
							volatile int* o1,
							volatile std::size_t* o2) NOINLINE
	{
		*o1 = i1;
		*o2 = i2.size();
	};
	auto λCapturelessInline = [](std::string& i2,
								  volatile int* o1,
								  volatile std::size_t* o2) 
	{
		*o1 = I1;
		*o2 = i2.size();
	};
	auto λCapturelessNoninlined = [](std::string& i2,
									 volatile int* o1,
									 volatile std::size_t* o2) NOINLINE
	{
		*o1 = I1;
		*o2 = i2.size();
	};

	pretty::Table tableInline;
	pretty::Table tableNoninlined;
	pretty::Table tableMoved;
	pretty::Table tableInlineHeap;
	pretty::Table tableDelegateAsParameter;
	pretty::Table tableEvent;
	pretty::Table tableArgumentPassing;
	std::vector tables = 
	{
		&tableInline,
		&tableNoninlined,
		&tableInlineHeap,
		&tableMoved,
		&tableDelegateAsParameter
	};
	for(auto t : tables)
		t->addRow("invocable", "λ with capture", "captureless λ", "method", "function");

	Benchmarks<std::tuple<
		DirectCallBenchmark,
		DelegateBenchmark,
		OwningDelegateBenchmark/*,
		BitwizeshiftDelegateBenchmark*/>> benchmarks;

	pretty::RowN row{1};//skip header row
	pretty::ColN col{};

	tableInline.title("Stack-allocated delegates, inlinable target");
	tableNoninlined.title("Stack-allocated delegates, noninlinable target");
	
	{ //column: delegate names
		{
			ResultHandler h{ tableInline, row, col };
			benchmarks.GatherNames(h);
		}
		{
			ResultHandler h{ tableNoninlined, row, col };
			benchmarks.GatherNames(h);
		}
	}

	{ //column: lambda with capture
		++col;
		{
			ResultHandler h{ tableInline, row, col };
			benchmarks.BenchmarkFunctor(λInline, h);
		}
		{
			ResultHandler h{ tableNoninlined, row, col };
			benchmarks.BenchmarkFunctor(λNoninlined, h);
		}
	}

	{ //column: captureless lambda
		++col;
		{
			ResultHandler h{ tableInline, row, col };
			benchmarks.BenchmarkFunctor(λCapturelessInline, h);
		}
		{
			ResultHandler h{ tableNoninlined, row, col };
			benchmarks.BenchmarkFunctor(λCapturelessNoninlined, h);
		}
	}

	{ //column: method
		++col;
		TargetObject targetObj;
		{
			ResultHandler h{ tableInline, row, col };
			benchmarks.BenchmarkMethod<&TargetObject::InlineMethod>(targetObj, h);
		}
		{
			ResultHandler h{ tableNoninlined, row, col };
			benchmarks.BenchmarkMethod<&TargetObject::NonInlinedMethod>(targetObj, h);
		}
	}

	{ //column: function
		++col;
		{
			ResultHandler h{ tableInline, row, col };
			benchmarks.BenchmarkFunction<&InlineFreeFunction>(h);
		}
		{
			ResultHandler h{ tableNoninlined, row, col };
			benchmarks.BenchmarkFunction<&NoninlinedFreeFunction>(h);
		}
	}

	{//move benchmarks
		tableMoved.title("Moved stack-allocated delegates, inlinable target");
		Benchmarks<std::tuple<
			DelegateMoveCtorBenchmark,
			DelegateMoveAssignedBenchmark,
			EmptyDelegateMoveAssignedBenchmark,

			OwningDelegateMoveCtorBenchmark,
			OwningDelegateMoveAssignedBenchmark,
			EmptyOwningDelegateMoveAssignedBenchmark>> moveBenchmarks;

		row = 1;
		col = 0;

		ResultHandler h{ tableMoved, row, col };
		{
			moveBenchmarks.GatherNames(h);
		}
		{
			h.RowCol(row, ++col);
			moveBenchmarks.BenchmarkFunctor(λInline, h);
		}
		{
			h.RowCol(row, ++col);
			moveBenchmarks.BenchmarkFunctor(λCapturelessInline, h);
		}
		{
			TargetObject targetObj;
			h.RowCol(row, ++col);
			moveBenchmarks.BenchmarkMethod<&TargetObject::InlineMethod>(targetObj, h);
		}
		{
			h.RowCol(row, ++col);
			moveBenchmarks.BenchmarkFunction<&InlineFreeFunction>(h);
		}
	}

	{//heap-allocated delegates
		tableInlineHeap.title("Heap-allocated delegates, inlinable target");

		Benchmarks<std::tuple<
			DelegateOnHeapBenchmark,
			DelegateInObjectOnHeapBenchmark,
			DelegateInVectorBenchmark>> heapBenchmarks;

		row = 1;
		col = 0;
		ResultHandler h{ tableInlineHeap, row, col };
		{
			heapBenchmarks.GatherNames(h);
		}
		{
			h.RowCol(row, ++col);
			heapBenchmarks.BenchmarkFunctor(λInline, h);
		}
		{
			h.RowCol(row, ++col);
			heapBenchmarks.BenchmarkFunctor(λCapturelessInline, h);
		}
		{
			TargetObject targetObj;
			h.RowCol(row, ++col);
			heapBenchmarks.BenchmarkMethod<&TargetObject::InlineMethod>(targetObj, h);
		}
		{
			h.RowCol(row, ++col);
			heapBenchmarks.BenchmarkFunction<&InlineFreeFunction>(h);
		}
	}

	{
		tableDelegateAsParameter.title("Stack-allocated delegates passed to functions, inlinable target");

		Benchmarks<std::tuple<
			InlineDelegateDependentFunctionBenchmark,
			NoninlinedDelegateDependentFunctionBenchmark>> functionBenchmarks;

		row = 1;
		col = 0;
		ResultHandler h{ tableDelegateAsParameter, row, col };
		{
			functionBenchmarks.GatherNames(h);
		}
		{
			h.RowCol(row, ++col);
			functionBenchmarks.BenchmarkFunctor(λInline, h);
		}
		{
			h.RowCol(row, ++col);
			functionBenchmarks.BenchmarkFunctor(λCapturelessInline, h);
		}
		{
			TargetObject targetObj;
			h.RowCol(row, ++col);
			functionBenchmarks.BenchmarkMethod<&TargetObject::InlineMethod>(targetObj, h);
		}
		{
			h.RowCol(row, ++col);
			functionBenchmarks.BenchmarkFunction<&InlineFreeFunction>(h);
		}
	}

	{
		tableArgumentPassing.title("Argument passing/reference forwarding (noninlinable target)");
		tables.push_back(&tableArgumentPassing);

		tableArgumentPassing.addRow(
			"direct call",
			toString(ArgumentPassingBenchmark::BenchmarkDirectCall())
		);
		tableArgumentPassing.addRow(
			"Delegate",
			toString(ArgumentPassingBenchmark::BenchmarkDelegate())
		);
		tableArgumentPassing.addRow(
			"Event",
			toString(ArgumentPassingBenchmark::BenchmarkEvent())
		);
	}

	{
		tableEvent.title("Stack-allocated event");
		tables.push_back(&tableEvent);

		tableEvent.addRow("direct callback call, inlinable", 
						  toString(EventBenchmark::BenchmarkDirectCall<&TargetObject::InlineMethod>())
		);
		tableEvent.addRow("direct callback call, noninlinable", 
						  toString(EventBenchmark::BenchmarkDirectCall<&TargetObject::NonInlinedMethod>())
		);
		tableEvent.addRow("call array of pointers, inlinable", 
						  toString(EventBenchmark::BenchmarkArrayOfPointers<&TargetObject::InlineMethod>())
		);
		tableEvent.addRow("raised event, inlinable", 
						  toString(EventBenchmark::BenchmarkEventRaise<&TargetObject::InlineMethod>())
		);
	}

	pretty::Printer print;
	for(auto t : tables)
	{
		if(t==&tableArgumentPassing) print.headerSeparator(false);

		std::cout << "  " << t->title() << endl << print(*t);
	}

	return 0;
}
