/*****************************************************************************
* \file delegate.hpp
*
* \brief This header contains the definition of a light-weight utility for
*        type-erasing bound functions at compile-time.
*****************************************************************************/

/*
Copyright (c) 2017, 2018, 2020-2021 Matthew Rodusek
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
https://www.boost.org/LICENSE_1_0.txt)
*/
#ifndef DELEGATE_DELEGATE_HPP
#define DELEGATE_DELEGATE_HPP

#if defined(_MSC_VER)
# pragma once
#endif // defined(_MSC_VER)

#include <type_traits>  // std::enable_if, std::is_constructible, etc
#include <functional>   // std::invoke
#include <new>          // placement new, std::launder
#include <cassert>      // assert
#include <utility>      // std::forward
#include <cstring>      // std::memcmp
#include <stdexcept>    // std::runtime_error

#if defined(__clang__) || defined(__GNUC__)
# define DELEGATE_INLINE_VISIBILITY __attribute__((visibility("hidden"), always_inline))
#elif defined(_MSC_VER)
# define DELEGATE_INLINE_VISIBILITY __forceinline
#else
# define DELEGATE_INLINE_VISIBILITY
#endif

#if defined(DELEGATE_NAMESPACE)
# define DELEGATE_NAMESPACE_INTERNAL DELEGATE_NAMESPACE
#else
# define DELEGATE_NAMESPACE_INTERNAL cpp
#endif
#define DELEGATE_NS_IMPL DELEGATE_NAMESPACE_INTERNAL::bitwizeshift

#if defined(NDEBUG)
#  define DELEGATE_ASSERT(x) ((void)0)
#else
#  define DELEGATE_ASSERT(x) (x ? ((void)0) : [&]{ assert(x); }())
#endif

#if defined(_MSC_VER)
# pragma warning(push)
// MSVC complains about a [[noreturn]] function returning non-void, but this
// is needed to satisfy the stub interface
# pragma warning(disable:4646)
// MSVC warns that `alignas` will cause padding. Like alignment does.
# pragma warning(disable:4324)
#endif

