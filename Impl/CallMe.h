/*
* MIT License
*
* Copyright (c) 2023 Dmitry Kim <https://github.com/bitsbakery/callme>
*
* Permission is hereby  granted, free of charge, to any  person obtaining a copy
* of this software and associated  documentation files (the "Software"), to deal
* in the Software  without restriction, including without  limitation the rights
* to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell 
* copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
* furnished to do so, subject to the following conditions:
*  
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
* IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
* FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
* AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
* LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#pragma once

#ifdef COMPILE_INVALID_CTORS
#include <stdexcept>
#endif

#include <type_traits>

namespace CallMe
{
	#if defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER)
		#define GCC_COMPILER
	#endif

	#ifdef GCC_COMPILER
		#define GCC_PRAGMA_TO_STR(x) _Pragma(#x)
		#define GCC_SUPPRESS_WARNING_PUSH _Pragma("GCC diagnostic push")
		#define GCC_SUPPRESS_WARNING(w) GCC_PRAGMA_TO_STR(GCC diagnostic ignored w)
		#define GCC_SUPPRESS_WARNING_POP _Pragma("GCC diagnostic pop")
		#define GCC_SUPPRESS_WARNING_WITH_PUSH(w)\
		    GCC_SUPPRESS_WARNING_PUSH GCC_SUPPRESS_WARNING(w)
	#else
		#define GCC_SUPPRESS_WARNING_PUSH
		#define GCC_SUPPRESS_WARNING(w)
		#define GCC_SUPPRESS_WARNING_POP
		#define GCC_SUPPRESS_WARNING_WITH_PUSH(w)
	#endif

	#ifdef __clang__
		#define CLANG_PRAGMA_TO_STR(x) _Pragma(#x)
		#define CLANG_SUPPRESS_WARNING_PUSH _Pragma("clang diagnostic push")
		#define CLANG_SUPPRESS_WARNING(w) CLANG_PRAGMA_TO_STR(clang diagnostic ignored w)
		#define CLANG_SUPPRESS_WARNING_POP _Pragma("clang diagnostic pop")
		#define CLANG_SUPPRESS_WARNING_WITH_PUSH(w)\
			CLANG_SUPPRESS_WARNING_PUSH CLANG_SUPPRESS_WARNING(w)
	#else
		#define CLANG_SUPPRESS_WARNING_PUSH
		#define CLANG_SUPPRESS_WARNING(w)
		#define CLANG_SUPPRESS_WARNING_POP
		#define CLANG_SUPPRESS_WARNING_WITH_PUSH(w)
	#endif

	#ifdef _MSC_VER
		#define MSVC_SUPPRESS_WARNING_PUSH __pragma(warning(push))
		#define MSVC_SUPPRESS_WARNING(w) __pragma(warning(disable : w))
		#define MSVC_SUPPRESS_WARNING_POP __pragma(warning(pop))
		#define MSVC_SUPPRESS_WARNING_WITH_PUSH(w)\
		    MSVC_SUPPRESS_WARNING_PUSH MSVC_SUPPRESS_WARNING(w)
	#else
		#define MSVC_SUPPRESS_WARNING_PUSH
		#define MSVC_SUPPRESS_WARNING(w)
		#define MSVC_SUPPRESS_WARNING_POP
		#define MSVC_SUPPRESS_WARNING_WITH_PUSH(w)
	#endif

    namespace internal
    {
		constexpr static bool LiteFunctorsAsFunctions = false;

        /*
    		1. Utilities for template argument deduction
    		2. Factory functions and syntactic sugar for all delegates
        */

		/* Whatever is marked as "clang-tidy bug"
		gives wrong design-time warnings in CLion
		about "never being used" */
		#define FixClang [[maybe_unused]]

		//deduces signatures of member functions
        template<typename T>
        struct MemberFunctionDeducer;

        template<typename Ret, typename T, typename...Args>
        struct MemberFunctionDeducer<Ret(T::*)(Args...) const>
    	{
        	using NonmemberSignature FixClang = Ret(Args...);
            using NonmemberFnPtr FixClang = Ret(*)(Args...);
            using RetType FixClang = Ret;
            using ClassType FixClang = T;
            constexpr static bool ConstFunction = true;
        };

        template<typename Ret, typename T, typename...Args>
        struct MemberFunctionDeducer<Ret(T::*)(Args...)>
    	{
            using NonmemberSignature FixClang = Ret(Args...);
			using NonmemberFnPtr FixClang = Ret(*)(Args...);
            using RetType FixClang = Ret;
            using ClassType FixClang = T;
            constexpr static bool ConstFunction = false;
        };

		//deduces signatures of functors' operator()
        template<typename T>
        struct CallOperatorDeducer : MemberFunctionDeducer<decltype(&T::operator())>
    	{
        };

		//deduces signatures of non-member functions
		template<typename T>
		struct FunctionDeducer;

		template<typename Ret, typename...Args>
		struct FunctionDeducer<Ret(*)(Args...)>
    	{
			using Signature FixClang = Ret(Args...);
		};

