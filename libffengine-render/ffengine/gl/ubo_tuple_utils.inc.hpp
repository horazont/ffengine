/**********************************************************************
File name: ubo_tuple_utils.inc.hpp
This file is part of: SCC (working title)

LICENSE

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see <http://www.gnu.org/licenses/>.

FEEDBACK & QUESTIONS

For feedback and questions about SCC please e-mail one of the authors named in
the AUTHORS file.
**********************************************************************/
#ifndef SCC_ENGINE_GL_UBO_TUPLE_UTILS_H
#define SCC_ENGINE_GL_UBO_TUPLE_UTILS_H

template <std::size_t I, typename Type, typename... Types>
inline typename std::enable_if<(I > 0), typename std::tuple_element<I, std::tuple<Type, Types...>>::type>::type get(const wrapped_tuple<Type, Types...> &tpl)
{
    return get<I-1, Types...>(tpl.m_next);
}

template <std::size_t I, typename Type, typename... Types>
inline typename std::enable_if<I == 0, typename std::tuple_element<I, std::tuple<Type, Types...>>::type>::type get(const wrapped_tuple<Type, Types...> &tpl)
{
    typedef ubo_wrap_type<Type> helper;
    return helper::unpack(tpl.m_data);
}

template <std::size_t I, typename Type, typename... Types>
inline typename std::enable_if<(I > 0), typename std::tuple_element<I, std::tuple<Type, Types...>>::type>::type &get_ref(wrapped_tuple<Type, Types...> &tpl)
{
    return get<I-1, Types...>(tpl.m_next);
}

template <std::size_t I, typename Type, typename... Types>
inline typename std::enable_if<I == 0, typename std::tuple_element<I, std::tuple<Type, Types...>>::type>::type &get_ref(wrapped_tuple<Type, Types...> &tpl)
{
    typedef ubo_wrap_type<Type> helper;
    return helper::ref(tpl.m_data);
}

/*template <std::size_t I, typename Type, typename... Types>
inline const typename std::enable_if<(I > 0), typename std::tuple_element<I, std::tuple<Type, Types...>>::type>::type &get(const wrapped_tuple<Type, Types...> &tpl)
{
    return get<I-1, Types...>(tpl.m_next);
}

template <std::size_t I, typename Type, typename... Types>
inline const typename std::enable_if<I == 0, typename std::tuple_element<I, std::tuple<Type, Types...>>::type>::type &get(const wrapped_tuple<Type, Types...> &tpl)
{
    typedef ubo_wrap_type<Type> helper;
    return helper::extract_const_ref(tpl.m_data);
}*/


template <std::size_t I, typename value_t, typename Type, typename... Types>
inline typename std::enable_if<(I > 0), void>::type set(wrapped_tuple<Type, Types...> &tpl, value_t &&value)
{
    return set<I-1, value_t, Types...>(tpl.m_next, std::forward<value_t>(value));
}

template <std::size_t I, typename value_t, typename Type, typename... Types>
inline typename std::enable_if<(I == 0), void>::type set(wrapped_tuple<Type, Types...> &tpl, value_t &&value)
{
    typedef ubo_wrap_type<Type> helper;
    tpl.m_data = helper::pack(value);
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

