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

#define USE_SMALL_VECTOR

#include <cassert>
#include <optional>
#include <vector>

#ifdef USE_SMALL_VECTOR
#define GCH_DISABLE_CONCEPTS
#include "small_vector.h"
#endif

#include "CallMe.h"

namespace CallMe
{
	/*

		Subscription1{ <------------+			
			Event*					|
			SubscriptionIndex(0)	|
		}							|
	+-> Subscription2{				|
	|		Event*					|
	|		SubscriptionIndex(1)	|
	|	}							|
	|								|
	|	vector subscriptions{		|
	|		SubscriptionRecord1{	|
	|			delegate1			|
	|			Subscription* ------+
	|		},		
	|		SubscriptionRecord2{	
	|			delegate2
	+---------- Subscription*
			},
			...
			SubscriptionRecordN{...}
		}

	*/

	class Subscription;

	namespace internal
	{
		/* the default number of expected subscriptions for which
		Event initially allocates memory */
		constexpr auto ExpectedSubscriptionsDefault = 5;

		/* Subscribed delegates are stored in a contiguous ordered
		vector-like container. SubscriptionIndex is the index
		of the delegate in the container. */
		using SubscriptionIndex = std::ptrdiff_t;

		#ifndef NDEBUG
		constexpr static SubscriptionIndex InvalidSubscriptionIndex = -1;
		#endif

		template<typename>
		class SubscriptionRecord;

		//non-owning pointer
		#define viewptr *

		//Signature-erased event interface for Subscription
		class ErasedEvent
		{
		public:
			virtual ~ErasedEvent() = default;

			virtual void unsubscribe(SubscriptionIndex toRemove) = 0;
			virtual void changeOwner(SubscriptionIndex toChange, Subscription viewptr newOwner) = 0;
			virtual void moveDelegate(SubscriptionIndex from, SubscriptionIndex to) = 0;

		#ifdef NDEBUG
			void validate() {}
		#else
			virtual void validate() = 0;
		#endif
		};

		/* In this internal version of Event, ExpectedSubscriptions has
		no default value and goes first in template parameters. This allows
		decomposing the signature, which enables better compiler errors
		for calls of .raise(...) with invalid arguments */
		template<unsigned ExpectedSubscriptions, typename Signature>
		class Event;

		template<unsigned ExpectedSubscriptions, typename R, typename...ClassArgs>
		class Event<ExpectedSubscriptions, R(ClassArgs...)> : public ErasedEvent
		{
			static_assert(std::same_as<R, void>, "only void return types are supported");

			using Signature = R(ClassArgs...);

			using DelegateT = Delegate<Signature>;
			
			using SubscriptionRecordT = SubscriptionRecord<Signature>;
			
		#ifdef USE_SMALL_VECTOR
			using VectorT = gch::small_vector<SubscriptionRecordT, ExpectedSubscriptions>;
		#else
			using VectorT = std::vector<SubscriptionRecordT>;
		#endif

			VectorT _records;

		#ifdef NDEBUG
			//empty impl in base
		#else
			void validate() override
			{
				for(SubscriptionRecordT& r : _records)
				{
					assert(r._owner != nullptr);
					assert(r._owner->_event == this);
					assert(r._owner->_index < std::ssize(_records));
					assert(&_records[r._owner->_index] == &r);
				}
			}
		#endif

			void unsubscribe(SubscriptionIndex toRemove) override
			{
				assert(!_records.empty());
				assert(0 <= toRemove && toRemove < std::ssize(_records));
			
				_records[toRemove] = std::move(_records.back());
				_records[toRemove]._owner->_index = toRemove;

				_records.pop_back();

				validate();
			}

			void moveDelegate(SubscriptionIndex from, SubscriptionIndex to) override
			{
				assert(0 <= from && from < std::ssize(_records));
				assert(0 <= to && to < std::ssize(_records));

				_records[to]._delegate = std::move(_records[from]._delegate);
			}

			void changeOwner(SubscriptionIndex toChange, Subscription viewptr newOwner) override
			{
				assert(0 <= toChange && toChange < std::ssize(_records));

				_records[toChange].changeOwner(newOwner);

				validate();
			}

		public:
			Event(const Event& other) = delete;
			Event& operator=(const Event& other) = delete;

			Event(Event&& other) noexcept :
				_records(std::move(other._records))
			{
				other._records.clear();

				//update pointers to Event in subscriptions
				for (std::ptrdiff_t i = 0; i!=std::ssize(_records); ++i)
					_records[i].changeEvent(this);
			}

