# CallMe - fast delegates and events

- [CallMe - fast delegates and events](#callme---fast-delegates-and-events)
  - [Features](#features)
  - [Known solutions/prior art](#known-solutionsprior-art)
  - [How to use](#how-to-use)
  - [Singlecast delegates](#singlecast-delegates)
    - [Functors](#functors)
    - [Member functions](#member-functions)
    - [Functions](#functions)
      - [Functions known at compile-time and static member functions](#functions-known-at-compile-time-and-static-member-functions)
      - [Functions unknown at compile-time](#functions-unknown-at-compile-time)
      - [Functions and calling conventions](#functions-and-calling-conventions)
  - [Events](#events)
    - [Subscription management](#subscription-management)
    - [Double subscription](#double-subscription)
  - [Compile-time errors](#compile-time-errors)
  - [Multithreading](#multithreading)


`CallMe` is a header-only library of fast delegates and events for C++20.

## Features

* Type-erased fast delegates. Target callables' types are erased by templated thunk functions.
* Singlecast and multicast delegates (aka "events")
* Supported targets are functions, static and non-static member functions, functors (including lambda functions).
* Singlecast delegates come in two flavors,  non-owning (aka function_ref/function reference/function_view/function view) and owning ones.
* The delegates are very lightweight and have a fixed size for holding 2 or 3 pointers.
* Non-owning delegates don't allocate memory on the heap.
* Very low overhead compared to directly calling target callables. In many cases the overhead is zero, see [benchmark](Benchmark/readme.md).
* Easy-to-use, high performance events with low overhead compared to directly calling non-inlined targets. Events hold their callbacks in a contiguous container with user-adjustable small buffer optimization, allowing to avoid not only memory reallocation, but the use of heap memory altogether, giving better data locality and higher cache hits. Comfortably manage event subscriptions RAII-style, unsubscribe automatically. Adjust the inline storage and run both subscription and unsubscription in O(1) time.
* Meaningful compiler errors. The library tries hard to make most likely compile-time errors caused by invalid use of its API readable and meaningful to users, despite heavy template metaprogramming involved.
* Minimal and compact implementation. The library provides a very thin abstraction layer over raw function pointers. A lot of source code implements zero-cost abstractions and compile-time checks, leaving minimal code executable at runtime. This makes intentions transparent for optimizers, that are often able to fully optimize away all library code at call-sites, inline the callable target, and give **zero overhead compared to directly calling the inlined callable target**. See [benchmark](Benchmark/readme.md).  
* Powerful type deduction. The library heavily relies on C++17 features for CTAD, and adds lots of deduction guides. All singlecast delegates can deduce proper signatures from target callables. Avoid duplicate code by leaning on type deduction.
* Factory functions for delegates. In addition to overloaded constructors of delegates, there is a set of factory functions covering all the constructor overloads. Factories like `fromFunction(...)` or `fromFunctor(...)` narrow down the overload resolution set for a particular class of targets. The factories are syntactically cleaner and easier to use than delegate constructors, especially for new users.
* The library heavily relies on C++20 constrains and concepts. While nothing prevents porting the library to C++17 and SFINAE, this is a non-goal.
* Equality testing for delegates is a non-goal. Due to implementation details, there is no single universally acceptable definition of "equal" delegates. If you need it, you will have to implement equality testing for your particular case yourself.
* Thorough tests (functional tests, tests for invalid uses of API, benchmarks) give a good base for regression testing. People who want to hack, e.g. to fine-tune performance for special cases, will have a productive start.

## Known solutions/prior art

One of the early works that became widely known is [Don Clugston's research on implementation of member function pointers in different C++ compilers](https://www.codeproject.com/Articles/7150/Member-Function-Pointers-and-the-Fastest-Possible). Based on that research, Don proposed an early implementation of what is now called "C++ fast delegates".

That triggered a couple of other articles on CodeProject.

https://www.codeproject.com/Articles/11015/The-Impossibly-Fast-C-Delegates by Sergey Ryazanov tries to improve upon Don Clugston's work.

https://www.codeproject.com/Articles/1170503/The-Impossibly-Fast-Cplusplus-Delegates-Fixed by Sergey Kryukov tries to improve upon Sergey Ryazanov's work.

Those three works served as a basis for numerous implementations of fast delegates all over the Internet over years.

Among more modern examples is a series of articles and implementation by Matthew Rodusek:

https://bitwizeshift.github.io/posts/2021/02/24/creating-a-fast-and-efficient-delegate-type-part-1/

https://bitwizeshift.github.io/posts/2021/02/26/creating-a-fast-and-efficient-delegate-type-part-2/

https://bitwizeshift.github.io/posts/2021/02/26/creating-a-fast-and-efficient-delegate-type-part-3/

https://github.com/bitwizeshift/Delegate

Matthew shows how to use C++17 and its class template argument deduction for automatically deducing the signatures of delegates. 

C++ proposal [P0792](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p0792r10.html) is an attempt to standardize `function_ref`, a "type-erased callable reference". The proposal mentions the following implementations of `function_ref`:

* [`llvm::function_ref`](https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/ADT/STLExtras.h) – from LLVM

* [`tl::function_ref`](https://github.com/TartanLlama/function_ref/blob/master/include/tl/function_ref.hpp) – by Sy Brand

* [`folly::FunctionRef`](https://github.com/facebook/folly/blob/main/folly/Function.h) – from Meta

* [`gdb::function_view`](https://github.com/bminor/binutils-gdb/blob/master/gdbsupport/function-view.h) – from GNU

* [`type_safe::function_ref`](https://github.com/foonathan/type_safe/blob/main/include/type_safe/reference.hpp) – by Jonathan Müller

* [`absl::function_ref`](https://github.com/abseil/abseil-cpp/blob/master/absl/functional/function_ref.h) – from Google

By 2023, almost every large well-known C++ project, such as a GUI framework or a game engine, has its own implementation of fast delegates. There are dozens of implementations of fast delegates in circulation, most of which are proprietary.

## How to use

`CallMe` is a header-only library. 

* To use only [singlecast] delegates: copy `CallMe.h` to your project directory and `#include CallMe.h`.

* To use events: copy `small_vector.h`, `CallMe.h`, `CallMe.Event.h` to your project directory and `#include CallMe.Event.h`. The latter includes `CallMe.h`, so including `CallMe.Event.h` gives access to singlecast delegates and events.

The public API is in the namespace `CallMe`. 

The following compilers have been tested and can compile and pass all `CallMe` tests:

* GCC 12.2.1 x86_64 on Linux
* Clang 15.0.7 x86_64 on Linux
* Visual C++ 2022 Version 17.5.3, platform toolset "Visual Studio 2022 (v143)" x64 and x86 on Windows

## Singlecast delegates

*`CallMe` has extensive functional tests with many examples covering all supported overloads and features.*

There are two kinds of singlecast delegates, `Delegate<...>` and `OwningDelegate<...>`.

Delegates are called using the `operator()` or `invoke(...)` member functions. Both functions are identical. 

`Delegate<...>` does not own target callables, it references them through pointers. `Delegate<...>` is in the same category as `function_ref` from proposal p0792. When working with `Delegate<...>`, you have to make sure that all referenced targets are valid/alive for as long as you need to call them via `Delegate<...>`. 

`OwningDelegate<...>` owns its targets, so it is always safe to call `OwningDelegate<...>`. The ownership is exclusive, similar to `unique_ptr`. That is why `OwningDelegate<...>` cannot be copied, it is a move-only type like `unique_ptr`. 

Both `Delegate<...>` and `OwningDelegate<...>` can be default-constructed. Invoking default-constructed delegates has no effects. 

`CallMe` does not have such notions as to "bind" or "rebind" a target to a delegate. A delegate is constructed with its target right away.    

If you need to "rebind" a delegate to a new target, construct a new delegate from the new target and assign the new delegate to the old one.   

Default-constructed delegates are used in this way too: there is a variable with a default-constructed delegate and you assign a new delegate to the variable.

`Delegate<...>` can be both copy-constructed/assigned and move-constructed/assigned, though in the current implementation there is no difference.

`OwningDelegate<...>` can be move-constructed/assigned.

### Functors
Delegates for functors are constructed as follows: 

```cpp
int a = 1;
auto lambda = [a](int i) mutable{    
    return i + a;
};
        
Delegate<int(int)> delegate1(&lambda);
Delegate delegate2(&lambda);
auto delegate3 = fromFunctor(&lambda);
```

`delegate1` has an explicitly declared signature, `delegate2` and `delegate3` have the same signature deduced. These three basic ways of constructing a delegate are available for both `Delegate<...>` and `OwningDelegate<...>` and all kinds of targets.

Notice that `lambda` is a variable instantiated before delegates. `Delegate<...>` does not own targets, it can only reference existing targets, and you have to make sure that the targets are alive for as long as you invoke them via `Delegate<...>` (if that is unacceptable for your scenario, use `OwningDelegate<...>` instead). The `lambda` variable with automatic storage duration in the sample above is a simple way for the target to span the lifetime of `Delegate<...>` objects that have automatic storage duration as well.

When using `CallMe`, do not use the resource-heavy `std::function` [to store and own a lambda](https://stackoverflow.com/a/9186537) with the aim to call the `std::function` via `Delegate<...>`. That undermines the purpose of `CallMe` - to be a library of fast delegates for use cases that require better performance than `std::function`. If you absolutely need lambda-functions whose lifetime is managed by delegates, that is what `OwningDelegate<...>` is for. Nevertheless, `Delegate<...>` should be preferred in most cases.

It is also possible to [store a lambda in a class member variable](https://stackoverflow.com/a/32280985) next to the member variable that holds the `Delegate<...>` itself.

`Delegate<...>` does not accept r-value references to targets:

```cpp
int a = 1;
Delegate<int(int)> delegate1([a](int i) mutable{    
    return i + a;
});//error, r-value references are not accepted
```
Some implementations of fast delegates accept use cases like the above and copy the target to the delegate itself by implementing inline storage for small targets. `CallMe` does not do this, `Delegate<...>` is strictly a non-owning reference to a target that must exist somewhere outside.

`Delegate<...>` constructors and `fromFunctor(...)` are overloaded and also accept l-value references to functors.

`OwningDelegate<...>`, on the other hand, accepts only pointers to targets, and assumes that a target has just been allocated and instantiated on the heap with `new`:

```cpp
int a = 1;
OwningDelegate<int(int)> delegate1(new auto([a](int i) mutable {
    return i + a;
}));
OwningDelegate delegate2(new auto([a](int i) mutable {
    return i + a;
}));
auto delegate3 = fromFunctorOwned(new auto([a](int i) mutable {
    return i + a;
}));
```
`OwningDelegate<...>` then assumes exclusive ownership over the passed pointer. It is recommended to allocate and instantiate arguments for `OwningDelegate<...>` like in the sample above, by using `new auto` right at the argument site. `new auto` is optimized by major compilers and performs copy elision for the lambda constructor/closure object:

* the closure object is allocated and instantiated right on the heap
* captured values are copied once - to the closure object

Other ways to allocate a lambda on the heap usually cause:

* first copying captured values to the closure object
* then allocating memory on the heap
* then copying the closure object to the heap

Using `new auto` right at the argument site prevents a possible leaked pointer if an exception is thrown after a target is instantiated but before the pointer is passed to `OwningDelegate<...>`.

###  Member functions
Unlike functors, targeting member functions requires also specifying the function to be called:
```cpp
TestObject target;
Delegate<int(int i)> delegate1(&target, tag<&TestObject::set>());
Delegate delegate2(&target, tag<&TestObject::set>());
auto delegate3 = fromMethod<&TestObject::set>(&target);
```

`tag` is a helper `struct` that allows to pass member function pointers to delegate constructors as non-type template parameters of those constructors. It is a limitation of C++ that explicitly specifying template parameters of constructors is not allowed:

```cpp
TestObject target;
Delegate<int(int i)> delegate1<&TestObject::set>(&target); //error, not supported by C++
```

This is one of the reasons why using factory functions like `fromMethod(...)` above is syntactically cleaner and easier.

As usual, the constructors and the factory function are overloaded and also accept the target object by l-value reference. Member functions are only accepted as pointers-to-member-functions.

`OwningDelegate<...>` accepts target objects only as pointers (see Functors):

```cpp
OwningDelegate<int(int i)> set(new TestObject, tag<&TestObject::set>());
OwningDelegate get(new TestObject, tag<&TestObject::get>());
auto get2 = fromMethodOwned<&TestObject::get>(new TestObject);
```

Target member functions may be virtual. Calling virtual functions via `CallMe` delegates does not preclude their virtual dispatch. 

const-member functions are supported, but delegate signatures do not include `const` in such cases. The same applies to lambda-functions with a mutable and implicitly-const `operator()`: delegate signatures do not reflect such differences.

### Functions

#### Functions known at compile-time and static member functions

Targeting non-member functions is done like this:

```cpp
Delegate<std::size_t(int i, const std::string& s)> delegate{tag<&freeFunction>()};

Delegate delegate2{tag<&freeFunction>()};

auto delegate3 = fromFunction<&freeFunction>();
```

Notice the use of the uniform initialization syntax - in this case it is necessary as we hit the most vexing parse problem.

Static member functions are handled by the same constructor and factory function overloads:

```cpp
Delegate<std::size_t(int i, const std::string& s)> delegate{tag<&TestObject::staticMemberFn>()};

Delegate delegate2{tag<&TestObject::staticMemberFn>()};

auto delegate3 = fromFunction<&TestObject::staticMemberFn>();
```

Targeting static member functions as ordinary member functions is not allowed.

`OwningDelegate<...>` cannot target non-member functions and static member functions at all. The purpose of `OwningDelegate<...>` is to own an object (with state) and to control the lifetime of that object. As non-member functions and static member functions do not have associated object state, there is nothing to own. Use `Delegate<...>` for these kinds of targets.

If a lambda-function does not capture anything and does not use virtual functions, its `operator()` is converted by compilers to an ordinary function. That is why such lambda-functions cannot be owned by `OwningDelegate<...>` as well. Use `Delegate<...>` for such stateless lambda-functions.

#### Functions unknown at compile-time

Sometimes there is a pointer to a function with a known signature, but what the pointer points to becomes known only at runtime. For example:

* a function is implemented by a library or operating system and is loaded from a DLL at runtime 

* runtime logic selects a function from several candidates at runtime

For such cases there are special constructor and factory overloads that accept pointers to functions as parameters:

```cpp
HMODULE hKernel = GetModuleHandleA("kernel32");
auto    pSet    = (void (WINAPI *)(DWORD err))GetProcAddress(hKernel, "SetLastError");
auto    pGet    = (DWORD(WINAPI *)())GetProcAddress(hKernel, "GetLastError");

Delegate set(pSet);
Delegate get(pGet);
auto set2 = fromFunction(pSet);
auto get2 = fromFunction(pGet);
```

Functions known at compile-time are passed to delegate constructors/factories as non-type template parameters.

Functions unknown at compile-time are passed to delegate constructors/factories as [ordinary] parameters of those constructors/factories.

If a function is statically-known to the compiler, the compiler might inline it or apply other call-site optimizations.

For functions whose pointers are obtained at runtime, such call-site optimizations are not available. Whenever possible, use the overloads accepting function pointers as template parameters.

#### Functions and calling conventions

Functions are often used as a basis in system APIs of operating systems and in APIs of libraries developed in C. That is why functions are the only kind of targets for which `CallMe` supports non-default calling conventions. 

A "default" calling convention is whatever a C++ compiler uses when it deals with function-pointers without any calling convention specified. This convention is compiler- and platform-specific.

 `CallMe` supports non-default calling conventions only for Microsoft Visual C++, and they are all MSVC-specific. The supported conventions are: `__cdecl`, `__stdcall`, `__fastcall`, `__vectorcall`. 

The sample code above (Functions unknown at compile-time) shows the use of non-default calling conventions, namely `WINAPI`, which is `typedef`ed to `__stdcall`. If a calling convention is specified, it is a part of the function signature. 

`CallMe` works with such signatures, namely:

* template argument deduction works, delegate signatures are deduced
* delegates call such functions according to their calling conventions

Other than explicitly specifying a proper calling convention as a part of a function signature, you do not need to do anything. Use and call such delegates as usual.

Notice that different calling conventions are important in Win32/x86. On ARM and x64, Windows uses `__fastcall`, and MSVC ignores calling conventions specified in source code. I.e., when compiling for ARM and x64, specifying `__cdecl` or `__stdcall` works as if `__fastcall` was specified.

For all other kinds of targets except [free] functions, non-default calling conventions are not supported, as are not they supported for platforms other than Windows/MSVC.

## Events

*For `Event<...>`,  there are useful reference comments for its member functions in source code. Also, there are tests covering many non-trivial scenarios involving subscription management and move semantics of `Event<...>` and its subscriptions. Before using `Event<...>` seriously, it is highly recommended to read the tests and the reference comments in source code for insights that this document will not give.*

`Delegate<...>` and `OwningDelegate<...>` are singlecast delegates. `Event<...>` is a multicast delegate that maintains a set of subscription callbacks and the infrastructure for subscribing and unsubscribing the callbacks at runtime.

Call `raise(...)` or `operator(...)` member functions to notify/invoke all subscribed callbacks. The order of invocation of subscribed callbacks relative to each other is unspecified. The parameters of `raise(...)` and `operator(...)` are defined by the `Event<...>` signature. If there are parameters, usually callbacks should not mutate them - accepting arguments by value or by const-reference is recommended. If callbacks mutate their parameters, carefully think out how that will interact with callbacks of other subscriptions.

The definition of `Event<...>` accepts a couple of template parameters:

```cpp
template<typename Signature = void(),
        unsigned ExpectedSubscriptions = internal::ExpectedSubscriptionsDefault>
class Event ...
```

The `Signature` is what all subscribed callbacks will have to match to. The return type is always `void`.

`Event<...>` uses a vector-like container with a small-buffer optimization to store subscriptions. This vector container stores its elements inline if there are up to `ExpectedSubscriptions` (template parameter of `Event<...>`) subscriptions. For higher number of subscriptions the container allocates space on the heap and moves subscriptions from the inline storage to the heap.

Skillful use of `ExpectedSubscriptions` allows to squeeze maximum performance out of `Event<...>` and your hardware. Properly adjusting `ExpectedSubscriptions` allows to fully avoid reallocation while subscribing, and makes both subscribing and unsubscribing [a single callback] an O(1) operation. If such an `Event<...>` is stack-allocated, it is completely heap-free. The performance of raising/invoking an `Event<...>` additionally benefits from the data locality and compiler optimizations of inline storage.

There are cases when storing subscriptions inline is undesirable or impossible, for example:

* There are too many subscriptions, while an event is allocated on the stack. Trying to store subscriptions inline would cause stack overflow.
* Move construction and move-assignment of events with inline storage of subscriptions is as expensive as copy construction and copy-assignment. If there are many subscriptions, and you need to move-construct or move-assign such heavy events, using heap and a level of indirection instead of inline storage allows to avoid the deep-copying of subscription callbacks.

In order to use heap storage:

* Leave `ExpectedSubscriptions` at its default value or set to 0.

* Call `reserve(unsigned expectedSubscriptions)` on the event to reserve enough heap memory.  There is also constructor `Event(unsigned expectedSubscriptions)` that has the same effect as calling `reserve(...)` right after the default constructor. 

Using `reserve(unsigned expectedSubscriptions)` or `Event(unsigned expectedSubscriptions)` makes sense only for `expectedSubscriptions > ExpectedSubscriptions`. Whenever `expectedSubscriptions <= ExpectedSubscriptions`, inline storage is used, and it makes no sense to reserve anything.

For details and examples of using `ExpectedSubscriptions` and `reserve(...)`, see the tests and reference comments in the source code of `Event<...>`.

### Subscription management

`Event<...>` uses `Delegate<...>` objects as callbacks. Make sure that all targets of subscribed callback delegates are alive/valid for as long as you need to notify them via `Event<...>` (read on to see how).

In order to subscribe a target to `Event<...>`, wrap the target into `Delegate<...>` and pass the `Delegate<...>` to `subscribe(...)`:

```cpp
struct Subscriber
{
    void notify()
    {
    }
}
auto makeCallback(Subscriber& s)
{
    return fromMethod<&Subscriber::notify>(s);
}
Event event;
Subscriber alice;
Subscription subscription = event.subscribe(makeCallback(alice));
```

The returned `Subscription` object is a RAII-wrapper for managing the lifetime of the subscription. Whenever `Subscription` is destroyed, it unsubscribes the callback it manages. `Subscription` objects are allowed to outlive the event they originate from, in which case they do not unsubscribe from the destroyed event.

Usually an object such as `Subscriber` in the sample above subscribes its member-function to an `Event<...>` in its constructor and the subscription needs to last for the lifetime of `Subscriber`. In this case, store `Subscription` in a member variable of `Subscriber`:

```cpp
struct Subscriber
{
    void notify()
    {
    }
    
    Subscriber(Event<>& e) : 
        subscription(e.subscribe(makeCallback(*this)))
    {
    }
    
    Subscription subscription;           
}
Event event;
Subscriber alice(event);
```
The sample above does not require any additional explicit actions with `alice.subscription`, and the subscribed target (`alice`) is alive/valid for the whole lifetime of the subscription - by design.

`Subscription` is not default-constructible, so all member variables of the `Subscription` type must be initialized in a member initializer list as shown above. If you need to weaken this requirement in order to have `Subscription` member variables that may be initialized later and/or optionally, wrap a subscription in `std::optional`:

```cpp
struct Subscriber
{
    void notify()
    {
    }
    
    void subscribeAll(Event<>& e)
    {
        optionalSubscription.emplace(e.subscribe(makeCallback(*this)));
    }
    
    std::optional<Subscription> optionalSubscription;
}
Event event;
Subscriber alice;
alice.subscribeAll(event);

//some time later
alice.optionalSubscription.reset();
```

The use of `std::optional ` around `Subscription` objects also allows to explicitly unsubscribe without waiting for the `Subscriber` to be destroyed. 

If a `Subscriber` manages more than one subscription, it may be convenient to store all subscriptions in `std::vector<Subscription>`. `Event<...>` provides an overload of `subscribe(...)` that accepts by reference a vector of subscriptions and emplaces the new subscription into the vector before returning:
```cpp
struct Subscriber
{
    void notify()
    {
    }
    
    void subscribeAll(Event<>& e)
    {
        e.subscribe(makeCallback(*this), subscriptions);
    }
    
    std::vector<Subscription> subscriptions;
}
Event event;
Subscriber alice;
alice.subscribeAll(event);

//optionally, some time later
alice.subscriptions.clear();
```

`Subscription` objects are agnostic to the signature of the event that generated them. Therefore, it is possible to store subscriptions originating from different events with different signatures in a single `std::vector<Subscription>`.

To recap:

* If there are several subscriptions and they don't require custom lifetime management, use a member variable of the `std::vector<Subscription>` type
* If a subscription requires custom lifetime management, use a member variable of the `std::optional<Subscription>` type
* If there is a single subscription that does not require custom lifetime management, use a member variable of the `Subscription` type

### Double subscription

When subscribing, `Event<...>` does not check if the target is already subscribed to it. Repeatedly subscribing a target creates new independent subscriptions:

```cpp
Event event;
Subscriber alice;

std::vector<Subscription> subscriptions;

event.subscribe(makeCallback(alice), subscriptions);
event.subscribe(makeCallback(alice), subscriptions);

event.raise();//alice is notified twice
```

If you store a subscription in a member variable of the `Subscription` type, then subscription is unconditional and always takes place in member initializer lists of constructors, so there is no such problem as possible double subscription.

If subscription logic runs after constructors, and you anticipate possible double subscription, you have to guard against it using any means external to `Event<...>`. For example, if a subscription is stored in `std::optional<Subscription>`, the guard might be like:

```cpp
Event event;
Subscriber alice;
std::optional<Subscription> subscription;

if(!subscription.has_value())
    subscription.emplace(event.subscribe(makeCallback(alice));
```

## Compile-time errors

Clang and GCC provide enough information for diagnosing compile-time errors originating in template code. 

In MSVC, error messages can be cryptic when template code is involved. 

`CallMe` tries to ease the decrypting of most likely errors by explicitly deleting functions with wrong parameters. That may not be enough when such a deleted function is hit by template code of a standard container. **If you encounter a compile-time error due to a deleted function, and you cannot decrypt the error**, try to 

```cpp
#define COMPILE_INVALID_CTORS
```

before including `CallMe.Event.h` or `CallMe.h`. This has the effect that deleted invalid functions become compilable and unconditionally throw exceptions at runtime. In other words, some compile-time errors turn into easier-to-debug run-time errors. `COMPILE_INVALID_CTORS` is used by `CallMe` internally for testing. Do not forget to delete the `#define` when errors are resolved.

## Multithreading

`CallMe` is not designed for multithreaded use. That said, some parts of `CallMe` may be used in multithreaded scenarios.

For `Delegate<...>`, mutable operations are construction/destruction, copy construction and assignment, move construction and assignment.

For `OwningDelegate<...>`, mutable operations are construction/destruction, move construction and assignment.

For `Event<...>`, mutable operations are construction/destruction, move construction and assignment, subscribing/unsubscribing callbacks to/from events, the functions `.reserve(...)` and `.clear()`, move-construction and move-assignment of `Subscription` objects, destruction of `Subscription` objects (causes unsubscription/mutates the event).

If the listed mutable operations are invoked on the same object on more than one thread at a time, that certainly will wreak havoc.

However, invoking delegates with the functions `.invoke(...)`/`operator()`, and invoking `Event<...>` with the functions `.raise()`/`operator()` does not mutate `Delegate<...>`/`OwningDelegate<...>`/`Event<...>` themselves. Invoking the same delegate/event object on more than one thread at a time will not break that delegate/event object. But this says nothing about the targets and their ability to cope with such multithreaded calls. For example, if a target somehow protects itself with synchronization primitives, or its invocation does not mutate the target itself, or the target is fully stateless, then its multithreaded invocation via `CallMe` is safe.

`CallMe` currently does not mark `invoke(...)`/`operator()`/`raise()` with the `const` qualifier, keeping transitive immutability in mind: some targets may mutate themselves when invoked via delegates, but it is their business. Lifting const-correctness from targets up to the level of delegates/events would complicate the implementation of the latter. For example, `Event<...>` currently can have many subscribed callbacks, some of which may mutate their subscribers while others may not.
