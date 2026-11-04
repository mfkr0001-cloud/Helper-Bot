#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>

#include "../src/LocalGenerator.hpp"
#include "../src/DecorationPlacer.hpp"

namespace {
    std::vector<aideco::GeometryAnchor> makeLayout(float left, float right, float xStep = 72.f) {
        std::vector<aideco::GeometryAnchor> geometry;
        int index = 0;
        for (float x = left + 28.f; x < right - 45.f; x += xStep, ++index) {
            // A basic ground/platform structure, then varied blocks above it.
            geometry.push_back({x, 18.f, x + 54.f, 42.f});
            if (index % 2 == 0) {
                geometry.push_back({x + 14.f, 42.f, x + 38.f, 64.f});
            }
            if (index % 3 == 0) {
                geometry.push_back({x + 30.f, 118.f, x + 58.f, 148.f});
            }
            if (index % 4 == 1) {
                geometry.push_back({x + 5.f, 80.f, x + 21.f, 100.f});
            }
        }
        return geometry;
    }

    float distanceToRect(float x, float y, aideco::GeometryAnchor const& rect) {
        float dx = std::max({rect.left - x, 0.f, x - rect.right});
        float dy = std::max({rect.bottom - y, 0.f, y - rect.top});
        return std::sqrt(dx * dx + dy * dy);
    }
}

