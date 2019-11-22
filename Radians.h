#ifndef Radians_h_INCLUDED
#define Radians_h_INCLUDED

#include "Unit.h"
#include <cmath>  // sin/cos/tan

struct Degrees;

struct Radians : UnitBase<double, Radians>
{
    Radians() : UnitBase(0) {}
    explicit Radians(double t) : UnitBase(t) {}
    inline /*implicit*/ Radians(Degrees d);

    // these are better than calling get()
    // as they make it more clear what the number is
    // ie std::cout << angle.degrees() prints the angle in degrees
    // regardless to whether the angle is in radians or degrees
    double radians() const { return get(); }
    double degrees() const;

    static Radians asin(double s) { return Radians(std::asin(s)); }
    static Radians acos(double c) { return Radians(std::acos(c)); }
    static Radians atan(double t) { return Radians(std::atan(t)); }
    static Radians atan2(double y, double x) { return Radians(std::atan2(y, x)); }
};

struct Degrees : UnitBase<double, Degrees>
{
    Degrees() : UnitBase(0) {}
    explicit Degrees(double t) : UnitBase(t) {}
    inline /*implicit*/ Degrees(Radians r);

    // these are better than calling get()
    // as they make it more clear what the number is
    // ie std::cout << angle.degrees() prints the angle in degrees
    //    std::cout << angle.get() prints I'm not sure without looking at type of angle
    double degrees() const { return get(); }
    double radians() const;

    static Degrees asin(double s) { return Radians::asin(s); }
    static Degrees acos(double c) { return Radians::acos(c); }
    static Degrees atan(double t) { return Radians::atan(t); }
    static Degrees atan2(double y, double x) { return Radians(std::atan2(y, x)); }
};

inline /*implicit*/ Degrees::Degrees(Radians r) : UnitBase((180/3.14159265359) * r.get()) {}
inline /*implicit*/ Radians::Radians(Degrees d) : UnitBase((3.14159265359/180) * d.get()) {}
inline double Radians::degrees() const { return Degrees(*this).get(); }
inline double Degrees::radians() const { return Radians(*this).get(); }

// use these for Degrees as well:
inline double cos(Radians r) { return std::cos(r.get()); }
inline double sin(Radians r) { return std::sin(r.get()); }
inline double tan(Radians r) { return std::tan(r.get()); }

#endif  // _h
