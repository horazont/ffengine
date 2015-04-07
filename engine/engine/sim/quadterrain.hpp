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
typedef Vector<terrain_coord_t, 3> TerrainVector;


struct QuadNode
{
public:
    enum class Type {
        NORMAL,
        LEAF,
        HEIGHTMAP,
    };

    typedef std::basic_string<terrain_height_t> Heightmap;

    enum Direction {
        NORTH = 4,
        NORTHWEST = 0,
        WEST = 5,
        SOUTHWEST = 2,
        SOUTH = 6,
        SOUTHEAST = 3,
        EAST = 7,
        NORTHEAST = 1,
    };

    enum SampleDirection {
        SAMPLE_EAST,
        SAMPLE_SOUTH
    };

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
    QuadNode *get_root();
    void heightmap_recalculate_height();
    void init_data();
    void normal_recalculate_height();

protected:
    QuadNode *find_node_at(terrain_coord_t x,
                           terrain_coord_t y,
                           const terrain_coord_t lod = 1);
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
    QuadNode *find_node_at(const TerrainRect::point_t &p,
                           const terrain_coord_t lod = 1);
    std::tuple<float, bool> hittest(const Ray &ray);
    QuadNode *neighbour(const Direction dir);
    terrain_height_t sample_int(const terrain_coord_t x,
                                const terrain_coord_t y);
    terrain_height_t sample_local_int(const terrain_coord_t x,
                                      const terrain_coord_t y);
    void sample_line(std::vector<TerrainVector> &dest,
                     const terrain_coord_t x0,
                     const terrain_coord_t y0,
                     const SampleDirection dir,
                     terrain_coord_t n);
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
    inline terrain_coord_t width() const
    {
        return m_size;
    }

    inline terrain_coord_t height() const
    {
        return m_size;
    }

public:
    QuadNode *root()
    {
        return &m_root;
    }

};


}

#endif // SCC_ENGINE_SIM_QUADTERRAIN_H