int main() {
    auto russianPrompt = aideco::makePlan(
        "肖校孝校袪袠小孝袠效袝小袣袠袡 袚袨袪袨袛, 小袠袧袠袡 袧袝袨袧 袠 小袙袝孝携些袠袝小携 袥袠袧袠袠",
        "AUTO", "HIGH", "#28A9FF", "TRIAD", "RHYTHM"
    );
    assert(russianPrompt.style == "NEON");
    assert(russianPrompt.suggestedObjectCount == 54);
    assert(russianPrompt.suggestedLayerCount >= 4);
    assert(!russianPrompt.layers.empty());
    assert(russianPrompt.summary.find("小袥袨袠 袨肖袨袪袦袥袝袧袠携") != std::string::npos);

    auto industrialPrompt = aideco::makePlan(
        "Dark mechanical factory with orange lights",
        "AUTO", "ULTRA", "not-a-color", "MONO", "ASYMMETRIC"
    );
    assert(industrialPrompt.style == "TECHNO");
    assert(industrialPrompt.primaryColor == "#28A9FF");
    assert(industrialPrompt.secondaryColor == industrialPrompt.primaryColor);
    assert(industrialPrompt.suggestedObjectCount == 76);
    assert(industrialPrompt.layers.size() == 6);

    const aideco::SelectionRect bounds{-180.f, 0.f, 1320.f, 360.f};
    const auto layout = makeLayout(bounds.left, bounds.right);
    assert(layout.size() > 20);

    auto placements = aideco::makePlacements(russianPrompt, bounds, layout, 12345u);
    assert(placements.valid);
    assert(!placements.objects.empty());
    assert(placements.analyzedAnchors > 20);
    assert(placements.activeSegments > 0);
    assert(placements.safeRect.left >= bounds.left);
    assert(placements.safeRect.right <= bounds.right);
    assert(placements.safeRect.bottom >= bounds.bottom);
    assert(placements.safeRect.top <= bounds.top);

    for (auto const& placement : placements.objects) {
        assert(placement.x >= placements.safeRect.left - 0.01f);
        assert(placement.x <= placements.safeRect.right + 0.01f);
        assert(placement.y >= placements.safeRect.bottom - 0.01f);
        assert(placement.y <= placements.safeRect.top + 0.01f);
        assert(placement.objectID == 1753 || placement.objectID == 1754 ||
               placement.objectID == 1764 || placement.objectID == 1765 ||
               placement.objectID == 1766 || placement.objectID == 1767 ||
               placement.objectID == 1768 || placement.objectID == 1836 ||
               placement.objectID == 1837 || placement.objectID == 1838 ||
               placement.objectID == 1886 || placement.objectID == 1887 ||
               placement.objectID == 1888);

        // Generated objects must not be placed inside the original gameplay geometry.
        const float clearance = std::max(3.f, placement.scale * 12.f) + 4.49f;
        for (auto const& anchor : layout) {
            assert(distanceToRect(placement.x, placement.y, anchor) + 0.01f >= clearance);
        }
    }

    // Same seed + same measured layout must produce an identical plan.
    auto repeat = aideco::makePlacements(russianPrompt, bounds, layout, 12345u);
    assert(repeat.valid);
    assert(repeat.objects.size() == placements.objects.size());
    for (size_t i = 0; i < placements.objects.size(); ++i) {
        assert(repeat.objects[i].objectID == placements.objects[i].objectID);
        assert(repeat.objects[i].x == placements.objects[i].x);
        assert(repeat.objects[i].y == placements.objects[i].y);
    }

    // Empty canvas is rejected: Auto Deco decorates an existing layout rather than
    // inventing arbitrary objects when there is no gameplay geometry to follow.
    auto empty = aideco::makePlacements(
        russianPrompt, {0.f, 0.f, 1200.f, 360.f}, {}, 7u
    );
    assert(!empty.valid);
    assert(empty.error.find("layout") != std::string::npos);

    // All 50 named styles remain available and each style can decorate the same layout.
    auto const& styles = aideco::availableStyles();
    assert(styles.size() == 50);
    for (size_t i = 0; i < styles.size(); ++i) {
        assert(std::find(styles.begin(), styles.begin() + static_cast<long>(i), styles[i]) ==
               styles.begin() + static_cast<long>(i));
        auto stylePlan = aideco::makePlan(
            "level decor", styles[i], "MEDIUM", "#28A9FF", "AUTO", "BALANCED"
        );
        if (styles[i] == "AUTO") assert(stylePlan.style != "AUTO");
        else assert(stylePlan.style == styles[i]);
        auto styleBatch = aideco::makePlacements(stylePlan, bounds, layout, 1000u + static_cast<unsigned>(i));
        assert(styleBatch.valid);
        assert(!styleBatch.objects.empty());
        assert(styleBatch.objects.size() <= 520);
    }

    // Long levels create more analyzed geometry sections and more decor than short ones.
    auto shortBounds = aideco::SelectionRect{0.f, 0.f, 900.f, 360.f};
    auto longBounds = aideco::SelectionRect{0.f, 0.f, 4200.f, 360.f};
    auto shortLayout = makeLayout(shortBounds.left, shortBounds.right);
    auto longLayout = makeLayout(longBounds.left, longBounds.right);
    auto shortPass = aideco::makePlacements(industrialPrompt, shortBounds, shortLayout, 202655u);
    auto longPass = aideco::makePlacements(industrialPrompt, longBounds, longLayout, 202656u);
    assert(shortPass.valid && longPass.valid);
    assert(longPass.activeSegments > shortPass.activeSegments);
    assert(longPass.objects.size() > shortPass.objects.size());

    // A tiny or invalid work area is rejected safely.
    auto compact = aideco::makePlacements(
        russianPrompt, {0.f, 0.f, 12.f, 12.f}, makeLayout(0.f, 12.f), 8u
    );
    assert(!compact.valid);
    auto tiny = aideco::makePlacements(russianPrompt, {0.f, 0.f, 0.f, 0.f}, layout, 1u);
    assert(!tiny.valid);

    // Stress pass: a long, dense level must remain bounded and deterministic.
    auto stressBounds = aideco::SelectionRect{0.f, 0.f, 18000.f, 360.f};
    auto stressLayout = makeLayout(stressBounds.left, stressBounds.right, 42.f);
    auto stressPlan = aideco::makePlan(
        "neon cyber city", "NEON", "ULTRA", "#28A9FF", "TRIAD", "BALANCED"
    );
    auto stressBatch = aideco::makePlacements(stressPlan, stressBounds, stressLayout, 991177u);
    assert(stressBatch.valid);
    assert(stressBatch.objects.size() <= 520);
    assert(stressBatch.analyzedAnchors > 100);
    auto stressRepeat = aideco::makePlacements(stressPlan, stressBounds, stressLayout, 991177u);
    assert(stressRepeat.valid);
    assert(stressRepeat.objects.size() == stressBatch.objects.size());
    for (size_t i = 0; i < stressBatch.objects.size(); ++i) {
        assert(stressBatch.objects[i].objectID == stressRepeat.objects[i].objectID);
        assert(stressBatch.objects[i].x == stressRepeat.objects[i].x);
        assert(stressBatch.objects[i].y == stressRepeat.objects[i].y);
    }

    return 0;
}
