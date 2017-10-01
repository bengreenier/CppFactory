# CppFactory

Modern c++ object factory implementation in <200 lines :package: :factory:

![build status](https://b3ngr33ni3r.visualstudio.com/_apis/public/build/definitions/47f8d118-934e-48ed-82d8-52d850a66d71/2/badge)

I needed a simple to use, easy to mock c++ factory library (that could be used for dependecy injection) and a quick search didn't yield what I was looking for. So I built this one. Read the API docs - ([raw](./docs)) - ([hosted](https://bengreenier.github.io/CppFactory)).

## Concepts

> Some terminology you may not be familiar with, but you'll want to know.

### Object Lifecycle

In CppFactory, objects are all captured in `std::shared_ptr` wrappers, so there is no need to worry about manually deleting objects that you retrieve via the `Object<TObject>::Get()` call. However, you may choose to modify how the runtime handles object allocation to optimize for your use case.

By default, all objects are `ObjectLifecycle::Untracked` which means there is no internal management of objects, and when the object returned (by `Object<TObject>::Get()`) falls out of scope it is destroyed. This works great for object types that are not re-used. However, for something with a longer life (perhaps a representation of a database connection) you may specify `ObjectLifecycle::Global`. This tells CppFactory that you wish to keep a reference to the object inside the factory, and that on subsequent requests the factory should return the same object. This looks like the following:

```
Object<TObject, ObjectLifecycle::Global>::Get()
```

Note that you may also explicitly specify `ObjectLifecycle::Untracked`, even though it is the default.

### Object Zones

In CppFactory, objects of the same type are all retrieved from the same factory, so it can become difficult to work with different instances of the same type. To deal with this, CppFactory provides a concept of zones.

Zones enable you to draw logical boundries throughout your codebase and to retrieve instances only for that zone. To represent this, you may pass an `int` to `Get()`, `RegisterAllocator()` and `RemoveGlobal()` as a parameter. The int is how CppFactory represents a zone, and tracks object allocation within that zone. Here's an example:

```
Object<TObject>::Get(10)
```

You may also use an `enum` (backed by an `int`) to represent zones, which looks like the following:

```
enum Zones
{
    ZoneOne,
    ZoneTwo
}

Object<TObject>::Get(Zones::ZoneOne)
```

Note that you'll likely find this most useful when coupled with `ObjectLifecycle::Global`.

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

    return 0;
}
```

See [the tests](./CppFactory.UnitTests/CppFactoryTests.cpp) for more examples.

## Timing

> Note: Test runs `10000` iterations for each type below. See the code [here](./CppFactory.UnitTests/CppFactoryTests.cpp#L174).

```
Untracked, normal alloc: 26ms (0.002600s / iteration)
Global, normal alloc: 32ms (0.003200s / iteration)
Untracked, slow alloc: 105181ms (10.518100s / iteration)
Global, slow alloc: 43ms (0.004300s / iteration)
```

## License

MIT