			Event& operator=(Event&& other) noexcept
			{
				if (this == &other)
					return *this;

				//existing subscriptions release ownership
				//as _records is about to be overwritten
				for (std::ptrdiff_t i = 0; i!=std::ssize(_records); ++i)
					_records[i].releaseOwnership();

				_records = std::move(other._records);
				other._records.clear();

				//update pointers to Event in subscriptions
				for (std::ptrdiff_t i = 0; i!=std::ssize(_records); ++i)
					_records[i].changeEvent(this);

				return *this;
			}

			explicit Event()
			{
				#ifndef USE_SMALL_VECTOR
				//for std::vector, reserve ExpectedSubscriptions on the heap
				reserve(ExpectedSubscriptions);
				#endif
			}

			explicit Event(unsigned expectedSubscriptions)
			{
				reserve(expectedSubscriptions);
			}

			/* Reserve space for @expectedSubscriptions anticipated subscriptions.

			If less than @ExpectedSubscriptions[the Event class template parameter]
			subscriptions are anticipated, they are all saved inline
			(small buffer optimization) in the Event itself and there is no need
			to call this function.

			When the number of subscriptions exceeds @ExpectedSubscriptions, that
			triggers reallocation and the subscriptions are moved from Event's
			inline storage to the heap.

			Call .reserve(...) when @expectedSubscriptions > @ExpectedSubscriptions
			in order to allocate enough memory on the heap and avoid reallocation
			during upcoming .subscribe(...) calls.

			In most cases, you should not call .reserve(...). Instead, just specify
			proper @ExpectedSubscriptions. In real projects, most events have a few
			subscriptions, so having them inline is highly preferred.

			Sometimes specifying large enough @ExpectedSubscriptions is not an option.
			For example, if Event is stack-allocated and you anticipate large number
			of subscriptions for that Event, allocating them inline in the Event can
			cause stack overflow. In such cases leave @ExpectedSubscriptions at its
			default low value or set to 0 and preallocate enough heap memory with
			.reserve(...) */
			void reserve(unsigned expectedSubscriptions)
			{
				_records.reserve(expectedSubscriptions);
			}

			/* Quickly unsubscribe everyone in bypass of the standard
			unsubscription mechanism provided by ~Subscription().
			It is NOT necessary to call this in 99% of cases.

			One use case is:
			1. You anticipate many subscriptions to the event
			2. The event outlives the subscriptions
			3. There is bulk destruction of the subscriptions right
				before the event itself is destroyed anyway

			Such conditions can be encountered at program shutdown
			or when unloading [large] object graphs. This function
			allows to speed up the shutdown/unloading of such object
			graphs with "heavy" events.

			An alternative shutdown optimization for heavy events is
			to make sure they don't outlive their subscriptions.

			NDEBUG complexity: O(Event::count()) */
			void clear()
			{
				for (std::ptrdiff_t i = 0; i!=std::ssize(_records); ++i)
					_records[i].releaseOwnership();

				_records.clear();
			}

			/* Subscriptions are allowed to outlive the Event.

			NDEBUG complexity: O(Event::count()) */
			~Event() override
			{
				clear();
			}

			/* Notify all subscribers, i.e., invoke all their callbacks and pass
			 them the given arguments @args.

			The order of invocation of subscribed callbacks relative to each other
			is unspecified.

			NDEBUG complexity: O(Event::count())
			*/
			MSVC_SUPPRESS_WARNING_WITH_PUSH(26800) //use of a moved object
			void raise(ClassArgs...args)
			{
				for (std::ptrdiff_t i = 0; i!=std::ssize(_records); ++i)
					_records[i]._delegate.invoke(std::forward<ClassArgs>(args)...);

				/*for(std::ptrdiff_t i = 0; i<std::ssize(_records); ++i)
					_records[i]._delegate.invoke(std::forward<Args>(args)...);*/

				/*for(std::ptrdiff_t i = std::ssize(_records)-1; i>=0; --i)
					_records[i]._delegate.invoke(std::forward<Args>(args)...);*/

				/*for(SubscriptionRecordT& r : _records)
					r._delegate.invoke(std::forward<Args>(args)...);*/
			}
			MSVC_SUPPRESS_WARNING_POP

			// the same as .raise(...)
			void operator()(ClassArgs...args)
			{
				raise(std::forward<ClassArgs>(args)...);
			}

			/* Subscribe @callback to the Event.

			The function returns a Subscription object. Keep this object alive
			for as long as you need @callback to be subscribed. Subscriptions
			are allowed to outlive the Event.

			@callback is of type CallMe::Delegate<...>. Use constructors of
			CallMe::Delegate<...> or delegate factories to make @callback,
			see unit tests for examples.

			As only non-owning delegates are accepted as @callback, make sure
			that all referenced callable targets are alive/valid for the
			lifetime of the returned Subscription object.

			The signature of @callback must match the signature of the Event.

			If @callback accepts parameters, usually it should not mutate them -
			accepting arguments by value or by const-reference is recommended.
			If @callback mutates its parameters, carefully think out how that
			will interact with callbacks of other subscriptions
			(the order of invocation of subscribed callbacks is unspecified).

			NDEBUG complexity:
			* If Event has enough allocated space: O(1)
			* Otherwise: reallocation + O(Event::count())
			*/
			[[nodiscard]] auto subscribe(DelegateT&& callback)
			{
				_records.emplace_back(std::move(callback));
				return Subscription(_records.size()-1, this);
			}

