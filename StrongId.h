#ifndef StrongId_h_INCLUDED
#define StrongId_h_INCLUDED

//
// a StrongId class is a more type-safe string or int etc,
// when using them as "IDs" for something.
//
// So you can have FooIds and BarIds, which are both StrongIds,
// but you won't accidentally mix FooIds and BarIds in code - the compiler will complain.
//
// usage:
//
// struct FooIdTag {}; // a type, that is useless, except it has a name and is a unique type
//
// using FooId = StrongId<int, FooIdTag>;
// (which is the same as: typedef StrongId<int, FooIdTag> FooId; )
//
// also, you can actually just introduce the tag using 'struct':
//
// using BarId = StrongId<int, struct BarTag>;
//


template</*Regular*/typename IdType, typename Tag = void>
class StrongId;

//
// specialized for Tag == void,
// which we will use for the base type
// Having a common base is useful sometimes when you want
// to deal with a bunch of different IDs somewhat generically
// (without dipping into templates)
//
template<typename IdType>
class StrongId<IdType, void>
{
    IdType id{};
public:
    using value_type = IdType;
    using tag_type = void;

    StrongId() = default;
    explicit StrongId(IdType id) : id(std::move(id))
    {
    }
    // Note: this class is implicitly moveable and copyable

    explicit operator IdType() const { return id; }

    // we don't want all the operations of the IdType,
    // (ie we don't want id1 + id2 or id1 - id2, etc)
    // but equality and sorting tend to be useful
    // (todo - add <= etc)
    friend bool operator<(StrongId const & x, StrongId const & y) { return x.id < y.id; }
    friend bool operator==(StrongId const & x, StrongId const & y) { return x.id == y.id; }
    friend bool operator!=(StrongId const & x, StrongId const & y) { return x.id != y.id; }
};

// the general template
// derives from the above void specialization
//
template<typename IdType, typename Tag>
class StrongId : public StrongId<IdType, void>
{
public:
    using StrongId<IdType,void>::StrongId; // inherit base class constructors

    StrongId() = default;
    // you can convert from other Id types,
    // but it is explicit
    explicit StrongId(StrongId<IdType> const &id) : StrongId<IdType>(id) {}
};

//
// StrongId<int> == StrongId<int> is valid,
// and
// StrongId<int,Foo> == StrongId<int,Foo> is valid
// but
// StrongId<int,Foo> == StrongId<int,Bar> should not be valid
//
// Since StrongId<int,Foo> decays into its base class StrongId<int>,
// we need to explicitly delete the mixed overloads
// (otherwise StrongId<int,T> == StrongId<int,U> would automatically call StrongId<int> == StrongId<int>)
//
template<typename IdType, typename T, typename U>
bool operator==(StrongId<IdType,T> const & x, StrongId<IdType,U> const & y) = delete;
// and now "bring back" non mixed-type comparisons:
template<typename IdType, typename T>
bool operator==(StrongId<IdType,T> const & x, StrongId<IdType,T> const & y)
{
    return static_cast<StrongId<IdType> const &>(x) == static_cast<StrongId<IdType> const &>(y);
}

// same with !=
template<typename IdType, typename T, typename U>
bool operator!=(StrongId<IdType, T> const & x, StrongId<IdType, U> const & y) = delete;
// and now "bring back" non mixed-type comparisons:
template<typename IdType, typename T>
bool operator!=(StrongId<IdType,T> const & x, StrongId<IdType,T> const & y)
{
    return static_cast<StrongId<IdType> const &>(x) != static_cast<StrongId<IdType> const &>(y);
}

// and <
template<typename IdType, typename T, typename U>
bool operator<(StrongId<IdType,T> const & x, StrongId<IdType,U> const & y) = delete;
// and now "bring back" non mixed-type comparisons:
template<typename IdType, typename T>
bool operator<(StrongId<IdType,T> const & x, StrongId<IdType,T> const & y)
{
    return static_cast<StrongId<IdType> const &>(x) < static_cast<StrongId<IdType> const &>(y);
}


//
// this somewhat sucks because it is a pretty generic overload
// thus increasing the overload set for any use of <<
// :-(
template<typename Out, typename IdType>
inline Out & operator<<(Out & s, StrongId<IdType> const & id)
{
    return s << IdType(id);
}

// this is #if 0 to avoid #include <string>
#if 0
// common id types:
template <typename Tag = void>
using StringId = StrongId<std::string, Tag>;
template <typename Tag = void>
using IntId = StrongId<int, Tag>;
#endif

#endif // _h
