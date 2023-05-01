#include "doctest.h"
#include "testutil.h"

#define COMPILE_INVALID_CTORS
#include "CallMe.Event.h"

using namespace CallMe;

/*
	Tests for most likely invalid use cases of API. The tests verify that:

	1. A particular invalid case resolves to a specific function or constructor.

	2. There is no ambiguity in resolving overloaded functions.

	3. The resolved function/constructor is specifically intended
		for handling the invalid use of API-under-test and reacts accordingly.
		In production, such invalid cases break compilation.
		For these tests, they compile and throw exceptions.
		 
	4. Class template argument deduction works even for invalid use cases.
*/
TEST_SUITE("API error tests")
{
	TEST_CASE("OwningDelegate/stateless functor not allowed") {
		SUBCASE("ctor"){
			CHECK_THROWS_WITH(
				auto _ = OwningDelegate(new auto([](){return 0;})),
				MsgStatelessFunctorNotAllowed);
		}
		SUBCASE("factory"){
			CHECK_THROWS_WITH(
				fromFunctorOwned(new auto([](){return 0;})),
				MsgStatelessFunctorNotAllowed);
		}
	}

	TEST_CASE("OwningDelegate/const functor* not allowed") {
		int stub = 0;
		SUBCASE("ctor") {
			const auto* pFunctor = new auto([&stub]() { return stub; });
			CHECK_THROWS_WITH(
				auto _ = OwningDelegate(pFunctor),
				MsgConstOwnedObjectsNotAllowed);
		}
		SUBCASE("factory") {
			const auto* pFunctor = new auto([&stub]() { return stub; });
			CHECK_THROWS_WITH(
				fromFunctorOwned(pFunctor),
				MsgConstOwnedObjectsNotAllowed);
		}
	}

	TEST_CASE("OwningDelegate/const object* not allowed") {
		#undef TestCtor
		#define TestCtor(Method, obj)					\
			CHECK_THROWS_WITH(							\
				OwningDelegate d(obj, tag<Method>()),	\
				MsgConstOwnedObjectsNotAllowed			\
			);

		#undef TestFactory
		#define TestFactory(Method, obj)				\
			CHECK_THROWS_WITH(							\
				fromMethodOwned<Method>(obj),			\
				MsgConstOwnedObjectsNotAllowed			\
			);

		SUBCASE("ctor") {
			SUBCASE("const object*") {
				SUBCASE("non-const method") {
					const auto* constObj = new TestObject2;
					TestCtor(&TestObject::get, constObj);
				}
				SUBCASE("const method") {
					const auto* constObj = new TestObject2;
					TestCtor(&TestObject::getConst, constObj);
				}
			}
		}
		SUBCASE("factory") {
			SUBCASE("const object*") {
				SUBCASE("non-const method") {
					const auto* constObj = new TestObject2;
					TestFactory(&TestObject::get,constObj)
				}
				SUBCASE("const method") {
					const auto* constObj = new TestObject2;
					TestFactory(&TestObject::getConst,constObj)
				}
			}
		}
	}

	TEST_CASE("Delegate/static member functions use free function constructors") {
		SUBCASE("target*"){
			TestObject target;
			SUBCASE("ctor"){
				CHECK_THROWS_WITH(
					Delegate(&target, tag<&TestObject::staticMemberFn>()),
					MsgStaticMethodNotAllowed);
			}
			SUBCASE("factory"){
				CHECK_THROWS_WITH(
					fromMethod<&TestObject::staticMemberFn>(&target),
					MsgStaticMethodNotAllowed);
			}
		}
		SUBCASE("target&"){
			TestObject target;
			SUBCASE("ctor"){
				CHECK_THROWS_WITH(
					Delegate(target, tag<&TestObject::staticMemberFn>()),
					MsgStaticMethodNotAllowed);
			}
			SUBCASE("factory"){
				CHECK_THROWS_WITH(
					fromMethod<&TestObject::staticMemberFn>(target),
					MsgStaticMethodNotAllowed);
			}
		}
		SUBCASE("const target&"){
			const TestObject target;
			SUBCASE("ctor"){
				CHECK_THROWS_WITH(
					Delegate(target, tag<&TestObject::staticMemberFn>()),
					MsgStaticMethodNotAllowed);
			}
			SUBCASE("factory"){
				CHECK_THROWS_WITH(
					fromMethod<&TestObject::staticMemberFn>(target),
					MsgStaticMethodNotAllowed);
			}
		}
	}

	TEST_CASE("OwningDelegate/static member functions use free function constructors") {
		SUBCASE("target*"){
			SUBCASE("ctor"){
				CHECK_THROWS_WITH(
					OwningDelegate(new TestObject, tag<&TestObject::staticMemberFn>()),
					MsgStaticMethodNotAllowed);
			}
			SUBCASE("factory"){
				CHECK_THROWS_WITH(
					fromMethodOwned<&TestObject::staticMemberFn>(new TestObject),
					MsgStaticMethodNotAllowed);
			}
		}
		SUBCASE("target&"){
			TestObject target;
			SUBCASE("ctor"){
				CHECK_THROWS_WITH(
					OwningDelegate(target, tag<&TestObject::staticMemberFn>()),
					MsgStaticMethodNotAllowed);
			}
			SUBCASE("factory"){
				CHECK_THROWS_WITH(
					fromMethodOwned<&TestObject::staticMemberFn>(target),
					MsgStaticMethodNotAllowed);
			}
		}
		SUBCASE("const target&"){
			const TestObject target;
			SUBCASE("ctor"){
				CHECK_THROWS_WITH(
					OwningDelegate(target, tag<&TestObject::staticMemberFn>()),
					MsgStaticMethodNotAllowed);
			}
			SUBCASE("factory"){
				CHECK_THROWS_WITH(
					fromMethodOwned<&TestObject::staticMemberFn>(target),
					MsgStaticMethodNotAllowed);
			}
		}
	}

	TEST_CASE("Delegate/r-value non-lite functors not allowed") {
		int stub = 0;
		SUBCASE("ctor"){
			CHECK_THROWS_WITH(
				Delegate([&stub](){return stub;}),
				MsgNoRValueObjects);
		}
		SUBCASE("factory"){
			CHECK_THROWS_WITH(
				fromFunctor([&stub](){return stub;}),
				MsgNoRValueObjects);
		}
	}

	TEST_CASE("Delegate/r-value lite functors allowed") {
		SUBCASE("ctor"){
			auto d = Delegate([](int i1, int i2) {return i1 + i2;});
			CHECK(d(1, 2) == 3);
		}
		SUBCASE("factory"){
			auto d = fromFunctor([](int i1, int i2) {return i1 + i2;});
			CHECK(d(1, 2) == 3);
		}
	}

	TEST_CASE("Delegate/r-value objects not allowed") {
		SUBCASE("ctor"){
			CHECK_THROWS_WITH(
				Delegate(TestObject(), tag<&TestObject::getConst>()),
				MsgNoRValueObjects);
		}
		SUBCASE("factory"){
			CHECK_THROWS_WITH(
				fromMethod<&TestObject::getConst>(TestObject()),
				MsgNoRValueObjects);
		}
	}

	TEST_CASE("Delegate/const functor with non-const operator()") {
		int stub;
		const auto functor = [&stub]() mutable { stub = -1; return 0; };

		SUBCASE("ctor"){
			SUBCASE("const functor&"){
				CHECK_THROWS_WITH(
					Delegate{functor},
					MsgOnlyConstFunctions);
			}
			SUBCASE("const functor*"){
				CHECK_THROWS_WITH(
					Delegate{&functor},
					MsgOnlyConstFunctions);
			}
		}

		SUBCASE("factory"){
			SUBCASE("const functor&"){
				CHECK_THROWS_WITH(
					fromFunctor(functor),
					MsgOnlyConstFunctions);
			}
			SUBCASE("const functor*"){
				CHECK_THROWS_WITH(
					fromFunctor(&functor),
					MsgOnlyConstFunctions);
			}
		}
	}

	TEST_CASE("Delegate/Member function is from another class") {
		TestObject2 obj;
		const TestObject2 constObj;

		#undef TestCtor
		#define TestCtor(Method, obj)			\
			CHECK_THROWS_WITH(					\
				Delegate d(obj, tag<Method>()),	\
				MsgMemberFnFromAnotherClass		\
			);

		#undef TestFactory
		#define TestFactory(Method, obj)		\
			CHECK_THROWS_WITH(					\
				fromMethod<Method>(obj),		\
				MsgMemberFnFromAnotherClass		\
			);

		SUBCASE("ctor"){
			SUBCASE("object&"){
				SUBCASE("non-const method"){
					TestCtor(&TestObject::get, obj);
				}
				SUBCASE("const method"){
					TestCtor(&TestObject::getConst, obj);
				}
			}
			SUBCASE("const object&"){
				SUBCASE("non-const method"){
					TestCtor(&TestObject::get, constObj);
				}
				SUBCASE("const method"){
					TestCtor(&TestObject::getConst, constObj);
				}
			}
			SUBCASE("object*"){
				SUBCASE("non-const method"){
					TestCtor(&TestObject::get, &obj);
				}
				SUBCASE("const method"){
					TestCtor(&TestObject::getConst, &obj);
				}
			}
			SUBCASE("const object*"){
				SUBCASE("non-const method"){
					TestCtor(&TestObject::get, &constObj);
				}
				SUBCASE("const method"){
					TestCtor(&TestObject::getConst, &constObj);
				}
			}
		}

		SUBCASE("factory"){
			SUBCASE("object&"){
				SUBCASE("non-const method"){
					TestFactory(&TestObject::get, obj)
				}
				SUBCASE("const method"){
					TestFactory(&TestObject::getConst, obj);
				}
			}
			SUBCASE("const object&"){
				SUBCASE("non-const method"){
					TestFactory(&TestObject::get,constObj)
				}	
				SUBCASE("const method"){
					TestFactory(&TestObject::getConst,constObj)
				}
			}
			SUBCASE("object*"){
				SUBCASE("non-const method"){
					TestFactory(&TestObject::get,&obj)
				}
				SUBCASE("const method"){
					TestFactory(&TestObject::getConst,&obj)
				}
			}
			SUBCASE("const object*"){
				SUBCASE("non-const method"){
					TestFactory(&TestObject::get,&constObj)
				}
				SUBCASE("const method"){
					TestFactory(&TestObject::getConst,&constObj)
				}
			}
		}
	}

	TEST_CASE("Delegate/const object with non-const member function") {
		const TestObject obj;

		#undef TestCtor
		#define TestCtor(Method, obj)					\
					CHECK_THROWS_WITH(					\
						Delegate d(obj, tag<Method>()),	\
						MsgOnlyConstFunctions			\
					);

		#undef TestFactory
		#define TestFactory(Method, obj)				\
					CHECK_THROWS_WITH(					\
						fromMethod<Method>(obj),		\
						MsgOnlyConstFunctions			\
					);

		SUBCASE("ctor"){
			SUBCASE("const object&"){
				SUBCASE("non-const method"){
					TestCtor(&TestObject::get, obj);
				}
			}
			SUBCASE("const object*"){
				SUBCASE("non-const method"){
					TestCtor(&TestObject::get, &obj);
				}
			}
		}

		SUBCASE("factory"){
			SUBCASE("const object&"){
				SUBCASE("non-const method"){
					TestFactory(&TestObject::get, obj)
				}	
			}
			SUBCASE("const object*"){
				SUBCASE("non-const method"){
					TestFactory(&TestObject::get, &obj)
				}
			}
		}
	}

	TEST_CASE("OwningDelegate/Member function is from another class") {
		#undef TestCtor
		#define TestCtor(Method, obj)					\
			CHECK_THROWS_WITH(							\
				OwningDelegate d(obj, tag<Method>()),	\
				MsgMemberFnFromAnotherClass				\
			);

		#undef TestFactory
		#define TestFactory(Method, obj)				\
			CHECK_THROWS_WITH(							\
				fromMethodOwned<Method>(obj),			\
				MsgMemberFnFromAnotherClass				\
			);

		SUBCASE("ctor"){
			SUBCASE("object*"){
				SUBCASE("non-const method"){
					TestCtor(&TestObject::get, new TestObject2);
				}
				SUBCASE("const method"){
					TestCtor(&TestObject::getConst, new TestObject2);
				}
			}
		}
		SUBCASE("factory"){
			SUBCASE("object*"){
				SUBCASE("non-const method"){
					TestFactory(&TestObject::get, new TestObject2)
				}
				SUBCASE("const method"){
					TestFactory(&TestObject::getConst, new TestObject2)
				}
			}
		}
	}

	TEST_CASE("OwningDelegate/object& not allowed") {
		TestObject obj;
		const TestObject constObj;

		#undef TestCtor
		#define TestCtor(Method, obj)					\
			CHECK_THROWS_WITH(							\
				OwningDelegate d(obj, tag<Method>()),	\
				MsgRefToObjNotAllowed					\
			);

		#undef TestFactory
		#define TestFactory(Method, obj)				\
			CHECK_THROWS_WITH(							\
				fromMethodOwned<Method>(obj),			\
				MsgRefToObjNotAllowed					\
			);

		SUBCASE("ctor"){
			SUBCASE("object&"){
				SUBCASE("non-const method"){
					TestCtor(&TestObject::get, obj);
				}
				SUBCASE("const method"){
					TestCtor(&TestObject::getConst, obj);
				}
			}
			SUBCASE("const object&"){
				SUBCASE("non-const method"){
					TestCtor(&TestObject::get, constObj);
				}
				SUBCASE("const method"){
					TestCtor(&TestObject::getConst, constObj);
				}
			}
		}
		SUBCASE("factory"){
			SUBCASE("object&"){
				SUBCASE("non-const method"){
					TestFactory(&TestObject::get, obj)
				}
				SUBCASE("const method") {
					TestFactory(&TestObject::getConst, obj)
				}
			}
			SUBCASE("const object&"){
				SUBCASE("non-const method"){
					TestFactory(&TestObject::get, constObj)
				}
				SUBCASE("const method") {
					TestFactory(&TestObject::getConst, constObj)
				}
			}
		}
	}

	TEST_CASE("OwningDelegate/functor& not allowed") {
		int stub = 0;
		auto functor = [&stub]() { return stub; };
		const auto constFunctor = [&stub]() { return stub; };

		#undef TestCtor
		#define TestCtor(fun)							\
			CHECK_THROWS_WITH(							\
				OwningDelegate d(fun),					\
				MsgRefToObjNotAllowed					\
			);

		#undef TestFactory
		#define TestFactory(fun)						\
			CHECK_THROWS_WITH(							\
				fromFunctorOwned(fun),					\
				MsgRefToObjNotAllowed					\
			);

		SUBCASE("ctor"){
			SUBCASE("functor&"){
				TestCtor(functor);
			}
			SUBCASE("const functor&"){
				TestCtor(constFunctor);
			}
		}
		SUBCASE("factory"){
			SUBCASE("functor&"){
				TestFactory(functor);
			}
			SUBCASE("const functor&"){
				TestFactory(constFunctor);
			}
		}
	}

	TEST_CASE("Event/callback signature mismatch") {

		Event<void(std::string&,int)> event;

		SUBCASE("subscribe(DelegateT&& callback)"){
			CHECK_THROWS_WITH(
				event.subscribe(fromFunctor([](std::string&) {})),
				MsgEventCallbackMismatch
			);
			CHECK_THROWS_WITH(
				event.subscribe(fromFunctor([](std::string&, int, int) {})),
				MsgEventCallbackMismatch
			);
			CHECK_THROWS_WITH(
				event.subscribe(fromFunctor([](const std::string&, int) {})),
				MsgEventCallbackMismatch
			);
			CHECK_THROWS_WITH(
				event.subscribe(fromFunctor([](std::string&, int) { return 0; })),
				MsgEventCallbackMismatch
			);
			CHECK_THROWS_WITH(
				event.subscribe(fromFunctor([](std::string, int) { return 0; })),
				MsgEventCallbackMismatch
			);
		}

		SUBCASE("subscribe(DelegateT&& callback,VectorOfSubscriptions& dst)"){
			std::vector<Subscription> s;
			CHECK_THROWS_WITH(
				event.subscribe(fromFunctor([](std::string&) {}),s),
				MsgEventCallbackMismatch
			);
			CHECK_THROWS_WITH(
				event.subscribe(fromFunctor([](std::string&, int, int) {}),s),
				MsgEventCallbackMismatch
			);
			CHECK_THROWS_WITH(
				event.subscribe(fromFunctor([](const std::string&, int) {}),s),
				MsgEventCallbackMismatch
			);
			CHECK_THROWS_WITH(
				event.subscribe(fromFunctor([](std::string&, int) { return 0; }),s),
				MsgEventCallbackMismatch
			);
			CHECK_THROWS_WITH(
				event.subscribe(fromFunctor([](std::string, int) { return 0; }),s),
				MsgEventCallbackMismatch
			);
		}
	}
}	