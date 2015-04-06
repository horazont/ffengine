#ifndef SCC_ENGINE_SIM_QUADTERRAIN_H
#define SCC_ENGINE_SIM_QUADTERRAIN_H

#include <cassert>
#include <cstdint>
#include <string>

#include "engine/math/ray.hpp"
#include "engine/math/shapes.hpp"


namespace sim {

typedef std::uint_least16_t terrain_height_t;
typedef std::uint_least16_t terrain_coord_t;
typedef std::uint_least32_t intermediate_terrain_height_t;
typedef GenericRect<terrain_coord_t> TerrainRect;


struct QuadNode
{
public:
    enum class Type {
        NORMAL,
        LEAF,
        HEIGHTMAP,
    };

    static constexpr unsigned int NORTHWEST = 0;
    static constexpr unsigned int NORTHEAST = 1;
    static constexpr unsigned int SOUTHWEST = 2;
    static constexpr unsigned int SOUTHEAST = 3;

    typedef std::basic_string<terrain_height_t> Heightmap;

public:
    QuadNode(QuadNode *parent,
             Type type,
             const terrain_coord_t x0,
             const terrain_coord_t y0,
             const terrain_coord_t size,
             const terrain_height_t height);

    QuadNode(const QuadNode &ref) = delete;
    QuadNode(QuadNode &&src) = delete;
    QuadNode &operator=(const QuadNode &ref) = delete;
    QuadNode &operator=(QuadNode &&src) = delete;

    ~QuadNode();

private:
    const TerrainRect m_rect;
    const terrain_coord_t m_size;
    QuadNode *const m_parent;
    Type m_type;
    terrain_height_t m_height;

    union {
        QuadNode *children[4];
        Heightmap *heightmap;
    } m_data;

    bool m_dirty;
    bool m_changed;
    bool m_child_changed;

private:
    void free_data();
    void init_data();
    void heightmap_recalculate_height();
    void normal_recalculate_height();

protected:
    void from_heightmap(Heightmap &src,
                        const terrain_coord_t x0,
                        const terrain_coord_t y0,
                        const terrain_coord_t src_size);
    void to_heightmap(Heightmap &dest,
                      const terrain_coord_t x0,
                      const terrain_coord_t y0,
                      const terrain_coord_t dest_size);

public: /* general */
    void cleanup();
    std::tuple<float, bool> hittest(const Ray &ray);
    terrain_height_t sample(const float x, const float y);
    void set_height_rect(const TerrainRect &rect,
                         const terrain_height_t new_height);

    inline terrain_coord_t x0() const
    {
        return m_rect.x0();
    }

    inline terrain_coord_t y0() const
    {
        return m_rect.y0();
    }

    inline terrain_coord_t size() const
    {
        return m_size;
    }

    inline QuadNode *parent() const
    {
        return m_parent;
    }

    inline terrain_height_t height() const
    {
        return m_height;
    }

    inline Type type() const
    {
        return m_type;
    }

    inline bool changed() const
    {
        return m_changed;
    }

    inline bool subtree_changed() const
    {
        return m_changed | m_child_changed;
    }

    inline bool dirty() const
    {
        return m_dirty;
    }

public: /* general quadtree */
    void heightmapify();

public: /* quadtree leaf */
    void subdivide();

public: /* quadtree inner node */
    inline QuadNode *child(unsigned int n) const
    {
        assert(m_type == Type::NORMAL);
        return m_data.children[n];
    }

    void merge();

public: /* heightmap */
    inline std::basic_string<terrain_height_t> *heightmap() const
    {
        assert(m_type == Type::HEIGHTMAP);
        return m_data.heightmap;
    }

    void mark_heightmap_dirty();
    void quadtreeify();

};


class QuadTerrain
{
public:
    QuadTerrain(terrain_coord_t max_subdivisions,
                terrain_height_t initial_height);

private:
    const terrain_coord_t m_max_subdivisions;
    const terrain_coord_t m_size;
    QuadNode m_root;

public:



};


}

#endif // SCC_ENGINE_SIM_QUADTERRAIN_H
