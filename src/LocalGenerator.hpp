#pragma once

#include <string>
#include <vector>

namespace aideco {
    struct DecorationLayerPlan {
        std::string name;
        std::string objectHint;
        std::string colorRole;
        std::string purpose;
        int count = 0;
    };

    struct GenerationPlan {
        std::string style;
        std::string primaryColor;
        std::string accentColor;
        std::string secondaryColor;
        std::string density;
        std::string layout;
        std::string accentMode;
        std::string detectedThemes;
        std::string placementStrategy;
        int suggestedObjectCount = 0;
        int suggestedLayerCount = 0;
        std::vector<DecorationLayerPlan> layers;
        std::string summary;
    };

    GenerationPlan makePlan(
        std::string const& prompt,
        std::string const& chosenStyle,
        std::string const& chosenDensity,
        std::string const& chosenColor,
        std::string const& chosenAccentMode,
        std::string const& chosenLayout
    );
}
