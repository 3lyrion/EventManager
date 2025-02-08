# Header-only C++20 simple and fast event bus

## Description

- EventManager is a singleton.

- All events must inherit the EventBase class. Due to this, a mechanism is provided to stop notifying the remaining receivers.

- All event receivers must inherit the EventReceiver class. When the event receiver is destroyed, he automatically unsubscribes from all events.

- The event receiver subscribes to the event by passing a reference to itself and a pointer to the method that handles the event, or the corresponding lambda expression. 

- Unsubscribing occurs by passing the event type and a pointer to the event receiver; it is also possible to unsubscribe from all events at once.

- You can schedule an urgent action which is a one-time callback for the next (any) event or an event action which is a one-time callback for a specific type of event.

## Example

```cpp
#define self *this

class App
{
public:
    class E_Update : public el::EventBase
    {
    public:
        explicit E_Update() = default;
    }

    class E_Tick : public el::EventBase
    {
    public:
        float deltaTime;

        explicit E_Tick(float theDeltaTime) :
            deltaTime(theDeltaTime) { }
    }

    void run()
    {
        auto& EM = el::EventManager::get(); // or you can use the EVENT_MANAGER_GET macro

        // ...

        EM.publish(E_Update());

        // ...s

        EM.publish(E_Tick(dt));

        // ...
    }
}

class MyReceiver : el::EventReceiver
{
protected:
    MyReceiver()
    {
        // EM is inherited
        EM.subscribe(self, &MyReceiver::onUpdate);
    }

    virtual ~MyReceiver() = default;

    virtual void onUpdate(App::E_Update const& e)
    {
        // ...
    }
};

class Actor : public MyReceiver
{
    Actor()
    {
        EM.subscribe(self, &Actor::onTick);

        EM.unsubscribe<App::E_Tick>(self);
    }

private:
    void onUpdate(E_Update const& e)
    {
        // ...

        if (/* requires stopping informing other receivers */)
            e.handled = true; // handled is mutable
    }
};
```

## License

This project is under MIT License.

Copyright (c) 2025 3lyrion

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