#ifdef _MSC_VER
        //on 64-bit targets all specifiers of calling conventions
		//are ignored and all calls are effectively __fastcall
		using defaultcallT	= void(*)();
		using cdeclT		= void(__cdecl*)();
		using stdcallT		= void(__stdcall*)();
		using fastcallT		= void(__fastcall*)();
		using vectorcallT	= void(__vectorcall*)();

		template<typename Ret, typename...Args>
			requires (not std::is_same_v<cdeclT, defaultcallT>)
		struct FunctionDeducer<Ret(__cdecl*)(Args...)>
		{
			using Signature FixClang = Ret(Args...);
		};

		template<typename Ret, typename...Args>
			requires (not std::is_same_v<stdcallT, defaultcallT>)
		struct FunctionDeducer<Ret(__stdcall*)(Args...)>
		{
			using Signature FixClang = Ret(Args...);
		};

		template<typename Ret, typename...Args>
			requires (not std::is_same_v<fastcallT, defaultcallT>)
		struct FunctionDeducer<Ret(__fastcall*)(Args...)>
		{
			using Signature FixClang = Ret(Args...);
		};

		template<typename Ret, typename...Args>
			requires (not std::is_same_v<vectorcallT, defaultcallT>)
		struct FunctionDeducer<Ret(__vectorcall*)(Args...)>
		{
			using Signature FixClang = Ret(Args...);
		};
