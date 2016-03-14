#ifndef SCC_ENGINE_COMMON_SEQUENCE_VIEW_H
#define SCC_ENGINE_COMMON_SEQUENCE_VIEW_H

namespace ffe {


template <typename Container>
class SequenceView
{
public:
    using value_type = typename Container::value_type;
    using reference = typename Container::reference;
    using const_reference = typename Container::const_reference;
    using pointer = typename Container::pointer;
    using iterator = typename Container::iterator;
    using const_iterator = typename Container::const_iterator;
    using size_type = typename Container::size_type;
    using difference_type = typename Container::difference_type;

public:
    SequenceView(Container &container):
        m_container(&container)
    {

    }

    SequenceView(const SequenceView &ref) = default;
    SequenceView &operator=(const SequenceView &ref) = default;

private:
    Container *m_container;

public:
    inline bool empty() const
    {
        return m_container->empty();
    }

    inline size_type size() const
    {
        return m_container->size();
    }

    inline iterator begin()
    {
        return m_container->begin();
    }

    inline const_iterator begin() const
    {
        return m_container->begin();
    }

    inline const_iterator cbegin() const
    {
        return m_container->cbegin();
    }

    inline iterator end()
    {
        return m_container->end();
    }

    inline const_iterator end() const
    {
        return m_container->end();
    }

    inline const_iterator cend() const
    {
        return m_container->cend();
    }

    inline reference at(size_type n)
    {
        return m_container->at(n);
    }

    inline const_reference at(size_type n) const
    {
        return m_container->at(n);
    }

    inline reference operator[](size_type n)
    {
        return (*m_container)[n];
    }

    inline const_reference operator[](size_type n) const
    {
        return (*m_container)[n];
    }

    inline reference front()
    {
        return m_container->front();
    }

    inline const_reference front() const
    {
        return m_container->front();
    }

    inline reference back()
    {
        return m_container->back();
    }

    inline const_reference back() const
    {
        return m_container->back();
    }

    inline bool operator==(const SequenceView &other) const
    {
        return (*m_container) == (*other.m_container);
    }

    inline bool operator!=(const SequenceView &other) const
    {
        return (*m_container) != (*other.m_container);
    }

    inline bool operator>=(const SequenceView &other) const
    {
        return (*m_container) >= (*other.m_container);
    }

    inline bool operator<=(const SequenceView &other) const
    {
        return (*m_container) <= (*other.m_container);
    }

    inline bool operator>(const SequenceView &other) const
    {
        return (*m_container) > (*other.m_container);
    }

    inline bool operator<(const SequenceView &other) const
    {
        return (*m_container) < (*other.m_container);
    }

    inline const Container &container() const
    {
        return *m_container;
    }

};


template <typename SequenceView, typename Container>
bool operator==(const SequenceView &v, const Container &c)
{
    return v.container() == c;
}

template <typename SequenceView, typename Container>
bool operator==(const Container &c, const SequenceView &v)
{
    return c == v.container();
}

template <typename SequenceView, typename Container>
bool operator!=(const SequenceView &v, const Container &c)
{
    return v.container() != c;
}

template <typename SequenceView, typename Container>
bool operator!=(const Container &c, const SequenceView &v)
{
    return c != v.container();
}

template <typename SequenceView, typename Container>
bool operator>=(const SequenceView &v, const Container &c)
{
    return v.container() >= c;
}

template <typename SequenceView, typename Container>
bool operator>=(const Container &c, const SequenceView &v)
{
    return c >= v.container();
}

template <typename SequenceView, typename Container>
bool operator<=(const SequenceView &v, const Container &c)
{
    return v.container() <= c;
}

template <typename SequenceView, typename Container>
bool operator<=(const Container &c, const SequenceView &v)
{
    return c <= v.container();
}

template <typename SequenceView, typename Container>
bool operator<(const SequenceView &v, const Container &c)
{
    return v.container() < c;
}

template <typename SequenceView, typename Container>
bool operator<(const Container &c, const SequenceView &v)
{
    return c < v.container();
}

template <typename SequenceView, typename Container>
bool operator>(const SequenceView &v, const Container &c)
{
    return v.container() > c;
}

template <typename SequenceView, typename Container>
bool operator>(const Container &c, const SequenceView &v)
{
    return c > v.container();
}


}

#endif
