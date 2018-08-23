#include "ffengine/math/sky.hpp"

#include <future>

#include "ffengine/math/algo.hpp"
#include "ffengine/math/shading.hpp"

#include "ffengine/math/plane.hpp"
#include "ffengine/math/ray.hpp"
#include "ffengine/math/intersect.hpp"

extern "C" {
#include "ArHosekSkyModel.h"
}

namespace ffe {


const Vector3f SkyGenerator::up(0, 0, 1);

EnvironmentMapGenerator::~EnvironmentMapGenerator()
{

}

SkyGenerator::SkyGenerator(const Vector3f sun_direction):
    m_sun_direction(sun_direction.normalized())
{

}

SkyGenerator::~SkyGenerator()
{

}

HosekWilkieGenerator::HosekWilkieGenerator(const float turbidity,
                                           const Vector3f sun_direction,
                                           const float ground_albedo):
    SkyGenerator(sun_direction),
    m_model(arhosek_rgb_skymodelstate_alloc_init(turbidity,
                                                 ground_albedo,
                                                 std::acos(m_sun_direction * up)))
{

}

/* we override so that std::unique_ptr does not need the complete type at
 * class declaration type. See <https://stackoverflow.com/a/6089065/1248008>:
 * ~std::unique_ptr<> requires the complete type. If we don’t add an explicit
 * implementation of the destructor here, the implicit definition needs to be
 * instantiable from just knowing the class declaration, which isn’t possible
 * if the complete type of ArHosekSkyModelState isn’t available. */
HosekWilkieGenerator::~HosekWilkieGenerator()
{

}

Vector3f HosekWilkieGenerator::sample_hw(const float theta, const float gamma) const
{
    if (theta >= M_PI_2) {
        return Vector3f(0, 0, 0);
    }
    Vector3f result;
    ArHosekSkyModelState *model = m_model.get();
    for (unsigned int ch = 0; ch < 3; ++ch) {
        const float vf = arhosek_tristim_skymodel_radiance(
                    model, theta, gamma, ch) /* / model->radiances[ch] */;
        result[ch] = vf;
    }
    return result;
}

float normalize_angle(float alpha)
{
    while (alpha < 0) {
        alpha += 2 * M_PI;
    }
    while (alpha >= 2 * M_PI) {
        alpha -= 2 * M_PI;
    }
    return alpha;
}

Vector3f HosekWilkieGenerator::sample(const float theta, const float phi) const
{
    return this->HosekWilkieGenerator::sample(angles_to_vector(theta, phi));
}

Vector3f HosekWilkieGenerator::sample(const Vector3f direction) const
{
    const Vector3f dir_norm = direction.normalized();
    const float theta = std::acos(dir_norm[eZ]);
    const float gamma = std::acos(dir_norm * m_sun_direction);
    return sample_hw(theta, gamma);
}

/* from "A Practical Analytic Model for Daylight" */
Vector3f sun_direction(const float latitude,
                       const float time_of_day,
                       const float julian_day)
{
    /* we don’t do the conversion here because the timezone isn’t defined anyways */
    const float solar_time = time_of_day;
    const float solar_declination = 0.4093 * std::sin(M_PI*2*(julian_day - 81) / 368);

    const float theta_s = M_PI_2 - std::asin(
                std::sin(latitude)*std::sin(solar_declination) -
                std::cos(latitude)*std::cos(solar_declination)*std::cos(M_PI*solar_time/12)
                );
    const float phi_s = std::atan(
                -std::cos(solar_declination)*std::sin(M_PI*solar_time/12) /
                (std::cos(latitude)*std::sin(solar_declination) -
                 std::sin(latitude)*std::cos(solar_declination)*std::cos(M_PI*solar_time/12)));

    return Vector3f(std::cos(phi_s)*std::sin(theta_s),
                    std::sin(phi_s)*std::sin(theta_s),
                    std::cos(theta_s));
}

CubeMapSkyBuffer::CubeMapSkyBuffer(const std::size_t size):
    m_size(size)
{
    for (auto &buffer: m_sides) {
        buffer.resize(size*size);
    }
}

void sample_cubemap_side(const EnvironmentMapGenerator &source,
                         const std::size_t size,
                         const Vector3f vtx,
                         const Vector3f vty,
                         const float vconst_scale, const float factor,
                         std::vector<Vector3f> &buffer)
{
    Vector3f *out = &buffer[0];

    const Vector3f vconst = (vtx % vty) * vconst_scale;

    for (unsigned int ty = 0; ty < size; ++ty) {
        const float vy = ((ty + 0.5) / size - 0.5) * 2;
        for (unsigned int tx = 0; tx < size; ++tx) {
            const float vx = ((tx + 0.5) / size - 0.5) * 2;
            const Vector3f dir = (vtx * vx + vty * vy + vconst).normalized();
            *out++ = source.sample(dir) * factor;
        }
    }
}

void CubeMapSkyBuffer::copy_from(const EnvironmentMapGenerator &source,
                                 const float factor)
{
    static const std::array<std::tuple<Side, Vector3f, Vector3f, float>, 6> sides = {
        {
            { POSITIVE_X, Vector3f(0, 0, -1), Vector3f(0, -1, 0), -1 },
            { POSITIVE_Y, Vector3f(1, 0, 0), Vector3f(0, 0, 1), -1 },
            { POSITIVE_Z, Vector3f(1, 0, 0), Vector3f(0, -1, 0), -1 },
            { NEGATIVE_X, Vector3f(0, 0, 1), Vector3f(0, -1, 0), -1 },
            { NEGATIVE_Y, Vector3f(1, 0, 0), Vector3f(0, 0, -1), -1 },
            { NEGATIVE_Z, Vector3f(-1, 0, 0), Vector3f(0, -1, 0), -1 },
        }
    };

    std::array<std::future<void>, 6> tasks;

    const std::size_t size = m_size;

    for (unsigned int i = 0; i < sides.size(); ++i) {
        auto side_info = sides[i];
        Side side;
        Vector3f vtx, vty;
        float vconst_scale;
        std::tie(side, vtx, vty, vconst_scale) = side_info;
        std::vector<Vector3f> &side_buffer = m_sides[side];

        tasks[i] = ThreadPool::global().submit_task(std::packaged_task<void()>([&source, vtx, vty, vconst_scale, size, factor, &side_buffer](){
            sample_cubemap_side(source, size,
                                vtx, vty, vconst_scale,
                                factor,
                                side_buffer);
        }));
    }

    for (auto &fut: tasks) {
        fut.wait();
    }
}

Vector3f CubeMapSkyBuffer::sample(const float theta, const float phi) const
{
    if (theta <= M_PI_2) {

    }
}

Vector3f CubeMapSkyBuffer::sample(const Vector3f direction) const
{

}

LonLatEnvironmentMapBuffer::LonLatEnvironmentMapBuffer(const std::size_t size):
    LonLatEnvironmentMapBuffer(size, size*2)
{

}

LonLatEnvironmentMapBuffer::LonLatEnvironmentMapBuffer(
        const std::size_t theta_res,
        const std::size_t phi_res):
    m_theta_res(theta_res),
    m_phi_res(phi_res),
    m_buffer(theta_res * phi_res)
{

}

Vector3f LonLatEnvironmentMapBuffer::get(const int theta_i, const int phi_i) const
{
    return m_buffer[theta_i * m_phi_res + phi_i];
}

void LonLatEnvironmentMapBuffer::clear()
{
    m_buffer.clear();
    m_buffer.resize(m_phi_res*m_theta_res);
}

void sample_lonlat_range(const EnvironmentMapGenerator &source,
                         const unsigned int theta0,
                         const unsigned int theta1,
                         const unsigned int phi0,
                         const unsigned int phi1,
                         const unsigned int theta_res,
                         const unsigned int phi_res,
                         std::vector<Vector3f> &buffer)
{
    for (unsigned int theta_i = theta0; theta_i < theta1; ++theta_i) {
        const float theta = (theta_i + 0.5) / theta_res * M_PI;
        Vector3f *out = &buffer[theta_i * phi_res];
        for (unsigned int phi_i = phi0; phi_i < phi1; ++phi_i) {
            const float phi = (phi_i + 0.5) / phi_res * 2 * M_PI;
            *out++ = source.sample(theta, phi);
        }
    }
}

void accum_lonlat_range(const EnvironmentMapGenerator &source,
                        const unsigned int theta0,
                        const unsigned int theta1,
                        const unsigned int phi0,
                        const unsigned int phi1,
                        const unsigned int theta_res,
                        const unsigned int phi_res,
                        std::vector<Vector3f> &buffer,
                        const float decay)
{
    const float ndecay = 1.f - decay;
    for (unsigned int theta_i = theta0; theta_i < theta1; ++theta_i) {
        const float theta = (theta_i + 0.5) / theta_res * M_PI;
        Vector3f *out = &buffer[theta_i * phi_res];
        for (unsigned int phi_i = phi0; phi_i < phi1; ++phi_i) {
            const float phi = (phi_i + 0.5) / phi_res * 2 * M_PI;
            *out = *out * ndecay + source.sample(theta, phi) * decay;
            ++out;
        }
    }
}

void LonLatEnvironmentMapBuffer::copy_from(
        const EnvironmentMapGenerator &source)
{
    sample_lonlat_range(source,
                        0, m_theta_res,
                        0, m_phi_res,
                        m_theta_res, m_phi_res,
                        m_buffer);
}

void LonLatEnvironmentMapBuffer::copy_from(
        const EnvironmentMapGenerator &source,
        ThreadPool &pool)
{
    const std::size_t phi_res = m_phi_res;
    const std::size_t theta_res = m_theta_res;
    const unsigned int range_step = std::max((theta_res + pool.workers() - 1) / pool.workers(), std::size_t(16));
    std::vector<std::future<void> > tasks;
    tasks.reserve(pool.workers());
    std::vector<Vector3f> &buffer = m_buffer;
    for (unsigned int theta0 = 0; theta0 < theta_res; theta0 += range_step) {
        const unsigned int theta1 = std::min(theta_res, std::size_t(theta0 + range_step));

        tasks.emplace_back(pool.submit_task(std::packaged_task<void()>(
            [=, &source, &buffer](){
                sample_lonlat_range(source,
                                    theta0, theta1,
                                    0, phi_res,
                                    theta_res, phi_res,
                                    buffer);
            }
        )));
    }

    for (auto &fut: tasks) {
        fut.wait();
    }
}

void LonLatEnvironmentMapBuffer::accumulate_from(
        const EnvironmentMapGenerator &source,
        const float decay)
{
    accum_lonlat_range(source,
                       0, m_theta_res,
                       0, m_phi_res,
                       m_theta_res, m_phi_res,
                       m_buffer,
                       decay);
}

void LonLatEnvironmentMapBuffer::accumulate_from(
        const EnvironmentMapGenerator &source,
        const float decay,
        ThreadPool &pool)
{
    const std::size_t phi_res = m_phi_res;
    const std::size_t theta_res = m_theta_res;
    const unsigned int range_step = std::max((theta_res + pool.workers() - 1) / pool.workers(), std::size_t(16));
    std::vector<std::future<void> > tasks;
    tasks.reserve(pool.workers());
    std::vector<Vector3f> &buffer = m_buffer;
    for (unsigned int theta0 = 0; theta0 < theta_res; theta0 += range_step) {
        const unsigned int theta1 = std::min(theta_res, std::size_t(theta0 + range_step));

        tasks.emplace_back(pool.submit_task(std::packaged_task<void()>(
            [=, &source, &buffer](){
                accum_lonlat_range(source,
                                   theta0, theta1,
                                   0, phi_res,
                                   theta_res, phi_res,
                                   buffer,
                                   decay);
            }
        )));
    }

    for (auto &fut: tasks) {
        fut.wait();
    }
}

static std::tuple<int, int, float> map_coordinate(const float coord,
                                                  const float frange,
                                                  const unsigned int irange)
{
    const float normalized = fmodpositive(coord, frange) / frange;
    const float scaled = normalized * irange - 0.5;
    int integerized = scaled;
    const float remainder = scaled - integerized;
    int successor = integerized + 1;
    if (integerized < 0) {
        integerized += irange;
    }
    if (successor >= irange) {
        successor = 0;
    }
    return std::make_tuple(integerized, successor, remainder);
}

Vector3f LonLatEnvironmentMapBuffer::sample(const float theta, const float phi) const
{
    float theta_rem, phi_rem;
    int theta_i0, theta_i1, phi_i0, phi_i1;
    std::tie(theta_i0, theta_i1, theta_rem) = map_coordinate(theta, M_PI, m_theta_res);
    std::tie(phi_i0, phi_i1, phi_rem) = map_coordinate(phi, 2*M_PI, m_phi_res);

    const Vector3f p00 = get(theta_i0, phi_i0);
    const Vector3f p01 = get(theta_i0, phi_i1);
    const Vector3f p10 = get(theta_i1, phi_i0);
    const Vector3f p11 = get(theta_i1, phi_i1);

    const Vector3f i0 = interp_linear(p00, p01, phi_rem);
    const Vector3f i1 = interp_linear(p10, p11, phi_rem);

    return interp_linear(i0, i1, theta_rem);
}

Vector3f LonLatEnvironmentMapBuffer::sample(const Vector3f direction) const
{
    const Vector2f theta_phi = vector_to_angles(direction);
    return sample(theta_phi[0], theta_phi[1]);
}


static const Plane ground_plane(0.f, Vector3f(0, 0, 1));

GroundPlaneInjector::GroundPlaneInjector(
        const EnvironmentMapGenerator &lighting_source,
        const EnvironmentMapGenerator &onto,
        const float viewer_height):
    m_lighting_source(lighting_source),
    m_onto(onto),
    m_viewer_height(viewer_height)
{

}

Vector3f GroundPlaneInjector::sample_impl(const Vector3f direction) const
{
    Ray r(Vector3f(0, 0, m_viewer_height), direction);
    PlaneSide side;
    float t;
    std::tie(t, side) = isect_plane_ray(ground_plane, r);
    if (side != PlaneSide::BOTH) {
        return Vector3f(0, 0, 0);
    }

    const Vector3f p = r.origin + r.direction * t;

    /* we don’t need to do full shading here, it’ll be fine if we approximate
     * things */

    const Vector3f incident_ray = -r.direction;
    const Vector3f normal(0, 0, 1);

    const float nDotV = clamp(normal * incident_ray, 0.f, 1.f);
    const Vector3f light_ray = 2 * nDotV * normal - incident_ray;

    return m_lighting_source.sample(light_ray);
}

Vector3f GroundPlaneInjector::sample(const float theta, const float phi) const
{
    if (theta <= M_PI) {
        return m_onto.sample(theta, phi);
    }

    return sample_impl(angles_to_vector(theta, phi));
}

Vector3f GroundPlaneInjector::sample(const Vector3f direction) const
{
    if (direction[eZ] >= 0.f) {
        return m_onto.sample(direction);
    }

    return sample_impl(direction);
}


EnvironmentMapPrefilter::EnvironmentMapPrefilter(const EnvironmentMapGenerator &source,
        const float roughness,
        const std::size_t nsamples):
    m_source(source),
    m_roughness(roughness),
    m_nsamples(nsamples),
    m_distribution(0, 1)
{
}

Vector3f EnvironmentMapPrefilter::sample(const float theta, const float phi) const
{
    const Vector3f R = angles_to_vector(theta, phi);
    return this->EnvironmentMapPrefilter::sample(R);
}

Vector3f EnvironmentMapPrefilter::sample(const Vector3f R) const
{
    const Vector3f N = R;
    const Vector3f V = R;

    const std::size_t nsamples = m_nsamples;

    Vector3f prefiltered(0, 0, 0);
    float weight = 0;
    for (unsigned int i = 0; i < nsamples; ++i) {
        const Vector2f Xi(m_distribution(m_engine), m_distribution(m_engine));
        const Vector3f H = importance_sample_ggx(Xi, m_roughness, N);
        const Vector3f L = 2 * (V * H) * H - V;

        const float NoL = clamp(N * L, 0.f, 1.f);
        if (NoL > 0) {
            prefiltered += m_source.sample(L) * NoL;
            weight += NoL;
        }
    }

    return prefiltered / weight;
}

template <typename engine_t>
static Vector3f generate_sphere_vector(engine_t &engine)
{
    std::uniform_real_distribution<float> dz(0, 1);
    std::uniform_real_distribution<float> dphi(-M_PI, M_PI);

    const float phi = dphi(engine);
    const float z = dz(engine);
    const float nz = std::sqrt(1 - sqr(z));

    return Vector3f(
                std::cos(phi)*nz,
                std::sin(phi)*nz,
                z);
}

SHCoefficients SHCoefficients::sampled_from_environment(
        const EnvironmentMapGenerator &source,
        const std::size_t nsamples,
        const float scale)
{
    std::random_device dev;
    std::mt19937_64 engine(dev());

    SHCoefficients result;

    const float weight = 1./nsamples;

    for (unsigned int i = 0; i < nsamples; ++i) {
        const Vector3f sampler = generate_sphere_vector(engine);
        const Vector4f colour = Vector4f(source.sample(sampler) * scale, 1.f);

        const float weight1 = weight * 4 / 17;
        const float weight2 = weight * 8 / 17;
        const float weight3 = weight * 15 / 17;
        const float weight4 = weight * 5 / 68;
        const float weight5 = weight * 15 / 68;

        result.values[0] += colour * weight1;

        result.values[1] += colour * weight2 * sampler[eX];
        result.values[2] += colour * weight2 * sampler[eY];
        result.values[3] += colour * weight2 * sampler[eZ];

        result.values[4] += colour * weight3 * sampler[eX] * sampler[eZ];
        result.values[5] += colour * weight3 * sampler[eZ] * sampler[eY];
        result.values[6] += colour * weight3 * sampler[eY] * sampler[eX];

        result.values[7] += colour * weight4 * (3.0 * sqr(sampler[eZ]) - 1.0);
        result.values[8] += colour * weight5 * (sqr(sampler[eX]) - sqr(sampler[eY]));
    }

    for (auto &vec: result.values) {
        vec *= 4 * M_PI / nsamples;
    }

    return result;
}

}
