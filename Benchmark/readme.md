# CallMe benchmark 

For Visual Studio users the benchmark project `Benchmark.vcxproj` is available as a part of `Event.sln`.

For CMake users, the root level folder of `CallMe` contains `CMakeLists.txt` that builds unit tests and the benchmark.

The benchmark profiles most basic scenarios of using `CallMe`.

Most callable targets in the benchmark share the following signature:

```cpp
using FreeSignature = void(std::string&, volatile int*, volatile std::size_t*);
```

The benchmark calls a target-under-test a certain number of times and measures the time it took for all the calls to complete. The time is shown in results in microseconds [us]. The benchmark is single-threaded. Of course, there is some dispersion. If you are worried about the dispersion, you have to run the benchmark yourself several times to see the respective effects.

Most targets have very minimal implementation, like copying a couple of values:

```cpp
void InlineFreeFunction(std::string& i2, volatile int* o1, volatile std::size_t* o2)
{
    *o1 = I1;
    *o2 = i2.size();
}
```

If the parameters were not marked as `volatile`,  then compilers would optimize too much, up to fully eliminating all code. Thanks to `volatile`, the memory pointed to by `o1` and `o2` in the code above is "non-transparent" for compilers, forcing them to leave at least two `mov` assembly instructions at the call site.

Compilers are able to inline many calls in the benchmark. This means that the delegate is fully optimized out and the target is inlined at the call-site as if the target was called there directly and was inlined too. In such cases `CallMe` has the ultimate zero overhead, there is no executable code from `CallMe` at all, while the library has not prevented the inlining of the target. In benchmark results, a time of 5000-6000us means that the target was inlined and `CallMe` was optimized away. 

There are nonstandard compiler-specific keywords that allow to mark a function as non-inlinable, which prohibits the compiler to inline it. The benchmark profiles delegates both with inlinable and non-inlinable targets. In test outputs, "inlinable" means that the target does not prohibit the compiler to inline it, however the compiler may or may not inline such targets at its own discretion. 

One important case that can prevent the compiler from inlining small targets like `InlineFreeFunction` above is if the delegate implementation is too complex, so that it is not transparent enough to the compiler what is going on at the call-site. As a result, the compiler cannot optimize out the delegate and then inline the target. Therefore, it is very important to keep the delegate implementation as small and simple as possible. If you want to add new abstractions, cool features and improvements to any fast delegates library, recall this, and have a benchmark for all basic use cases for regression testing of performance.

Another case that prevents the compiler from inlining is introducing too much indirection and/or dynamic memory when accessing a delegate. One example is when a delegate is stored in `std::vector`. `std::vector` introduces both one level of indirection and a heap-allocated buffer. That is too much for all tested compilers to see the intentions of the code. For best performance, try to avoid dynamic memory and unnecessary indirection when accessing `CallMe` delegates, store delegate objects on the stack, try to make your intentions clear to the compiler at compile time, run logic as early as possible (compile time, initialization) rather than at later stages.

## Frequent terms used in the benchmark

**λ with capture** - the target is a lambda function that captures some state.

**captureless λ** - the target is a lambda function that does not capture state.

**method** - the target is an object and its member function.

**function** - the target is a non-member function.

**direct call** - the target is invoked directly (without delegates).

**Delegate** - the target is invoked via `Delegate<...>`

**OwningDelegate** - the target is invoked via `OwningDelegate<...>`

## Results

This readme document is absolutely not enough to understand all the benchmark results. It is necessary to study the source code of the benchmark anyway.

All optimization-related compiler and linker flags are whatever the "Release" configuration in Visual Studio and "release" build variant in CMake set automatically, no manual tweaking was done.

Here are the results for the three major C++ compilers:

[Benchmark results, Visual C++](msvc.txt)

[Benchmark results, Clang](clang.txt)

[Benchmark results, GCC](gcc.txt)

Visual C++ performs decent inlining of targets. 

But Clang does much better than VC++. Out of the three tested compilers, Clang shows best results in terms of inlining of targets and optimizing out `CallMe`.

GCC, on the other hand, almost does not inline even in simplest cases and shows worst performance among the tested compilers. 

According to the results, it looks like Clang is the best compiler to use with `CallMe`, while using GCC is not recommended.

