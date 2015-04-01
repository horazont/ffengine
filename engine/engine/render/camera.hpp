#ifndef SCC_ENGINE_RENDER_CAMERA_H
#define SCC_ENGINE_RENDER_CAMERA_H

#include "engine/common/types.hpp"

#include "engine/math/ray.hpp"

#include "engine/render/rendergraph.hpp"

namespace engine {

class CameraController
{
public:
    CameraController();

private:
    Vector3f m_pos;
    Vector3f m_pos_vel;
    Vector3f m_pos_accel;

    Vector2f m_rot;
    Vector2f m_rot_vel;
    Vector2f m_rot_accel;

    float m_distance;
    float m_distance_vel;
    float m_distance_accel;

    bool m_moving, m_rotating;

public:
    inline const Vector3f &pos() const
    {
        return m_pos;
    }

    inline const Vector2f &rot() const
    {
        return m_rot;
    }

    inline const float &distance() const
    {
        return m_distance;
    }

    void set_pos(const Vector3f &pos, bool reset_mechanics = true);
    void set_rot(const Vector2f &rot, bool reset_mechanics = true);
    void set_distance(const float distance, bool reset_mechanics = true);

    void boost_movement(const Vector3f &by);
    void boost_rotation(const Vector2f &by);
    void boost_zoom(const float by);

    void stop_all();

public:
    void advance(TimeInterval seconds);

};


/**
 * @brief The Camera class
 *
 * A camera is responsible for setting up both the projection and the view
 * matrices in a RenderContext. When a scene graph is rendered, a Camera needs
 * to be specified which will set up the basic RenderContext.
 */
class Camera
{
public:
    Camera();
    virtual ~Camera();

protected:
    Matrix4f m_render_projection;
    Matrix4f m_render_view;

public:
    virtual void advance(TimeInterval seconds);
    void configure_context(RenderContext &context);
    virtual void sync() = 0;

};


class OrthogonalCamera: public Camera
{
public:
    OrthogonalCamera(
            float viewport_width,
            float viewport_height);

private:
    CameraController m_controller;

    float m_viewport_width;
    float m_viewport_height;
    float m_znear;
    float m_zfar;

    Matrix4f m_projection;

protected:
    void update_projection();

public:
    inline float viewport_height() const
    {
        return m_viewport_height;
    }

    inline float viewport_width() const
    {
        return m_viewport_width;
    }

    inline float zfar() const
    {
        return m_zfar;
    }

    inline float znear() const
    {
        return m_znear;
    }

    inline CameraController &controller()
    {
        return m_controller;
    }

public:
    void set_viewport(const float width, const float height);
    void set_znear(const float znear);
    void set_zfar(const float zfar);

public:
    void advance(TimeInterval seconds) override;
    void sync() override;

};


class PerspectivalCamera: public Camera
{
public:
    PerspectivalCamera();

private:
    CameraController m_controller;

    float m_viewport_width;
    float m_viewport_height;
    float m_znear;
    float m_zfar;

    float m_fovy;

    Matrix4f m_projection;

protected:
    Matrix4f calc_view() const;
    Matrix4f calc_inv_view() const;
    void update_projection();

public:
    inline float viewport_height() const
    {
        return m_viewport_height;
    }

    inline float viewport_width() const
    {
        return m_viewport_width;
    }

    inline float zfar() const
    {
        return m_zfar;
    }

    inline float znear() const
    {
        return m_znear;
    }

    inline float fovy() const
    {
        return m_fovy;
    }

    inline CameraController &controller()
    {
        return m_controller;
    }

public:
    Ray ray(const Vector2f viewport_pos) const;

public:
    void set_fovy(const float fovy);
    void set_viewport(const float width, const float height);
    void set_znear(const float znear);
    void set_zfar(const float zfar);

public:
    void advance(TimeInterval seconds) override;
    void sync() override;

};

}

#endif
