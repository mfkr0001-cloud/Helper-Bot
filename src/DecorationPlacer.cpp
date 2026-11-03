#include "DecorationPlacer.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <vector>

namespace aideco {
    namespace {
        struct ObjectChoice {
            int id;
            float minScale;
            float maxScale;
            unsigned char colorSlot;
            unsigned char minOpacity;
            unsigned char maxOpacity;
            bool canRotate;
        };

        struct Point {
            float x;
            float y;
            float radius;
        };

        float clampf(float value, float low, float high) {
            return std::max(low, std::min(value, high));
        }

        int densityTarget(std::string const& density) {
            if (density == "LOW") return 10;
            if (density == "HIGH") return 34;
            if (density == "ULTRA") return 48;
            return 22;
        }

        std::vector<ObjectChoice> choicesForStyle(std::string const& style) {
            std::vector<ObjectChoice> choices = {
                {1764, 0.34f, 0.72f, 2, 150, 230, true},
                {1765, 0.34f, 0.70f, 1, 150, 230, true},
                {1766, 0.34f, 0.72f, 2, 150, 235, true},
                {1767, 0.34f, 0.72f, 0, 145, 225, true},
                {1768, 0.34f, 0.68f, 1, 155, 235, true},
                {1753, 0.45f, 0.90f, 0, 125, 200, true},
                {1754, 0.40f, 0.82f, 1, 135, 215, true},
                {1887, 0.34f, 0.66f, 1, 115, 205, false}
            };

            if (style == "TECH" || style == "TECHNO" || style == "FUTURISM" || style == "INDUSTRIAL" || style == "PIXEL") {
                choices.insert(choices.end(), {
                    {1753, 0.55f, 1.05f, 0, 150, 220, true},
                    {1754, 0.45f, 0.85f, 1, 145, 220, true},
                    {1765, 0.42f, 0.76f, 2, 160, 235, true},
                    {1767, 0.42f, 0.78f, 0, 155, 230, true},
                    {1768, 0.38f, 0.72f, 1, 155, 235, true}
                });
            } else if (style == "GLOW" || style == "NEON" || style == "SPACE") {
                choices.insert(choices.end(), {
                    {1754, 0.50f, 0.92f, 1, 150, 230, true},
                    {1886, 0.38f, 0.70f, 1, 105, 190, false},
                    {1887, 0.36f, 0.72f, 2, 110, 200, false},
                    {1888, 0.32f, 0.58f, 0, 95, 175, false},
                    {1764, 0.38f, 0.76f, 2, 150, 235, true}
                });
            } else if (style == "NATURE" || style == "ORGANIC") {
                choices.insert(choices.end(), {
                    {1766, 0.42f, 0.82f, 1, 155, 235, true},
                    {1764, 0.42f, 0.82f, 0, 155, 230, true},
                    {1837, 0.32f, 0.52f, 2, 130, 205, true},
                    {1838, 0.40f, 0.66f, 1, 145, 220, true}
                });
            } else if (style == "DARK" || style == "GOTHIC") {
                choices.insert(choices.end(), {
                    {1754, 0.46f, 0.82f, 1, 105, 180, true},
                    {1765, 0.40f, 0.72f, 0, 145, 215, true},
                    {1887, 0.32f, 0.62f, 1, 90, 160, false}
                });
            } else if (style == "MINIMAL") {
                choices = {
                    {1753, 0.42f, 0.78f, 0, 125, 185, true},
                    {1764, 0.34f, 0.56f, 1, 145, 210, true},
                    {1767, 0.34f, 0.56f, 2, 140, 205, true},
                    {1887, 0.30f, 0.50f, 1, 95, 155, false}
                };
            }
            return choices;
        }

        float randomFloat(std::mt19937& rng, float low, float high) {
            std::uniform_real_distribution<float> dist(low, high);
            return dist(rng);
        }

        int randomInt(std::mt19937& rng, int low, int high) {
            std::uniform_int_distribution<int> dist(low, high);
            return dist(rng);
        }

        bool spacedEnough(std::vector<Point> const& points, float x, float y, float radius, float factor) {
            for (auto const& point : points) {
                auto dx = x - point.x;
                auto dy = y - point.y;
                auto minDistance = (radius + point.radius) * factor;
                if (dx * dx + dy * dy < minDistance * minDistance) return false;
            }
            return true;
        }

