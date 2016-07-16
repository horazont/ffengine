#include "ffengine/common/stable_index_vector.hpp"

namespace ffe {

constexpr std::size_t StableIndexVectorBase::BLOCK_SIZE;
constexpr IndexMapBase::RawIndex IndexMapBase::INVALID_INDEX;


/* ffe::IndexMapBase::Region */

IndexMapBase::Region::Region(
        const IndexMapBase::RawIndex src_first,
        const IndexMapBase::RawIndex dest_first,
        const IndexMapBase::RawIndex count):
    m_src_first(src_first),
    m_dest_first(dest_first),
    m_count(count)
{

}

IndexMapBase::RawIndex IndexMapBase::map(RawIndex from) const
{
    auto iter = std::upper_bound(
                m_regions.begin(),
                m_regions.end(),
                from,
                [](RawIndex index, const Region &region){return index < region.m_src_first;});
    if (iter == m_regions.begin()) {
        return INVALID_INDEX;
    }
    --iter;

    if (from >= iter->m_src_first + iter->m_count) {
        return INVALID_INDEX;
    }
    return (from - iter->m_src_first) + iter->m_dest_first;
}


}