			#define MsgEventCallbackMismatch "Callback signature mismatches the signature of Event"

			template<typename MismatchingSignature>
				requires (not std::same_as<MismatchingSignature, Signature>)
			DELETE_FUNCTION(auto subscribe(Delegate<MismatchingSignature>&&),
							MsgEventCallbackMismatch)


			/* Subscribe @callback to the Event, see the other overload.

			The function saves a Subscription object in @dst.

			Customize the type of subscription vector @dst by passing a @dst
			of any vector-like container type such as
			gch::small_vector<Subscription, nSubscriptions>, see unit tests
			for examples.

			NDEBUG complexity:
			* If both Event and @dst have enough allocated space: O(1)
			* Otherwise, up to two reallocations, whatever triggers:
				* reallocation + O(Event::count()),
				* reallocation + O(@dst.count())
			*/
			template<typename VectorOfSubscriptions = std::vector<Subscription>>
			void subscribe(DelegateT&& callback,
						   VectorOfSubscriptions& dst)
			{
				_records.emplace_back(std::move(callback));
				dst.emplace_back(_records.size()-1, this);
			}

			template<typename MismatchingSignature, typename VectorOfSubscriptions>
				requires (not std::same_as<MismatchingSignature, Signature>)
			DELETE_FUNCTION(void subscribe(Delegate<MismatchingSignature>&&, 
										   VectorOfSubscriptions&),
							MsgEventCallbackMismatch)

			// the number of current subscriptions 
			[[nodiscard]] std::ptrdiff_t count() const
			{
				return std::ssize(_records);
			}

			// true IFF there are currently no subscriptions
			[[nodiscard]] bool empty() const
			{
				return _records.empty();
			}
		};
	}

	/* Event (aka "multicast delegate") maintains a set of subscription callbacks
	and the infrastructure for subscribing and unsubscribing the callbacks at
	runtime.

	Call Event::raise(...) or Event::operator(...) to notify/invoke all the
	callbacks.

	Specify the signature of Event as the template parameter @Signature.
	@Signature must have void return type.

	Only callbacks with the same signature can be subscribed to Event.

	Adjust the size of Event's inline(small buffer optimization) buffer
	for subscriptions by specifying the template parameter @ExpectedSubscriptions.
	Properly specifying @ExpectedSubscriptions guarantees that all Event operations
	are 1) reallocation-free 2) heap-free if the Event itself is stack-allocated.
	Also see .reserve(...).
	*/
	template<typename Signature = void(),
		unsigned ExpectedSubscriptions = internal::ExpectedSubscriptionsDefault>
	class Event : public internal::Event<ExpectedSubscriptions, Signature>
	{
		using BaseT = internal::Event<ExpectedSubscriptions, Signature>;

	public:
		explicit Event() : BaseT()
		{
		}

		/* Immediately allocate memory for @expectedSubscriptions.
		See .reserve(...) */
		explicit Event(unsigned expectedSubscriptions) :
			BaseT(expectedSubscriptions)
		{
		}
	};

	Event() -> Event<void()>;

	/* RAII-wrapper for managing the lifetime of a subscription.
	One Subscription object represents one delegate object
	(subscriber's callback) registered in an Event. 
	*/
	class Subscription
	{
		template<unsigned, typename>
		friend class internal::Event;

		template<typename>
		friend class internal::SubscriptionRecord;

		//the index of the owned subscription record
		internal::SubscriptionIndex _index;

		/* whenever _event != nullptr, the subscription is "attached"
		to the event and owns the respective subscription record in
		the event. Otherwise, the subscription is "detached" from the
		event (e.g. after having been moved from) and does not own
		anything. */
		internal::ErasedEvent viewptr _event;

		void releaseOwnership()
		{
			_event = nullptr;

			#ifndef NDEBUG
			_index = internal::InvalidSubscriptionIndex;
			#endif
		}

		void unsubscribe()
		{
			if(_event)
			{
				_event->unsubscribe(_index);
				releaseOwnership();
			}
		}

	public:
		DELETE_FUNCTION(Subscription(), 
						"For optional subscriptions, use std::optional<Subscription>")		 

