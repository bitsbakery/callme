#include <optional>
#include <memory>

#include "doctest.h"

#define GCH_DISABLE_CONCEPTS
#include "small_vector.h"

#include "CallMe.Event.h"


using namespace CallMe;

struct Subscriber
{
	Subscriber(const Subscriber& other)                = delete;
	Subscriber(Subscriber&& other) noexcept            = delete;
	Subscriber& operator=(const Subscriber& other)     = delete;
	Subscriber& operator=(Subscriber&& other) noexcept = delete;

	Subscriber() = default;

	void notify()
	{
		++_timesNotified;
	}

	void checkNotifiedTotal(int times)
	{
		CHECK(_timesNotified == times);
	}

	void takeSnapshot()
	{
		_timesNotifiedSnapshot = _timesNotified;
	}

	void checkNotified(int nTimes)
	{
		CHECK(_timesNotified == _timesNotifiedSnapshot + nTimes);
	}

	void checkUnnotified()
	{
		CHECK(_timesNotified == _timesNotifiedSnapshot);
	}

	std::optional<Subscription> subscription;

private:

	int _timesNotified { 0 };
	int _timesNotifiedSnapshot { 0 };
};

/* how many notifications are expected on the
	Subscriber since the most recent snapshot */
struct count
{
	Subscriber* s;
	int nTimes;

	count(Subscriber* s, int nTimes = 1) :
		s(s), nTimes(nTimes)
	{
	}
};

template<typename T>
concept SubscriberPtrOrCount =
	std::same_as<T, Subscriber*> or std::same_as<T, count>;

struct SubscriberList
{
	std::vector<count> subscribers;

	template<SubscriberPtrOrCount...Subs>
	explicit SubscriberList(Subs...subscribers) :
		subscribers({subscribers...})
	{
	}

	SubscriberList() = default;
};

using notified = SubscriberList;
using unnotified = SubscriberList;

template<typename Signature, unsigned N>
void Check(Event<Signature,N>& e, notified&& n, unnotified&& un = {})
{
	for (count& ns : n.subscribers)
		ns.s->takeSnapshot();
	for (count& us : un.subscribers)
		us.s->takeSnapshot();

	e.raise();

	for (count& ns : n.subscribers)
		ns.s->checkNotified(ns.nTimes);
	for (count& us : un.subscribers)
		us.s->checkUnnotified();
}

inline auto makeCallback(Subscriber& s)
{
	return fromMethod<&Subscriber::notify>(s);
}

