#include "engine/render/camera.hpp"

namespace engine {

io::Logger &camera_logger = io::logging().get_logger("engine.render.camera");

CameraPlaneController::CameraPlaneController():
    m_pos(0, 0),
    m_pos_vel(0, 0),
    m_pos_accel(0, 0),
    m_rot(0, 0),
    m_rot_vel(0, 0),
    m_rot_accel(0, 0),
    m_distance(1),
    m_distance_vel(0),
    m_distance_accel(0)
{

}

void CameraPlaneController::set_pos(const Vector2f &pos, bool reset_mechanics)
{
    if (reset_mechanics) {
        m_pos_vel = Vector2f();
        m_pos_accel = Vector2f();
    }

    m_pos = pos;
}

void CameraPlaneController::set_rot(const Vector2f &rot, bool reset_mechanics)
{
    if (reset_mechanics) {
        m_rot_vel = Vector2f();
        m_rot_accel = Vector2f();
    }

    m_rot = rot;
}

void CameraPlaneController::set_distance(const float distance, bool reset_mechanics)
{
    if (reset_mechanics) {
        m_distance_vel = 0;
        m_distance_accel = 0;
    }

    m_distance = distance;
}

void CameraPlaneController::advance(TimeInterval seconds)
{
    const TimeInterval seconds_sqr = seconds * seconds;

    m_pos += m_pos_accel * seconds_sqr / 2. + m_pos_vel * seconds;
    m_pos_vel += m_pos_accel * seconds;

    m_pos_accel /= 180 * seconds;
    m_pos_vel /= 110 * seconds;

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

void Camera::configure_context(RenderContext &context)
{
    context.set_projection(m_render_projection);
    context.set_view(m_render_view);
}


OrthogonalCamera::OrthogonalCamera(
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

    const Vector2f &pos = m_controller.pos();
    const Vector2f &rot = m_controller.rot();
    const float distance = m_controller.distance();

    m_render_view = translation4(Vector3f(pos[eX], pos[eY], 0.0f))
            * rotation4(eX, -rot[eX])
            * rotation4(eZ, rot[eY])
            * scale4(Vector3(1, 1, 1)/distance);

    camera_logger.log(io::LOG_DEBUG)
            << "view = " << m_render_view
            << io::submit;
    camera_logger.log(io::LOG_DEBUG)
            << "proj = " << m_render_projection
            << io::submit;
}


PerspectivalCamera::PerspectivalCamera():
    Camera(),
    m_controller(),
    m_viewport_width(0),
    m_viewport_height(0),
    m_znear(1.0),
    m_zfar(100),
    m_fovy(45.0),
    m_projection(Identity)
{

}

void PerspectivalCamera::update_projection()
{
    m_projection = proj_perspective(m_fovy,
                                    m_viewport_width/m_viewport_height,
                                    m_znear, m_zfar);
}

void PerspectivalCamera::set_viewport(const float width, const float height)
{
    m_viewport_width = width;
    m_viewport_height = height;
    update_projection();
}

void PerspectivalCamera::set_znear(const float znear)
{
    m_znear = znear;
    update_projection();
}

void PerspectivalCamera::set_zfar(const float zfar)
{
    m_zfar = zfar;
    update_projection();
}

void PerspectivalCamera::advance(TimeInterval seconds)
{
    m_controller.advance(seconds);
}

void PerspectivalCamera::sync()
{
    camera_logger.log(io::LOG_DEBUG, "synchronizing camera");

    // put 0, 0, 0 into the viewports center
    m_render_projection = m_projection;

    const Vector2f &pos = m_controller.pos();
    const Vector2f &rot = m_controller.rot();
    const float distance = m_controller.distance();

    m_render_view = translation4(Vector3f(pos[eX], pos[eY], 0.f))
            * rotation4(eX, -rot[eX])
            * rotation4(eZ, rot[eY])
            * translation4(Vector3f(0, 0, -distance));

    camera_logger.log(io::LOG_DEBUG)
            << "view = " << m_render_view
            << io::submit;
    camera_logger.log(io::LOG_DEBUG)
            << "proj = " << m_render_projection
            << io::submit;
}

}