        void addPlacement(
            PlacementBatch& batch,
            std::vector<Point>& occupied,
            int objectID,
            float x,
            float y,
            float scale,
            float rotation,
            unsigned char opacity,
            unsigned char colorSlot,
            bool checkSpacing = false
        ) {
            auto radius = std::max(2.5f, scale * 10.f);
            if (checkSpacing && !spacedEnough(occupied, x, y, radius, 0.72f)) return;
            batch.objects.push_back({objectID, x, y, scale, rotation, opacity, colorSlot});
            occupied.push_back({x, y, radius});
        }

        ObjectPlacement randomPlacement(
            std::mt19937& rng,
            std::vector<ObjectChoice> const& choices,
            SelectionRect const& rect,
            std::string const& layout,
            int index,
            int total
        ) {
            auto const& choice = choices[static_cast<size_t>(randomInt(rng, 0, static_cast<int>(choices.size()) - 1))];
            float x = randomFloat(rng, rect.left, rect.right);
            float y = randomFloat(rng, rect.bottom, rect.top);

            if (layout == "CENTERED") {
                auto cx = (rect.left + rect.right) * 0.5f;
                auto cy = (rect.bottom + rect.top) * 0.5f;
                auto angle = static_cast<float>(index) * 2.39996323f;
                auto radius = std::sqrt(static_cast<float>(index + 1) / static_cast<float>(total + 1));
                x = cx + std::cos(angle) * rect.width() * 0.34f * radius;
                y = cy + std::sin(angle) * rect.height() * 0.34f * radius;
            } else if (layout == "ASYMMETRIC") {
                float bias = index % 2 == 0 ? 0.30f : 0.70f;
                x = rect.left + rect.width() * clampf(randomFloat(rng, bias - 0.20f, bias + 0.20f), 0.06f, 0.94f);
            } else if (layout == "RHYTHM" && total > 1) {
                float t = static_cast<float>(index) / static_cast<float>(total - 1);
                x = rect.left + rect.width() * t;
                float row = static_cast<float>(index % 3) / 2.f;
                y = rect.bottom + rect.height() * (0.20f + row * 0.60f);
                y += randomFloat(rng, -rect.height() * 0.035f, rect.height() * 0.035f);
            } else if (layout == "MIRRORED") {
                // Each consecutive pair shares a height and mirrors around the vertical centre axis.
                int pairIndex = index / 2;
                float offset = randomFloat(rng, 0.12f, 0.36f);
                float leftX = rect.left + rect.width() * offset;
                float mirroredX = rect.right - (leftX - rect.left);
                float yFraction = 0.18f + std::fmod(static_cast<float>(pairIndex + 1) * 0.38196601f, 0.64f);
                x = (index % 2 == 0) ? leftX : mirroredX;
                y = rect.bottom + rect.height() * yFraction;
            }

            auto maxAllowedScale = clampf(std::min(rect.width(), rect.height()) / 76.f, 0.22f, 0.92f);
            auto minScale = std::min(choice.minScale, maxAllowedScale);
            auto maxScale = std::max(minScale, std::min(choice.maxScale, maxAllowedScale));
            ObjectPlacement result;
            result.objectID = choice.id;
            result.x = clampf(x, rect.left, rect.right);
            result.y = clampf(y, rect.bottom, rect.top);
            result.scale = randomFloat(rng, minScale, maxScale);
            result.rotation = choice.canRotate ? static_cast<float>(randomInt(rng, 0, 7) * 45) : 0.f;
            if (layout == "MIRRORED" && index % 2 == 1) result.rotation = -result.rotation;
            result.opacity = static_cast<unsigned char>(randomInt(rng, choice.minOpacity, choice.maxOpacity));
            result.colorSlot = choice.colorSlot;
            return result;
        }

