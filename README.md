# code

A place to put random bits of code.


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


