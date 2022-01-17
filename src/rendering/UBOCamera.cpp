#include <rendering/UBOCamera.h>
#include <helpers/uboBindings.h>


void UBOCamera::use(int windowWidth, int windowHeight, bool doUpdate /*=true*/)
{
    bind();
    if (changed_ && doUpdate)
        update(windowWidth, windowHeight);
}

void UBOCamera::init()
{
    glGenBuffers(1, &ubo_);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(CameraData), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UBOCamera::bind()
{
    glBindBufferBase(GL_UNIFORM_BUFFER, CAMBINDING, ubo_);
}

void UBOCamera::uploadUboData()
{
    glBindBuffer(GL_UNIFORM_BUFFER, ubo_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(CameraData), &data_);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
