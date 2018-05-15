# CppFactory

Modern c++ object factory implementation in <200 lines :package: :factory:

![build status](https://b3ngr33ni3r.visualstudio.com/_apis/public/build/definitions/47f8d118-934e-48ed-82d8-52d850a66d71/2/badge)

I needed a simple to use, easy to mock c++ factory library (that could be used for dependecy injection) and a quick search didn't yield what I was looking for. So I built this one. Read the API docs - ([raw](./docs)) - ([hosted](https://bengreenier.github.io/CppFactory)).

## Concepts

> Some terminology you may not be familiar with, but you'll want to know.

### Object Lifecycle

In CppFactory, objects are all captured in `std::shared_ptr` wrappers, so there is no need to worry about manually deleting objects that you retrieve via the `Object<TObject>::Get()` call. However, you may choose to modify how the runtime handles object allocation to optimize for your use case.

By default, `Object`s are unmanaged, meaning when the object returned (by `Object<TObject>::Get()`) falls out of scope it is destroyed. This works great for object types that are not re-used (or if you wish to manage lifetime yourself). However, for something with a longer life (perhaps a representation of a database connection) that will be reused in many places, you may use a `GlobalObject`. `GlobalObject`s are powered by `Object`s under the hood, meaning that you can still configure allocators at the `Object` level. All `GlobalObject` provides is a lightweight caching wrapper around the `Object<TObject>::Get()` method. This looks like the following:

```
GlobalObject<TObject>::Get()
```

To clear the `GlobalObject` cache, simply call `GlobalObject<TObject>::Reset()` or (to reset only a single zone, for example `10`) `GlobalObject<TObject>::Reset<10>()`.

### Object Zones

In CppFactory, objects of the same type are all retrieved from the same factory, so it can become difficult to work with different instances of the same type. To deal with this, CppFactory provides a concept of zones.

Zones enable you to draw logical boundries throughout your codebase and to retrieve instances only for that zone. To represent this, you may pass an `int` to `Get()`, `RegisterAllocator()` and `RemoveGlobal()` as a template parameter. The int is how CppFactory represents a zone, and tracks object allocation within that zone. Here's an example:

```
Object<TObject>::Get<10>()
```

You may also use an `enum` (backed by an `int`) to represent zones, which looks like the following:

```
enum Zones
{
    ZoneOne,
    ZoneTwo
}

Object<TObject>::Get<Zones::ZoneOne>()
```

Note that you'll likely find this most useful when coupled with `GlobalObject`.

## Usage

Using constructors and destructors:

```
#include <CppFactory/CppFactory.hpp>

using namespace CppFactory;

struct Data
{
    int Value;
};

int main()
{
    // using default allocator (ctor)
    std::shared_ptr<Data> object = Object<Data>::Get();
    object->Value = 2;

    // a global object using the default allocator (ctor)
    std::shared_ptr<Data> global = GlobalObject<Data>::Get();

    return 0;
}
```

Using custom allocators and deallocators:

```
#include <CppFactory/CppFactory.hpp>

using namespace CppFactory;

struct Data
{
    int Value;
};

int main()
{
    Object<Data>::RegisterAllocator([] {
        // custom allocator
        auto ptr = (Data*)malloc(sizeof(Data));
        ptr->Value = 0;

        // custom deallocator
        return std::shared_ptr<Data>(ptr, [](Data* data) { free(data); });
    });

    // use defined allocator
    std::shared_ptr<Data> object = Object<Data>::Get();
    object->Value = 2;

    // a global object using the allocator
    std::shared_ptr<Data> global = GlobalObject<Data>::Get();

    return 0;
}
```

Using factory object pattern (old school):

```
#include <CppFactory/CppFactory.hpp>

using namespace CppFactory;

struct Data
{
    int Value;
};

int main()
{
    Factory<Data> factory;

    // use defined allocator
    std::shared_ptr<Data> object = factory.Allocate();
    object->Value = 2;

    return 0;
}
```

Using factory object pattern (custom factory):

```
#include <CppFactory/CppFactory.hpp>

using namespace CppFactory;

struct DataArgs
{
    int Value;
    int Value2;

    DataArgs(int value, int value2) : Value(value), Value2(value2) {}
};

int main()
{
    Factory<DataArgs, int, int> factory;

    // use defined allocator
    std::shared_ptr<Data> object = factory.Allocate(10, 20);
    // object->Value == 10;
    // object->Value2 == 20;

    return 0;
}
```

See [the tests](./CppFactory.UnitTests/CppFactoryTests.cpp) for more examples.

## Timing

> Note: Test runs `10000` iterations for each type below. See the code [here](./CppFactory.UnitTests/CppFactoryTests.cpp#L176).

```
Untracked, normal alloc: 34ms (0.003400ms / iteration)
Global, normal alloc: 36ms (0.003600ms / iteration)
Untracked, slow alloc: 110694ms (11.069400ms / iteration)
Global, slow alloc: 62ms (0.006200ms / iteration)
```

## License

MIT