namespace DELEGATE_NAMESPACE_INTERNAL {
    inline namespace bitwizeshift {
        namespace detail {
            template <typename T, typename = void>
            struct is_equality_comparable : std::false_type{};

            template <typename T>
            struct is_equality_comparable<T,std::void_t<decltype(std::declval<const T&>()==std::declval<const T&>())>>
                : std::true_type{};

            //----------------------------------------------------------------------------

            template <typename Type>
            struct effective_signature_impl;

            //----------------------------------------------------------------------------

            template <typename R, typename...Args>
            struct effective_signature_impl<R(*)(Args...)> {
                using type = R(Args...);
            };

            template <typename R, typename...Args>
            struct effective_signature_impl<R(*)(Args...,...)> {
                using type = R(Args...,...);
            };

            template <typename R, typename...Args>
            struct effective_signature_impl<R(*)(Args...) noexcept> {
                using type = R(Args...);
            };

            template <typename R, typename...Args>
            struct effective_signature_impl<R(*)(Args...,...) noexcept> {
                using type = R(Args...,...);
            };

            //----------------------------------------------------------------------------

            template <typename R, typename C, typename...Args>
            struct effective_signature_impl<R(C::*)(Args...)> {
                using type = R(Args...);
            };

            template <typename R, typename C, typename...Args>
            struct effective_signature_impl<R(C::*)(Args...,...)> {
                using type = R(Args...,...);
            };

            template <typename R, typename C, typename...Args>
            struct effective_signature_impl<R(C::*)(Args...) noexcept> {
                using type = R(Args...);
            };

            template <typename R, typename C, typename...Args>
            struct effective_signature_impl<R(C::*)(Args...,...) noexcept> {
                using type = R(Args...,...);
            };

            //----------------------------------------------------------------------------

            template <typename R, typename C, typename...Args>
            struct effective_signature_impl<R(C::*)(Args...) const> {
                using type = R(Args...);
            };

            template <typename R, typename C, typename...Args>
            struct effective_signature_impl<R(C::*)(Args...,...) const> {
                using type = R(Args...,...);
            };

            template <typename R, typename C, typename...Args>
            struct effective_signature_impl<R(C::*)(Args...) const noexcept> {
                using type = R(Args...);
            };

            template <typename R, typename C, typename...Args>
            struct effective_signature_impl<R(C::*)(Args...,...) const noexcept> {
                using type = R(Args...,...);
            };

            //----------------------------------------------------------------------------

            template <typename Type>
            using effective_signature = typename effective_signature_impl<Type>::type;

        } // namespace detail

          //==============================================================================
          // class : bad_delegate_call
          //==============================================================================

          ////////////////////////////////////////////////////////////////////////////////
          /// \brief An exception thrown when a `delegate` is invoked without a bound
          ///        function
          ////////////////////////////////////////////////////////////////////////////////
        class bad_delegate_call : public std::runtime_error
        {
        public:
            bad_delegate_call();
        };

        //==============================================================================
        // Binding
        //==============================================================================

        inline namespace targets {

            template <auto Function>
            struct function_bind_target{};

            template <auto MemberFunction, typename T>
            struct member_bind_target{ T* instance; };

            template <typename Signature>
            struct opaque_function_bind_target;

            template <typename R, typename...Args>
            struct opaque_function_bind_target<R(Args...)>{ R(*target)(Args...); };

            template <typename Callable>
            struct callable_ref_bind_target{ Callable* target; };

            template <typename Callable>
            struct empty_callable_bind_target{};

            template <typename Callable>
            struct callable_bind_target{ Callable target; };

        } // inline namespace targets

          /// \brief Binds a function pointer to create a function bind target
          ///
          /// \tparam Function the funtion to bind
          /// \return the created target
        template <auto Function>
        constexpr auto bind() noexcept -> function_bind_target<Function>;

        /// \brief Binds a member pointer to create a function bind target
        ///
        /// \pre `p != nullptr`
        ///
        /// \tparam MemberFunction the member functon
        /// \param p the instance pointer
        /// \return the created target
        template <auto MemberFunction, typename T>
        constexpr auto bind(T* p) noexcept -> member_bind_target<MemberFunction, T>;

        /// \brief Binds a pointer to a callable function as a bind target
        ///
        /// \pre `fn != nullptr`
        ///
        /// \param fn the callable object to bind
        /// \return the created target
        template <typename Callable>
        constexpr auto bind(Callable* fn) noexcept -> callable_ref_bind_target<Callable>;

        /// \brief Binds an opaque function pointer to create a function bind target
        ///
        /// \param fn the opaque function to pointer to bind
        /// \return the created target
        template <typename R, typename...Args>
        constexpr auto bind(R(*fn)(Args...)) noexcept -> opaque_function_bind_target<R(Args...)>;

        /// \brief Binds an empty, default-constructible `Callable` object as a bind
        ///        target
        ///
        /// \tparam Callable the target to bind
        /// \return the created target
        template <typename Callable>
        constexpr auto bind() noexcept -> empty_callable_bind_target<Callable>;

        /// \brief Binds an empty, default-constructible `Callable` object as a bind
        ///        target
        ///
        /// \param callable an instance of the callable to bind
        /// \return the created target
        template <typename Callable,
            typename = std::enable_if_t<(
                std::is_empty_v<Callable> &&
                std::is_default_constructible_v<Callable>
                )>>
            constexpr auto bind(Callable callable) noexcept -> empty_callable_bind_target<Callable>;

        /// \brief Binds a trivially-copyable callable object as a bind target
        ///
        /// \param callable an instance of the callable to bind
        /// \return the created target
        template <typename Callable,
            typename = std::enable_if_t<(
                !std::is_empty_v<std::decay_t<Callable>> &&
                std::is_trivially_copyable_v<std::decay_t<Callable>> &&
                std::is_trivially_destructible_v<std::decay_t<Callable>>
                )>>
            constexpr auto bind(Callable&& callable) noexcept -> callable_bind_target<std::decay_t<Callable>>;

        //==============================================================================
        // class : delegate
        //==============================================================================

        template <typename Signature>
        class delegate;

        ////////////////////////////////////////////////////////////////////////////////
        /// \brief A lightweight 0-overhead fromper for binding function objects.
        ///
        /// `delegate` objects have more guaranteed performance characteristics over the
        /// likes of `std::function`. Most functions will be statically bound using
        /// C++17's auto template parameters, which provides more hints ot the compiler
        /// while also ensuring that function pointers will optimize almost directly
        /// into a singular function call without indirection.
        ///
        /// This type is small -- weighing in at only 2 function pointers in size -- and
        /// trivial to copy. This allows for `delegate` objects to easily be passed
        /// into functions by registers. Additionally this ensures that any stored
        /// callable objects are always small and immediately close to the data.
        ///
        /// \warning
        /// Due to its small size, most stored callable objects for a `delegate` will
        /// simply *view* the lifetime of the captured object. As a result, care needs
        /// to be taken to ensure that any captured lifetime does no expire while it is
        /// in use by a delegate. Any operations that view lifetime can easily be seen
        /// as they only operate on pointers.
        ///
        /// See the `bind` series of free functions for more details.
        ///
        /// ### Examples
        ///
        /// ```cpp
        /// delegate d = bind<&std::strlen>();
        ///
        /// assert(d("hello world") == 11);
        /// ```
        ///
        /// \tparam R the return type
        /// \tparam Args the arguments to the function
        ////////////////////////////////////////////////////////////////////////////////
        template <typename R, typename...Args>
        class delegate<R(Args...)>
        {
            // Make the storage size hold at least a function pointer or a void pointer,
            // whichever is larger.
            static constexpr auto storage_size = (
                (sizeof(void*) < sizeof(void(*)()))
                ? sizeof(void(*)())
                : sizeof(void*)
                );

            static constexpr auto storage_align = (
                alignof(void*)
                );

            template <typename U>
            using fits_storage = std::bool_constant<(
                (sizeof(U) <= storage_size) &&
                (alignof(U) <= storage_align)
                )>;

            //----------------------------------------------------------------------------
            // Constructors / Assignment
            //----------------------------------------------------------------------------
        public:

            /// \brief Default constructs this delegate without a bound target
            constexpr delegate() noexcept;

            /// \brief Constructs this delegate bound to the specified `Function`
            ///
            /// \note
            /// The `Function` argument needs to be some callable object type, such as a
            /// function pointer, or a C++20 constexpr functor.
            ///
            /// ### Examples
            ///
            /// Basic Use:
            ///
            /// ```cpp
            /// delegate<int(const char*)> d1 = bind<&std::strlen>();
            /// ```
            ///
            /// \param target the target to bind
            template <auto Function,
                typename = std::enable_if_t<(
                    std::is_invocable_r_v<R,decltype(Function),Args...>
                    )>>
                constexpr delegate(function_bind_target<Function> target) noexcept;

            /// \{
            /// \brief Makes a delegate object that binds the `MemberFunction` in the
            ///        target
            ///
            /// \note
            /// Technically the `MemberFunction` needs to only be some callable object
            /// that accepts the target instance as the first argument. This could be a
            /// regular function pointer, a member function, or some C++20 constexpr
            /// functor
            ///
            /// \pre `target.target != nullptr`. You cannot bind a null instance to a
            ///      member function.
            ///
            /// \warning
            /// A `delegate` constructed this way only *views* the lifetime of
            /// \p instance; it does not capture or contribute to its lifetime. Care must
            /// be taken to ensure that the constructed `delegate` does not outlive the
            /// bound lifetime -- or at least is not called after outliving it, otherwise
            /// this will be undefined behavior.
            ///
            /// ### Examples
            ///
            /// Basic Use:
            ///
            /// ```cpp
            /// std::string s;
            /// delegate<std::size_t()> d = bind<&std::string::size>(&s);
            /// ```
            ///
            /// \param target the target to bind
            template <auto MemberFunction, typename T,
                typename = std::enable_if_t<(
                    std::is_invocable_r_v<R,decltype(MemberFunction),T&,Args...>
                    )>>
                constexpr delegate(member_bind_target<MemberFunction,T> target) noexcept;
            template <auto MemberFunction, typename T,
                typename = std::enable_if_t<(
                    std::is_invocable_r_v<R,decltype(MemberFunction),const T&,Args...>
                    )>>
                constexpr delegate(member_bind_target<MemberFunction,const T> target) noexcept;
            /// \}

            /// \{
            /// \brief Constructs this delegate by binding an instance of some callable
            ///        function
            ///
            /// \note
            /// The bound function may be any invocable object, such as a functor or a
            /// lambda.
            ///
            /// \pre `target.target != nullptr`. You cannot bind a null function
            ///
            /// \warning
            /// A `delegate` constructed this way only *views* the lifetime of
            /// the callable object; it does not capture or contribute to its lifetime.
            /// Care must be taken to ensure that the constructed `delegate` does not
            /// outlive the bound lifetime -- or at least is not called after outliving
            /// it, otherwise this will be undefined behavior.
            ///
            /// ### Examples
            ///
            /// Basic Use:
            ///
            /// ```cpp
            /// auto object = std::hash<int>{};
            /// delegate<std::size_t(int)> d = bind(&object);
            /// ```
            ///
            /// \param target the target to bind
            template <typename Fn,
                typename = std::enable_if_t<(
                    std::is_invocable_r_v<R,Fn,Args...> &&
                    !std::is_function_v<Fn>
                    )>>
                constexpr delegate(callable_ref_bind_target<Fn> target) noexcept;
            template <typename Fn,
                typename = std::enable_if_t<(
                    std::is_invocable_r_v<R,const Fn,Args...> &&
                    !std::is_function_v<Fn>
                    )>>
                constexpr delegate(callable_ref_bind_target<const Fn> target) noexcept;
            /// \}

            /// \brief Constructs this delegate from some empty, default-constructible
            ///        callable object
            ///
            /// \note
            /// Unlike the lifetime-viewing `make` functions, the acllable object is not
            /// referenced during capture; it is ephemerally constructed each time it is
            /// invoked.
            ///
            /// ### Examples
            ///
            /// Basic Use:
            ///
            /// ```cpp
            /// delegate<std::size_t(int)> d = bind<std::hash<int>>();
            /// ```
            ///
            /// Automatic empty detection:
            ///
            /// ```cpp
            /// delegate<std::size_t(int)> d = bind(std::hash<int>{});
            /// ```
            ///
            /// \param target the target to bind
            template <typename Fn,
                typename = std::enable_if_t<(
                    std::is_empty_v<Fn> &&
                    std::is_default_constructible_v<Fn> &&
                    std::is_invocable_r_v<R,Fn,Args...>
                    )>>
                constexpr delegate(empty_callable_bind_target<Fn> target) noexcept;

            /// \brief Constructs a delegate object by binding a small-sized trivially
            ///        copyable callable object
            ///
            /// \note
            /// This overload stores the small-sized callable object internally, allowing
            /// for the lifetime to effectively be captured.
            ///
            /// \param target the target to bind
            template <typename Fn,
                typename = std::enable_if_t<(
                    !std::is_empty_v<std::decay_t<Fn>> &&
                    !std::is_function_v<std::remove_pointer_t<Fn>> &&
                    fits_storage<std::decay_t<Fn>>::value &&
                    std::is_constructible_v<std::decay_t<Fn>,Fn> &&
                    std::is_trivially_destructible_v<std::decay_t<Fn>> &&
                    std::is_trivially_copyable_v<std::decay_t<Fn>> &&
                    std::is_invocable_r_v<R,const Fn&,Args...>
                    )>>
                delegate(callable_bind_target<Fn> target) noexcept;

            /// \brief Constructs this delegate by binding an opaque function pointer
            ///
            /// \note
            /// This overload allows for binding opaque function pointers, such as the
            /// result of a `::dlsym` call.
            ///
            /// \pre `target.target != nullptr`
            ///
            /// \param target the target to bind
            template <typename UR, typename...UArgs,
                typename = std::enable_if_t<(
                    std::is_invocable_r_v<R,UR(*)(UArgs...),Args...>
                    )>>
                delegate(opaque_function_bind_target<UR(UArgs...)> target) noexcept;

            /// \brief Trivially copies the contents of \p other
            ///
            /// \param other the other delegate to copy
            delegate(const delegate& other) = default;

            //----------------------------------------------------------------------------

            /// \brief Trivially assigns the contents from \p other
            ///
            /// \param other the other delegate to copy
            /// \return reference to `(*this)`
            auto operator=(const delegate& other) -> delegate& = default;

            //----------------------------------------------------------------------------
            // Binding
            //----------------------------------------------------------------------------
        public:

            /// \brief Binds the specified \p Function to this `delegate`
            ///
            /// \note
            /// The \p Function argument needs to be some callable object type, such as a
            /// function pointer, or a C++20 constexpr functor.
            ///
            /// \tparam Function the function argument
            /// \return reference to `(*this)`
            template <auto Function,
                typename = std::enable_if_t<(
                    std::is_invocable_r_v<R,decltype(Function),Args...>
                    )>>
                constexpr auto bind() noexcept -> delegate&;

            /// \{
            /// \brief Binds the \p MemberFunction with the specified \p instance
            ///
            /// \note
            /// Technically the \p MemberFunction needs to only be some callable object
            /// that accepts \p instance as the first argument. This could be a regular
            /// function pointer, a member function, or some C++20 constexpr functor
            ///
            /// \pre `instance != nullptr`. You cannot bind a null instance to a member
            ///      function.
            ///
            /// \warning
            /// A `delegate` constructed this way only *views* the lifetime of
            /// \p instance; it does not capture or contribute to its lifetime. Care must
            /// be taken to ensure that the constructed `delegate` does not outlive the
            /// bound lifetime -- or at least is not called after outliving it, otherwise
            /// this will be undefined behavior.
            ///
            /// \tparam MemberFunction The MemberFunction pointer to invoke
            /// \param instance an instance pointer
            /// \return reference to `(*this)`
            template <auto MemberFunction, typename T,
                typename = std::enable_if_t<(
                    std::is_invocable_r_v<R,decltype(MemberFunction),const T&,Args...>
                    )>>
                constexpr auto bind(const T* instance) noexcept -> delegate&;
            template <auto MemberFunction, typename T,
                typename = std::enable_if_t<(
                    std::is_invocable_r_v<R,decltype(MemberFunction),T&,Args...>
                    )>>
                constexpr auto bind(T* instance) noexcept -> delegate&;
            /// \}

            /// \{
            /// \brief Binds an instance of some callable function \p fn
            ///
            /// \note
            /// \p fn may be any invocable object, such as a functor or a lambda.
            ///
            /// \pre `fn != nullptr`. You cannot bind a null function
            ///
            /// \warning
            /// A `delegate` constructed this way only *views* the lifetime of
            /// \p fn; it does not capture or contribute to its lifetime. Care must
            /// be taken to ensure that the constructed `delegate` does not outlive the
            /// bound lifetime -- or at least is not called after outliving it, otherwise
            /// this will be undefined behavior.
            ///
            /// \param fn A pointer to the callable object to view
            /// \return reference to `(*this)`
            template <typename Fn,
                typename = std::enable_if_t<(
                    std::is_invocable_r_v<R,Fn,Args...> &&
                    !std::is_function_v<Fn>
                    )>>
                constexpr auto bind(Fn* fn) noexcept -> delegate&;
            template <typename Fn,
                typename = std::enable_if_t<(
                    std::is_invocable_r_v<R,Fn,Args...> &&
                    !std::is_function_v<Fn>
                    )>>
                constexpr auto bind(const Fn* fn) noexcept -> delegate&;
            /// \}

            /// \brief Constructs a delegate object from some empty, default-constructible
            ///        callable object
            ///
            /// \note
            /// This overload only works with the type itself, such as an empty functor or
            /// `std::hash`-like object
            ///
            /// Unlike the lifetime-viewing `make` functions,the `Fn` is not referenced
            /// during capture
            ///
            /// \tparam Fn the function type to ephemerally instantiate on invocations
            /// \return the constructed delegate
            template <typename Fn,
                typename = std::enable_if_t<(
                    std::is_empty_v<Fn> &&
                    std::is_default_constructible_v<Fn> &&
                    std::is_invocable_r_v<R,Fn,Args...>
                    )>>
                constexpr auto bind() noexcept -> delegate&;

            /// \brief Binds a default-constructible callable object
            ///
            /// \note
            /// This overload allows for C++20 non-capturing lambdas to be easily captured
            /// without requiring any extra storage overhead. This also allows for any
            /// functor without storage space, such as `std::hash`, to be ephemerally
            /// created during invocation rather than stored
            ///
            /// Unlike the lifetime-viewing `make` functions, \p fn is passed by-value
            /// here.
            ///
            /// \param fn the function to store
            /// \return reference to `(*this)`
            template <typename Fn,
                typename = std::enable_if_t<(
                    std::is_empty_v<Fn> &&
                    std::is_default_constructible_v<Fn> &&
                    std::is_invocable_r_v<R,Fn,Args...>
                    )>>
                constexpr auto bind(Fn fn) noexcept -> delegate&;

            /// \brief Binds a small-sized trivially copyable callable object
            ///
            /// \note
            /// This overload stores the small-sized callable object internally, allowing
            /// for the lifetime to effectively be captured.
            ///
            /// \param fn the function to store
            /// \return reference to `(*this)`
            template <typename Fn,
                typename = std::enable_if_t<(
                    !std::is_empty_v<std::decay_t<Fn>> &&
                    !std::is_function_v<std::remove_pointer_t<Fn>> &&
                    fits_storage<std::decay_t<Fn>>::value &&
                    std::is_constructible_v<std::decay_t<Fn>,Fn> &&
                    std::is_trivially_destructible_v<std::decay_t<Fn>> &&
                    std::is_trivially_copyable_v<std::decay_t<Fn>> &&
                    std::is_invocable_r_v<R,const Fn&,Args...>
                    )>>
                constexpr auto bind(Fn&& fn) noexcept -> delegate&;

            /// \brief Binds a non-member function pointer to this `delegate`
            ///
            /// \note
            /// This overload allows for binding opaque function pointers, such as the
            /// result of a `::dlsym` call.
            ///
            /// \pre `fn != nullptr`
            ///
            /// \param fn the function to store
            /// \return reference to `(*this)`
            template <typename R2, typename...Args2,
                typename = std::enable_if_t<(
                    std::is_invocable_r_v<R,R2(*)(Args...),Args2...>
                    )>>
                constexpr auto bind(R2(*fn)(Args2...)) noexcept -> delegate&;

            //----------------------------------------------------------------------------

            /// \brief Resets this delegate so that it is no longer bounds
            ///
            /// \post `(*this).has_target()` will return `false`
            constexpr auto reset() noexcept -> void;

            //----------------------------------------------------------------------------
            // Observers
            //----------------------------------------------------------------------------
        public:

            /// \brief Contextually convertible to `true` if this `delegate` is already
            ///        bound
            ///
            /// This is equivalent to call `has_target()`
            constexpr explicit operator bool() const noexcept;

#if defined(__clang__)
# pragma clang diagnostic push
            // clang incorrectly flags '\return' when 'R = void' as being an error, but
            // there is no way to conditionally document a return type
# pragma clang diagnostic ignored "-Wdocumentation"
#endif

            /// \brief Invokes the underlying bound function
            ///
            /// \param args the arguments to forward to the function
            /// \return the result of the bound function
            template <typename...UArgs,
                typename = std::enable_if_t<std::is_invocable_v<R(*)(Args...),UArgs...>>>
            constexpr auto operator()(UArgs&&...args) const -> R;

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

            //----------------------------------------------------------------------------

            /// \brief Queries whether this `delegate` has any function bound to it
            ///
            /// \return `true` if a delegate is bound
            [[nodiscard]]
            constexpr auto has_target() const noexcept -> bool;

            /// \brief Queries whether this `delegate` has a `Function` object bound to it
            ///
            /// \tparam Function the function to query
            /// \return `true` if the delegate is bound with the specified `Function`
            template <auto Function,
                typename = std::enable_if_t<std::is_invocable_r_v<R,decltype(Function),Args...>>>
            [[nodiscard]]
            constexpr auto has_target() const noexcept -> bool;

            /// \brief Queries whether this `delegate` has a `MemberFunction` object bound
            ///        to it with the specified \p instance
            ///
            /// \tparam MemberFunction the function to query
            /// \param instance the instance to query aganst
            /// \return `true` if the delegate is bound with the specified `MemberFunction`
            template <auto MemberFunction, typename T,
                typename = std::enable_if_t<std::is_invocable_r_v<R,decltype(MemberFunction),const T&,Args...>>>
            [[nodiscard]]
            constexpr auto has_target(const T* instance) const noexcept -> bool;

            /// \brief Queries whether this `delegate` has a `MemberFunction` object bound
            ///        to it with the specified \p instance
            ///
            /// \tparam MemberFunction the function to query
            /// \param instance the instance to query aganst
            /// \return `true` if the delegate is bound with the specified `MemberFunction`
            template <auto MemberFunction, typename T,
                typename = std::enable_if_t<std::is_invocable_r_v<R,decltype(MemberFunction),T&,Args...>>>
            [[nodiscard]]
            constexpr auto has_target(T* instance) const noexcept -> bool;

            /// \{
            /// \brief Queries whether this `delegate` is bound viewing the specified
            ///        callable \p fn
            ///
            /// \param fn the function to query
            /// \return `true` if the delegate is bound with `fn`
            template <typename Fn,
                typename = std::enable_if_t<(
                    std::is_invocable_r_v<R,Fn,Args...> &&
                    !std::is_function_v<Fn>
                    )>>
                [[nodiscard]]
            constexpr auto has_target(Fn* fn) const noexcept -> bool;
            template <typename Fn,
                typename = std::enable_if_t<(
                    std::is_invocable_r_v<R,Fn,Args...> &&
                    !std::is_function_v<Fn>
                    )>>
                [[nodiscard]]
            constexpr auto has_target(const Fn* fn) const noexcept -> bool;
            /// \}

            /// \brief Queries whether this `delegate` is bound with the specified
            ///        empty callable object Fn
            ///
            /// \tparam Fn the type to query
            /// \return `true` if the delegate is bound with `fn`
            template <typename Fn,
                typename = std::enable_if_t<(
                    std::is_empty_v<Fn> &&
                    std::is_default_constructible_v<Fn> &&
                    std::is_invocable_r_v<R,Fn,Args...>
                    )>>
                [[nodiscard]]
            constexpr auto has_target() const noexcept -> bool;

            /// \brief Queries whether this `delegate` is bound with the specified
            ///        empty function \p fn
            ///
            /// \param fn the function to query
            /// \return `true` if the delegate is bound with `fn`
            template <typename Fn,
                typename = std::enable_if_t<(
                    std::is_empty_v<Fn> &&
                    std::is_default_constructible_v<Fn> &&
                    std::is_invocable_r_v<R,Fn,Args...>
                    )>>
                [[nodiscard]]
            constexpr auto has_target(Fn fn) const noexcept -> bool;

            /// \brief Queries whether this `delegate` is bound with the specified
            ///        trivial non-empty function \p fn
            ///
            /// \param fn the function to query
            /// \return `true` if the delegate is bound with `fn`
            template <typename Fn,
                typename = std::enable_if_t<(
                    !std::is_empty_v<Fn> &&
                    !std::is_function_v<std::remove_pointer_t<Fn>> &&
                    fits_storage<Fn>::value &&
                    std::is_constructible_v<Fn,Fn> &&
                    std::is_trivially_destructible_v<Fn> &&
                    std::is_trivially_copyable_v<Fn> &&
                    std::is_invocable_r_v<R,const Fn&,Args...>
                    )>>
                [[nodiscard]]
            auto has_target(const Fn& fn) const noexcept -> bool;

            /// \brief Queries this delegate has been bound with the late-bound function
            ///        pointer \p fn
            ///
            /// \note
            /// This will only compare for functions that were bound using the
            /// `bind` or `make` overload for runtime function-pointer values; otherwise
            /// this will return `false`.
            ///
            /// \pre `fn != nullptr`
            ///
            /// \param fn the function to query
            /// \return `true` if this delegate is bound to the runtime function \p fn
            template <typename R2, typename...Args2,
                typename = std::enable_if_t<(
                    std::is_invocable_r_v<R,R2(*)(Args2...),Args...>
                    )>>
                [[nodiscard]]
            constexpr auto has_target(R2(*fn)(Args2...)) const noexcept -> bool;

            //----------------------------------------------------------------------------
            // Private Member Types
            //----------------------------------------------------------------------------
        private:

            // [expr.reinterpret.cast/6] explicitly allows for a conversion between
            // any two function pointer-types -- provided that the function pointer type
            // is not used through the wrong pointer type.
            // So we normalize all pointers to a simple `void(*)()` to allow late-bound
            // pointers
            using any_function = void(*)();

            using stub_function = R(*)(const delegate*, Args...);

            //----------------------------------------------------------------------------
            // Stub Functions
            //----------------------------------------------------------------------------
        private:

#if defined(__clang__)
# pragma clang diagnostic push
            // clang incorrectly flags '\return' when 'R = void' as being an error, but
            // there is no way to conditionally document a return type
# pragma clang diagnostic ignored "-Wdocumentation"
#endif

            /// \brief Throws an exception by default
            [[noreturn]]
            static auto null_stub(const delegate*, Args...) -> R;

            /// \brief A stub function for statically-specific functions (or other
            ///        callables)
            ///
            /// \tparam Function the statically-specific functions
            /// \param args the arguments to forward to the function
            /// \return the result of the Function call
            template <auto Function>
            static auto function_stub(const delegate*, Args...args) -> R;

            /// \brief A stub function for statically-specific member functions (or other
            ///        callables)
            ///
            /// \tparam MemberFunction the statically-specific member function
            /// \tparam T the type of the first pointer
            /// \param self an instance to the delegate that contains the instance pointer
            /// \param args the arguments to forward to the function
            /// \return the result of the MemberFunction call
            template <auto MemberFunction, typename T>
            static auto member_function_stub(const delegate* self, Args...args) -> R;

            /// \brief A stub function for non-owning view of callable objects
            ///
            /// \tparam Fn the function to reference
            /// \param self an instance to the delegate that contains \p fn
            /// \param args the arguments to forward to the function
            /// \return the result of invoking \p fn
            template <typename Fn>
            static auto callable_view_stub(const delegate* self, Args...args) -> R;

            /// \brief A stub function empty callable objects
            ///
            /// \tparam Fn the empty function to invoke
            /// \param args the arguments to forward to the function
            /// \return the result of invoking \p fn
            template <typename Fn>
            static auto empty_callable_stub(const delegate*, Args...args) -> R;

            /// \brief A stub function for small callable objects
            ///
            /// \tparam Fn the small-storage function to invoke
            /// \param self an instance to the delegate that contains the function
            /// \param args the arguments to forward to the function
            /// \return the result of invoking \p fn
            template <typename Fn>
            static auto small_callable_stub(const delegate* self, Args...args) -> R;

            /// \brief A stub function for function pointers
            ///
            /// \tparam R2 the return type
            /// \tparam Args2 the arguments
            /// \param self an instance to the delegate that contains the function pointers
            /// \param args the arguments to forward to the function
            /// \return the result of invoking the function
            template <typename R2, typename...Args2>
            static auto function_ptr_stub(const delegate* self, Args...args) -> R;

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

            //----------------------------------------------------------------------------
            // Private Members
            //----------------------------------------------------------------------------
        private:

            struct empty_type{};

            //////////////////////////////////////////////////////////////////////////////
            /// The underlying storage, which may be either a (possibly const) pointer,
            /// or a char buffer of storage.
            //////////////////////////////////////////////////////////////////////////////
            union {
                empty_type m_empty; // Default type does nothing
                void* m_instance{};
                const void* m_const_instance;
                any_function m_function;
                alignas(storage_align) unsigned char m_storage[storage_size];
            };
            stub_function m_stub;
        };

        //------------------------------------------------------------------------------
        // Deduction Guides
        //------------------------------------------------------------------------------

        template <typename R, typename...Args>
        delegate(targets::opaque_function_bind_target<R(Args...)>)
            -> delegate<R(Args...)>;

        template <auto Function>
        delegate(targets::function_bind_target<Function>)
            -> delegate<detail::effective_signature<decltype(Function)>>;

        template <auto MemberFunction, typename T>
        delegate(targets::member_bind_target<MemberFunction, T>)
            -> delegate<detail::effective_signature<decltype(MemberFunction)>>;

        //==============================================================================
        // class : bad_delegate_call
        //==============================================================================

        inline
            bad_delegate_call::bad_delegate_call()
            : runtime_error{"delegate called without being bound"}
        {

        }

        //------------------------------------------------------------------------------
        // Binding
        //------------------------------------------------------------------------------

        template <auto Function>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto bind()
            noexcept -> function_bind_target<Function>
        {
            return {};
        }

        template <auto MemberFunction, typename T>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto bind(T* p)
            noexcept -> member_bind_target<MemberFunction, T>
        {
            DELEGATE_ASSERT(p != nullptr);
            return {p};
        }

        template <typename Callable>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto bind(Callable* fn)
            noexcept -> callable_ref_bind_target<Callable>
        {
            DELEGATE_ASSERT(fn != nullptr);
            return {fn};
        }

        template <typename R, typename...Args>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto bind(R(*fn)(Args...))
            noexcept -> opaque_function_bind_target<R(Args...)>
        {
            DELEGATE_ASSERT(fn != nullptr);
            return {fn};
        }

        template <typename Callable>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto bind()
            noexcept -> empty_callable_bind_target<Callable>
        {
            return {};
        }

        template <typename Callable, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto bind(Callable)
            noexcept -> empty_callable_bind_target<Callable>
        {
            return {};
        }

        template <typename Callable, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto bind(Callable&& callable)
            noexcept -> callable_bind_target<std::decay_t<Callable>>
        {
            return {std::forward<Callable>(callable)};
        }

        //==============================================================================
        // class : delegate<R(Args...)>
        //==============================================================================

        //------------------------------------------------------------------------------
        // Constructors
        //------------------------------------------------------------------------------

        template <typename R, typename... Args>
        inline constexpr
            delegate<R(Args...)>::delegate()
            noexcept
            : m_empty{},
            m_stub{&null_stub}
        {

        }

        template <typename R, typename... Args>
        template <auto Function, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            delegate<R(Args...)>::delegate(function_bind_target<Function>)
            noexcept
            : m_empty{},
            m_stub{&function_stub<Function>}
        {

        }

        template <typename R, typename... Args>
        template <auto MemberFunction, typename T, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            delegate<R(Args...)>::delegate(member_bind_target<MemberFunction, T> target)
            noexcept
            : m_instance{target.instance},
            m_stub{&member_function_stub<MemberFunction, T>}
        {

        }

        template <typename R, typename... Args>
        template <auto MemberFunction, typename T, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            delegate<R(Args...)>::delegate(member_bind_target<MemberFunction, const T> target)
            noexcept
            : m_const_instance{target.instance},
            m_stub{&member_function_stub<MemberFunction, const T>}
        {

        }

        template <typename R, typename... Args>
        template <typename Fn, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            delegate<R(Args...)>::delegate(callable_ref_bind_target<Fn> target)
            noexcept
            : m_instance{target.target},
            m_stub{&callable_view_stub<Fn>}
        {

        }

        template <typename R, typename... Args>
        template <typename Fn, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            delegate<R(Args...)>::delegate(callable_ref_bind_target<const Fn> target)
            noexcept
            : m_const_instance{target.target},
            m_stub{&callable_view_stub<const Fn>}
        {

        }

        template <typename R, typename... Args>
        template <typename Fn, typename>
        inline DELEGATE_INLINE_VISIBILITY
            delegate<R(Args...)>::delegate(callable_bind_target<Fn> target)
            noexcept
            : m_empty{},
            m_stub{&small_callable_stub<Fn>}
        {
            new (static_cast<void*>(m_storage)) Fn(std::move(target.target));
        }

        template <typename R, typename... Args>
        template <typename Fn, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            delegate<R(Args...)>::delegate(empty_callable_bind_target<Fn>)
            noexcept
            : m_empty{},
            m_stub{&empty_callable_stub<Fn>}
        {

        }

        template <typename R, typename... Args>
        template <typename UR, typename... UArgs, typename>
        inline DELEGATE_INLINE_VISIBILITY
            delegate<R(Args...)>::delegate(opaque_function_bind_target<UR(UArgs...)> target)
            noexcept
            : m_function{reinterpret_cast<any_function>(target.target)},
            m_stub{&function_ptr_stub<UR, UArgs...>}
        {

        }

        //------------------------------------------------------------------------------
        // Binding
        //------------------------------------------------------------------------------

        template <typename R, typename...Args>
        template <auto Function, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::bind()
            noexcept -> delegate&
        {
            return ((*this) = delegate{
                targets::function_bind_target<Function>{}
                    });
        }

        template <typename R, typename...Args>
        template <auto MemberFunction, typename T, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::bind(const T* instance)
            noexcept -> delegate&
        {
            return ((*this) = delegate{
                targets::member_bind_target<MemberFunction, const T>{instance}
                    });
        }

        template <typename R, typename...Args>
        template <auto MemberFunction, typename T, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::bind(T* instance)
            noexcept -> delegate&
        {
            return ((*this) = delegate{
                targets::member_bind_target<MemberFunction, T>{instance}
                    });
        }

        template <typename R, typename...Args>
        template <typename Fn, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::bind(Fn* fn)
            noexcept -> delegate&
        {
            return ((*this) = delegate{
                targets::callable_ref_bind_target<Fn>{fn}
                    });
        }

        template <typename R, typename...Args>
        template <typename Fn, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::bind(const Fn* fn)
            noexcept -> delegate&
        {
            return ((*this) = delegate{
                targets::callable_ref_bind_target<const Fn>{fn}
                    });
        }

        template <typename R, typename...Args>
        template <typename Fn, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::bind()
            noexcept -> delegate&
        {
            return ((*this) = delegate{
                targets::empty_callable_bind_target<Fn>{}
                    });
        }

        template <typename R, typename...Args>
        template <typename Fn, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::bind(Fn)
            noexcept -> delegate&
        {
            return ((*this) = delegate{
                targets::empty_callable_bind_target<Fn>{}
                    });
        }

        template <typename R, typename...Args>
        template <typename Fn, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::bind(Fn&& fn)
            noexcept -> delegate&
        {
            return ((*this) = delegate{
                targets::callable_bind_target<std::decay_t<Fn>>{
                std::forward<Fn>(fn)
            }
                    });
        }

        template <typename R, typename...Args>
        template <typename R2, typename...Args2, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::bind(R2(*fn)(Args2...))
            noexcept -> delegate&
        {
            return ((*this) = delegate{
                targets::opaque_function_bind_target<R2(Args2...)>{fn}
                    });
        }

        //------------------------------------------------------------------------------

        template <typename R, typename...Args>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::reset()
            noexcept -> void
        {
            m_stub = &null_stub;
        }

        //------------------------------------------------------------------------------
        // Observers
        //------------------------------------------------------------------------------

        template <typename R, typename...Args>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            delegate<R(Args...)>::operator bool()
            const noexcept
        {
            return has_target();
        }

        template <typename R, typename...Args>
        template <typename...UArgs, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::operator()(UArgs&&...args)
            const -> R
        {
            return std::invoke(m_stub, this, std::forward<UArgs>(args)...);
        }

        //------------------------------------------------------------------------------

        template <typename R, typename...Args>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::has_target()
            const noexcept -> bool
        {
            return m_stub != &null_stub;
        }

        template <typename R, typename...Args>
        template <auto Function, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::has_target()
            const noexcept -> bool
        {
            return m_stub == &function_stub<Function>;
        }

        template <typename R, typename...Args>
        template <auto MemberFunction, typename T, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::has_target(const T* instance)
            const noexcept -> bool
        {
            DELEGATE_ASSERT(instance != nullptr);

            return (m_stub == &member_function_stub<MemberFunction, const T>) &&
                (m_const_instance == instance);
        }

        template <typename R, typename...Args>
        template <auto MemberFunction, typename T, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::has_target(T* instance)
            const noexcept -> bool
        {
            DELEGATE_ASSERT(instance != nullptr);

            return (m_stub == &member_function_stub<MemberFunction, T>) &&
                (m_instance == instance);
        }

        template <typename R, typename...Args>
        template <typename Fn, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::has_target(Fn* fn)
            const noexcept -> bool
        {
            DELEGATE_ASSERT(fn != nullptr);

            return (m_stub == &callable_view_stub<Fn>) &&
                (m_instance == fn);
        }

        template <typename R, typename...Args>
        template <typename Fn, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::has_target(const Fn* fn)
            const noexcept -> bool
        {
            DELEGATE_ASSERT(fn != nullptr);

            return (m_stub == &callable_view_stub<const Fn>) &&
                (m_const_instance == fn);
        }

        template <typename R, typename...Args>
        template <typename Fn, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::has_target()
            const noexcept -> bool
        {
            return (m_stub == &empty_callable_stub<Fn>);
        }

        template <typename R, typename...Args>
        template <typename Fn, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::has_target(Fn)
            const noexcept -> bool
        {
            return has_target<Fn>();
        }

        template <typename R, typename...Args>
        template <typename Fn, typename>
        inline DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::has_target(const Fn& fn)
            const noexcept -> bool
        {
            // If 'Fn' has equality operators, use that.
            // Otherwise fall back to comparing raw bytes
            if constexpr (detail::is_equality_comparable<Fn>::value) {
                return (m_stub == &small_callable_stub<Fn>) &&
                    ((*reinterpret_cast<const Fn*>(m_storage)) == fn);
            } else {
                static_assert(
                    std::has_unique_object_representations_v<Fn>,
                    "Specified `Fn` type for `has_target` does not have a unique object "
                    "representation! "
                    "Target testing can only be reliably done for functions whose identity can "
                    "be established by the uniqueness of their bits."
                    );

                return (m_stub == &small_callable_stub<Fn>) &&
                    (std::memcmp(m_storage, &fn, sizeof(Fn)) == 0);
            }
        }

        template <typename R, typename...Args>
        template <typename R2, typename...Args2, typename>
        inline constexpr DELEGATE_INLINE_VISIBILITY
            auto delegate<R(Args...)>::has_target(R2(*fn)(Args2...))
            const noexcept -> bool
        {
            DELEGATE_ASSERT(fn != nullptr);

            // The result of a comparing pointers of the wrong type is undefined behavior;
            // but thanks to each of our function stubs being uniquely instantiated based
            // on the template signature of the stub function, we can guarantee that if
            // the stubs are the same function, then the underlying data must also be the
            // same type which allows us to do this comparison without violating strict
            // aliasing!
            using underlying = R2(*)(Args2...);

            return (m_stub == &function_ptr_stub<R2, Args2...>) &&
                (reinterpret_cast<underlying>(m_function) == fn);
        }

        //------------------------------------------------------------------------------
        // Stub Functions
        //------------------------------------------------------------------------------

        template <typename R, typename...Args>
        inline
            auto delegate<R(Args...)>::null_stub(const delegate*, Args...)
            -> R
        {
            // Default stub throws unconditionally
            throw bad_delegate_call{};
        }

        template <typename R, typename... Args>
        template <auto Function>
        inline
            auto delegate<R(Args...)>::function_stub(const delegate*, Args...args)
            -> R
        {
            if constexpr (std::is_void_v<R>) {
                std::invoke(Function, std::forward<Args>(args)...);
            } else {
                return std::invoke(Function, std::forward<Args>(args)...);
            }
        }

        template <typename R, typename... Args>
        template <auto MemberFunction, typename T>
        inline
            auto delegate<R(Args...)>::member_function_stub(const delegate* self,
                                                            Args...args)
            -> R
        {
            // This stub is used for both `const` and non-`const` `T` types. To extract
            // the pointer from the correct data member of the union, this uses an
            // immediately-invoking lambda (IIL) with `if constexpr`
            auto* const c = [&self]{
                if constexpr (std::is_const_v<T>) {
                    return static_cast<T*>(self->m_const_instance);
                } else {
                    return static_cast<T*>(self->m_instance);
                }
            }();

            if constexpr (std::is_void_v<R>) {
                std::invoke(MemberFunction, *c, std::forward<Args>(args)...);
            } else {
                return std::invoke(MemberFunction, *c, std::forward<Args>(args)...);
            }
        }

        template <typename R, typename... Args>
        template <typename Fn>
        inline
            auto delegate<R(Args...)>::callable_view_stub(const delegate* self,
                                                          Args...args)
            -> R
        {
            // This stub is used for both `const` and non-`const` `Fn` types. To extract
            // the pointer from the correct data member of the union, this uses an
            // immediately-invoking lambda (IIL) with `if constexpr`
            auto* const f = [&self]{
                if constexpr (std::is_const_v<Fn>) {
                    return static_cast<Fn*>(self->m_const_instance);
                } else {
                    return static_cast<Fn*>(self->m_instance);
                }
            }();

            if constexpr (std::is_void_v<R>) {
                std::invoke(*f, std::forward<Args>(args)...);
            } else {
                return std::invoke(*f, std::forward<Args>(args)...);
            }
        }

        template <typename R, typename... Args>
        template <typename Fn>
        inline
            auto delegate<R(Args...)>::empty_callable_stub(const delegate*,
                                                           Args...args)
            -> R
        {
            if constexpr (std::is_void_v<R>) {
                std::invoke(Fn{}, std::forward<Args>(args)...);
            } else {
                return std::invoke(Fn{}, std::forward<Args>(args)...);
            }
        }

        template <typename R, typename... Args>
        template <typename Fn>
        inline
            auto delegate<R(Args...)>::small_callable_stub(const delegate* self,
                                                           Args...args)
            -> R
        {
            const auto& f = *std::launder(reinterpret_cast<const Fn*>(self->m_storage));

            if constexpr (std::is_void_v<R>) {
                std::invoke(f, std::forward<Args>(args)...);
            } else {
                return std::invoke(f, std::forward<Args>(args)...);
            }
        }

        template <typename R, typename... Args>
        template <typename R2, typename... Args2>
        inline
            auto delegate<R(Args...)>::function_ptr_stub(const delegate* self,
                                                         Args...args)
            -> R
        {
            const auto f = reinterpret_cast<R2(*)(Args...)>(self->m_function);

            if constexpr (std::is_void_v<R>) {
                std::invoke(f, std::forward<Args>(args)...);
            } else {
                return std::invoke(f, std::forward<Args>(args)...);
            }
        }

    } // inline namespace bitwizeshift
} // namespace DELEGATE_NAMESPACE_INTERNAL

#if defined(_MSC_VER)
# pragma warning(pop)
#endif

#endif /* DELEGATE_DELEGATE_HPP */