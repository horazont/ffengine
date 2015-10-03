/**********************************************************************
File name: array.hpp
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
#ifndef SCC_ENGINE_GL_HWELEMENTBUF_H
#define SCC_ENGINE_GL_HWELEMENTBUF_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <cassert>

#include <epoxy/gl.h>

#include "ffengine/gl/object.hpp"
#include "ffengine/gl/util.hpp"

#include "ffengine/io/log.hpp"

namespace ffe {

extern io::Logger &gl_array_logger;

typedef unsigned int GLArrayRegionID;

class GLArrayRegion
{
public:
    GLArrayRegion(GLArrayRegionID id,
                  unsigned int start,
                  unsigned int count):
        m_id(id),
        m_start(start),
        m_count(count),
        m_in_use(false),
        m_dirty(false)
    {
    }

public:
    GLArrayRegionID m_id;
    unsigned int m_start;
    unsigned int m_count;
    bool m_in_use;
    bool m_dirty;

};


template <typename _buffer_t>
class GLArrayAllocation
{
public:
    typedef _buffer_t buffer_t;
    typedef typename buffer_t::element_t element_t;

public:
    GLArrayAllocation():
        m_region_id(0),
        m_buffer(0),
        m_elements_per_block(0),
        m_nblocks(0)
    {

    }

    GLArrayAllocation(
            buffer_t *const buffer,
            const unsigned int elements_per_block,
            const unsigned int nblocks,
            const GLArrayRegionID region_id):
        m_region_id(region_id),
        m_buffer(buffer),
        m_elements_per_block(elements_per_block),
        m_nblocks(nblocks)
    {

    }

    GLArrayAllocation(const GLArrayAllocation &ref) = delete;

    GLArrayAllocation(GLArrayAllocation &&src):
        m_region_id(src.m_region_id),
        m_buffer(src.m_buffer),
        m_elements_per_block(src.m_elements_per_block),
        m_nblocks(src.m_nblocks)
    {
        src.m_region_id = 0;
        src.m_buffer = nullptr;
        src.m_elements_per_block = 0;
        src.m_nblocks = 0;
    }

    GLArrayAllocation &operator=(GLArrayAllocation &&src)
    {
        m_region_id = src.m_region_id;
        m_buffer = src.m_buffer;
        m_elements_per_block = src.m_elements_per_block;
        m_nblocks = src.m_nblocks;

        src.m_buffer = nullptr;
        src.m_region_id = 0;
        src.m_elements_per_block = 0;
        src.m_nblocks = 0;

        return *this;
    }

    GLArrayAllocation &operator=(std::nullptr_t)
    {
        if (m_buffer) {
            m_buffer->region_release(m_region_id);
        }
        m_buffer = nullptr;
        m_region_id = 0;
        m_elements_per_block = 0;
        m_nblocks = 0;

        return *this;
    }

    ~GLArrayAllocation()
    {
        if (m_buffer) {
            m_buffer->region_release(m_region_id);
        }
    }

private:
    GLArrayRegionID m_region_id;
    buffer_t *m_buffer;
    unsigned int m_elements_per_block;
    unsigned int m_nblocks;

public:
    inline buffer_t *buffer() const
    {
        return m_buffer;
    }

    inline unsigned int elements_per_block() const
    {
        return m_elements_per_block;
    }

    inline std::size_t offset() const
    {
        return m_buffer->region_offset(m_region_id);
    }

    inline unsigned int base() const
    {
        return m_buffer->region_base(m_region_id);
    }

    inline unsigned int length() const
    {
        return m_nblocks;
    }

public:
    void mark_dirty() const
    {
        m_buffer->region_mark_dirty(m_region_id);
    }

    element_t *get() const
    {
        return m_buffer->region_get_ptr(m_region_id);
    }

    operator bool() const
    {
        return bool(m_buffer);
    }

};


template <typename _element_t,
          GLuint gl_target,
          GLuint gl_binding_type,
          typename buffer_t>
class GLArray: public GLObject<gl_binding_type>
{
public:
    typedef _element_t element_t;
    typedef GLArrayAllocation<buffer_t> allocation_t;
    typedef std::vector<std::unique_ptr<GLArrayRegion> > region_container;

public:
    GLArray():
        GLObject<gl_binding_type>(),
        m_usage(GL_DYNAMIC_DRAW),
        m_block_length(0),
        m_local_buffer(),
        m_regions(),
        m_region_map(),
        m_any_dirty(false),
        m_remote_size(0),
        m_region_id_ctr()
    {
        glGenBuffers(1, &this->m_glid);
        glBindBuffer(gl_target, this->m_glid);
        glBufferData(gl_target, 0, nullptr, GL_STATIC_DRAW);
        raise_last_gl_error();
        glBindBuffer(gl_target, 0);
    }

protected:
    GLuint m_usage;
    unsigned int m_block_length;

    std::basic_string<element_t> m_local_buffer;
    std::vector<std::unique_ptr<GLArrayRegion> > m_regions;
    std::unordered_map<GLArrayRegionID, GLArrayRegion*> m_region_map;
    bool m_any_dirty;

    unsigned int m_remote_size;
    GLArrayRegionID m_region_id_ctr;

protected:
    GLArrayRegion &append_region(unsigned int start,
                                 unsigned int count)
    {
        const GLArrayRegionID region_id = ++m_region_id_ctr;
        m_regions.emplace_back(new GLArrayRegion(
                                   region_id,
                                   start,
                                   count));
        GLArrayRegion &new_region = **(m_regions.end() - 1);
        m_region_map[region_id] = &new_region;
        return new_region;
    }

    inline unsigned int block_size() const
    {
        return m_block_length * sizeof(element_t);
    }

    region_container::iterator compact_regions(
            region_container::iterator iter,
            const unsigned int nregions)
    {
        unsigned int total = 0;
        unsigned int i = nregions;
        do {
            i -= 1;
            --iter;
            total += (*iter)->m_count;
            assert(!(*iter)->m_in_use);
        } while (i > 0);

        (*iter)->m_count = total;
        return m_regions.erase(iter+1, iter+nregions);
    }

    inline bool merge_aggregated(region_container::iterator &iterator,
                                 region_container::iterator &best,
                                 unsigned int &aggregation_backlog,
                                 const unsigned int nblocks)
    {
        gl_array_logger.logf(io::LOG_DEBUG,
                             "compacting %d regions",
                             aggregation_backlog);
        iterator = compact_regions(
                    iterator,
                    aggregation_backlog);
        GLArrayRegion &merged = **(iterator-1);
        gl_array_logger.logf(io::LOG_DEBUG,
                             "resulting region (%d) has %d elements",
                             merged.m_id,
                             merged.m_count);
        if (merged.m_count >= nblocks) {
            gl_array_logger.logf(io::LOG_DEBUG,
                                 "suggesting region %d",
                                 merged.m_id);
            best = iterator-1;
            return true;
        }

        return false;
    }

    region_container::iterator compact_or_expand(
            const unsigned int nblocks)
    {
        auto iterator = m_regions.begin();
        unsigned int aggregation_backlog = 0;
        bool found = false;
        auto best = m_regions.end();

        for (; iterator != m_regions.end(); iterator++)
        {
            GLArrayRegion &region = **iterator;
            if (region.m_in_use)
            {
                if (aggregation_backlog > 1) {
                    if (merge_aggregated(iterator, best,
                                         aggregation_backlog, nblocks)) {
                        found = true;
                        break;
                    }
                    // iterator might point behind the list after erase
                    if (iterator == m_regions.end()) {
                        break;
                    }
                }
                aggregation_backlog = 0;
                continue;
            } else {
                // this is safe, we only erase during this loop, which only
                // invalidates iterators *behind* the erasure
                // in addition, if this region will be merged away, it will be
                // a "best" region and found will be true.
                if (region.m_count >= nblocks) {
                    gl_array_logger.logf(io::LOG_DEBUG,
                                         "candidate region (%p) %d: start=%u, in_use=%d, count=%u",
                                         &region, region.m_id,
                                         region.m_start,
                                         region.m_in_use,
                                         region.m_count);
                    if (found && region.m_count < (*best)->m_count) {
                        // use smaller region if possible
                        // this is only a weak heuristic; merge_aggregated is
                        // likely to use a larger region than we currently have
                        best = iterator;
                    } else if (!found) {
                        best = iterator;
                        found = true;
                    }
                }
            }

            aggregation_backlog += 1;
        }

        if (!found && aggregation_backlog > 1) {
            found = merge_aggregated(iterator, best,
                                     aggregation_backlog, nblocks);
        }

        if (found) {
            gl_array_logger.logf(io::LOG_DEBUG,
                                 "using region %d with %d elements",
                                 (*best)->m_id,
                                 (*best)->m_count);
            return best;
        }

        unsigned int required_blocks = nblocks;
        gl_array_logger.log(io::LOG_DEBUG,
                            "out of luck, we have to reallocate");
        if (m_regions.size() > 0) {
            gl_array_logger.log(io::LOG_DEBUG, "but we have regions");
            GLArrayRegion &last_region = **m_regions.rbegin();
            if (!last_region.m_in_use) {
                gl_array_logger.log(io::LOG_DEBUG, "and the last one is not in use");
                assert(last_region.m_count < nblocks);
                required_blocks -= last_region.m_count;
            }
        }

        gl_array_logger.logf(io::LOG_DEBUG,
                             "requesting expansion by %d (out of %d) blocks",
                             required_blocks, nblocks);

        expand(required_blocks);
        return m_regions.end() - 1;
    }

    void delete_globject() override
    {
        glDeleteBuffers(1, &this->m_glid);
        m_remote_size = 0;
        GLObject<gl_binding_type>::delete_globject();
    }

    void expand(const unsigned int at_least_by_blocks)
    {
        const unsigned int required_blocks =
                m_local_buffer.size() / m_block_length + at_least_by_blocks;
        reserve(required_blocks);
    }

    void reserve(const unsigned int min_blocks)
    {
        const unsigned int old_blocks = m_local_buffer.size() / m_block_length;
        unsigned int new_blocks = old_blocks;
        if (new_blocks == 0) {
            new_blocks = 1;
        }
        while (min_blocks > new_blocks) {
            new_blocks *= 2;
        }
        if (new_blocks <= old_blocks) {
            return;
        }

        const unsigned int new_size = new_blocks * m_block_length;

        gl_array_logger.logf(io::LOG_DEBUG,
                             "reserve: reallocating to %u elements (%u blocks)",
                             new_size,
                             new_blocks);

        m_local_buffer.resize(new_size);
        if (m_regions.size() > 0) {
            GLArrayRegion &last_region = **(m_regions.end() - 1);
            if (!last_region.m_in_use) {
                gl_array_logger.logf(io::LOG_DEBUG,
                                     "reserve: appending %d blocks to existing region", new_blocks - old_blocks);
                last_region.m_count += new_blocks - old_blocks;
                return;
            }
        }

        GLArrayRegion &region = append_region(old_blocks, new_blocks - old_blocks);

        gl_array_logger.logf(io::LOG_DEBUG,
                             "reserve: created region %u with %u blocks",
                             region.m_id,
                             region.m_count);
    }

    bool reserve_remote()
    {
        if (m_remote_size >= m_local_buffer.size())
        {
            return false;
        }

        gl_array_logger.logf(io::LOG_INFO, "(glid=%d) GPU reallocation",
                             this->m_glid);

        glBufferData(gl_target,
                     m_local_buffer.size() * sizeof(element_t),
                     m_local_buffer.data(),
                     m_usage);
        m_remote_size = m_local_buffer.size();
        return true;
    }

    GLArrayRegion &split_region(
            region_container::iterator iter,
            const unsigned int blocks_for_first)
    {
        GLArrayRegion *first_region = &**iter;

        const GLArrayRegionID new_region_id = ++m_region_id_ctr;
        auto new_region_iter = m_regions.emplace(
                    iter+1,
                    new GLArrayRegion(
                        new_region_id,
                        first_region->m_start + blocks_for_first,
                        first_region->m_count - blocks_for_first)
                    );
        GLArrayRegion &new_region = **new_region_iter;
        first_region = &**(new_region_iter-1);
        m_region_map[new_region_id] = &new_region;

        first_region->m_count = blocks_for_first;

        return *first_region;
    }

    void upload_dirty()
    {
        gl_array_logger.logf(io::LOG_DEBUG,
                             "upload dirty called on array (glid=%d, local_size=%d)",
                             this->m_glid,
                             m_local_buffer.size());

        if (reserve_remote())
        {
            gl_array_logger.log(
                        io::LOG_DEBUG,
                        "remote reallocation took place, no need to retransfer"
                        );
            // reallocation took place, this uploads all data
            for (auto &region: m_regions) {
                region->m_dirty = false;
            }
            m_any_dirty = false;
            return;
        }

        if (!m_any_dirty) {
            gl_array_logger.log(
                        io::LOG_DEBUG,
                        "not dirty, bailing out");
            // std::cout << "nothing to upload (m_any_dirty=false)" << std::endl;
            return;
        }

        unsigned int left_block = m_local_buffer.size();
        unsigned int right_block = 0;

        for (auto &region: m_regions)
        {
            if (!region->m_in_use || !region->m_dirty)
            {
                continue;
            }

            if (region->m_start < left_block)
            {
                left_block = region->m_start;
            }
            const unsigned int end = region->m_start + region->m_count;
            if (end > right_block)
            {
                right_block = end;
            }

            region->m_dirty = false;
        }

        if (right_block > 0) {
            const unsigned int offset = left_block * block_size();
            const unsigned int size = (right_block - left_block) * block_size();
            gl_array_logger.log(io::LOG_DEBUG)
                    << "uploading "
                    << size << " bytes at offset "
                    << offset
                    << " (glid=" << this->m_glid << "; bound=" << gl_get_integer(gl_binding_type) << ")" << io::submit;
            glBufferSubData(gl_target, offset, size, m_local_buffer.data() + offset/sizeof(element_t));
        } else {
            // std::cout << "nothing to upload (right_block=0)" << std::endl;
        }

        m_any_dirty = false;
    }

public:
    allocation_t allocate(unsigned int nblocks)
    {
        gl_array_logger.logf(io::LOG_DEBUG,
                             "(glid=%d) trying to allocate %d blocks",
                             this->m_glid,
                             nblocks);

        auto iterator = m_regions.end();
        /*for (; iterator != m_regions.end(); ++iterator)
        {
            GLArrayRegion &region = **iterator;
            gl_array_logger.logf(io::LOG_DEBUG,
                                 "region (%p) %d: start=%d, in_use=%d, count=%d",
                                 &region,
                                 region.m_id,
                                 region.m_start,
                                 region.m_in_use,
                                 region.m_count);
            if (region.m_in_use) {
                gl_array_logger.logf(io::LOG_DEBUG,
                                     "region %d in use, skipping",
                                     region.m_id);
                continue;
            }
            if (region.m_count < nblocks) {
                gl_array_logger.logf(io::LOG_DEBUG,
                                     "region %d too small (%d), skipping",
                                     region.m_id,
                                     region.m_count);
                continue;
            }
            gl_array_logger.logf(io::LOG_DEBUG,
                                 "region %d looks suitable",
                                 region.m_id);
            break;
        }*/

        if (iterator == m_regions.end())
        {
            gl_array_logger.log(io::LOG_DEBUG,
                                "always using compact_or_expand");
            // out of memory
            iterator = compact_or_expand(nblocks);
            gl_array_logger.logf(io::LOG_DEBUG,
                                 "compact_or_expand returned region %d (count=%d)",
                                 (*iterator)->m_id,
                                 (*iterator)->m_count);
        }

        GLArrayRegion *region_to_use = &**iterator;
        if (region_to_use->m_count > nblocks)
        {
            // split region
            gl_array_logger.logf(io::LOG_DEBUG,
                                 "region %d too large, splitting",
                                 region_to_use->m_id);
            region_to_use = &split_region(iterator, nblocks);
            gl_array_logger.logf(io::LOG_DEBUG,
                                 "now using region %d (start=%d, count=%d)",
                                 region_to_use->m_id,
                                 region_to_use->m_start,
                                 region_to_use->m_count);
        }

        region_to_use->m_in_use = true;
        region_to_use->m_dirty = false;

        gl_array_logger.logf(io::LOG_DEBUG,
                             "allocated %d blocks to region %d",
                             nblocks,
                             region_to_use->m_id);

        gl_array_logger.logf(io::LOG_DEBUG,
                             "region (%p) %d: start=%d, in_use=%d, count=%d",
                             region_to_use,
                             region_to_use->m_id,
                             region_to_use->m_start,
                             region_to_use->m_in_use,
                             region_to_use->m_count);

        return allocation_t((buffer_t*)this,
                            m_block_length,
                            nblocks,
                            region_to_use->m_id);
    }

    void dump_remote_raw()
    {
        if (m_remote_size == 0 || this->m_glid == 0) {
            std::cout << "no remote data" << std::endl;
            return;
        }

        std::cout << "BEGIN OF BUFFER DUMP (glid = " << this->m_glid << ")" << std::endl;

        std::basic_string<element_t> buf;
        buf.resize(m_remote_size, (element_t)(0xdeadbeefdeadbeefULL));
        glGetBufferSubData(gl_target, 0, m_remote_size*sizeof(element_t), &buf.front());

        unsigned int p = 0;
        for (auto &item: buf) {
            std::cout << p << "  " << item << std::endl;
            ++p;
        }

        std::cout << "END OF BUFFER DUMP (glid = " << this->m_glid << "; count = " << buf.size() << ")" << std::endl;
    }

    element_t *region_get_ptr(const GLArrayRegionID region_id)
    {
        return (&m_local_buffer.front()) + m_region_map[region_id]->m_start*m_block_length;
    }

    void region_mark_dirty(const GLArrayRegionID region_id)
    {
        m_region_map[region_id]->m_dirty = true;
        m_any_dirty = true;
    }

    void region_release(const GLArrayRegionID region_id)
    {
        gl_array_logger.logf(io::LOG_DEBUG, "(glid=%d) region %d released",
                             this->m_glid,
                             region_id);
        m_region_map[region_id]->m_in_use = false;
    }

    std::size_t region_offset(const GLArrayRegionID region_id)
    {
        return m_region_map[region_id]->m_start*m_block_length*sizeof(element_t);
    }

    unsigned int region_base(const GLArrayRegionID region_id)
    {
        return m_region_map[region_id]->m_start;
    }

    std::size_t vertices() const
    {
        return m_local_buffer.size();
    }

    void bind() override
    {
        glBindBuffer(gl_target, this->m_glid);
        this->bound();
    }

    void sync() override
    {
        bind();
        upload_dirty();
    }

    void unbind() override
    {
        glBindBuffer(gl_target, 0);
    }

};

}

#endif