		//only for internal use
		Subscription(internal::SubscriptionIndex index, 
					 internal::ErasedEvent viewptr event) noexcept :
			_index(index),
			_event(event)
		{
			_event->changeOwner(_index, this);
			_event->validate();
		}

		/* Unregister/unsubscribe from the Event.
		NDEBUG complexity: O(1) */
		~Subscription()
		{
			unsubscribe();
		}

		Subscription(const Subscription& src)            = delete;
		Subscription& operator=(const Subscription& src) = delete;

		Subscription(Subscription&& src) noexcept :
			_index(src._index),
			_event(src._event)
		{
			if(_event)
			{
				/* subscription records remain unchanged, just
				src's owned record is now owned by [this] instead
				of src */
				_event->changeOwner(_index, this);
				src.releaseOwnership();
				_event->validate();
			}
		}

		Subscription& operator=(Subscription&& src) noexcept
		{
			if (this == &src)
				return *this;

			if(src._event)
			{
				if(_event)
				{
					/* [src]'s delegate is moved to [this]'s subscription
					record. The record stays in its place (_index unchanged)
					and is still owned by [this]. src's subscription record
					is no longer needed and is removed from _event by
					unsubscribing, after which src no longer owns anything. */
					_event->moveDelegate(src._index, this->_index);
					src.unsubscribe();
				}
				else//[this] is default-constructed or moved-from
				{
					//[this] acquires ownership of the src's subscription record
					this->_index = src._index;
					this->_event = src._event;

					//redirect src's record to the new owner
					_event->changeOwner(src._index, this);

					//src no longer owns anything
					src.releaseOwnership();
				}
				_event->validate();
			}
			else
			{
				//[this] is overwritten by an empty subscription
				unsubscribe();
			}

			return *this;
		}

		//alternative implementation, slightly worse performance
		/* Subscription& operator=(Subscription&& src) noexcept
		{
			if (this == &src)
				return *this;

			//remove the current subscription record owned by [this]
			this->unsubscribe();

			if(src._event)
			{	
				//[this] acquires ownership of the src's subscription record
				this->_index = src._index;
				this->_event = src._event;

				//redirect src's record to the new owner
				_event->changeOwner(src._index, this);

				//src no longer owns anything
				src.releaseOwnership();

				_event->validate();
			}

			return *this;
		} */

		/* Move Subscription to @dst.

		Customize the type of subscription vector @dst by passing a @dst
		of any vector-like container type such as
		gch::small_vector<Subscription, nSubscriptions>, see unit tests
		for examples.

		There is an overload of Event::subscribe(...) that accepts
		VectorOfSubscriptions& as a parameter and saves a new subscription
		there. That overload is more efficient and recommended whenever
		you want to manage the lifetime of a new subscription with a
		vector of subscriptions right away.

		This overload is appropriate for the nontrivial scenario when you
		first need to manage a subscription individually, by saving it in
		a scalar variable, but later runtime logic determines that individual
		lifetime management is no longer needed and the subscription can be
		managed with a vector of subscriptions.

		NDEBUG complexity:
		* If @dst has enough allocated space: O(1)
		* Otherwise: reallocation + O(@dst.count())
		*/
		template<typename VectorOfSubscriptions = std::vector<Subscription>>
		void move(VectorOfSubscriptions& dst)
		{
			dst.push_back(std::move(*this));
		}
	};

	namespace internal
	{
		template<typename Signature>
		class SubscriptionRecord
		{
			template<unsigned, typename>
			friend class CallMe::internal::Event;

			using DelegateT = Delegate<Signature>;
			DelegateT _delegate;

			// this pointer is always updated by the Subscription
			Subscription viewptr _owner;

			void changeOwner(Subscription viewptr newOwner)
			{
				_owner = newOwner;
			}

			void changeEvent(ErasedEvent viewptr newEvent)
			{
				_owner->_event = newEvent;
			}

			void releaseOwnership()
			{
				_owner->releaseOwnership();
			}
		public:
			SubscriptionRecord(const SubscriptionRecord& src) = delete;
			SubscriptionRecord& operator=(const SubscriptionRecord& src) = delete;

			SubscriptionRecord(DelegateT&& delegate) noexcept :
				_delegate(std::move(delegate)),
				_owner(nullptr)
			{
			}

			SubscriptionRecord(SubscriptionRecord&& src) noexcept :
				_delegate(std::move(src._delegate)),
				_owner(src._owner)
			{
				/*src._delegate = DelegateT{};
				src.changeOwner(nullptr);*/
			}

			SubscriptionRecord& operator=(SubscriptionRecord&& src) noexcept
			{
				if (this == &src)
					return *this;

				this->_delegate = std::move(src._delegate);
				this->changeOwner(src._owner);

				/*src._delegate = DelegateT{};
				src.changeOwner(nullptr);*/

				return *this;
			}
		};
	}
}