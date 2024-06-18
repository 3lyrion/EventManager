/*
EventManager

Header-only C++20 simple and fast event bus https://github.com/Mouseunder/EventManager

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
Copyright (c) 2024 Mouseunder

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files( the "Software" ), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <list>
#include <unordered_map>
#include <typeindex>


// Base class for custom events
class AppEvent
{
public:
	// This variable needs to be changed when the application shuts down inside the event response
	mutable bool handled = false;

	virtual ~AppEvent() = default;

protected:
	AppEvent() = default;
};


// A namespace that hides the internal workings of the event manager
namespace internal
{
	// Base class for event handlers, allowing you to store them in a list
	class EventDispatcher
	{
	public:
		virtual ~EventDispatcher() = default;

		virtual void dispatch(AppEvent const& e) const = 0;

	protected:
		EventDispatcher() = default;
	};

	template<typename Receiver, typename EventType>
	class EventHandler : public EventDispatcher
	{
	public:
		using Method = void(Receiver::*)(EventType const&);

		EventHandler(Receiver* receiver, Method method) : 
			m_receiver (receiver), 
			m_method   (method) { }

		~EventHandler() = default;

		void dispatch(AppEvent const& e) const override final
		{
			(m_receiver->*m_method)(static_cast<EventType const&>(e));
		}

		Receiver* getReceiver() const
		{
			return m_receiver;
		}

	private:
		Receiver* m_receiver;
		Method    m_method;
	};

	// A class that stores event handlers in a list
	class EventHandlerList
	{
	public:
		EventHandlerList()  = default;
		~EventHandlerList() = default;

		inline void dispatch(AppEvent const& e)
		{
			m_executing = true;  // for nested events

			for (auto const& h : m_handlers)
			{
				if (h) h->dispatch(e);

				if (e.handled)
					break;
			}

			m_executing = false; // for nested events
			
			cleanup();           // for nested events
		}

		template<typename Receiver>
		void signal(Receiver* receiver, AppEvent const& e)
		{
			for (auto& h : m_handlers)
			{
				if (!h) continue;

				auto handler = static_cast<EventHandler<Receiver, AppEvent>*>(h);

				if (handler->getReceiver() == receiver)
				{
					h->dispatch(e);

					break;
				}
			}
		}

		inline void add(EventDispatcher* handler)
		{
			m_handlers.emplace_back(handler);
		}

		template<typename Receiver>
		void remove(Receiver* receiver)
		{
			if (m_executing) // for nested events
			{
				m_needsCleanUp = true;

				for (auto& h : m_handlers)
				{
					if (!h) continue;

					auto handler = static_cast<EventHandler<Receiver, AppEvent>*>(h);

					if (handler->getReceiver() == receiver)
					{
						h = nullptr;

						break;
					}
				}
			}

			else
			{
				for (auto hi = m_handlers.begin(); hi != m_handlers.end(); hi++)
				{
					if (!*hi) continue;

					auto handler = static_cast<EventHandler<Receiver, AppEvent>*>(*hi);

					if (handler->getReceiver() == receiver)
					{
						m_handlers.erase(hi);

						break;
					}
				}
			}
		}

	private:
		// List with event handlers
		std::list<EventDispatcher*> m_handlers;

		bool m_executing    = false; // for nested events
		bool m_needsCleanUp = false; // for nested events

		// For nested events
		inline void cleanup()
		{
			if (m_needsCleanUp)
			{
				m_needsCleanUp = false;

				std::erase_if(m_handlers, 
					[](EventDispatcher* const& h)
					{
						return (h == nullptr);
					}
				);
			}
		}
	};

} // namespace internal


#define EVENT_MANAGER_GET auto& EM = EventManager::get

// Singleton
class EventManager
{
	using Dispatcher  = internal::EventDispatcher;
	using HandlerList = internal::EventHandlerList;

public:
	EventManager(EventManager const&)              = delete;
    EventManager& operator = (EventManager const&) = delete;

	static EventManager& get()
	{
		static EventManager instance;

		return instance;
	}

	// Publish an event to all subscribers
	template<typename EventType>
	void publish(EventType&& e)
	{
		auto entry = m_subscriptions.find(typeid(EventType));

		if (entry != m_subscriptions.end())
			entry->second.dispatch(e);
	}

	// Publish an event to a specific subscriber
	template<typename Receiver, typename EventType>
	void signal(Receiver* receiver, EventType const& e)
	{
		auto entry = m_subscriptions.find(typeid(EventType));

		if (entry != m_subscriptions.end())
			entry->second.signal(receiver, e);
	}

	// Subscribe to event
	template<typename Receiver, typename EventType>
	void subscribe(Receiver* receiver, void(Receiver::* method)(EventType const&))
	{
		HandlerList& handlers = m_subscriptions[typeid(EventType)];

		handlers.add(
			new internal::EventHandler<Receiver, EventType>(receiver, method)
		);
	}

	// Unsubscribe from a specific event
	template<typename EventType, typename Receiver>
	void unsubscribe(Receiver* receiver)
	{
		auto entry = m_subscriptions.find(typeid(EventType));

		if (entry != m_subscriptions.end())
			entry->second.remove(receiver);
	}

	// Unsubscribe from all events
	template<typename Receiver>
	void unsubscribeAll(Receiver* receiver)
	{
		for (auto& [_, handlers] : m_subscriptions)
			handlers.remove(receiver);
	}

private:
	std::unordered_map<std::type_index, HandlerList> m_subscriptions;
	
	EventManager() { }
	~EventManager() { }
};