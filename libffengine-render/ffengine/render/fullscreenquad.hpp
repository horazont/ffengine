#ifndef SCC_RENDER_FULLSCREENQUAD_H
#define SCC_RENDER_FULLSCREENQUAD_H

#include "ffengine/render/scenegraph.hpp"
#include "ffengine/render/renderpass.hpp"


namespace ffe {

class FullScreenQuadNode: public scenegraph::Node
{
public:
    FullScreenQuadNode();

private:
    ffe::Material m_material;

    IBOAllocation m_ibo_alloc;
    VBOAllocation m_vbo_alloc;

    bool m_linked;

public:
    ffe::MaterialPass &make_pass_material(RenderPass &pass) {
        ffe::MaterialPass &mpass = m_material.make_pass_material(pass);
        mpass.set_depth_mask(false);
        mpass.set_depth_test(false);
        mpass.set_order(-1000);
        return mpass;
    }

public:
    void prepare(RenderContext &context) override;
    void render(RenderContext &context) override;
    void sync() override;
};

}

#endif
