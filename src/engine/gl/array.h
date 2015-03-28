#ifndef SCC_ENGINE_GL_HWELEMENTBUF_H
#define SCC_ENGINE_GL_HWELEMENTBUF_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include <cassert>

#include <GL/glew.h>

#include "object.h"

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


template <typename buffer_t>
class GLArrayAllocation
{
public:
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

    ~GLArrayAllocation()
    {
        m_buffer->region_release(m_region_id);
    }

private:
    GLArrayRegionID m_region_id;
    buffer_t *m_buffer;
    unsigned int m_elements_per_block;
    unsigned int m_nblocks;

public:
    inline buffer_t *buffer()
    {
        return m_buffer;
    }

    inline unsigned int elements_per_block()
    {
        return m_elements_per_block();
    }

    inline unsigned int length()
    {
        return m_nblocks;
    }

public:
    void mark_dirty()
    {
        m_buffer->region_mark_dirty(m_region_id);
    }

    element_t *get()
    {
        return m_buffer->region_get_ptr(m_region_id);
    }

    operator bool()
    {
        return bool(m_buffer);
    }

};


template <typename _element_t,
          GLuint gl_target,
          GLuint gl_binding_type>
class GLArray: public GLObject<gl_binding_type>
{
public:
    typedef _element_t element_t;
    typedef GLArrayAllocation<GLArray> allocation_t;

public:
    GLArray():
        GLObject<gl_binding_type>(),
        m_usage(GL_STATIC_DRAW),
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
    std::vector<GLArrayRegion> m_regions;
    std::unordered_map<GLArrayRegionID, GLArrayRegion*> m_region_map;
    bool m_any_dirty;

    unsigned int m_remote_size;
    GLArrayRegionID m_region_id_ctr;

protected:
    GLArrayRegion &append_region(unsigned int start,
                                 unsigned int count)
    {
        const GLArrayRegionID region_id = ++m_region_id_ctr;
        m_regions.emplace_back(
                    region_id,
                    start,
                    count
        );
        GLArrayRegion &new_region = *(m_regions.end() - 1);
        m_region_map[region_id] = &new_region;
        return new_region;
    }

    inline unsigned int block_size() const
    {
        return m_block_length * sizeof(element_t);
    }

    std::vector<GLArrayRegion>::iterator compact_regions(
            std::vector<GLArrayRegion>::iterator iter,
            const unsigned int nregions)
    {
        unsigned int total = 0;
        unsigned int i = nregions;
        do {
            i -= 1;
            --iter;
            total += iter->m_count;
            assert(!iter->m_in_use);
        } while (i > 0);

        iter->m_count = total;
        return m_regions.erase(iter, iter+nregions);
    }

    std::vector<GLArrayRegion>::iterator compact_or_expand(
            const unsigned int nblocks)
    {
        auto iterator = m_regions.begin();
        unsigned int aggregation_backlog = 0;
        auto best = m_regions.end();
        unsigned int best_metric = m_local_buffer.size() / m_block_length;

        for (; iterator != m_regions.end(); iterator++)
        {
            GLArrayRegion &region = *iterator;
            if (region.m_in_use)
            {
                if (aggregation_backlog > 1) {
                    iterator = compact_regions(
                                iterator,
                                aggregation_backlog);
                    GLArrayRegion &merged = *(iterator-1);
                    if (merged.m_count >= nblocks) {
                        unsigned int metric = merged.m_count - nblocks;
                        if (metric < best_metric)
                        {
                            best = iterator-1;
                            best_metric = metric;
                        }
                    }
                }
                aggregation_backlog = 0;
                continue;
            }

            aggregation_backlog += 1;
        }

        if (best != m_regions.end()) {
            return best;
        }

        unsigned int required_blocks = nblocks;
        if (m_regions.size() > 0) {
            GLArrayRegion &last_region = *m_regions.rbegin();
            if (!last_region.m_in_use) {
                assert(last_region.m_count < nblocks);
                required_blocks -= last_region.m_count;
            }
        }

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
        const unsigned int required_size = min_blocks * m_block_length;
        const unsigned int old_size = m_local_buffer.size();
        unsigned int new_size = old_size;
        if (new_size == 0) {
            new_size = 1;
        }
        while (required_size > new_size) {
            new_size *= 2;
        }
        if (new_size <= old_size) {
            return;
        }

        m_local_buffer.resize(new_size);
        append_region(old_size / m_block_length,
                      (new_size - old_size) / m_block_length);
    }

