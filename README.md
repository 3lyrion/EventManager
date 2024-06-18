# Header-only C++20 simple and fast event bus

## Example

**Creating some event classes**
```cpp
class AppEvent_Update : public AppEvent
{
public:
    AppEvent_Update()          = default;
    virtual ~AppEvent_Update() = default;
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
        ...
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
        ...

        if (the_application_needs_to_exit)
          aEvent.handled = true; // handled is mutable
    }
};
```

**Publishing and signalling**
```cpp
Actor actor1;
Actor actor2;

auto& EM = EventManager.get(); // or you can use the EVENT_MANAGER_GET macro

EM.publish(AppEvent_Tick(0.33f));

EM.signal(actor2, AppEvent_Update());
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