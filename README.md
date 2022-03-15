# code

A place to put random bits of code.

- StrongId
- Unit
- sample()

Short descriptions below; see the [wiki](wiki) for more indepth descriptions.


### StrongId

A template for making ID types that are unique types for unique uses. ie WidgetIds are not the same as EmployeeIds, and it is nice if the compiler prevents you from mixing one with the other.


    struct FooTag;
    using FooId = StrongId<int, FooTag>;
    struct BarTag;
    using BarId = StrongId<int, BarTag>;
    
    template <typename Tag>
    using StringId = StrongId<std::string, Tag>;
    
    class Cat { /*...*/ };
    class Dog { /*...*/ };
    using CatName = StringId<Cat>;
    using DogName = StringId<Dog>;
    
    void incompatibleTypes()
    {
        char test(CatName const & x);
        long test(...);

        // that test(DogName()) returns a long, not a char,
        // implies that DogName doesn't convert into a CatName const &,
        // ie this static assert checks that they are different types
        static_assert(std::is_same<long, decltype(test(DogName()))>::value);
    }

(P.S. Note that you could do a one-line version where you embed the `struct FooTag` declaration: `using FooId = StrongId<int, struct FooTag>;`. Don't do that. You can get strange ADL repercussions. There was a C++Now 2021 lightning talk about this...)

### Unit

    using Apples = Unit<int, struct ApplesTag>;
    using Oranges = Unit<int, struct OrangesTag>;
    
    Apples apples(17);
    Oranges oranges(17);
    
    apples == oranges; // does not compile - can't compare Apples and Oranges
    // also can't add them, multiply them, etc
    // but can scale them:
    apples = apples * 3; // 3 times as many apples!
    // and add, etc
    apples = apples + apples; // moar apples

    int f(Apples);
    int x = f(17); // doesn't compile - no conversion
    int y = f(oranges); // doesn't compile - no conversion
    
For units you want to be able to mix, like radians and degrees:

    struct Radians;
    struct Degrees;

    struct Radians : UnitBase<double, Radians>
    {
        explicit Radians(double t) : UnitBase(t) {}
        inline /*implicit*/ Radians(Degrees d);
    };
    struct Degrees : UnitBase<double, Degrees>
    {
        explicit Degrees(double t) : UnitBase(t) {}
        inline /*implicit*/ Degrees(Radians r);
    };

    inline /*implicit*/ Degrees::Degrees(Radians r) : UnitBase(r.get() * (180 / std::math::pi)) {}
    inline /*implicit*/ Radians::Radians(Degrees d) : UnitBase(d.get() * (std::math::pi / 180)) {}

Also note you never need to overload on Radians/Degrees, just pick one, and the other "just works":

   
    void setLaunchAngle(Radians angle);
    Radians calcLaunchAngle();
    
    void f()
    {
        // this function works in Degrees, and doesn't care what calcLaunchAngle and setLaunchAngle work with
        Degrees angle = calcLaunchAngle();
        if (closeTo(angle, 90)) ask("Angle is almost straight up, things that go up must come down. Are you sure?");
        setLaunchAngle(angle);
    }

It is tempting to do

    using Angle = Radians;
    
    void setLaunchAngle(Angle angle);
    
Which works great for function params, but it also means someone will do

    Angle angle{3.14159265359};

which leaves us back to making assumptions that are not clearly visible in the code. :-(  
(so don't do that)


### sample()

Similar to C++17 `std::sample()` but I don't have C++17 yet.  And I like the `out()`function better than an output iterator.

This is one of the functions in sample.h, but it also is an example of how to use the generic `sample(beg,end,count,urng,out)` function.

    template<typename T, typename Transform>
    std::vector<T> sample(std::vector<T> const & in, int count, Transform const & transform)
    {
        std::vector<T> out;
        if (in.size() < count)
            out = in;
        else {
            std::random_device rd;
            std::mt19937 gen(rd());
            out.reserve(count);
            sample(in.begin(), in.end(), count, gen, [&transform, &out](T const & elem) { out.push_back(transform(elem)); });
        }
        return out;
    }

And then how to use the nice and easy version:

    float estimateThreshold(std::vector<float> const & errors)
    {
        std::vector<float> absErrs = sample(errors, maxSampleSize, [](float val) {return abs(val);});
        
        ... do stuff with sample ...
    }

### any_tidy_ptr

`any_tidy_ptr<T>` is basically a unique_ptr with a std::function as its deleter. (And like unique_ptr, it is move-only.)

But it is great!

There are many times where I need to own a pointer to something, but I don't want the deleter to be part of the type of the pointer - I don't care how it gets deleted, as long as it happens.

You could use shared_ptr for this, because it already type-erases the deleter.  But shared_ptr implies... sharing! Go figure.

I much prefer unique_ptr (because sharing is actually usually bad). But I need the deletion to be more run-timey.

And by the way, any_tidy_ptr can hold a shared_ptr! It does the right thing (which is to decrement that ref count when the any_tidy_ptr goes away).

#### An example use case:

I have an Image class. It holds (and, typically, owns!) pixels.

But every once in a while I need to wrap a buffer of pixels into an Image temporarily, without copying the pixels.  Then do Image processing on the buffer, then delete the Image without the buffer going away (because the Image didn't own the pixels in this case).

Now, that is, in fact, bad, dangerous, etc. It breaks the rules of Image (which should own the pixels).  But sometimes performance is queen, and we bow to her.

So, Image uses an any_tidy_ptr to hold the pixels, and, in this case, the deleter is empty - because it doesn't own the pixels.

And on other occasions, maybe it shares the pixels with another Image (via shared_ptr to any_tidy_ptr conversion). Again, for performance.