#endif

		template <typename Functor>
		concept HasFnOperator = requires
		{
			{&Functor::operator()};
		};

		template <typename Functor>
		concept AnyFunctor =
			std::is_object_v<Functor> and
			HasFnOperator<Functor>;

		template <typename Functor, typename R, typename...ClassArgs>
		concept HasGivenFnOperator = requires(Functor& f, ClassArgs&&...args)
		{
			{f.operator()(std::forward<ClassArgs>(args)...)} -> std::same_as<R>;
		};

	    template <typename Functor, typename R, typename...ClassArgs>
		concept FunctorSignature =
			std::is_object_v<Functor> and
			HasGivenFnOperator<Functor, R, ClassArgs...>;

		/* "lite functor" - a stateless functor with a non-virtual operator()
		that can be converted to a free function */
		template <typename Functor>
		concept LiteFunctor =
			AnyFunctor<Functor> and
			std::is_convertible_v<Functor, typename internal::CallOperatorDeducer<Functor>::NonmemberFnPtr>;

		template <typename Functor>
		concept NonLiteFunctor =
			AnyFunctor<Functor> and
			not std::is_convertible_v<Functor, typename internal::CallOperatorDeducer<Functor>::NonmemberFnPtr>;

		template <typename Method>
		concept MemberFunction =
			std::is_member_function_pointer_v<Method>;

		template <typename PFunction>
		concept FreeFunction =
			std::is_function_v<std::remove_pointer_t<PFunction>>;

		template <typename T>
		concept Mutable =
			not std::is_const_v<T>;

		template <typename T>
		concept Const =
			std::is_const_v<T>;

		template <typename Cls>
		concept Class =
			not std::is_pointer_v<Cls> and 
			std::is_object_v<Cls>;

		template <typename Cls>
		concept MutableClass =
			Class<Cls> and
			not std::is_const_v<Cls>;
	                
		template<auto Method, typename Object>
		concept MethodMatchesClass = std::is_same_v<std::decay_t<Object>,
			typename internal::MemberFunctionDeducer<decltype(Method)>::ClassType>;
	               
		template<typename Function, typename ExpectedSignature>
		concept FunctionSignature =
			std::same_as<typename internal::FunctionDeducer<Function>::Signature, ExpectedSignature>;

	}

	#define MsgOnlyConstFunctions "Only const member functions are allowed with const-receivers"
	#define MsgConstOwnedObjectsNotAllowed "Owned object cannot be const, as it needs to be destroyed"
	#define MsgRefToObjNotAllowed "Reference to object is not allowed, pointer required"
	#define MsgStatelessFunctorNotAllowed "This stateless functor is convertible to a function. There is nothing to own, use Delegate instead"
	#define MsgMemberFnFromAnotherClass "The member function is from another class[?]"
	#define MsgNoRValueObjects "r-value arguments are not supported. "\
		"Pass l-value references or pointers to an object. Make sure the object's "\
		"lifetime >= lifetime of the Delegate"
	#define MsgStaticMethodNotAllowed "For static member functions, use free function constructors/factories"

	//COMPILE_INVALID_CTORS is only for testing,
	//don't define in production
	#ifdef COMPILE_INVALID_CTORS
		#define DELETE_FUNCTION(fn, msg) fn								\
			{															\
				throw std::logic_error(msg);							\
			}
		#define DELETE_OWNING_FUNCTION(fn, paramToDelete, msg)			\
			fn															\
			{															\
				delete paramToDelete;									\
				throw std::logic_error(msg);							\
			}
		#define DELETE_OWNING_CTOR(ctor, paramToDelete, initList, msg)	\
			ctor : initList												\
			{															\
				delete paramToDelete;									\
				throw std::logic_error(msg);							\
			}
		#define	CONSTEVAL
		#define	NOEXCEPT
	#else
		#define DELETE_FUNCTION(fn, msg) [[deprecated(msg)]] fn = delete;
		#define DELETE_OWNING_FUNCTION(fn, paramToDelete, msg) [[deprecated(msg)]] fn = delete;
		#define DELETE_OWNING_CTOR(ctor, paramToDelete, initList, msg) [[deprecated(msg)]] ctor = delete;
		#define	CONSTEVAL consteval
		#define	NOEXCEPT noexcept
	#endif

	namespace internal
	{
		template<auto Method, typename Object>
		CONSTEVAL void EnforceClassAndMethodMatch()
		{
		#ifdef COMPILE_INVALID_CTORS
			if constexpr (not MethodMatchesClass<Method,Object>)
				throw std::logic_error(MsgMemberFnFromAnotherClass);
		#else
			static_assert(MethodMatchesClass<Method,Object>,
						  MsgMemberFnFromAnotherClass);
		#endif
		}
		
		template<typename MethodInfo>
		CONSTEVAL void EnforceOnlyConstFunctions()
		{
		#ifdef COMPILE_INVALID_CTORS
			if constexpr (not MethodInfo::ConstFunction)
				throw std::logic_error(MsgOnlyConstFunctions);
		#else
			static_assert(MethodInfo::ConstFunction, 
						  MsgOnlyConstFunctions);
		#endif
		}

		template<typename Function>
		consteval void EnforceFunction()
		{
			static_assert(FreeFunction<Function>,
						  "Incorrect signature: expected a pointer to a nonmember function");
		}

		using PErasedObject = void*;
		
		template<typename R, typename...ClassArgs>
		using PErasedInvoker = R(*)(PErasedObject object, ClassArgs...);

		//implements ErasedInvoker
		template<typename R, typename...ClassArgs>
		R NullInvoke(PErasedObject, ClassArgs...)
		{
			return R();
		}

		template<auto Function, typename R, typename...ClassArgs>
			requires FunctionSignature<decltype(Function), R(ClassArgs...)>
		struct FunctionInvoker
		{
			//implements ErasedInvoker
			static R invoke(PErasedObject, ClassArgs...args)
			{
				return Function(std::forward<ClassArgs>(args)...);
			}
		};

		template<typename Function, typename R, typename...ClassArgs>
			requires FunctionSignature<Function, R(ClassArgs...)>
		struct DynamicFunctionInvoker
		{
			//implements ErasedInvoker
			static R invoke(PErasedObject function, ClassArgs...args)
			{
				return reinterpret_cast<Function>(function)
					(std::forward<ClassArgs>(args)...);
			}
		};

		template<typename Functor, typename R, typename...ClassArgs>
			requires FunctorSignature<Functor, R, ClassArgs...>
		struct FunctorInvoker 
		{
			//implements ErasedInvoker
			static R invoke(PErasedObject functor, ClassArgs...args)
			{
				return reinterpret_cast<Functor*>(functor)->operator()(
					std::forward<ClassArgs>(args)...);
			}

			static PErasedObject erase(const Functor* functor)
			{
				return const_cast<Functor*>(functor);
			}

			static PErasedObject erase(const Functor& functor)
			{
				return const_cast<Functor*>(&functor);
			}
		};

		//specialization for stateless functors with non-virtual
		//operator() convertible to a free function
		template<LiteFunctor Functor, typename R, typename...ClassArgs>
			requires FunctorSignature<Functor,R,ClassArgs...> && LiteFunctorsAsFunctions
		struct FunctorInvoker<Functor, R, ClassArgs...> 
		{
			//implements ErasedInvoker
			static R invoke(PErasedObject, ClassArgs...args)
			{
				using fn = R(*)(ClassArgs...);
				constexpr fn operatorFunction = fn(std::declval<Functor>());

				//invoke as function
				return operatorFunction(std::forward<ClassArgs>(args)...);
			}

			static PErasedObject erase(const Functor*)
			{
				return nullptr;
			}

			static PErasedObject erase(const Functor&)
			{
				return nullptr;
			}
		};

		template<auto Method, typename Object, typename R, typename...ClassArgs>
			requires MethodMatchesClass<Method, Object>
		struct MethodInvoker
		{
			//implements ErasedInvoker
			static R invoke(PErasedObject object, ClassArgs...args)
			{
				return (reinterpret_cast<Object*>(object)->*Method)(
					std::forward<ClassArgs>(args)...);
			}
		};

		template<typename R, typename...ClassArgs>
		class ErasedDelegate
		{
		protected:
			PErasedInvoker<R, ClassArgs...> _invoker;
			PErasedObject _object;

		public:
			R invoke(ClassArgs...args)
			{
				return (*_invoker)(_object, std::forward<ClassArgs>(args)...);
			}

			R operator()(ClassArgs...args)
			{
				return invoke(std::forward<ClassArgs>(args)...);
			}

			PErasedObject object()
			{
				return _object;
			}

			ErasedDelegate(const ErasedDelegate& other) = default;

			ErasedDelegate& operator=(const ErasedDelegate& other) = default;

	        constexpr ErasedDelegate() noexcept:
	            _invoker(NullInvoke<R,ClassArgs...>),
	            _object(nullptr) 
	        {}

	        FixClang
	        explicit ErasedDelegate(PErasedInvoker<R, ClassArgs...> invoker,
									PErasedObject object) noexcept :
				_invoker(invoker),
				_object(object)
			{}

			ErasedDelegate(ErasedDelegate&& other) noexcept :
				_invoker(other._invoker),
				_object(other._object)
			{
				other._object = nullptr;
			}

			ErasedDelegate& operator=(ErasedDelegate&& other) noexcept
			{
				if (this == &other)
					return *this;

				_invoker = other._invoker;
				_object = other._object;

				other._object = nullptr;

				return *this;
			}
		};
	}

	//facilitates deduction of constructor template parameters 
	//https://stackoverflow.com/a/16944262
	template<auto T>
	struct tag
	{
		tag() = default;
	};

	template<typename Signature>
	class Delegate;

	template<typename R, typename...ClassArgs>
	class Delegate<R(ClassArgs...)> :
		public internal::ErasedDelegate<R, ClassArgs...>
	{
		using Erased = internal::ErasedDelegate<R, ClassArgs...>;

		template<typename Functor>
		using FunctorInvokerT = internal::FunctorInvoker<Functor, R, ClassArgs...>;

		template<typename Object, auto Method>
		using MethodInvokerT = internal::MethodInvoker<Method,Object,R,ClassArgs...>;

	public:
		template<internal::FreeFunction auto Function>
			requires internal::FunctionSignature<decltype(Function),R(ClassArgs...)>
		explicit Delegate(tag<Function>) noexcept : 
			Erased(internal::FunctionInvoker<Function,R,ClassArgs...>::invoke,
				   nullptr)
		{
			assert(Function!=nullptr);
		}
		
		template<internal::FreeFunction DynamicFunction>
			requires std::is_pointer_v<DynamicFunction> and
					 internal::FunctionSignature<DynamicFunction, R(ClassArgs...)>
		explicit Delegate(DynamicFunction function) noexcept : 
			Erased(internal::DynamicFunctionInvoker<DynamicFunction,R,ClassArgs...>::invoke,
                   reinterpret_cast<internal::PErasedObject>(function))
		{
			assert(function!=nullptr);
		}

		template<typename Functor>
			requires internal::FunctorSignature<Functor, R, ClassArgs...>
		explicit Delegate(Functor* functor) noexcept :
			Erased(FunctorInvokerT<Functor>::invoke,
				   FunctorInvokerT<Functor>::erase(functor))
		{
			assert(functor!=nullptr);
		}

		template<typename Functor>
			requires internal::FunctorSignature<Functor, R, ClassArgs...>
		explicit Delegate(const Functor* functor) NOEXCEPT :
		Erased(FunctorInvokerT<Functor>::invoke,
			   FunctorInvokerT<Functor>::erase(functor))
		{
			assert(functor!=nullptr);

			using methodInfo = internal::CallOperatorDeducer<Functor>;
			internal::EnforceOnlyConstFunctions<methodInfo>();
		}

		template<typename Functor>
			requires internal::FunctorSignature<Functor, R, ClassArgs...>
		explicit Delegate(Functor& functor) noexcept :
			Erased(FunctorInvokerT<Functor>::invoke,
				   FunctorInvokerT<Functor>::erase(functor))
		{
		}

		template<typename Functor>
			requires internal::FunctorSignature<Functor, R, ClassArgs...>
		explicit Delegate(const Functor& functor) NOEXCEPT :
		Erased(FunctorInvokerT<Functor>::invoke,
			   FunctorInvokerT<Functor>::erase(functor))
		{
			using methodInfo = internal::CallOperatorDeducer<Functor>;
			internal::EnforceOnlyConstFunctions<methodInfo>();
		}

		template<internal::NonLiteFunctor Functor,
			typename = std::enable_if_t<not std::is_same_v<Functor, Delegate>>>
			//requires (not std::is_same_v<Delegate,Functor>)//clang-tidy is not happy with this
		DELETE_FUNCTION(Delegate(Functor&&),
						MsgNoRValueObjects)

		template<internal::MemberFunction auto Method, internal::Class Object,
			typename = std::enable_if_t<not std::is_same_v<Object, Delegate>>>
		DELETE_FUNCTION(Delegate(Object&&, tag<Method>),
						MsgNoRValueObjects)

		template<internal::Class Object,
			//MemberFunction //MSVC++ bug
			auto Method>
			requires internal::MethodMatchesClass<Method, Object> and internal::MemberFunction<decltype(Method)>
		explicit Delegate(Object* object, tag<Method>) noexcept :
			Erased(MethodInvokerT<Object,Method>::invoke,
				   (internal::PErasedObject)object)
		{
			assert(object!=nullptr);
		}

		template<internal::Class Object, R(Object::*Method)(ClassArgs...)>
		DELETE_FUNCTION(Delegate(const Object*, tag<Method>), 
						MsgOnlyConstFunctions)

		template<internal::Class Object, R(Object::*Method)(ClassArgs...)>
		DELETE_FUNCTION(Delegate(const Object&, tag<Method>),
						MsgOnlyConstFunctions)

		// this handles both Object* and Object&
		template<typename Object, internal::FreeFunction auto StaticMethod>
		DELETE_FUNCTION(Delegate(Object, tag<StaticMethod>),
						MsgStaticMethodNotAllowed)

		template<internal::Class Object,
				//MemberFunction //MSVC++ bug
				auto Method>
			requires internal::MethodMatchesClass<Method, Object> and
					 internal::MemberFunction<decltype(Method)>
		explicit Delegate(Object& object, tag<Method>) noexcept :
			Erased(MethodInvokerT<Object,Method>::invoke,
				   (internal::PErasedObject)&object)
		{
		}

		// both Object* and Object& cases
		template<typename Object,
				// internal::MemberFunction MSVC bug
				auto Method>
			requires internal::MemberFunction<decltype(Method)> and
					 (not internal::MethodMatchesClass<Method,Object>)
		DELETE_FUNCTION(Delegate(Object, tag<Method>),
						MsgMemberFnFromAnotherClass)

        explicit Delegate() noexcept :
			Erased()
        {
        }

        Delegate(Delegate&& other) noexcept :
			Erased(std::move(other))
		{
		}

		Delegate& operator=(Delegate&& other) noexcept
		{
			if (this == &other)
				return *this;
			Erased::operator=(std::move(other));
			return *this;
		}

		Delegate(const Delegate& other)            = default;
		Delegate& operator=(const Delegate& other) = default;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// deduction guides
	//
	// Deduction guides are intentionally overpermissive,
	// they allow many wrong use cases of the API
	//
	template<internal::Class Functor, typename = std::enable_if_t<internal::AnyFunctor<Functor>>>
	//requires AnyFunctor<Functor> //This does not work in Clang,VC++
	explicit Delegate(Functor&) ->
		Delegate<typename internal::CallOperatorDeducer<Functor>::NonmemberSignature>;

	template<internal::Class Functor, typename = std::enable_if_t<internal::AnyFunctor<Functor>>>
	explicit Delegate(Functor&&) ->
		Delegate<typename internal::CallOperatorDeducer<Functor>::NonmemberSignature>;

    template<internal::Class Functor, typename = std::enable_if_t<internal::AnyFunctor<Functor>>>
	explicit Delegate(Functor*) ->
		Delegate<typename internal::CallOperatorDeducer<Functor>::NonmemberSignature>;

	template<internal::FreeFunction auto Function>
	explicit Delegate(tag<Function>) ->
		Delegate<typename internal::FunctionDeducer<decltype(Function)>::Signature>;

	template<internal::FreeFunction DynamicFunction>
	explicit Delegate(DynamicFunction) ->
		Delegate<typename internal::FunctionDeducer<DynamicFunction>::Signature>;

	template<internal::FreeFunction auto StaticMethod, internal::Class Object>
	explicit Delegate(Object*, tag<StaticMethod>) ->
		Delegate<typename internal::FunctionDeducer<decltype(StaticMethod)>::Signature>;

	template<internal::FreeFunction auto StaticMethod, internal::Class Object>
	explicit Delegate(Object&, tag<StaticMethod>) ->
		Delegate<typename internal::FunctionDeducer<decltype(StaticMethod)>::Signature>;

	template<internal::MemberFunction auto Method, internal::Class Object>
	explicit Delegate(const Object*, tag<Method>) ->
		Delegate<typename internal::MemberFunctionDeducer<decltype(Method)>::NonmemberSignature>;
	
	template<internal::MemberFunction auto Method, internal::Class Object>
	explicit Delegate(const Object&, tag<Method>) ->
		Delegate<typename internal::MemberFunctionDeducer<decltype(Method)>::NonmemberSignature>;
	//
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////

	namespace internal
	{
		/* Invalid constructors of OwningDelegate delete passed
		pointers before throwing, but don't delete passed references
		or values */
		template<typename Object>
		Object* ownedPtrToDelete(Object* ptr)
		{
			return ptr;
		}

		template<typename Object>
		Object* ownedPtrToDelete(Object&)
		{
			return nullptr;
		}
	}

	template<typename Signature>
	class OwningDelegate;

	template<typename R, typename...ClassArgs>
	class OwningDelegate<R(ClassArgs...)> :
		public internal::ErasedDelegate<R, ClassArgs...>
	{
		using Erased = internal::ErasedDelegate<R, ClassArgs...>;

		using ErasedDeleter = void(*)(internal::PErasedObject object);

		template<typename Functor>
		using FunctorInvokerT = internal::FunctorInvoker<Functor, R, ClassArgs...>;

		template<typename Object, auto Method>
		using MethodInvokerT = internal::MethodInvoker<Method, Object, R, ClassArgs...>;

		ErasedDeleter _delete;

		template<typename Object>
		static void ErasedDeleterImpl(internal::PErasedObject object)
		{
			delete reinterpret_cast<Object*>(object);
		}
		
		static void NullErasedDeleterImpl(internal::PErasedObject)
		{
		}

	public:
        //Functor*
		template<internal::NonLiteFunctor Functor>
			requires internal::FunctorSignature<Functor, R, ClassArgs...>
		explicit OwningDelegate(Functor* functor) noexcept :
			Erased(FunctorInvokerT<Functor>::invoke,
				   FunctorInvokerT<Functor>::erase(functor)),
			_delete(&ErasedDeleterImpl<Functor>)
		{
			assert(functor!=nullptr);
		}

		//Functor* convertible to function
		template<internal::LiteFunctor Functor>
		DELETE_OWNING_CTOR(OwningDelegate(Functor* f),
							f,
							_delete(NullErasedDeleterImpl),
							MsgStatelessFunctorNotAllowed)

		//const Functor*
		template<internal::NonLiteFunctor Functor>
		DELETE_OWNING_CTOR(OwningDelegate(const Functor* f),
						   f,
						   _delete(NullErasedDeleterImpl),
						   MsgConstOwnedObjectsNotAllowed)

		//Functor&
		template<internal::AnyFunctor Functor>
		DELETE_OWNING_CTOR(OwningDelegate(Functor&),
						   static_cast<Functor*>(nullptr),
						   _delete(NullErasedDeleterImpl),
						   MsgRefToObjNotAllowed)

		//Object&, both const and non-const member functions
		template<internal::MemberFunction auto Method, internal::Class Object>
		DELETE_OWNING_CTOR(OwningDelegate(Object&, tag<Method>),
						   static_cast<Object*>(nullptr),
						   _delete(NullErasedDeleterImpl),
						   MsgRefToObjNotAllowed)

		#ifdef COMPILE_INVALID_CTORS
			MSVC_SUPPRESS_WARNING_WITH_PUSH(4702)//C4702 unreachable code
		#endif
		// this handles Object* and Object& (both const and non-const)
		template<internal::FreeFunction auto StaticMethod, typename Object>
		DELETE_OWNING_CTOR(OwningDelegate(Object obj, tag<StaticMethod>),
						   internal::ownedPtrToDelete(obj),
						   _delete(NullErasedDeleterImpl),
						   MsgStaticMethodNotAllowed)
		#ifdef COMPILE_INVALID_CTORS
			MSVC_SUPPRESS_WARNING_POP
		#endif

		//const Object*, both const and non-const member functions
		template<internal::MemberFunction auto Method, internal::Class Object>
		DELETE_OWNING_CTOR(OwningDelegate(const Object* object, tag<Method>),
							object,
							_delete(NullErasedDeleterImpl),
							MsgConstOwnedObjectsNotAllowed)

		//Object*, both const and non-const member functions
		template<internal::MemberFunction auto Method, internal::MutableClass Object>
			requires (not internal::MethodMatchesClass<Method,Object>)
		DELETE_OWNING_CTOR(OwningDelegate(Object* object, tag<Method>),
							object,
							_delete(NullErasedDeleterImpl),
							MsgMemberFnFromAnotherClass)

		//Object*, both const and non-const member functions
		template<internal::MemberFunction auto Method, internal::MutableClass Object>
		explicit OwningDelegate(Object* object, tag<Method>) noexcept :
			Erased(MethodInvokerT<Object,Method>::invoke,
				   object),
			_delete(&ErasedDeleterImpl<Object>)
		{
			assert(object!=nullptr);
		}

		explicit OwningDelegate() noexcept :
			Erased(),
			_delete(NullErasedDeleterImpl)
		{
		}

		~OwningDelegate()
		{
			_delete(this->_object);
		}

		OwningDelegate(OwningDelegate&& other) noexcept
			: Erased(std::move(other)),//nulls out other._object
			_delete(other._delete)
		{
		}

		OwningDelegate& operator=(OwningDelegate&& other) noexcept
		{
			if (this == &other)
				return *this;

        	//order dependency
			{
				_delete(this->_object);
				_delete = other._delete;

				Erased::operator=(std::move(other));
			}

			return *this;
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////
    // deduction guides
	//
	template<internal::AnyFunctor Functor>
	explicit OwningDelegate(Functor* functor) ->
		OwningDelegate<typename internal::CallOperatorDeducer<Functor>::NonmemberSignature>;

	template<internal::AnyFunctor Functor, typename = std::enable_if_t<not std::is_pointer_v<Functor>>>
	explicit OwningDelegate(Functor& functor) ->
		OwningDelegate<typename internal::CallOperatorDeducer<Functor>::NonmemberSignature>;

	template<internal::MemberFunction auto Method, internal::Class Object>
	explicit OwningDelegate(Object&, tag<Method>) ->
		OwningDelegate<typename internal::MemberFunctionDeducer<decltype(Method)>::NonmemberSignature>;

	template<internal::MemberFunction auto Method, internal::Class Object>
	explicit OwningDelegate(Object*, tag<Method>) ->
		OwningDelegate<typename internal::MemberFunctionDeducer<decltype(Method)>::NonmemberSignature>;

	template<internal::FreeFunction auto StaticMethod, internal::Class Object>
	explicit OwningDelegate(Object*, tag<StaticMethod>) ->
		OwningDelegate<typename internal::FunctionDeducer<decltype(StaticMethod)>::Signature>;

	template<internal::FreeFunction auto StaticMethod, internal::Class Object>
	explicit OwningDelegate(Object&, tag<StaticMethod>) ->
		OwningDelegate<typename internal::FunctionDeducer<decltype(StaticMethod)>::Signature>;
	//
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////

    inline namespace factory
    {
		template<auto Function>
    	auto fromFunction()
		{
			internal::EnforceFunction<decltype(Function)>();
			return Delegate(tag<Function>());
		}

		template<typename DynamicFunction>
		auto fromFunction(DynamicFunction function)
		{
			internal::EnforceFunction<DynamicFunction>();
			return Delegate(function);
		}

		template<internal::NonLiteFunctor Functor>
		DELETE_FUNCTION(auto fromFunctor(Functor&&), 
						MsgNoRValueObjects);

		template<internal::AnyFunctor Functor>
		auto fromFunctor(Functor& functor)
		{
			return Delegate(functor);
		}

		template<internal::AnyFunctor Functor>
		auto fromFunctor(const Functor& functor)
		{
			return Delegate(functor);
		}

		template<internal::AnyFunctor Functor>
		auto fromFunctor(Functor* functor)
		{
			return Delegate(functor);
		}

        template<internal::AnyFunctor Functor>
        auto fromFunctor(const Functor* functor)
        {
			return Delegate(functor);
        }

        namespace factoryInternal 
        {
			#ifdef COMPILE_INVALID_CTORS
				MSVC_SUPPRESS_WARNING_WITH_PUSH(4702)//C4702 unreachable code
			#endif
			template<internal::MemberFunction auto Method, internal::Class Object>
			auto fromMethod(Object* object)
			{
				internal::EnforceClassAndMethodMatch<Method,Object>();
				return Delegate(object, tag<Method>());
			}
			#ifdef COMPILE_INVALID_CTORS
				MSVC_SUPPRESS_WARNING_POP
			#endif
		}

		template<internal::MemberFunction auto Method, internal::Class Object>
		auto fromMethod(Object* object)
		{
			return factoryInternal::fromMethod<Method>(object);
		}

        template<internal::MemberFunction auto Method, internal::Class Object>
        auto fromMethod(const Object* object)
        {
			return factoryInternal::fromMethod<Method>(object);
        }

		template<internal::MemberFunction auto Method, internal::Class Object>
		auto fromMethod(Object& object)
		{
			return factoryInternal::fromMethod<Method>(&object);
		}

    	template<internal::MemberFunction auto Method, internal::Class Object>
		auto fromMethod(const Object& object)
		{
			return factoryInternal::fromMethod<Method>(&object);
		}

		// this handles both Object* and Object& cases
		template<internal::FreeFunction auto StaticMethod, typename Object>
		DELETE_FUNCTION(auto fromMethod(Object),
						MsgStaticMethodNotAllowed);

		template<internal::MemberFunction auto Method, internal::Class Object>
		DELETE_FUNCTION(auto fromMethod(Object&&),
						MsgNoRValueObjects)

		template<internal::AnyFunctor Functor>
		DELETE_OWNING_FUNCTION(auto fromFunctorOwned(const Functor* functor),
								functor,
							    MsgConstOwnedObjectsNotAllowed);

		template<internal::AnyFunctor Functor>
		DELETE_OWNING_FUNCTION(auto fromFunctorOwned(Functor&),
							   static_cast<Functor*>(nullptr),
							   MsgRefToObjNotAllowed);

		template<internal::AnyFunctor Functor>
		auto fromFunctorOwned(Functor* functor)
		{
			return OwningDelegate(functor);
		}

		template<internal::LiteFunctor Functor>
		DELETE_OWNING_FUNCTION(auto fromFunctorOwned(Functor*f),
								f,
								MsgStatelessFunctorNotAllowed)

		template<internal::MemberFunction auto Method, internal::Class Object>
		DELETE_OWNING_FUNCTION(auto fromMethodOwned(const Object*o),
								o,
							    MsgConstOwnedObjectsNotAllowed)

    	template<internal::MemberFunction auto Method, internal::Class Object>
		DELETE_OWNING_FUNCTION(auto fromMethodOwned(Object&),
							   static_cast<Object*>(nullptr),
							   MsgRefToObjNotAllowed)

		// this handles both Object* and Object& cases
		template<internal::FreeFunction auto StaticMethod, typename Object>
		DELETE_OWNING_FUNCTION(auto fromMethodOwned(Object object),
							   internal::ownedPtrToDelete(object),
							   MsgStaticMethodNotAllowed);

		#ifdef COMPILE_INVALID_CTORS
			MSVC_SUPPRESS_WARNING_WITH_PUSH(4702)//C4702 unreachable code
		#endif
    	template<internal::MemberFunction auto Method, internal::Class Object>
		auto fromMethodOwned(Object* object)
		{
			internal::EnforceClassAndMethodMatch<Method,Object>();
			return OwningDelegate(object, tag<Method>());
		}
		#ifdef COMPILE_INVALID_CTORS
			MSVC_SUPPRESS_WARNING_POP
		#endif
	}
}