TEST_SUITE("event tests")
{
	TEST_CASE("empty event may be raised") {
		Event event;
		event.raise();
	}

	TEST_CASE("subscribe/unsubscribe, 1 subscriber") {
		Event event;
		Subscriber alice;

		SUBCASE("unsubscribed instantly") {
			{				
				auto subscription = event.subscribe(makeCallback(alice));
			}
			Check(event, notified(), unnotified(&alice));
		}

		SUBCASE("unsubscribed after raise") {
			{
				auto subscription = event.subscribe(makeCallback(alice));
				Check(event, notified(&alice), unnotified());
			}
			//not notified after unsubscription
			Check(event, notified(), unnotified(&alice));
		}
	}

	TEST_CASE("subscribe/unsubscribe, 2 subscribers") {
		Event event;
		Subscriber alice;
		Subscriber bob;

		SUBCASE("LIFO unsubscription") {
			{				
				auto subA = event.subscribe(makeCallback(alice));
				{
					auto subB = event.subscribe(makeCallback(bob));
					Check(event, notified(&alice, &bob), unnotified());
				}
				//bob not notified after unsubscription
				Check(event, notified(&alice), unnotified(&bob));
			}
			//alice not notified after unsubscription
			Check(event, notified(), unnotified(&alice, &bob));
		}

		SUBCASE("FIFO unsubscription") {
			{
				std::optional subA(event.subscribe(makeCallback(alice)));
				auto subB = event.subscribe(makeCallback(bob));
				Check(event, notified(&alice, &bob), unnotified());
			
				subA.reset();

				//alice not notified after unsubscription
				Check(event, notified(&bob), unnotified(&alice));
			}
			//bob not notified after unsubscription
			Check(event, notified(), unnotified(&alice, &bob));
		}
	}

	TEST_CASE("double subscription notifies twice") {
		Event      event;
		Subscriber alice;
		auto       subscriptionA = event.subscribe(makeCallback(alice));
		auto       subscriptionB = event.subscribe(makeCallback(alice));
		Check(event, notified(count{&alice,2}), unnotified());
	}

	
	Subscription makeEmptySubscription()
	{
		Event event;
		Subscriber some;
		auto s = event.subscribe(makeCallback(some));
		Subscription s2 = std::move(s);//s is empty now

		MSVC_SUPPRESS_WARNING_WITH_PUSH(26800) //use of a moved object
		return s;
		MSVC_SUPPRESS_WARNING_POP
	}

	TEST_CASE("subscription/move") {
		Event event;
		Subscriber alice;
		Subscriber bob;
		
		SUBCASE("move-assign to an empty subscription"){
			Subscription empty = makeEmptySubscription();
			
			empty = event.subscribe(makeCallback(alice));	
			Check(event, notified(&alice), unnotified());
		}
		SUBCASE("move-assign empty subscription to a non-empty one"){
			alice.subscription = event.subscribe(makeCallback(alice));	
			Check(event, notified(&alice), unnotified());
			
			Subscription empty = makeEmptySubscription();

			alice.subscription = std::move(empty);
			Check(event, notified(), unnotified(&alice));
		}
		SUBCASE("move-assign empty subscription to another empty one"){
			Subscription empty1 = makeEmptySubscription();
			Subscription empty2 = makeEmptySubscription();
			empty1 = std::move(empty2);
		}
		SUBCASE("move-construct from an empty subscription"){
			Subscription empty = makeEmptySubscription();

			Subscription s(std::move(empty));
		}
		SUBCASE("move ctor, 1st moved"){
			auto subscriptionA = event.subscribe(makeCallback(alice));
			auto subscriptionB = event.subscribe(makeCallback(bob));
			Check(event, notified(&alice, &bob), unnotified());
			{
				auto subscriptionA2(std::move(subscriptionA));

				//alice notified through a moved subscription 
				Check(event, notified(&alice, &bob), unnotified());
			}
			Check(event, notified(&bob), unnotified(&alice));
		}
		SUBCASE("move ctor, 2nd moved"){
			auto subscriptionA = event.subscribe(makeCallback(alice));
			auto subscriptionB = event.subscribe(makeCallback(bob));
			Check(event, notified(&alice, &bob), unnotified());
			{
				auto subscriptionB2(std::move(subscriptionB));

				//alice notified through a moved subscription 
				Check(event, notified(&alice, &bob), unnotified());
			}
			Check(event, notified(&alice), unnotified(&bob));
		}
		SUBCASE("move="){
			SUBCASE("inner overwritten"){
				auto subscriptionA = event.subscribe(makeCallback(alice));
				{
					auto subscriptionB = event.subscribe(makeCallback(bob));
					Check(event, notified(&alice, &bob), unnotified());
					{
						//bob's subscription gets overwritten by alice's one
						subscriptionB = std::move(subscriptionA);
						CHECK(event.count()==1);
					
						//alice is notified through the moved subscription, bob has lost his subscription
						Check(event, notified(&alice), unnotified(&bob));
					}
				}
				Check(event, notified(), unnotified(&alice,&bob));
			}
			SUBCASE("outer overwritten"){
				{
					auto subscriptionA = event.subscribe(makeCallback(alice));
					{
						auto subscriptionB = event.subscribe(makeCallback(bob));
						Check(event, notified(&alice, &bob), unnotified());
						{
							//alice's subscription gets overwritten by bob's one
							subscriptionA = std::move(subscriptionB);
							CHECK(event.count()==1);

							//bob is notified through the moved subscription, alice has lost her subscription
							Check(event, notified(&bob), unnotified(&alice));
						}
					}
					//subscriptionA is still alive here
					Check(event, notified(&bob), unnotified(&alice));
				}
				Check(event, notified(), unnotified(&alice,&bob));
			}
			SUBCASE("&src == &dst"){
				{
					auto subscriptionA = event.subscribe(makeCallback(alice));
					CLANG_SUPPRESS_WARNING_WITH_PUSH("-Wself-move")
					subscriptionA = std::move(subscriptionA);
					CLANG_SUPPRESS_WARNING_POP
					Check(event, notified(&alice), unnotified());
				}
				Check(event, notified(), unnotified(&alice));
			}
		}
	}

	GCC_SUPPRESS_WARNING_WITH_PUSH("-Wmaybe-uninitialized")
	TEST_CASE("subscription is allowed to outlive event") {
        Subscriber alice;
		std::optional<Event<>> event(std::in_place);
		auto subscription = event.value().subscribe(makeCallback(alice));
        event.reset();
	}
	GCC_SUPPRESS_WARNING_POP

	TEST_CASE("vector of subscriptions") {
		constexpr int nSubs = 10;

		//allocate all subscription records on the stack in the event itself
		Event<void(),nSubs> event; 
		
		Subscriber alice;
		{
			std::vector<Subscription> stdVectorSubscriptions;
			stdVectorSubscriptions.reserve(nSubs);

			std::vector<Subscription> stdVectorSubscriptions2;
			stdVectorSubscriptions2.reserve(nSubs);

			gch::small_vector<Subscription, nSubs> inlineSubscriptions;
			gch::small_vector<Subscription, nSubs> inlineSubscriptions2;

			for(int i = nSubs; i; --i)
			{
				//test different ways of subscribing
				event.subscribe(makeCallback(alice)).move(stdVectorSubscriptions);
				event.subscribe(makeCallback(alice)).move(inlineSubscriptions);
				event.subscribe(makeCallback(alice), stdVectorSubscriptions2);
				event.subscribe(makeCallback(alice), inlineSubscriptions2);
			}
			Check(event, notified(count{&alice,4*nSubs}), unnotified());

			CHECK(stdVectorSubscriptions.size() == nSubs);
			CHECK(stdVectorSubscriptions2.size() == nSubs);
			CHECK(inlineSubscriptions.size() == nSubs);
			CHECK(inlineSubscriptions2.size() == nSubs);
		}
		Check(event, notified(), unnotified(&alice));
	}

#ifdef NDEBUG
	template<typename Signature, unsigned N>
	void TestHeavyEvent(Event<Signature, N>* e, int nSubs)
	{
		Subscriber alice;
		{
			std::vector<Subscription> subscriptions;
			subscriptions.reserve(nSubs);

			for(int i = nSubs; i; --i)
				e->subscribe(makeCallback(alice), subscriptions);

			e->raise();
			alice.checkNotifiedTotal(nSubs);

			e->clear();//optional shutdown optimization
		}
		e->raise();
		alice.checkNotifiedTotal(nSubs);
	}
	TEST_CASE("large number of subscriptions") {
		constexpr int nSubs = 100000;
		{
			/* Too many subscriptions, cannot allocate on the stack.
			But we can reserve memory [on the heap] to avoid reallocation
			while subscribing */
			Event stackEvent(nSubs);
			TestHeavyEvent(&stackEvent, nSubs);
		}
		{
			/* heapEvent is anyway allocated on the heap, so we can
			allocate subscription records in-place in the Event itself
			for better data locality/higher cache hits and to avoid
			one unnecessary level of indirection */
			auto heapEvent = std::make_unique<Event<void(),nSubs>>();
			TestHeavyEvent(heapEvent.get(), nSubs);
		}
	}
#endif

	TEST_CASE("subscriptions are signature-agnostic") {

		//one vector holds subscriptions from events
		//with different signatures
		std::vector<Subscription> subscriptions;

		Event<void()> event1;
		event1.subscribe(fromFunctor([]() {})).move(subscriptions);
		event1();

		Event<void(int)> event2;
		event2.subscribe(fromFunctor([](int) { })).move(subscriptions);
		event2.subscribe(fromFunctor([](int) { })).move(subscriptions);
		event2(1);

		Event<void(std::string&,int)> event3;
		event3.subscribe(fromFunctor([](std::string&, int) {})).move(subscriptions);
		std::string s;
		event3(s, 1);
	}

	TEST_CASE("event move") {

		using EventT = Event<void()>;
		std::optional<EventT> eventSrc { std::in_place };
		Subscriber alice;

		std::optional<Subscription> subA {
			std::in_place, eventSrc.value().subscribe(makeCallback(alice)) };
	
		Check(eventSrc.value(), notified(&alice), unnotified());

		SUBCASE("move ctor"){
			std::optional eventDst {std::move(eventSrc)};

			//move-source does not notify alice anymore
			MSVC_SUPPRESS_WARNING_WITH_PUSH(26800) //use of a moved object
			Check(eventSrc.value(), notified(), unnotified(&alice));
			MSVC_SUPPRESS_WARNING_POP
			CHECK(eventSrc.value().empty());

			//move-destination notifies alice
			Check(eventDst.value(), notified(&alice), unnotified());
			CHECK(eventDst.value().count()==1);

			SUBCASE("dtor order: eventDst/eventSrc/subA")
			{
				eventDst.reset();
				eventSrc.reset();
				subA.reset();
			}
			SUBCASE("dtor order: eventSrc/eventDst/subA")
			{
				eventSrc.reset();
				eventDst.reset();
				subA.reset();
			}
			SUBCASE("dtor order: subA/eventSrc/eventDst")
			{
				subA.reset();
				eventSrc.reset();
				eventDst.reset();
			}
		}
		SUBCASE("move="){
			Subscriber bob;

			std::optional<EventT> eventDst { std::in_place };

			std::optional<Subscription> subB { std::in_place,
				eventDst.value().subscribe(makeCallback(bob)) };

			Check(eventDst.value(), notified(&bob), unnotified());

			eventDst = std::move(eventSrc);

			//move-source does not notify alice anymore
			MSVC_SUPPRESS_WARNING_WITH_PUSH(26829) //empty optional unwrapped
			Check(eventSrc.value(), notified(), unnotified(&alice));
			MSVC_SUPPRESS_WARNING_POP

			CHECK(eventSrc.value().empty());

			//move-destination notifies alice, but no longer does it notify bob
			Check(eventDst.value(), notified(&alice), unnotified(&bob));
			CHECK(eventDst.value().count()==1);

			SUBCASE("dtor order: eventDst/eventSrc/SubA/SubB")
			{
				eventDst.reset();
				eventSrc.reset();
				subA.reset();
				subB.reset();
			}
			SUBCASE("dtor order: eventSrc/eventDst/SubB/SubA")
			{
				eventSrc.reset();
				eventDst.reset();
				subB.reset();
				subA.reset();
			}
			SUBCASE("dtor order: SubB/SubA/eventSrc/eventDst")
			{
				subB.reset();
				subA.reset();
				eventSrc.reset();
				eventDst.reset();
			}
			SUBCASE("dtor order: SubA/SubB/eventSrc/eventDst")
			{
				subA.reset();
				subB.reset();
				eventSrc.reset();
				eventDst.reset();
			}
			SUBCASE("dtor order: eventDst/SubA/SubB/eventSrc")
			{
				eventDst.reset();
				subA.reset();
				subB.reset();
				eventSrc.reset();
			}
		}
	}

	TEST_CASE("event move/empty events") {
		SUBCASE("move-construct from an empty event"){
			Event eventSrc;
			Event eventDst(std::move(eventSrc));

			MSVC_SUPPRESS_WARNING_WITH_PUSH(26800) //use of a moved object
			eventSrc.raise();
			MSVC_SUPPRESS_WARNING_POP

			eventDst.raise();
		}
		SUBCASE("move-assign an empty event to an empty event"){
			Event eventSrc;
			Event eventDst;
			eventDst = std::move(eventSrc);
			
			MSVC_SUPPRESS_WARNING_WITH_PUSH(26800) //use of a moved object
			eventSrc.raise();
			MSVC_SUPPRESS_WARNING_POP

			eventDst.raise();
		}
	}
}