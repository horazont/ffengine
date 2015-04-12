#include "engine/render/camera.hpp"

namespace engine {

io::Logger &camera_logger = io::logging().get_logger("engine.render.camera");


CameraController::CameraController():
    m_pos(0, 0, 0),
    m_pos_vel(0, 0, 0),
    m_pos_accel(0, 0, 0),
    m_rot(0, 0),
    m_rot_vel(0, 0),
    m_rot_accel(0, 0),
    m_distance(1),
    m_distance_vel(0),
    m_distance_accel(0)
{

}

std::tuple<bool, bool> CameraController::enforce_2d_restriction()
{
    bool bounces[2];
    for (unsigned int i = 0; i < 2; i++) {
        if (m_pos.as_array[i] < m_2d_min.as_array[i]) {
            m_pos.as_array[i] = m_2d_min.as_array[i];
            bounces[i] = true;
        } else if (m_pos.as_array[i] > m_2d_max.as_array[i]) {
            m_pos.as_array[i] = m_2d_max.as_array[i];
            bounces[i] = true;
        }
    }

    return std::make_tuple(bounces[0], bounces[1]);
}

void CameraController::boost_movement(const Vector3f &by)
{
    m_pos_accel += by;
}

void CameraController::boost_rotation(const Vector2f &by)
{
    m_rot_accel += by;
}

void CameraController::restrict_2d_box(const Vector2f &min, const Vector2f &max)
{
    m_2d_min = min;
    m_2d_max = max;
    m_2d_restricted = true;
}

void CameraController::unrestrict_2d_box()
{
    m_2d_restricted = false;
}

void CameraController::set_pos(const Vector3f &pos, bool reset_mechanics)
{
    if (reset_mechanics) {
        m_pos_vel = Vector3f();
        m_pos_accel = Vector3f();
    }

    m_pos = pos;
    if (m_2d_restricted) {
        enforce_2d_restriction();
    }
}

void CameraController::set_rot(const Vector2f &rot, bool reset_mechanics)
{
    if (reset_mechanics) {
        m_rot_vel = Vector2f();
        m_rot_accel = Vector2f();
    }

    m_rot = rot;
}

void CameraController::set_distance(const float distance, bool reset_mechanics)
{
    if (reset_mechanics) {
        m_distance_vel = 0;
        m_distance_accel = 0;
    }

    m_distance = distance;
}

void CameraController::advance(TimeInterval seconds)
{
    const TimeInterval seconds_sqr = seconds * seconds;

    m_pos += m_pos_accel * seconds_sqr / 2. + m_pos_vel * seconds;
    m_pos_vel += m_pos_accel * seconds;

    m_pos_accel /= 180 * seconds;
    m_pos_vel /= 110 * seconds;

    if (m_2d_restricted) {
        bool xbounce, ybounce;
        std::tie(xbounce, ybounce) = enforce_2d_restriction();
        if (xbounce) {
            m_pos_vel[eX] = -0.5*m_pos_vel[eX];
        }
        if (ybounce) {
            m_pos_vel[eY] = -0.5*m_pos_vel[eY];
        }
    }

    m_rot += m_rot_accel * seconds_sqr / 2. + m_rot_vel * seconds;
    m_rot_vel += m_rot_accel * seconds;

    m_rot_accel /= 180 * seconds;
    m_rot_vel /= 110 * seconds;
}


Camera::Camera()
{

}

Camera::~Camera()
{

}

void Camera::advance(TimeInterval)
{

}


/* OrthogonalCamera::OrthogonalCamera(
        float viewport_width,
        float viewport_height):
    Camera(),
    m_controller(),
    m_viewport_width(viewport_width),
    m_viewport_height(viewport_height),
    m_znear(0),
    m_zfar(100),
    m_projection(proj_ortho(0, 0, m_viewport_width, m_viewport_height,
                            m_znear, m_zfar))
{

}

void OrthogonalCamera::update_projection()
{
    m_projection = proj_ortho_center(
                0, 0,
                m_viewport_width, m_viewport_height,
                m_znear, m_zfar);
}

void OrthogonalCamera::set_viewport(const float width, const float height)
{
    m_viewport_width = width;
    m_viewport_height = height;
    update_projection();
}

void OrthogonalCamera::set_znear(const float znear)
{
    m_znear = znear;
    update_projection();
}

void OrthogonalCamera::set_zfar(const float zfar)
{
    m_zfar = zfar;
    update_projection();
}

void OrthogonalCamera::advance(TimeInterval seconds)
{
    m_controller.advance(seconds);
}

void OrthogonalCamera::sync()
{
    camera_logger.log(io::LOG_DEBUG, "synchronizing camera");

    // put 0, 0, 0 into the viewports center
    m_render_projection = m_projection;

    const Vector3f &pos = m_controller.pos();
    const Vector2f &rot = m_controller.rot();
    const float distance = m_controller.distance();

    m_render_view = translation4(pos)
            * rotation4(eX, -rot[eX])
            * rotation4(eZ, rot[eY])
            * scale4(Vector3(1, 1, 1)/distance);

    camera_logger.log(io::LOG_DEBUG)
            << "view = " << m_render_view
            << io::submit;
    camera_logger.log(io::LOG_DEBUG)
            << "proj = " << m_render_projection
            << io::submit;
} */


PerspectivalCamera::PerspectivalCamera():
    Camera(),
    m_controller(),
    m_znear(1.0),
    m_zfar(100),
    m_fovy(45.0)
{

}

Matrix4f PerspectivalCamera::calc_view() const
{
    const Vector3f &pos = m_controller.pos();
    const Vector2f &rot = m_controller.rot();
    const float distance = m_controller.distance();

    return translation4(Vector3f(0, 0, -distance))
                * rotation4(eX, rot[eX])
                * rotation4(eZ, rot[eY])
                * translation4(-pos);
}

Matrix4f PerspectivalCamera::calc_inv_view() const
{
    const Vector3f &pos = m_controller.pos();
    const Vector2f &rot = m_controller.rot();
    const float distance = m_controller.distance();

    return translation4(pos)
            * rotation4(eZ, -rot[eY])
            * rotation4(eX, -rot[eX])
            * translation4(Vector3f(0, 0, distance));
}

Ray PerspectivalCamera::ray(const Vector2f &viewport_pos,
                            const ViewportSize &viewport_size) const
{
    const float f = tan(M_PI_2 - m_fovy/2.);

    const Matrix3f inv_proj_partial(
                float(viewport_size[eX])/viewport_size[eY]/f, 0, 0,
                0, 1/f, 0,
                0, 0, 1);

    const Matrix4f inv_view = calc_inv_view();

    const Vector4f pos4 = inv_view * Vector4f(0, 0, 0, 1);
    const Vector3f pos(pos4[eX], pos4[eY], pos4[eZ]);

    const Vector3f on_view_plane = inv_proj_partial*Vector3f(
                2*viewport_pos[eX]/viewport_size[eX]-1.0,
                1.0-2*viewport_pos[eY]/viewport_size[eY],
                -1.0);

    const Vector4f direction = inv_view * Vector4f(on_view_plane, 0.);

    return Ray{pos, Vector3f(direction[eX],
                             direction[eY],
                             direction[eZ]).normalized()};
}

void PerspectivalCamera::set_fovy(const float fovy)
{
    m_fovy = fovy;
}

void PerspectivalCamera::set_znear(const float znear)
{
    m_znear = znear;
}

void PerspectivalCamera::set_zfar(const float zfar)
{
    m_zfar = zfar;
}

void PerspectivalCamera::advance(TimeInterval seconds)
{
    m_controller.advance(seconds);
}

Matrix4f PerspectivalCamera::render_projection(GLsizei viewport_width, GLsizei viewport_height)
{
    return proj_perspective(m_render_fovy,
                            float(viewport_width)/viewport_height,
                            m_render_znear, m_render_zfar);
}

void PerspectivalCamera::sync()
{
    camera_logger.log(io::LOG_DEBUG, "synchronizing camera");

    m_render_fovy = m_fovy;
    m_render_zfar = m_zfar;
    m_render_znear = m_znear;

    /* m_render_view = translation4(Vector3f(pos[eX], pos[eY], 0.f))
            * rotation4(eX, -rot[eX])
            * rotation4(eZ, rot[eY])
            * translation4(Vector3f(0, 0, -distance)); */

    m_render_view = calc_view();
    m_render_inv_view = calc_inv_view();
}

}
