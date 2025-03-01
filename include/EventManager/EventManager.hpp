﻿/*

███████╗██╗░░░██╗███████╗███╗░░██╗████████╗███╗░░░███╗░█████╗░███╗░░██╗░█████╗░░██████╗░███████╗██████╗░
██╔════╝██║░░░██║██╔════╝████╗░██║╚══██╔══╝████╗░████║██╔══██╗████╗░██║██╔══██╗██╔════╝░██╔════╝██╔══██╗
█████╗░░╚██╗░██╔╝█████╗░░██╔██╗██║░░░██║░░░██╔████╔██║███████║██╔██╗██║███████║██║░░██╗░█████╗░░██████╔╝
██╔══╝░░░╚████╔╝░██╔══╝░░██║╚████║░░░██║░░░██║╚██╔╝██║██╔══██║██║╚████║██╔══██║██║░░╚██╗██╔══╝░░██╔══██╗
███████╗░░╚██╔╝░░███████╗██║░╚███║░░░██║░░░██║░╚═╝░██║██║░░██║██║░╚███║██║░░██║╚██████╔╝███████╗██║░░██║
╚══════╝░░░╚═╝░░░╚══════╝╚═╝░░╚══╝░░░╚═╝░░░╚═╝░░░░░╚═╝╚═╝░░╚═╝╚═╝░░╚══╝╚═╝░░╚═╝░╚═════╝░╚══════╝╚═╝░░╚═╝

Header-only C++20 simple and fast event bus https://github.com/3lyrion/EventManager

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
Copyright (c) 2025 3lyrion

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files( the "Software" ), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

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

#define EVENT_MANAGER_GET auto& EM = el::EventManager::get

#include <functional>
#include <memory>
#include <list>
#include <unordered_map>
#include <typeindex>

namespace el
{

using Action = std::function<void()>;

class EventBase
{
public:
	// Stop notifying the remaining receivers
	mutable bool handled = false;

	virtual ~EventBase() = default;

protected:
	EventBase() = default;
};

class EventReceiver;

template <typename T>
concept DerivedFromEventBase = std::derived_from<T, EventBase>;

template <typename T>
concept DerivedFromEventReceiver = std::derived_from<T, EventReceiver>;

namespace internal
{

	class EventHandler
	{
	public:
		EventHandler()          = default;
		virtual ~EventHandler() = default;

		virtual void handle(void* handler, EventBase const& e) const = 0;
	};

	template <DerivedFromEventReceiver R, DerivedFromEventBase E>
	class MethodEventHandler : public EventHandler
	{
	public:
		using Method = void(R::*)(E const&);

		MethodEventHandler(Method method) : 
			m_method(method) { }

		void handle(void* handler, EventBase const& e) const final
		{
			(static_cast<R*>(handler)->*m_method)(static_cast<E const&>(e));
		}

	private:
		Method m_method{};
	};

	template <DerivedFromEventBase E>
	class LambdaEventHandler : public EventHandler
	{
	public:
		using Lambda = std::function<void(E const&)>;

		LambdaEventHandler(Lambda&& lambda) : 
			m_lambda(std::move(lambda)) { }

		void handle(void*, EventBase const& e) const final
		{
			m_lambda(static_cast<E const&>(e));
		}

	private:
		Lambda m_lambda{};
	};

	class EventHandlerList
	{
	public:
		using Handler = std::unique_ptr<EventHandler>;

		EventHandlerList() = default;

		inline void dispatch(EventBase const& e)
		{
			m_executing = true;     // for nested events

			for (auto handler : m_order)
			{
				if (!handler)
					continue;

				m_handlers[handler]->handle(handler, e);

				if (e.handled)
					break;
			}

			m_executing = false;   // for nested events

			cleanUp();             // for nested events
		}

		template <DerivedFromEventReceiver R, DerivedFromEventBase E>
		constexpr void add(R* receiver, void(R::* method)(E const&))
		{
			auto entry = m_handlers.find(receiver);
			if (entry == m_handlers.end())
			{
				m_handlers[receiver] = std::make_unique<MethodEventHandler<R, E> >(method);
				m_order.push_back(receiver);
			}
		}

		template <DerivedFromEventBase E>
		constexpr void add(void* receiver, std::function<void(E const&)>&& lambda)
		{
			auto entry = m_handlers.find(receiver);
			if (entry == m_handlers.end())
			{
				m_handlers[receiver] = std::make_unique<LambdaEventHandler<E> >(std::move(lambda));
				m_order.push_back(receiver);
			}
		}

		inline void remove(void* receiver)
		{
			auto entry = m_handlers.find(receiver);
			if (entry == m_handlers.end())
				return;

			m_handlers.erase(entry);

			if (m_executing)  // for nested events
			{
				m_needsCleanUp = true;

				auto it = std::find(m_order.begin(), m_order.end(), receiver);
				*it = nullptr;
			}

			else
				m_order.remove(receiver);
		}

		inline void clear()
		{
			m_handlers .clear();
			m_order    .clear();
		}

	private:
		std::unordered_map<void*, Handler>	m_handlers;
		std::list<void*>					m_order;

		bool m_executing{};
		bool m_needsCleanUp{};

		inline void cleanUp()
		{
			if (!m_needsCleanUp)
				return;

			m_needsCleanUp = false;

			m_order.remove_if(
				[](auto handler)
				{
					return !handler;
				}
			);
		}
	};

	class UrgentActionList
	{
	public:
		inline void add(Action&& action)
		{
			if (m_executing)
				m_buffer.push_back(std::move(action));

			else
				m_actions.push_back(std::move(action));
		}

		inline void exec()
		{
			m_executing = true;

			m_actions.splice(m_actions.end(), std::move(m_buffer));
			m_buffer.clear();

			if (!m_actions.empty())
			{
				for (const auto& action : m_actions)
					action();

				m_actions.clear();
			}

			m_executing = false;
		}

	private:
		using ActionList = std::list<Action>;

		bool		m_executing{};
		ActionList	m_actions;
		ActionList	m_buffer;
	};

	class EventActionList
	{
	public:
		inline void add(type_info const& tid, Action&& action)
		{
			if (m_executing)
				m_buffer[&tid].push_back(std::move(action));

			else
				m_actions[&tid].push_back(std::move(action));
		}

		inline void exec(type_info const& tid)
		{
			m_executing = true;

			if (!m_buffer.empty())
			{
				m_actions.merge(std::move(m_buffer));
				m_buffer.clear();
			}

			auto entry = m_actions.find(&tid);
			if (entry != m_actions.end())
			{
				auto& actions = entry->second;
				if (!actions.empty())
				{
					for (const auto& action : actions)
						action();

					actions.clear();
				}
			}

			m_executing = false;
		}

	private:
		using ActionList	= std::list<Action>;
		using EventActions	= std::unordered_map<type_info const*, ActionList>;

		bool			m_executing{};
		EventActions	m_actions;
		EventActions	m_buffer;
	};
		
} // namespace internal

class EventManager
{
public:
	inline static EventManager& get()
	{
		static EventManager instance;
		return instance;
	}

	EventManager(const EventManager&)              = delete;
	EventManager& operator = (const EventManager&) = delete;

	// Publish an event to all subscribers
	template <DerivedFromEventBase EventType>
	constexpr void publish(EventType&& e)
	{
		auto& tid = typeid(EventType);

		m_urgentActions.exec();

		{
			auto entry = m_subscriptions.find(tid);
			if (entry != m_subscriptions.end())
				entry->second.dispatch(e);
		}

		m_eventActions.exec(tid);
	}

	template <DerivedFromEventBase EventType>
	constexpr void schedule(Action&& action)
	{
		m_eventActions.add(typeid(EventType), std::move(action));
	}

	inline void schedule(Action&& urgentAction)
	{
		m_urgentActions.add(std::move(urgentAction));
	}

	// Subscribe to event
	template <typename Receiver, DerivedFromEventBase EventType>
	constexpr void subscribe(EventReceiver& receiver, void(Receiver::* method)(EventType const&))
	{
		auto receiver_ptr = &receiver;

		m_subscriptions[typeid(EventType)].add(static_cast<Receiver*>(receiver_ptr), method);
		(m_subsCount[receiver_ptr])++;
	}

	// Subscribe to event
	template <DerivedFromEventBase EventType>
	constexpr void subscribe(EventReceiver& receiver, std::function<void(EventType const&)>&& action)
	{
		auto receiver_ptr = &receiver;

		m_subscriptions[typeid(EventType)].add<EventType>(receiver_ptr, std::move(action));
		(m_subsCount[receiver_ptr])++;
	}

	// Unsubscribe from a specific event
	template <DerivedFromEventBase EventType>
	constexpr void unsubscribe(EventReceiver& receiver)
	{
		auto receiver_ptr = &receiver;

		auto sc_entry = m_subsCount.find(receiver_ptr);
		if (sc_entry == m_subsCount.end())
			return;

		auto _s_entry = m_subscriptions.find(typeid(EventType));
		if (_s_entry != m_subscriptions.end())
		{
			_s_entry->second.remove(receiver_ptr);
			sc_entry->second--;

			if (!sc_entry->second)
				m_subsCount.erase(sc_entry);
		}
	}

	// Unsubscribe from all events
	inline void unsubscribeAll(EventReceiver& receiver)
	{
		auto receiver_ptr = &receiver;

		auto sc_entry = m_subsCount.find(receiver_ptr);
		if (sc_entry == m_subsCount.end() || !sc_entry->second)
			return;

		for (auto& [_, handlers] : m_subscriptions)
			handlers.remove(receiver_ptr);

		m_subsCount.erase(sc_entry);
	}

private:
	using HandlerList		= internal::EventHandlerList;
	using SubscriptionMap	= std::unordered_map<std::type_index, HandlerList>;
	using EventActionList	= internal::EventActionList;
	using UrgentActionList	= internal::UrgentActionList;

	SubscriptionMap		m_subscriptions;
	EventActionList		m_eventActions;
	UrgentActionList	m_urgentActions;

	std::unordered_map<void*, uint32_t>	m_subsCount;

	EventManager()  = default;
	~EventManager() = default;
};


class EventReceiver
{
public:
	virtual ~EventReceiver()
	{
		EM.unsubscribeAll(*this);
	}

protected:
	EventManager& EM;

	EventReceiver() : 
		EM(EventManager::get()) { }
};

} // namespace el