        void addFrame(
            PlacementBatch& batch,
            std::vector<Point>& occupied,
            SelectionRect const& r
        ) {
            if (r.width() < 110.f || r.height() < 70.f) return;
            auto insetX = std::min(16.f, r.width() * 0.08f);
            auto insetY = std::min(12.f, r.height() * 0.10f);
            auto left = r.left + insetX;
            auto right = r.right - insetX;
            auto bottom = r.bottom + insetY;
            auto top = r.top - insetY;
            auto lineScale = clampf(r.width() / 250.f, 0.42f, 1.10f);

            // Thin rails create a deliberate frame; corner markers add a second visual layer.
            addPlacement(batch, occupied, 1753, (left + right) * 0.5f, top, lineScale, 0.f, 155, 0);
            addPlacement(batch, occupied, 1753, (left + right) * 0.5f, bottom, lineScale, 0.f, 155, 0);
            addPlacement(batch, occupied, 1753, left, (bottom + top) * 0.5f, lineScale, 90.f, 155, 0);
            addPlacement(batch, occupied, 1753, right, (bottom + top) * 0.5f, lineScale, 90.f, 155, 0);

            auto cornerScale = clampf(std::min(r.width(), r.height()) / 190.f, 0.34f, 0.68f);
            const float xs[4] = {left, right, left, right};
            const float ys[4] = {bottom, bottom, top, top};
            const float rotations[4] = {0.f, 90.f, 270.f, 180.f};
            for (int i = 0; i < 4; ++i) {
                addPlacement(batch, occupied, 1836, xs[i], ys[i], cornerScale, rotations[i], 190, 1);
            }
        }

        void addCompositionMotif(
            PlacementBatch& batch,
            std::vector<Point>& occupied,
            SelectionRect const& r,
            std::string const& layout
        ) {
            auto cx = (r.left + r.right) * 0.5f;
            auto cy = (r.bottom + r.top) * 0.5f;
            auto scale = clampf(std::min(r.width(), r.height()) / 180.f, 0.30f, 0.78f);

            if (layout == "CENTERED") {
                // A compact central emblem with four orbiting markers.
                addPlacement(batch, occupied, 1766, cx, cy, scale * 1.25f, 0.f, 205, 1);
                for (int i = 0; i < 4; ++i) {
                    auto angle = static_cast<float>(i) * 1.5707963f + 0.7853981f;
                    auto x = cx + std::cos(angle) * r.width() * 0.22f;
                    auto y = cy + std::sin(angle) * r.height() * 0.22f;
                    addPlacement(batch, occupied, 1887, x, y, scale * 0.48f, 0.f, 165, 2);
                }
            } else if (layout == "RHYTHM") {
                int marks = std::max(3, std::min(9, static_cast<int>(r.width() / 42.f)));
                for (int i = 0; i < marks; ++i) {
                    float t = marks == 1 ? 0.5f : static_cast<float>(i) / static_cast<float>(marks - 1);
                    auto x = r.left + r.width() * (0.14f + 0.72f * t);
                    auto y = r.bottom + r.height() * ((i % 2 == 0) ? 0.22f : 0.78f);
                    addPlacement(batch, occupied, 1765, x, y, scale * 0.42f, 45.f, 185, 1);
                }
            } else if (layout == "ASYMMETRIC") {
                auto anchorX = r.left + r.width() * 0.28f;
                auto anchorY = cy;
                addPlacement(batch, occupied, 1766, anchorX, anchorY, scale * 1.20f, 0.f, 195, 1);
                addPlacement(batch, occupied, 1754, r.right - r.width() * 0.22f, r.top - r.height() * 0.20f, scale, 0.f, 155, 2);
                addPlacement(batch, occupied, 1754, r.right - r.width() * 0.22f, r.bottom + r.height() * 0.20f, scale, 0.f, 155, 2);
            } else if (layout == "MIRRORED") {
                // Deliberate left/right pairs around the centre axis.
                for (int i = 0; i < 3; ++i) {
                    float xOffset = r.width() * (0.18f + 0.08f * static_cast<float>(i));
                    float y = cy + r.height() * (i - 1) * 0.22f;
                    int id = i == 1 ? 1766 : 1764;
                    float itemScale = scale * (i == 1 ? 0.72f : 0.46f);
                    addPlacement(batch, occupied, id, cx - xOffset, y, itemScale, 0.f, 185, 1, true);
                    addPlacement(batch, occupied, id, cx + xOffset, y, itemScale, 0.f, 185, 1, true);
                }
            } else {
                // Balanced layout: two staggered clusters keep the centre readable.
                for (int i = 0; i < 6; ++i) {
                    auto side = i % 2 == 0 ? 0.24f : 0.76f;
                    auto row = static_cast<float>(i / 2);
                    auto x = r.left + r.width() * side;
                    auto y = r.bottom + r.height() * (0.30f + row * 0.20f);
                    addPlacement(batch, occupied, (i % 3 == 0) ? 1766 : 1764,
                        x, y, scale * (i % 3 == 0 ? 0.68f : 0.42f),
                        static_cast<float>((i * 45) % 180), 175, static_cast<unsigned char>(i % 2), true);
                }
            }
        }
    }

