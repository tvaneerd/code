#ifndef Unit_h_INCLUDED
#define Unit_h_INCLUDED

//
// a general Unit class
// a Unit has most operations that a number would, except
// - conversion to/from number is explicit, not implicit
// - No Unit * Unit, as that would give a different Unit (ie meters * meters results in meters-squared, not meters)
//
// CRTP (https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern)
// is used (instead of, say a typedef like `using Meter = Unit<double, MeterTag>`, like StrongId does)
// because CRTP allows the derived class to have converting constructors (see Radians and Degrees for examples)
// whereas the typedef wouldn't allow that (and conversions can't be non-class functions)
//
// There is a good chance you want to use Unit<T, Tag> (see below), not this base class
//
template <typename Scalar, typename Derived>
struct UnitBase
{
    using scalar_type = Scalar;

    explicit UnitBase(Scalar v) : value(v) {}

    Scalar get() const { return value; }
    explicit operator Scalar() const { return value; }

    friend Derived operator+(UnitBase a, UnitBase b) { return Derived{ a.value + b.value }; }
    friend Derived operator-(UnitBase a, UnitBase b) { return Derived{ a.value - b.value }; }
    friend Derived operator*(UnitBase a, Scalar b) { return Derived{ a.value * b }; }
    friend Derived operator*(Scalar a, UnitBase b) { return Derived{ a * b.value }; }
    friend Derived operator/(UnitBase a, Scalar b) { return Derived{ a.value / b }; }
    Derived operator-() const { return Derived{ -value }; }
    Derived operator+() const { return Derived{ value }; }

    // Unit/Unit = a unitless (ie scalar) value
    friend auto operator/(UnitBase a, UnitBase b) { return a.value / b.value; }
    // Note the lack of Unit * Unit, as that would need to return a different Unit type! (ie Unit-squared)

    Derived & operator+=(UnitBase b) { a.value += b.value; return *this; }
    Derived & operator-=(UnitBase b) { a.value -= b.value; return *this; }
    Derived & operator*=(Scalar b) { a.value *= b; return *this; }
    Derived & operator/=(Scalar b) { a.value /= b; return *this; }

    friend bool operator==(UnitBase a, UnitBase b) { return a.get() == b.get(); }
    friend bool operator!=(UnitBase a, UnitBase b) { return a.get() != b.get(); }
    friend bool operator<(UnitBase a, UnitBase b) { return a.get() < b.get(); }
    friend bool operator<=(UnitBase a, UnitBase b) { return a.get() <= b.get(); }
    friend bool operator>(UnitBase a, UnitBase b) { return a.get() > b.get(); }
    friend bool operator>=(UnitBase a, UnitBase b) { return a.get() >= b.get(); }

private:
    Scalar value;
};

//
//
// a general Unit template, for making Units.
// ie
//   using Apples = Unit<int, struct AppleTag>;
//   using Oranges = Unit<int, struct OrangeTag>;
//
// if you want auto-conversion between units, use UnitBase
// (see Radians and Degrees for example)
// Otherwise, this is fine.  Or start with this, switch to UnitBase later, if needed.
//
template<typename T, typename Tag>
struct Unit : UnitBase<T, Unit<T, Tag> >
{
    using tag_type = Tag;

    explicit Unit(T t) : UnitBase(t) {}
};

#endif // _h

