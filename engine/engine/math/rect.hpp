#ifndef SCC_ENGINE_MATH_SHAPES_H
#define SCC_ENGINE_MATH_SHAPES_H

#include <ostream>
#include <stdexcept>
#include <cmath>

#include "engine/math/vector.hpp"


enum not_a_rect_t
{
    NotARect
};

template <typename _coord_t>
class GenericRect
{
public:
    typedef _coord_t coord_t;
    typedef Vector<coord_t, 2> point_t;

public:
    GenericRect():
        m_p0(1, 1),
        m_p1(0, 0)
    {
        // negative width+height rect is NotARect
    }

    GenericRect(coord_t x0, coord_t y0):
        m_p0(x0, y0),
        m_p1(x0, y0)
    {

    }

    explicit GenericRect(const point_t &p0):
        m_p0(p0),
        m_p1(p0)
    {

    }

    GenericRect(coord_t x0, coord_t y0, coord_t x1, coord_t y1):
        m_p0(x0, y0),
        m_p1(x1, y1)
    {

    }

    GenericRect(const point_t &p0, const point_t &p1):
        m_p0(p0),
        m_p1(p1)
    {

    }

    GenericRect(const GenericRect &ref):
        m_p0(ref.m_p0),
        m_p1(ref.m_p1)
    {

    }

    GenericRect(const not_a_rect_t&):
        m_p0(1, 1),
        m_p1(0, 0)
    {

    }

    GenericRect &operator=(const GenericRect &ref)
    {
        m_p0 = ref.m_p0;
        m_p1 = ref.m_p1;
        return *this;
    }

    GenericRect &operator=(const not_a_rect_t&)
    {
        m_p0 = point_t(1, 1);
        m_p1 = point_t(0, 0);
        return *this;
    }

protected:
    point_t m_p0, m_p1;

public:
    inline bool is_a_rect() const {
        return x1() >= x0() && y1() >= y0();
    }

    inline bool empty() const {
        return x1() <= x0() || y1() <= y0();
    }

    inline coord_t x0() const
    {
        return m_p0[eX];
    }

    inline coord_t x1() const
    {
        return m_p1[eX];
    }

    inline coord_t y0() const
    {
        return m_p0[eY];
    }

    inline coord_t y1() const
    {
        return m_p1[eY];
    }

    inline void set_x0(const coord_t value) {
        m_p0[eX] = value;
    }

    inline void set_y0(const coord_t value) {
        m_p0[eY] = value;
    }

    inline void set_x1(const coord_t value) {
        m_p1[eX] = value;
    }

    inline void set_y1(const coord_t value) {
        m_p1[eY] = value;
    }

public:
    coord_t area() const
    {
        return (x1() - x0()) * (y1() - y0());
    }

    template<typename other_coord_t>
    bool overlaps(const GenericRect<other_coord_t> &other)
    {
        if (other == NotARect) {
            return false;
        }

        return (x0() < other.x1() && x1() > other.x0() &&
                y0() < other.y1() && y1() > other.y0());
    }

    template<typename other_coord_t>
    GenericRect operator&=(const GenericRect<other_coord_t> &other)
    {
        if (other == NotARect)
        {
            return *this;
        }

        m_p0 = point_t(std::max(x0(), other.x0()),
                       std::max(y0(), other.y0()));
        m_p1 = point_t(std::min(x1(), other.x1()),
                       std::min(y1(), other.y1()));
        return *this;
    }

    template<typename other_coord_t>
    inline bool operator==(const GenericRect<other_coord_t> &other) const
    {
        return m_p0 == other.m_p0 && m_p1 == other.m_p1;
    }

    template<typename other_coord_t>
    inline bool operator!=(const GenericRect<other_coord_t> &other) const
    {
        return m_p0 != other.m_p0 || m_p1 != other.m_p1;
    }

    inline operator bool() const
    {
        return is_a_rect() && (area() > 0);
    }

};

template <typename coord_t>
static inline bool operator==(not_a_rect_t, const GenericRect<coord_t> &r)
{
    return !r.is_a_rect();
}

template <typename coord_t>
static inline bool operator==(const GenericRect<coord_t> &r, not_a_rect_t)
{
    return !r.is_a_rect();
}

template <typename coord_t>
static inline bool operator!=(not_a_rect_t, const GenericRect<coord_t> &r)
{
    return r.is_a_rect();
}

template <typename coord_t>
static inline bool operator!=(const GenericRect<coord_t> &r, not_a_rect_t)
{
    return r.is_a_rect();
}

template<typename coord1_t,
         typename coord2_t,
         typename result_coord_t = decltype(coord1_t(0) + coord2_t(0))>
static inline GenericRect<result_coord_t> operator&(
        const GenericRect<coord1_t> &a,
        const GenericRect<coord2_t> &b)
{
    GenericRect<result_coord_t> result(a);
    result &= b;
    return result;
}

template <typename coord_t>
static inline GenericRect<coord_t> operator&(const GenericRect<coord_t> &,
                                             not_a_rect_t)
{
    return NotARect;
}

template <typename coord_t>
static inline GenericRect<coord_t> operator&(not_a_rect_t,
                                             const GenericRect<coord_t>&)
{
    return NotARect;
}

template <typename coord_t>
static inline GenericRect<coord_t> operator| (not_a_rect_t,
                                              const GenericRect<coord_t>& r)
{
    return r;
}

template <typename coord_t>
static inline GenericRect<coord_t> bounds(const GenericRect<coord_t> &r1,
                                          const GenericRect<coord_t> &r2)
{
    if (!r1.is_a_rect()) {
        return r2;
    } else if (!r2.is_a_rect()) {
        return r1;
    }
    return GenericRect<coord_t>(std::min(r1.x0(), r2.x0()),
                                std::min(r1.y0(), r2.y0()),
                                std::max(r1.x1(), r2.x1()),
                                std::max(r1.y1(), r2.y1()));
}


namespace std {

static inline ostream& operator<<(ostream& stream, const not_a_rect_t&)
{
    return stream << "NotARect";
}

template <typename coord_t>
static inline ostream& operator<<(ostream& stream, const GenericRect<coord_t> &rect)
{
    if (rect == NotARect) {
        return stream << NotARect;
    } else {
        return stream << "GenericRect("
                      << "x0=" << rect.x0() << ", "
                      << "y0=" << rect.y0() << ", "
                      << "x1=" << rect.x1() << ", "
                      << "y1=" << rect.y1() << ")";
    }
}

template <typename coord_t>
struct hash<GenericRect<coord_t> >
{
    typedef typename hash<coord_t>::result_type result_type;
    typedef GenericRect<coord_t> argument_type;

private:
    hash<coord_t> m_coord_hash;

public:
    result_type operator()(const argument_type &value) const
    {
        return m_coord_hash(value.x0())
                ^ m_coord_hash(value.y0())
                ^ m_coord_hash(value.x1())
                ^ m_coord_hash(value.y1());

    }

};

}

#endif
