# code

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

        static_assert(std::is_same<long, decltype(test(DogName()))>::value);
    }




