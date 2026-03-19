#pragma once

#include "render/RenderState.h"

#include <cstdint>

class IScenePresenter {
  public:
    virtual ~IScenePresenter() = default;
    virtual RenderState tick(uint64_t t) = 0;
};