    bool reserve_remote()
    {
        if (m_remote_size >= m_local_buffer.size())
        {
            return false;
        }

        glBufferData(gl_target,
                     m_local_buffer.size() * sizeof(element_t),
                     m_local_buffer.data(),
                     m_usage);
        m_remote_size = m_local_buffer.size();
        return true;
    }

    void split_region(std::vector<GLArrayRegion>::iterator iter,
                      const unsigned int blocks_for_first)
    {
        GLArrayRegion &first_region = *iter;

        const GLArrayRegionID new_region_id = ++m_region_id_ctr;
        GLArrayRegion &new_region = *m_regions.emplace(
                    iter+1,
                    new_region_id,
                    first_region.m_start + blocks_for_first,
                    first_region.m_count - blocks_for_first
                    );
        m_region_map[new_region_id] = &new_region;

        first_region.m_count = blocks_for_first;
    }

    void upload_dirty()
    {
        // std::cout << "upload dirty (glid=" << this->m_glid << ", local_size=" << m_local_buffer.size() << ")" << std::endl;
        if (reserve_remote())
        {
            // reallocation took place, this uploads all data
            for (auto &region: m_regions) {
                region.m_dirty = false;
            }
            m_any_dirty = false;
            return;
        }

        if (!m_any_dirty) {
            // std::cout << "nothing to upload (m_any_dirty=false)" << std::endl;
            return;
        }

        unsigned int left_block = m_local_buffer.size();
        unsigned int right_block = 0;

        for (auto &region: m_regions)
        {
            if (!region.m_in_use || !region.m_dirty)
            {
                continue;
            }

            if (region.m_start < left_block)
            {
                left_block = region.m_start;
            }
            const unsigned int end = region.m_start + region.m_count;
            if (end > right_block)
            {
                right_block = end;
            }

            region.m_dirty = false;
        }

        if (right_block > 0) {
            const unsigned int offset = left_block * block_size();
            const unsigned int size = (left_block - right_block) * block_size();
            // std::cout << "uploading " << size << " bytes to " << offset << std::endl;
            glBufferSubData(gl_target, offset, size, m_local_buffer.data() + offset*m_block_length);
        } else {
            // std::cout << "nothing to upload (right_block=0)" << std::endl;
        }

        m_any_dirty = false;
    }

public:
    allocation_t allocate(unsigned int nblocks)
    {
        auto iterator = m_regions.begin();
        for (; iterator != m_regions.end(); ++iterator)
        {
            GLArrayRegion &region = *iterator;
            if (region.m_in_use) {
                continue;
            }
            if (region.m_count < nblocks) {
                continue;
            }

            break;
        }

        if (iterator == m_regions.end())
        {
            // out of memory
            iterator = compact_or_expand(nblocks);
        }

        GLArrayRegion &region_to_use = *iterator;
        if (region_to_use.m_count > nblocks)
        {
            // split region
            split_region(iterator, nblocks);
        }

        region_to_use.m_in_use = true;
        region_to_use.m_dirty = false;

        return allocation_t(this,
                            m_block_length,
                            nblocks,
                            region_to_use.m_id);
    }

    void dump_remote_raw()
    {
        if (m_remote_size == 0 || this->m_glid == 0) {
            std::cout << "no remote data" << std::endl;
            return;
        }

        std::cout << "BEGIN OF BUFFER DUMP (glid = " << this->m_glid << ")" << std::endl;

        std::basic_string<element_t> buf;
        buf.resize(m_remote_size);
        glGetBufferSubData(gl_target, 0, m_remote_size*sizeof(element_t), &buf.front());

        for (auto &item: buf) {
            std::cout << item << std::endl;
        }

        std::cout << "END OF BUFFER DUMP (glid = " << this->m_glid << ")" << std::endl;
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
        m_region_map[region_id]->m_in_use = false;
    }

    void bind() override
    {
        glBindBuffer(gl_target, this->m_glid);
        upload_dirty();
    }

    void unbind() override
    {
        glBindBuffer(gl_target, 0);
    }

};

#endif
