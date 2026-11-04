#pragma once

#include "LocalGenerator.hpp"

#include <cstdint>
#include <vector>

namespace aideco {
    struct SelectionRect {
        float left = 0.f;
        float bottom = 0.f;
        float right = 0.f;
        float top = 0.f;

        float width() const { return right - left; }
        float height() const { return top - bottom; }
        bool valid() const { return width() > 1.f && height() > 1.f; }
    };

    // A measured rectangle from an existing layout object. The planner uses these
    // anchors to follow real geometry instead of scattering decorations in a grid.
    struct GeometryAnchor {
        float left = 0.f;
        float bottom = 0.f;
        float right = 0.f;
        float top = 0.f;

        float width() const { return right - left; }
        float height() const { return top - bottom; }
        float centerX() const { return (left + right) * 0.5f; }
        float centerY() const { return (bottom + top) * 0.5f; }
        bool valid() const { return width() > 2.f && height() > 2.f; }
    };

    struct ObjectPlacement {
        int objectID = 1764;
        float x = 0.f;
        float y = 0.f;
        float scale = 1.f;
        float rotation = 0.f;
        unsigned char opacity = 255;
        unsigned char colorSlot = 0; // 0 = primary, 1 = accent, 2 = secondary
    };

    struct PlacementBatch {
        std::vector<ObjectPlacement> objects;
        SelectionRect safeRect;
        std::uint32_t seed = 0;
        int analyzedAnchors = 0;
        int activeSegments = 0;
        bool valid = false;
        std::string error;
    };

    // The placement planner uses measured layout geometry and does not touch the game/editor.
    PlacementBatch makePlacements(
        GenerationPlan const& plan,
        SelectionRect const& levelBounds,
        std::vector<GeometryAnchor> const& layoutGeometry,
        std::uint32_t seed
    );
}
