
#pragma once


#include "PostEffect.h"


class PassthruPostEffect : public PostEffect
{
public:

    PassthruPostEffect();

protected:

    void updateConstants() override;
};