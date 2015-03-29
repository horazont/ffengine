#ifndef SCC_ENGINE_GL_UBO_TUPLE_UTILS_H
#define SCC_ENGINE_GL_UBO_TUPLE_UTILS_H

template <std::size_t I, typename Type, typename... Types>
inline typename std::enable_if<(I > 0), typename std::tuple_element<I, std::tuple<Type, Types...>>::type>::type &get(wrapped_tuple<Type, Types...> &tpl)
{
    return get<I-1, Types...>(tpl.m_next);
}

template <std::size_t I, typename Type, typename... Types>
inline typename std::enable_if<I == 0, typename std::tuple_element<I, std::tuple<Type, Types...>>::type>::type &get(wrapped_tuple<Type, Types...> &tpl)
{
    typedef ubo_wrap_type<Type> helper;
    return helper::extract_ref(tpl.m_data);
}

template <std::size_t I, typename Type, typename... Types>
inline const typename std::enable_if<(I > 0), typename std::tuple_element<I, std::tuple<Type, Types...>>::type>::type &get(const wrapped_tuple<Type, Types...> &tpl)
{
    return get<I-1, Types...>(tpl.m_next);
}

template <std::size_t I, typename Type, typename... Types>
inline const typename std::enable_if<I == 0, typename std::tuple_element<I, std::tuple<Type, Types...>>::type>::type &get(const wrapped_tuple<Type, Types...> &tpl)
{
    typedef ubo_wrap_type<Type> helper;
    return helper::extract_const_ref(tpl.m_data);
}

template <std::size_t I, typename value_t, typename Type, typename... Types>
inline void set(wrapped_tuple<Type, Types...> &tpl, value_t &&value)
{
    get<I>(tpl) = value;
}

template <std::size_t I, typename Type, typename... Types>
inline typename std::enable_if<(I > 0), std::size_t>::type offset(wrapped_tuple<Type, Types...> &tpl)
{
    return sizeof(tpl.m_data) + offset<I-1, Types...>(tpl.m_next);
}

template <std::size_t I, typename Type, typename... Types>
inline typename std::enable_if<I==0, std::size_t>::type offset(wrapped_tuple<Type, Types...> &)
{
    return 0;
}

template <std::size_t I, typename Type, typename... Types>
inline typename std::enable_if<(I > 0), std::size_t>::type size(wrapped_tuple<Type, Types...> &tpl)
{
    return size<I-1, Types...>(tpl.m_next);
}

template <std::size_t I, typename Type, typename... Types>
inline typename std::enable_if<I==0, std::size_t>::type size(wrapped_tuple<Type, Types...> &tpl)
{
    return sizeof(tpl.m_data);
}

#endif // SCC_ENGINE_GL_UBO_TUPLE_UTILS_H