    PlacementBatch makePlacements(
        GenerationPlan const& plan,
        SelectionRect const& selection,
        std::uint32_t seed
    ) {
        PlacementBatch batch;
        batch.seed = seed;
        if (!selection.valid()) {
            batch.error = "Выделенная область слишком маленькая или не распознана.";
            return batch;
        }

        auto width = selection.width();
        auto height = selection.height();
        auto minDimension = std::min(width, height);
        auto margin = clampf(minDimension * 0.12f, 3.f, 16.f);
        if (width < 28.f) margin = std::min(margin, width * 0.04f);
        if (height < 28.f) margin = std::min(margin, height * 0.04f);
        batch.safeRect = {
            selection.left + margin,
            selection.bottom + margin,
            selection.right - margin,
            selection.top - margin
        };
        if (!batch.safeRect.valid()) batch.safeRect = selection;

        auto choices = choicesForStyle(plan.style);
        if (minDimension < 88.f) {
            choices.erase(std::remove_if(choices.begin(), choices.end(), [](ObjectChoice const& choice) {
                return choice.id == 1753 || choice.id == 1754 || choice.id == 1837 ||
                       choice.id == 1838 || choice.id == 1886 || choice.id == 1888;
            }), choices.end());
        }
        if (choices.empty()) {
            batch.error = "Для выбранного стиля нет доступных декоративных объектов.";
            return batch;
        }

        auto area = std::max(1.f, batch.safeRect.width() * batch.safeRect.height());
        auto areaFactor = clampf(std::sqrt(area) / 220.f, 0.25f, 1.f);
        auto target = std::max(5, static_cast<int>(std::round(densityTarget(plan.density) * areaFactor)));
        target = std::min(target, 48);

        std::mt19937 rng(seed == 0 ? 0xA1DEC0DEu : seed);
        std::vector<Point> occupied;
        batch.objects.reserve(static_cast<size_t>(target + 20));

        addFrame(batch, occupied, batch.safeRect);
        addCompositionMotif(batch, occupied, batch.safeRect, plan.layout);

        // Fill remaining negative space with spaced detail points rather than an unrestricted scatter.
        auto baseCount = static_cast<int>(batch.objects.size());
        auto fillTarget = std::max(0, target - baseCount);
        int accepted = 0;
        int attempts = 0;
        int maxAttempts = std::max(80, fillTarget * 24);
        while (accepted < fillTarget && attempts < maxAttempts) {
            ++attempts;
            auto placement = randomPlacement(rng, choices, batch.safeRect, plan.layout, accepted, std::max(1, fillTarget));
            auto radius = std::max(2.5f, placement.scale * 10.f);
            if (!spacedEnough(occupied, placement.x, placement.y, radius, 0.78f)) continue;
            batch.objects.push_back(placement);
            occupied.push_back({placement.x, placement.y, radius});
            ++accepted;
        }

        // Keep an approximate safety footprint for every generated object inside the safe rectangle.
        // The game sprite bounds vary by object ID, so this is deliberately more conservative
        // than checking only the object's centre. A final in-game visual check is still required.
        auto minSafeDimension = std::min(batch.safeRect.width(), batch.safeRect.height());
        if (minSafeDimension < 12.f) {
            batch.objects.clear();
            batch.error = "Область слишком мала для безопасного размещения декора.";
            return batch;
        }
        for (auto& object : batch.objects) {
            auto maxScaleForBounds = minSafeDimension / 34.f;
            object.scale = std::min(object.scale, maxScaleForBounds);
            auto footprint = std::max(2.f, object.scale * 16.f);
            auto minX = batch.safeRect.left + footprint;
            auto maxX = batch.safeRect.right - footprint;
            auto minY = batch.safeRect.bottom + footprint;
            auto maxY = batch.safeRect.top - footprint;
            if (minX > maxX || minY > maxY) {
                batch.valid = false;
                batch.error = "Не удалось подобрать безопасный размер объектов для этой области.";
                return batch;
            }
            object.x = clampf(object.x, minX, maxX);
            object.y = clampf(object.y, minY, maxY);
        }

        batch.valid = !batch.objects.empty();
        if (!batch.valid) batch.error = "Не удалось построить композицию декора.";
        return batch;
    }
}
