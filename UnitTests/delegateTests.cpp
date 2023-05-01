// ReSharper disable CppEntityUsedOnlyInUnevaluatedContext

#include "doctest.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#include <cassert>
#include <vector>

#include "CallMe.h"
#include "testutil.h"

#ifdef _MSC_VER
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif

using size_t = std::string::size_type;

struct Arg
{
	size_t a[8] = {1};
};

#ifdef _MSC_VER
	#undef CDECL
    #define CDECL __cdecl
    #define STDCALL __stdcall
    #define FASTCALL __fastcall
#else
    #define CDECL
    #define STDCALL
    #define FASTCALL
#endif

NOINLINE size_t CDECL fn_cdecl(Arg a, size_t* i, const std::string& s)
{
	*i = 1;
	return *i + a.a[0] + s.length();
}

NOINLINE size_t STDCALL fn_stdcall(Arg a, size_t* i, const std::string& s)
{
	*i = 2;
	return *i + a.a[0] + s.length();
}

NOINLINE size_t FASTCALL fn_fastcall(Arg a, size_t* i, const std::string& s)
{
	*i = 3;
	return *i + a.a[0] + s.length();
}

using namespace CallMe;

TEST_SUITE("delegate tests")
{
    void TestDelegateInvocation(auto& delegate)
	{
		CHECK(Counter::Val() == 0);
        CHECK(delegate.invoke(1) == 1);
		CHECK(Counter::Val() == 1);
	}

	TEST_CASE("OwningDelegate/functor"){
		Counter::reset();

		SUBCASE("ctor"){
            SUBCASE("non-const operator()") {
            	OwningDelegate<int(int)> delegate(new auto([c = Counter()](int i) mutable {
					c.Val(i);
					return i;
				}));
				int stub = 0;
                OwningDelegate deduced(new auto([&stub](int i) mutable {
                	return i + stub;
                }));
                static_assert(std::is_same_v<decltype(delegate),decltype(deduced)>);
                TestDelegateInvocation(delegate);
            }
            SUBCASE("const operator()") {
                Counter c;
                OwningDelegate<int(int)> delegate(new auto([&c](int i)  {
                    c.Val(i);
                    return i;
                }));
                OwningDelegate deduced(new auto([&c](int i)  {
                    c.Val(i);
                    return i;
                }));
                static_assert(std::is_same_v<decltype(delegate),decltype(deduced)>);
                TestDelegateInvocation(delegate);
            }
		}
		SUBCASE("factory"){
            SUBCASE("non-const operator()"){
                auto delegate = fromFunctorOwned(new auto([c = Counter()](int i) mutable {
                    c.Val(i);
                    return i;
                }));
                TestDelegateInvocation(delegate);
            }
            SUBCASE("const operator()") {
                Counter c;
                auto delegate = fromFunctorOwned(new auto([&c](int i)  {
                    c.Val(i);
                    return i;
                }));
                TestDelegateInvocation(delegate);
            }
		}

		//copy elision
		{
			CHECK(Counter::copies == 0);
			CHECK(Counter::moves == 0);
			CHECK(Counter::dtors == 1);
		}
	}

	TEST_CASE("OwningDelegate outlives target")
	{
		Counter::reset();

		SUBCASE("lambda")
		{
			OwningDelegate<int(int)> delegate;
			{
				auto lambda = new auto([c = Counter()](int i) mutable
				{
					c.Val(i);
                    return i;
				});
				OwningDelegate<int(int)> delegateTmp(lambda);
				delegate = std::move(delegateTmp);
			}//lambda's block exit, lambda survives 

			//still can invoke lambda
			{
				delegate.invoke(1);
				CHECK(Counter::Val() == 1);
			}
		}
		SUBCASE("object")
		{
			OwningDelegate<int(int)> delegate;
			{
				OwningDelegate<int(int)> delegateTmp(new TestObject, tag<&TestObject::set>());
				delegate = std::move(delegateTmp);
			}

			//still can invoke method
			{
				delegate.invoke(1);
				CHECK(Counter::Val() == 1);
			}
		}
	}

	TEST_CASE("Delegate outlives lambda block")
	{
		Counter::reset();
		Delegate<void(int)> delegate;
		{
			auto lambdaTmp = [c = Counter()](int) mutable
			{
				c.Val(1);
			};
			Delegate<void(int)> delegateTmp(lambdaTmp);
			delegate = std::move(delegateTmp);
		}//lambda's block exit, lambda destroyed

        //cannot invoke dead lambda
		CHECK_THROWS(delegate.invoke(0));
	}

	TEST_CASE("OwningDelegate/move")
	{
		Counter::reset();
		{
			OwningDelegate<void(int)> delegate(
				new auto([c = Counter()](int) mutable
				{
					c.Val(0);
				}));
			OwningDelegate<void(int)> delegate2(
				new auto([c = Counter()](int) mutable
				{
					c.Val(2);
				}));

			delegate = std::move(delegate2);
			delegate.invoke(2);
			CHECK(Counter::Val() == 2);
		}

		CHECK(Counter::copies == 0);
		CHECK(Counter::moves == 0);
		CHECK(Counter::ctors == 2);
		CHECK(Counter::dtors == 2);
	}

	TEST_CASE("Delegate/copy")
	{
		int i = -1;
		auto lambda1 = [&i](int i1, int i2)
		{
			return ++i + i1 + i2;
		};
		Delegate delegate(lambda1);
		CHECK(delegate(1,2)==3);
		CHECK(delegate(1,2)==4);

		Delegate delegate2(delegate);
		CHECK(delegate2(1,2)==5);		
		
		Delegate delegate3 = delegate2;
		CHECK(delegate3(1,2)==6);	
	}

	TEST_CASE("Delegate/move")
	{
		Counter::reset();
		{
			auto lambda1 = [c = Counter()](int) mutable
			{
				c.Val(1);
			};
			Delegate<void(int)> delegate(lambda1);
			delegate.invoke(0);

			auto lambda2 = [c = Counter()](int i) mutable
			{
				c.Val(i);
			};
			Delegate<void(int)> delegate2(lambda2);
			delegate2.invoke(0);

			delegate = std::move(delegate2);
			delegate.invoke(2);
			CHECK(Counter::Val() == 2);
		}

		CHECK(Counter::copies == 0);
		CHECK(Counter::moves == 0);
		CHECK(Counter::ctors == 2);
		CHECK(Counter::dtors == 2);
	}

	TEST_CASE("OwningDelegate/vector")
	{
		Counter::reset();
		{
			std::vector<OwningDelegate<void(int)>> v;
			v.emplace_back(new auto([c = Counter()](int) mutable
			{
				c.Val(1);
			}));
			v.emplace_back(new auto([c = Counter()](int) mutable
			{
				c.Val(2);
			}));
			v[0].invoke(0);
			CHECK(Counter::Val() == 1);

			v[1].invoke(0);
			CHECK(Counter::Val() == 2);
		}

		CHECK(Counter::copies == 0);
		CHECK(Counter::moves == 0);
		CHECK(Counter::ctors == 2);
		CHECK(Counter::dtors == 2);
	}

	TEST_CASE("Delegate/vector")
	{
		Counter::reset();
		{
			std::vector<Delegate<void(int)>> v;
			auto lambda = [c = Counter()](int) mutable
			{
				c.Val(1);
			};
			v.emplace_back(lambda);

			auto lambda2 = [c = Counter()](int) mutable
			{
				c.Val(2);
			};
			v.emplace_back(lambda2);
			v[0].invoke(0);
			CHECK(Counter::Val() == 1);

			v[1].invoke(0);
			CHECK(Counter::Val() == 2);
		}

		CHECK(Counter::copies == 0);
		CHECK(Counter::moves == 0);
		CHECK(Counter::ctors == 2);
		CHECK(Counter::dtors == 2);
	}

	TEST_CASE("Delegate/functor"){
		Counter::reset();

		SUBCASE("lambda*"){
			auto lambda = [c = Counter()](int i) mutable{
				c.Val(1);
                return i;
			};
			SUBCASE("ctor"){
				Delegate<int(int)> delegate(&lambda);
				Delegate deduced(&lambda);
                static_assert(std::is_same_v<decltype(delegate),decltype(deduced)>);
				TestDelegateInvocation(delegate);
			}
			SUBCASE("factory"){
				auto delegate = fromFunctor(&lambda);
				TestDelegateInvocation(delegate);
			}
		}
        SUBCASE("const lambda*"){
            Counter c;
            const auto lambda = [&c](int i){
                c.Val(1);
                return i;
            };
            SUBCASE("ctor"){
                Delegate<int(int i)> delegate(&lambda);
                Delegate deduced(&lambda);
                static_assert(std::is_same_v<decltype(delegate),decltype(deduced)>);
                TestDelegateInvocation(delegate);
            }
            SUBCASE("factory"){
                auto delegate = fromFunctor(&lambda);
                TestDelegateInvocation(delegate);
            }
        }
		SUBCASE("lambda&"){
			auto lambda = [c = Counter()](int i) mutable{
				c.Val(1);
                return i;
			};
			SUBCASE("ctor"){
				Delegate<int(int i)> delegate(lambda);
                Delegate deduced(lambda);
                static_assert(std::is_same_v<decltype(delegate),decltype(deduced)>);
				TestDelegateInvocation(delegate);
			}
			SUBCASE("factory"){
				auto delegate = fromFunctor(lambda);
				TestDelegateInvocation(delegate);
			}
		}
		SUBCASE("const lambda&"){
            Counter c;
			const auto lambda = [&c](int i){
				c.Val(1);
                return i;
			};

			SUBCASE("ctor"){
				Delegate<int(int i)> delegate(lambda);
                Delegate deduced(lambda);
                static_assert(std::is_same_v<decltype(delegate),decltype(deduced)>);
				TestDelegateInvocation(delegate);
			}
			SUBCASE("factory"){
				auto delegate = fromFunctor(lambda);
				TestDelegateInvocation(delegate);
			}
		}

		//copy elision
		{
			CHECK(Counter::copies == 0);
			CHECK(Counter::moves == 0);
			CHECK(Counter::dtors == 1);
		}
	}

	TEST_CASE("Delegate/captureless lambda"){

		auto lambda = [](int i, const std::string& s) -> std::size_t
		{
			return i + s.length();
		};

		auto testDelegate = [](auto& delegate)
		{
			CHECK(delegate(1, std::string("123")) == 4);
			if constexpr(internal::LiteFunctorsAsFunctions)
				CHECK(delegate.object() == nullptr);
		};

		SUBCASE("lambda*"){
			SUBCASE("ctor"){
				Delegate<std::size_t(int i, const std::string& s)> delegate(&lambda);
				Delegate deduced(&lambda);
				static_assert(std::is_same_v<decltype(delegate),decltype(deduced)>);

				testDelegate(delegate);
				testDelegate(deduced);
			}
			SUBCASE("factory"){
				auto delegate = fromFunctor(&lambda);
				testDelegate(delegate);
			}
		}
		SUBCASE("const lambda*"){
			const auto constLambda = lambda;
			SUBCASE("ctor"){
				Delegate<std::size_t(int i, const std::string& s)> delegate(&constLambda);
				Delegate deduced(&constLambda);
				static_assert(std::is_same_v<decltype(delegate),decltype(deduced)>);

				testDelegate(delegate);
				testDelegate(deduced);
			}
			SUBCASE("factory"){
				auto delegate = fromFunctor(&constLambda);
				testDelegate(delegate);
			}
		}
		SUBCASE("lambda&"){
			SUBCASE("ctor"){
				Delegate<std::size_t(int i, const std::string& s)> delegate(lambda);
				Delegate deduced(lambda);
				static_assert(std::is_same_v<decltype(delegate),decltype(deduced)>);

				testDelegate(delegate);
				testDelegate(deduced);
			}
			SUBCASE("factory"){
				auto delegate = fromFunctor(lambda);
				testDelegate(delegate);
			}
		}
		SUBCASE("const lambda&"){
			const auto constLambda = lambda;

			SUBCASE("ctor"){
				Delegate<std::size_t(int i, const std::string& s)> delegate(constLambda);
				Delegate deduced(constLambda);
				static_assert(std::is_same_v<decltype(delegate),decltype(deduced)>);

				testDelegate(delegate);
				testDelegate(deduced);
			}
			SUBCASE("factory"){
				auto delegate = fromFunctor(constLambda);
				testDelegate(delegate);
			}
		}
	}

	TEST_CASE("Delegate/member function"){
		Counter::reset();
		
		SUBCASE("target*"){
			TestObject target;
			SUBCASE("ctor"){
				SUBCASE("non-const method") {
					Delegate<int(int i)> delegate(&target, tag<&TestObject::set>());
					Delegate deduced(&target, tag<&TestObject::set>());
					static_assert(std::is_same_v<decltype(delegate), decltype(deduced)>);
					TestDelegateInvocation(delegate);
				}
				SUBCASE("const method") {
					Delegate<int()> delegate(&target, tag<&TestObject::getConst>());
					Delegate deduced(&target, tag<&TestObject::getConst>());
					static_assert(std::is_same_v<decltype(delegate),decltype(deduced)>);
					CHECK(delegate() == 0);
				}
			}
			SUBCASE("factory"){
				SUBCASE("non-const method") {
					auto delegate = fromMethod<&TestObject::set>(&target);
					TestDelegateInvocation(delegate);
				}
				SUBCASE("const method") {
					auto delegate = fromMethod<&TestObject::getConst>(&target);
					CHECK(delegate() == 0);
				}
			}
		}
        SUBCASE("const target*"){
            const TestObject target;
            SUBCASE("ctor"){
				SUBCASE("const method") {
					Delegate<int()> delegate(&target, tag<&TestObject::getConst>());
					Delegate deduced(&target, tag<&TestObject::getConst>());
					static_assert(std::is_same_v<decltype(delegate), decltype(deduced)>);
					CHECK(delegate() == 0);
				}
            }
            SUBCASE("factory"){
				SUBCASE("const method") {
					auto delegate = fromMethod<&TestObject::getConst>(&target);
					CHECK(delegate() == 0);
				}
            }
        }
		SUBCASE("target&"){
			TestObject target;
			SUBCASE("ctor"){
				SUBCASE("non-const method") {
					Delegate<int(int i)> delegate(target, tag<&TestObject::set>());
					Delegate deduced(target, tag<&TestObject::set>());
					static_assert(std::is_same_v<decltype(delegate), decltype(deduced)>);
					TestDelegateInvocation(delegate);
				}
				SUBCASE("const method") {
					Delegate<int()> delegate(target, tag<&TestObject::getConst>());
					Delegate deduced(target, tag<&TestObject::getConst>());
					static_assert(std::is_same_v<decltype(delegate),decltype(deduced)>);
					CHECK(delegate() == 0);
				}
			}
			SUBCASE("factory"){
				SUBCASE("non-const method") {
					auto delegate = fromMethod<&TestObject::set>(&target);
					TestDelegateInvocation(delegate);
				}
				SUBCASE("const method") {
					auto delegate = fromMethod<&TestObject::getConst>(target);
					CHECK(delegate() == 0);
				}
			}
		}
		SUBCASE("const target&"){
			const TestObject target;
			SUBCASE("ctor"){
				SUBCASE("const method") {
					Delegate<int()> delegate(target, tag<&TestObject::getConst>());
					Delegate deduced(target, tag<&TestObject::getConst>());
					static_assert(std::is_same_v<decltype(delegate),decltype(deduced)>);
					CHECK(delegate() == 0);
				}
			}
			SUBCASE("factory"){
				SUBCASE("const method") {
					auto delegate = fromMethod<&TestObject::getConst>(target);
					CHECK(delegate() == 0);
				}
			}
		}
	}

	TEST_CASE("Delegate/virtual member function") {
		SUBCASE("target*") {
			TestDerived derived;
			TestBase* ptr = &derived;
			SUBCASE("ctor") { 
				Delegate<std::string()> delegate(ptr, tag<&TestBase::fn>());
				Delegate deduced(ptr, tag<&TestBase::fn>());
				static_assert(std::is_same_v<decltype(delegate), decltype(deduced)>);
				CHECK(delegate() == "derived");
			}
			SUBCASE("factory") {
				auto delegate = fromMethod<&TestBase::fn>(ptr);
				CHECK(delegate() == "derived");
			}
		}
	}

	TEST_CASE("Delegate/function") {
		SUBCASE("free function"){
			SUBCASE("ctor"){
				Delegate<std::size_t(int i, const std::string& s)> delegate{tag<&freeFunction>()};
				Delegate deduced{tag<&freeFunction>()};
				static_assert(std::is_same_v<decltype(delegate),decltype(deduced)>);
				CHECK(delegate(1, std::string("123")) == 4);
			}
			SUBCASE("factory"){
				auto delegate = fromFunction<&freeFunction>();
				CHECK(delegate(1, std::string("123")) == 4);
			}
		}
		SUBCASE("static member function"){
			SUBCASE("ctor"){
				Delegate<std::size_t(int i, const std::string& s)> delegate{tag<&TestObject::staticMemberFn>()};
				Delegate deduced{tag<&TestObject::staticMemberFn>()};
				static_assert(std::is_same_v<decltype(delegate),decltype(deduced)>);
				CHECK(delegate(1, std::string("123")) == 4);
			}
			SUBCASE("factory"){
				auto delegate = fromFunction<&TestObject::staticMemberFn>();
				CHECK(delegate(1, std::string("123")) == 4);
			}
		}
	}

	//Calling conventions should be tested on x86.
	//On ARM and x64 they all work identically
#ifdef _WIN32
	TEST_CASE("Delegate/Win API functions")
	{
		SUBCASE("ctor"){
			Delegate<void(DWORD err)> set{tag<SetLastError>()};
			Delegate<DWORD()> get{tag<GetLastError>()};
			
			set(5);
			CHECK(get()==5);
			set(0);
		}
		SUBCASE("factory"){
			auto set = fromFunction<SetLastError>();
			auto get = fromFunction<GetLastError>();

			set(6);
			CHECK(get()==6);
			set(0);
		}
		SUBCASE("dynamically loaded function"){

			HMODULE hKernel = GetModuleHandleA("kernel32");
			REQUIRE(hKernel); 

			auto    pSet    = (void (WINAPI *)(DWORD err))GetProcAddress(hKernel, "SetLastError");
			auto	pGet    = (DWORD(WINAPI *)())GetProcAddress(hKernel, "GetLastError");
			REQUIRE(pSet); 
			REQUIRE(pGet); 

			SUBCASE("ctor"){
				//checks Delegate's type deduction as well
				Delegate set(pSet);
				Delegate get(pGet);

				set(7);
				CHECK(get()==7);
				set(0);
			}
			SUBCASE("factory"){
				auto set = fromFunction(pSet);
				auto get = fromFunction(pGet);

				set(8);
				CHECK(get()==8);
				set(0);
			}
		}
	}
#endif

	//Calling conventions should be tested on x86.
	//On ARM and x64 they all work identically
	TEST_CASE("Delegate/calling conventions")
	{
		using DelegateT = Delegate<size_t(Arg a, size_t* i, const std::string& s)>;

		auto TestDelegate = [](DelegateT& delegate)
		{
			std::string s("***");
			Arg a;
			size_t i = 0;
            auto res = delegate(a,&i,s);
            CHECK(res == i+a.a[0]+s.length());
		};
		
		SUBCASE("ctor/function as template parameter"){
			SUBCASE("cdecl"){
				Delegate delegate{tag<fn_cdecl>()};
				TestDelegate(delegate);
			}
			SUBCASE("stdcall"){
				Delegate delegate{tag<fn_stdcall>()};
				TestDelegate(delegate);
			}
			SUBCASE("fastcall"){
				Delegate delegate{tag<fn_fastcall>()};
				TestDelegate(delegate);
			}
		}
		SUBCASE("factory/function as template parameter"){
			SUBCASE("cdecl"){
				Delegate delegate = fromFunction<fn_cdecl>();
				TestDelegate(delegate);
			}
			SUBCASE("stdcall"){
				Delegate delegate = fromFunction<fn_stdcall>();
				TestDelegate(delegate);
			}
			SUBCASE("fastcall"){
				Delegate delegate = fromFunction<fn_fastcall>();
				TestDelegate(delegate);
			}
		}
		SUBCASE("ctor/function as ctor parameter"){
			SUBCASE("cdecl"){
				Delegate delegate(fn_cdecl);
				TestDelegate(delegate);
			}
			SUBCASE("stdcall"){
				Delegate delegate(fn_stdcall);
				TestDelegate(delegate);
			}
			SUBCASE("fastcall"){
				Delegate delegate(fn_fastcall);
				TestDelegate(delegate);
			}
		}
		SUBCASE("factory/function as factory parameter"){
			SUBCASE("cdecl"){
				Delegate delegate = fromFunction(fn_cdecl);
				TestDelegate(delegate);
			}
			SUBCASE("stdcall"){
				Delegate delegate = fromFunction(fn_stdcall);
				TestDelegate(delegate);
			}
			SUBCASE("fastcall"){
				Delegate delegate = fromFunction(fn_fastcall);
				TestDelegate(delegate);
			}
		}
		SUBCASE("compare asm"){
			Delegate delegate1(fn_cdecl);
			Delegate delegate2(fn_stdcall);
			Delegate delegate3(fn_fastcall);
			std::string s1("***");
			std::string s2("***");
			std::string s3("***");
			Arg a1;
			Arg a2;
			Arg a3;
			size_t i1;
			size_t i2;
			size_t i3;
			delegate1(a1,&i1,s1);
			delegate2(a2,&i2,s2);
			delegate3(a3,&i3,s3);
		}
	}

	TEST_CASE("Delegate/default ctor")
	{
		Counter::reset();

		Delegate<int(int)> delegate;

		//can invoke default-constructed delegate
		delegate.invoke(1);

		auto lambdaTmp = [c = Counter()](int i) mutable { c.Val(1); return i; };

		delegate = fromFunctor(lambdaTmp);

		TestDelegateInvocation(delegate);
	}

	TEST_CASE("OwningDelegate/default ctor")
	{
		Counter::reset();

		OwningDelegate<int(int)> delegate;

		//can invoke default-constructed delegate
		delegate.invoke(1);

		delegate = fromFunctorOwned(
			new auto ([c = Counter()](int i) mutable { c.Val(1); return i; })
		);

		TestDelegateInvocation(delegate);
	}

	TEST_CASE("OwningDelegate/member function"){
		Counter::reset();
		SUBCASE("target*") {
			SUBCASE("ctor") {
                OwningDelegate<int(int i)>  set(new TestObject, tag<&TestObject::set>());
                CHECK(set(-1) == -1);
                SUBCASE("non-const member function") {
                    OwningDelegate<int()> get(new TestObject, tag<&TestObject::get>());
                    OwningDelegate deduced(new TestObject, tag<&TestObject::get>());
                    static_assert(std::is_same_v<decltype(get),decltype(deduced)>);
                    CHECK(get() == -1);
                }
                SUBCASE("const member function") {
                    OwningDelegate<int()> get(new TestObject, tag<&TestObject::getConst>());
                    OwningDelegate deduced(new TestObject, tag<&TestObject::getConst>());
                    static_assert(std::is_same_v<decltype(get),decltype(deduced)>);
                    CHECK(get() == -1);
                }
			}
			SUBCASE("factory"){
				auto set = fromMethodOwned<&TestObject::set>(new TestObject);
                CHECK(set(-1) == -1);
                SUBCASE("non-const member function") {
                    auto get = fromMethodOwned<&TestObject::get>(new TestObject);
                    CHECK(get() == -1);
                }
                SUBCASE("const member function") {
                    auto get = fromMethodOwned<&TestObject::getConst>(new TestObject);
                    CHECK(get() == -1);
                }
			}
		}
	}
}
