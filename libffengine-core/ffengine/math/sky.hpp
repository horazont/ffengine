#ifndef SCC_ENGINE_MATH_SKY_H
#define SCC_ENGINE_MATH_SKY_H

#include <memory>
#include <functional>
#include <random>

#include "ffengine/common/utils.hpp"

#include "ffengine/math/vector.hpp"

extern "C" {
struct ArHosekSkyModelState;
}

namespace ffe {

class EnvironmentMapGenerator
{
public:
    virtual ~EnvironmentMapGenerator();

protected:
    static inline Vector3f angles_to_vector(const float theta, const float phi)
    {
        return Vector3f(std::cos(phi)*std::sin(theta),
                        std::sin(phi)*std::sin(theta),
                        std::cos(theta));
    }

    static inline Vector2f vector_to_angles(const Vector3f r)
    {
        const Vector2f xyplane = Vector2f(r).normalized();
        return Vector2f(std::acos(r.normalized()[eZ]),
                        std::atan2(xyplane[eY], xyplane[eX]));
    }

public:
    virtual Vector3f sample(const float theta, const float phi) const = 0;
    virtual Vector3f sample(const Vector3f direction) const = 0;

};

class SkyGenerator: public EnvironmentMapGenerator
{
public:
    SkyGenerator(const Vector3f sun_direction);
    virtual ~SkyGenerator();

protected:
    static const Vector3f up;
    Vector3f m_sun_direction;

protected:
    inline float sun_phi() const
    {
        const Vector2f xyplane = Vector2f(m_sun_direction).normalized();
        return std::atan2(xyplane[eX], xyplane[eY]);
    }

public:
    inline Vector3f sun_direction() const {
        return m_sun_direction;
    }

};


class HosekWilkieGenerator: public SkyGenerator
{

public:
    HosekWilkieGenerator(const float turbidity,
                         const Vector3f sun_direction,
                         const float ground_albedo);
    ~HosekWilkieGenerator() override;

private:
    std::unique_ptr<ArHosekSkyModelState> m_model;

public:
    Vector3f sample_hw(const float theta, const float gamma) const;
    Vector3f sample(const float theta, const float phi) const override;
    Vector3f sample(const Vector3f direction) const override;

};


class CubeMapSkyBuffer: public EnvironmentMapGenerator
{
public:
    enum Side {
        NEGATIVE_X = 0,
        NEGATIVE_Y = 1,
        NEGATIVE_Z = 2,
        POSITIVE_X = 3,
        POSITIVE_Y = 4,
        POSITIVE_Z = 5
    };

public:
    CubeMapSkyBuffer(const std::size_t size);

private:
    const std::size_t m_size;
    std::array<std::vector<Vector3f>, 6> m_sides;

private:
    inline Side detect_side(const float theta, const float gamma) {
        if (theta <= M_PI_4) {
            return POSITIVE_Z;
        } else if (theta >= M_PI_2 + M_PI_4) {
            return NEGATIVE_Z;
        }

        float gamma_idx = gamma + M_PI_4;
        while (gamma_idx < 0) {
            gamma_idx += M_PI * 2;
        }
        while (gamma_idx >= M_PI*2) {
            gamma_idx -= M_PI*2;
        }

        const unsigned index = gamma_idx / M_PI_4;
        static const Side map[] = {POSITIVE_X, NEGATIVE_Y, NEGATIVE_X, POSITIVE_Y};

        return map[index];
    }

public:
    void copy_from(const EnvironmentMapGenerator &source,
                   const float factor = 1.f);
    inline std::size_t size() const {
        return m_size;
    }
    inline const std::vector<Vector3f> &data(const Side which) const {
        return m_sides[which];
    }

    Vector3f sample(const float theta, const float phi) const override;
    Vector3f sample(const Vector3f direction) const override;

};


class LonLatEnvironmentMapBuffer: public EnvironmentMapGenerator
{
public:
    explicit LonLatEnvironmentMapBuffer(const std::size_t size);
    LonLatEnvironmentMapBuffer(const std::size_t theta_res,
                               const std::size_t phi_res);

private:
    const std::size_t m_theta_res;
    const std::size_t m_phi_res;
    std::vector<Vector3f> m_buffer;

private:
    Vector3f get(const int theta_i, const int phi_i) const;

public:
    void clear();
    void copy_from(const EnvironmentMapGenerator &source);
    void copy_from(const EnvironmentMapGenerator &source, ThreadPool &pool);
    void accumulate_from(const EnvironmentMapGenerator &source,
                         const float decay);
    void accumulate_from(const EnvironmentMapGenerator &source,
                         const float decay,
                         ThreadPool &pool);

    inline const std::vector<Vector3f> &data() const {
        return m_buffer;
    }

    Vector3f sample(const float theta, const float phi) const override;
    Vector3f sample(const Vector3f direction) const override;
};


class GroundPlaneInjector: public EnvironmentMapGenerator
{
public:
    GroundPlaneInjector(
            const EnvironmentMapGenerator &lighting_source,
            const EnvironmentMapGenerator &onto,
            const float viewer_height);

private:
    const EnvironmentMapGenerator &m_lighting_source;
    const EnvironmentMapGenerator &m_onto;
    const float m_viewer_height;

private:
    Vector3f sample_impl(const Vector3f direction) const;

public:
    Vector3f sample(const float theta, const float phi) const override;
    Vector3f sample(const Vector3f direction) const override;

};


class EnvironmentMapPrefilter: public EnvironmentMapGenerator
{
public:
    explicit EnvironmentMapPrefilter(
            const EnvironmentMapGenerator &source,
            const float roughness,
            const std::size_t nsamples);

private:
    const EnvironmentMapGenerator &m_source;
    float m_roughness;
    std::size_t m_nsamples;
    mutable std::mt19937_64 m_engine;
    mutable std::uniform_real_distribution<float> m_distribution;

public:
    inline float roughness() const {
        return m_roughness;
    }

    void set_roughness(const float new_value) {
        m_roughness = new_value;
    }

    Vector3f sample(const float theta, const float phi) const override;
    Vector3f sample(const Vector3f R) const override;
};


struct SHCoefficients {
    Vector4f values[9];

    static SHCoefficients sampled_from_environment(
            const EnvironmentMapGenerator &source,
            const std::size_t nsamples,
            const float scale = 1.f);
};


Vector3f sun_direction(const float latitude,
                       const float time_of_day,
                       const float julian_day);


}

inline std::ostream &operator<<(std::ostream &dest, const ffe::SHCoefficients &src) {
    dest << "SHCoefficients([" << src.values[0];
    for (unsigned i = 1; i < 9; ++i) {
        dest << ", " << src.values[i];
    }
    return dest << "])";
}

#endif
