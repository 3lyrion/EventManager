# Header-only C++20 simple and fast event bus

## Description

• EventManager is a singleton.

• An entity subscribes to an event by passing a pointer to itself and to a method that responds to the event. 

• Unsubscribing occurs by passing the event type and a pointer to the entity; it is also possible to unsubscribe from all events at once.

• You can publish events both for all subscribers and for a specific one.

## Example

**Creating some event classes**
```cpp
class AppEvent_Update : public AppEvent
{
public:
    AppEvent_Update() = default;
}

class AppEvent_Tick : public AppEvent
{
public:
    float deltaTime;

    AppEvent_Update(float deltaTime_) : deltaTime(deltaTime_) { }
}
```

**Subscribing and unsubscribing**
```cpp
class EventReceiver
{
public:
    virtual ~EventReceiver()
    {
        EM.unsubscribeAll(this);
    }
	
protected:
    EventManager& EM; // EventManager is a singleton

    EventReceiver() : EM(EventManager.get())
    {
        EM.subscribe(this, &EventReceiver::onTick);
    }

    virtual void onTick(AppEvent_Tick const& aEvent)
    {
        // ...
    }
};

class Actor : public EventListener
{
    Actor()
    {
        EM.subscribe(this, &Actor::onUpdate);

        EM.unsubscribe<AppEvent_Tick>(this);
    }

private:
    void onUpdate()
    {
        // ...

        if (/* requires application exit inside event response */)
          aEvent.handled = true; // handled is mutable
    }
};
```

**Publishing**
```cpp
Actor actor1;
Actor actor2;

auto& EM = EventManager.get();         // or you can use the EVENT_MANAGER_GET macro

EM.publish(AppEvent_Tick(0.33f));      // all subscribers will be notified

EM.signal(actor2, AppEvent_Update());  // only actor2 will be notified
```

## License

This project is under MIT License.

Copyright (c) 2024 Mouseunder

> Permission is hereby granted, free of charge, to any person obtaining a copy  
> of this software and associated documentation files (the "Software"), to deal  
> in the Software without restriction, including without limitation the rights  
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell  
> copies of the Software, and to permit persons to whom the Software is  
> furnished to do so, subject to the following conditions:  
> 
> 
> The above copyright notice and this permission notice shall be included in all  
> copies or substantial portions of the Software.  
> 
> 
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,  
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER  
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  
> SOFTWARE.
