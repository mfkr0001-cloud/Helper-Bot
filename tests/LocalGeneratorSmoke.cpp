#include <algorithm>
#include "../src/LocalGenerator.hpp"
#include "../src/DecorationPlacer.hpp"

#include <cassert>
#include <string>

int main() {
    auto russianPrompt = aideco::makePlan(
        "ФУТУРИСТИЧЕСКИЙ ГОРОД, СИНИЙ НЕОН И СВЕТЯЩИЕСЯ ЛИНИИ",
        "AUTO",
        "HIGH",
        "#28A9FF",
        "TRIAD",
        "RHYTHM"
    );

    assert(russianPrompt.style == "NEON");
    assert(russianPrompt.suggestedObjectCount == 76);
    assert(russianPrompt.suggestedLayerCount >= 4);
    assert(!russianPrompt.layers.empty());
    assert(russianPrompt.summary.find("СЛОИ ОФОРМЛЕНИЯ") != std::string::npos);

    auto industrialPrompt = aideco::makePlan(
        "Dark mechanical factory with orange lights",
        "AUTO",
        "ULTRA",
        "not-a-color",
        "MONO",
        "ASYMMETRIC"
    );

    assert(industrialPrompt.style == "TECHNO");
    assert(industrialPrompt.primaryColor == "#28A9FF");
    assert(industrialPrompt.secondaryColor == industrialPrompt.primaryColor);
    assert(industrialPrompt.suggestedObjectCount == 112);
    assert(industrialPrompt.layers.size() == 6);

    auto placements = aideco::makePlacements(
        russianPrompt,
        {-180.f, -70.f, 420.f, 210.f},
        12345u
    );
    assert(placements.valid);
    assert(!placements.objects.empty());
    assert(placements.safeRect.left >= -180.f);
    assert(placements.safeRect.right <= 420.f);
    assert(placements.safeRect.bottom >= -70.f);
    assert(placements.safeRect.top <= 210.f);
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
    }


    // Generation is reproducible for the same seed and selection.
    auto repeat = aideco::makePlacements(
        russianPrompt, {-180.f, -70.f, 420.f, 210.f}, 12345u
    );
    assert(repeat.valid);
    assert(repeat.objects.size() == placements.objects.size());
    for (size_t i = 0; i < placements.objects.size(); ++i) {
        assert(repeat.objects[i].objectID == placements.objects[i].objectID);
        assert(repeat.objects[i].x == placements.objects[i].x);
        assert(repeat.objects[i].y == placements.objects[i].y);
    }

    // A second composition strategy must produce a different arrangement.
    auto centeredPlan = aideco::makePlan(
        "space neon stars", "GLOW", "HIGH", "#28A9FF", "DUO", "CENTERED"
    );
    auto centered = aideco::makePlacements(
        centeredPlan, {-180.f, -70.f, 420.f, 210.f}, 12345u
    );
    assert(centered.valid);
    bool arrangementDiffers = centered.objects.size() != placements.objects.size();
    auto compareCount = std::min(centered.objects.size(), placements.objects.size());
    for (size_t i = 0; i < compareCount && !arrangementDiffers; ++i) {
        arrangementDiffers = centered.objects[i].x != placements.objects[i].x ||
                             centered.objects[i].y != placements.objects[i].y;
    }
    assert(arrangementDiffers);
    assert(placements.objects.size() <= 70);


    // v0.5.0 requested style presets all map to a valid real-object placement family.
    for (auto const& style : {std::string("FUTURISM"), std::string("TECHNO"),
                              std::string("NEON"), std::string("GOTHIC"),
                              std::string("NATURE")}) {
        auto presetPlan = aideco::makePlan(
            "purple neon mechanical forest", style, "MEDIUM",
            "#7755DD", "TRIAD", "BALANCED"
        );
        assert(presetPlan.style == style);
        auto presetBatch = aideco::makePlacements(
            presetPlan, {0.f, 0.f, 360.f, 220.f}, 9001u
        );
        assert(presetBatch.valid);
        assert(!presetBatch.objects.empty());
    }

    auto autoCity = aideco::makePlan(
        "futuristic city skyline", "AUTO", "MEDIUM",
        "#28A9FF", "AUTO", "BALANCED"
    );
    assert(autoCity.style == "FUTURISM");

    // v0.5.1: mirrored composition must be available and keep conservative footprints in bounds.
    auto mirroredPlan = aideco::makePlan(
        "symmetric mirrored neon architecture", "NEON", "HIGH",
        "#7755DD", "DUO", "MIRRORED"
    );
    auto mirrored = aideco::makePlacements(
        mirroredPlan, {-240.f, -120.f, 360.f, 240.f}, 424242u
    );
    assert(mirrored.valid);
    assert(!mirrored.objects.empty());
    for (auto const& placement : mirrored.objects) {
        float footprint = std::max(2.f, placement.scale * 16.f);
        assert(placement.x - footprint >= mirrored.safeRect.left - 0.01f);
        assert(placement.x + footprint <= mirrored.safeRect.right + 0.01f);
        assert(placement.y - footprint >= mirrored.safeRect.bottom - 0.01f);
        assert(placement.y + footprint <= mirrored.safeRect.top + 0.01f);
    }
    assert(mirrored.objects.size() <= 70);

    auto compact = aideco::makePlacements(
        russianPrompt, {0.f, 0.f, 12.f, 12.f}, 7u
    );
    assert(!compact.valid);

    auto tiny = aideco::makePlacements(russianPrompt, {0.f, 0.f, 0.f, 0.f}, 1u);
    assert(!tiny.valid);

    return 0;
}
