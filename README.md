# code

A place to put random bits of code.

- StrongId
- Unit
- sample()


### StrongId

A template for making ID types that are unique types for unique uses. ie WidgetIds are not the same as EmployeeIds, and it is nice if the compiler prevents you from mixing one with the other.


    using FooId = StrongId<int, struct FooTag>;
    using BarId = StrongId<int, struct BarTag>;
    
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


