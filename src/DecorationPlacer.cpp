#include "DecorationPlacer.hpp"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
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
            unsigned char opacity;
            bool canRotate;
        };

        struct Point {
            float x;
            float y;
            float radius;
        };

        struct LayoutSlice {
            bool active = false;
            float left = 0.f;
            float right = 0.f;
            float top = -std::numeric_limits<float>::max();
            float bottom = std::numeric_limits<float>::max();
            int objectCount = 0;
            GeometryAnchor topAnchor;
            GeometryAnchor bottomAnchor;
        };

        float clampf(float value, float low, float high) {
            return std::max(low, std::min(value, high));
        }

        float randomFloat(std::mt19937& rng, float low, float high) {
            std::uniform_real_distribution<float> dist(low, high);
            return dist(rng);
        }

        int segmentWidthForDensity(std::string const& density) {
            if (density == "LOW") return 160;
            if (density == "HIGH") return 92;
            if (density == "ULTRA") return 72;
            return 120;
        }

        std::vector<ObjectChoice> choicesForStyle(std::string const& style) {
            auto has = [&](std::initializer_list<char const*> keys) {
                for (auto key : keys) {
                    if (style.find(key) != std::string::npos) return true;
                }
                return false;
            };

            std::vector<ObjectChoice> choices = {
                {1764, 0.30f, 0.58f, 2, 190, true},
                {1765, 0.28f, 0.56f, 1, 190, true},
                {1766, 0.32f, 0.64f, 2, 205, true},
                {1767, 0.30f, 0.58f, 0, 185, true},
                {1768, 0.28f, 0.54f, 1, 190, true},
                {1753, 0.34f, 0.72f, 0, 175, true},
                {1754, 0.30f, 0.62f, 1, 180, true},
                {1887, 0.24f, 0.46f, 1, 155, false}
            };

            if (has({"TECH", "FUTUR", "CYBER", "HOLOGRAM", "LASER", "DIGITAL", "PIXEL", "ARCADE", "RETRO", "GEOMETRIC", "ABSTRACT", "INDUSTRIAL", "FACTORY", "MACHINE"})) {
                choices.insert(choices.end(), {
                    {1753, 0.42f, 0.78f, 0, 190, true},
                    {1754, 0.34f, 0.68f, 1, 190, true},
                    {1765, 0.30f, 0.58f, 2, 205, true},
                    {1767, 0.30f, 0.58f, 0, 195, true},
                    {1768, 0.28f, 0.54f, 1, 195, true}
                });
            } else if (has({"NEON", "SYNTHWAVE", "CELESTIAL", "GALAXY", "STARS", "SPACE"})) {
                choices.insert(choices.end(), {
                    {1754, 0.36f, 0.70f, 1, 190, true},
                    {1886, 0.24f, 0.50f, 1, 145, false},
                    {1887, 0.24f, 0.46f, 2, 155, false},
                    {1888, 0.22f, 0.42f, 0, 140, false},
                    {1764, 0.30f, 0.58f, 2, 200, true}
                });
            } else if (has({"NATURE", "ORGANIC", "FOREST", "JUNGLE", "MUSHROOM", "OCEAN", "UNDERWATER", "CORAL", "SKY", "CLOUD"})) {
                choices.insert(choices.end(), {
                    {1766, 0.34f, 0.64f, 1, 205, true},
                    {1764, 0.30f, 0.58f, 0, 195, true},
                    {1837, 0.22f, 0.40f, 2, 170, true},
                    {1838, 0.24f, 0.46f, 1, 180, true}
                });
            } else if (has({"DARK", "GOTHIC", "SHADOW", "ABYSS", "HAUNTED"})) {
                choices.insert(choices.end(), {
                    {1754, 0.32f, 0.62f, 1, 155, true},
                    {1765, 0.28f, 0.54f, 0, 185, true},
                    {1887, 0.22f, 0.44f, 1, 135, false}
                });
            } else if (has({"VOLCANIC", "LAVA", "FIRE", "DESERT", "ANCIENT", "RUINS", "TEMPLE", "EGYPTIAN", "ICE", "FROST", "SNOW", "CRYSTAL", "CANDY", "TOY", "CARTOON"})) {
                choices.insert(choices.end(), {
                    {1753, 0.36f, 0.72f, 0, 180, true},
                    {1764, 0.28f, 0.56f, 1, 195, true},
                    {1766, 0.32f, 0.64f, 2, 195, true},
                    {1768, 0.28f, 0.54f, 1, 185, true},
                    {1887, 0.22f, 0.44f, 2, 150, false}
                });
            } else if (style == "MINIMAL") {
                choices = {
                    {1753, 0.30f, 0.60f, 0, 145, true},
                    {1764, 0.26f, 0.46f, 1, 170, true},
                    {1767, 0.26f, 0.46f, 2, 160, true},
                    {1887, 0.20f, 0.38f, 1, 120, false}
                };
            }
            return choices;
        }

        float distanceSquaredToRect(float x, float y, GeometryAnchor const& r) {
            const float dx = std::max({r.left - x, 0.f, x - r.right});
            const float dy = std::max({r.bottom - y, 0.f, y - r.top});
            return dx * dx + dy * dy;
        }

        constexpr size_t MAX_DECOR_OBJECTS = 520;

        bool clearOfLayout(
            std::vector<GeometryAnchor> const& geometry,
            float x,
            float y,
            float radius
        ) {
            // A little extra clearance keeps added decoration from sitting on top of
            // the original gameplay blocks. The planner decorates their silhouette.
            const float minimumDistance = radius + 4.5f;
            const float minimumDistanceSquared = minimumDistance * minimumDistance;
            for (auto const& anchor : geometry) {
                // Squared-distance comparison avoids a square root for every
                // geometry candidate; this matters on long levels with many blocks.
                if (distanceSquaredToRect(x, y, anchor) < minimumDistanceSquared) return false;
            }
            return true;
        }

        bool spacedEnough(std::vector<Point> const& points, float x, float y, float radius) {
            for (auto const& point : points) {
                const auto dx = x - point.x;
                const auto dy = y - point.y;
                const auto minDistance = (radius + point.radius) * 0.72f;
                if (dx * dx + dy * dy < minDistance * minDistance) return false;
            }
            return true;
        }

        bool addPlannedObject(
            PlacementBatch& batch,
            std::vector<Point>& occupied,
            std::vector<GeometryAnchor> const& geometry,
            int objectID,
            float x,
            float y,
            float scale,
            float rotation,
            unsigned char opacity,
            unsigned char colorSlot
        ) {
            if (batch.objects.size() >= MAX_DECOR_OBJECTS) return false;
            scale = clampf(scale, 0.18f, 0.82f);
            const float radius = std::max(3.0f, scale * 12.0f);
            if (x - radius < batch.safeRect.left || x + radius > batch.safeRect.right ||
                y - radius < batch.safeRect.bottom || y + radius > batch.safeRect.top) {
                return false;
            }
            if (!clearOfLayout(geometry, x, y, radius)) return false;
            if (!spacedEnough(occupied, x, y, radius)) return false;

            batch.objects.push_back({objectID, x, y, scale, rotation, opacity, colorSlot});
            occupied.push_back({x, y, radius});
            return true;
        }

        std::vector<GeometryAnchor> filterGeometry(
            SelectionRect const& selection,
            std::vector<GeometryAnchor> const& input
        ) {
            std::vector<GeometryAnchor> result;
            result.reserve(input.size());
            for (auto const& source : input) {
                if (!source.valid()) continue;
                if (source.width() > 360.f || source.height() > 220.f) continue;
                if (source.centerY() < selection.bottom - 24.f || source.centerY() > selection.top + 24.f) continue;
                if (source.right < selection.left || source.left > selection.right) continue;

                GeometryAnchor clipped = source;
                clipped.left = clampf(clipped.left, selection.left, selection.right);
                clipped.right = clampf(clipped.right, selection.left, selection.right);
                clipped.bottom = clampf(clipped.bottom, selection.bottom, selection.top);
                clipped.top = clampf(clipped.top, selection.bottom, selection.top);
                if (clipped.valid()) result.push_back(clipped);
            }
            return result;
        }

        std::vector<LayoutSlice> analyzeLayout(
            SelectionRect const& safeRect,
            std::vector<GeometryAnchor> const& geometry,
            int segmentWidth
        ) {
            const auto count = std::max(1, std::min(256,
                static_cast<int>(std::ceil(safeRect.width() / static_cast<float>(segmentWidth)))));
            std::vector<LayoutSlice> slices(static_cast<size_t>(count));
            const float realWidth = safeRect.width() / static_cast<float>(count);

            for (int i = 0; i < count; ++i) {
                slices[static_cast<size_t>(i)].left = safeRect.left + realWidth * static_cast<float>(i);
                slices[static_cast<size_t>(i)].right = safeRect.left + realWidth * static_cast<float>(i + 1);
            }

            for (auto const& anchor : geometry) {
                int first = static_cast<int>((anchor.left - safeRect.left) / realWidth);
                int last = static_cast<int>((anchor.right - safeRect.left) / realWidth);
                first = std::max(0, std::min(count - 1, first));
                last = std::max(0, std::min(count - 1, last));
                for (int i = first; i <= last; ++i) {
                    auto& slice = slices[static_cast<size_t>(i)];
                    if (anchor.right < slice.left || anchor.left > slice.right) continue;
                    slice.active = true;
                    ++slice.objectCount;
                    if (anchor.top > slice.top) {
                        slice.top = anchor.top;
                        slice.topAnchor = anchor;
                    }
                    if (anchor.bottom < slice.bottom) {
                        slice.bottom = anchor.bottom;
                        slice.bottomAnchor = anchor;
                    }
                }
            }
            return slices;
        }

        void addSilhouetteDetails(
            PlacementBatch& batch,
            std::vector<Point>& occupied,
            std::vector<GeometryAnchor> const& geometry,
            std::vector<LayoutSlice> const& slices,
            std::vector<ObjectChoice> const& choices,
            std::string const& layout,
            std::mt19937& rng
        ) {
            if (choices.empty()) return;
            const float safeTop = batch.safeRect.top;
            const float safeBottom = batch.safeRect.bottom;
            const float verticalClearance = 14.f;

            for (size_t i = 0; i < slices.size(); ++i) {
                auto const& slice = slices[i];
                if (!slice.active) continue;

                const float x = (slice.left + slice.right) * 0.5f;
                const float bandWidth = slice.right - slice.left;
                const auto& topChoice = choices[(i * 3u + 1u) % choices.size()];
                const auto& bottomChoice = choices[(i * 5u + 2u) % choices.size()];

                // Main structure-following pass: one compact accent above the local
                // top silhouette and one below the local lower silhouette when space exists.
                if (slice.top < safeTop - verticalClearance * 1.7f) {
                    float scale = clampf(std::min(bandWidth / 170.f, 0.68f), topChoice.minScale, topChoice.maxScale);
                    float y = slice.top + verticalClearance + randomFloat(rng, -1.8f, 1.8f);
                    float rotation = topChoice.canRotate ? ((i % 2u) ? 90.f : 0.f) : 0.f;
                    addPlannedObject(batch, occupied, geometry, topChoice.id, x, y, scale,
                        rotation, topChoice.opacity, topChoice.colorSlot);
                }

                if (slice.bottom > safeBottom + verticalClearance * 1.7f) {
                    float scale = clampf(std::min(bandWidth / 185.f, 0.64f), bottomChoice.minScale, bottomChoice.maxScale);
                    float y = slice.bottom - verticalClearance + randomFloat(rng, -1.8f, 1.8f);
                    float rotation = bottomChoice.canRotate ? ((i % 2u) ? 0.f : 90.f) : 0.f;
                    addPlannedObject(batch, occupied, geometry, bottomChoice.id, x, y, scale,
                        rotation, bottomChoice.opacity, bottomChoice.colorSlot);
                }

                // Highlight substantial structures more strongly than isolated blocks.
                if (slice.objectCount >= 3 && i % 3u == 0u) {
                    const auto& anchor = slice.topAnchor;
                    float motifX = clampf(anchor.centerX(), slice.left + 8.f, slice.right - 8.f);
                    float motifY = anchor.top + 27.f;
                    if (motifY < safeTop - 8.f) {
                        const auto& motif = choices[(i + 4u) % choices.size()];
                        addPlannedObject(batch, occupied, geometry, motif.id, motifX, motifY,
                            clampf(motif.maxScale * 0.78f, 0.22f, 0.54f),
                            motif.canRotate ? 45.f : 0.f, motif.opacity, 1);
                    }
                }
            }

            // Connect neighbouring top/bottom silhouettes with short horizontal rails.
            // A rail is added only when the measured edge remains almost level and the
            // space between the two layout sections is genuinely clear.
            for (size_t i = 1; i < slices.size(); ++i) {
                auto const& previous = slices[i - 1];
                auto const& current = slices[i];
                if (!previous.active || !current.active) continue;
                if (std::abs(previous.top - current.top) <= 18.f) {
                    const auto& lineChoice = choices[(i + 2u) % choices.size()];
                    float x = (previous.right + current.left) * 0.5f;
                    float y = (previous.top + current.top) * 0.5f + 13.f;
                    float scale = clampf((current.left - previous.left) / 190.f, 0.22f, 0.56f);
                    addPlannedObject(batch, occupied, geometry, 1753, x, y, scale,
                        0.f, 160, lineChoice.colorSlot);
                }
                if (std::abs(previous.bottom - current.bottom) <= 18.f) {
                    const auto& lineChoice = choices[(i + 4u) % choices.size()];
                    float x = (previous.right + current.left) * 0.5f;
                    float y = (previous.bottom + current.bottom) * 0.5f - 13.f;
                    float scale = clampf((current.left - previous.left) / 190.f, 0.22f, 0.52f);
                    addPlannedObject(batch, occupied, geometry, 1753, x, y, scale,
                        0.f, 150, lineChoice.colorSlot);
                }
            }

            // Regular background marks are reserved for gaps between built structures.
            // They form a subtle rhythm instead of a random full-screen scatter.
            for (size_t i = 1; i + 1 < slices.size(); ++i) {
                if (slices[i].active || (!slices[i - 1].active && !slices[i + 1].active) || i % 2u != 0u) continue;
                const float x = (slices[i].left + slices[i].right) * 0.5f;
                const bool upper = (layout == "MIRRORED") ? (i % 4u < 2u) : (i % 4u == 0u);
                const float y = upper ? safeTop - 26.f : safeBottom + 26.f;
                const auto& choice = choices[(i * 7u) % choices.size()];
                addPlannedObject(batch, occupied, geometry, choice.id, x, y,
                    clampf(choice.minScale * 0.82f, 0.18f, 0.34f),
                    choice.canRotate ? ((i % 3u) * 45.f) : 0.f,
                    static_cast<unsigned char>(choice.opacity * 0.72f), 2);
            }
        }
    }

    PlacementBatch makePlacements(
        GenerationPlan const& plan,
        SelectionRect const& levelBounds,
        std::vector<GeometryAnchor> const& layoutGeometry,
        std::uint32_t seed
    ) {
        PlacementBatch batch;
        batch.seed = seed;
        if (!levelBounds.valid()) {
            batch.error = "Не удалось определить размеры открытого уровня.";
            return batch;
        }

        const float margin = clampf(std::min(levelBounds.width(), levelBounds.height()) * 0.025f, 4.f, 10.f);
        batch.safeRect = {
            levelBounds.left + margin,
            levelBounds.bottom + margin,
            levelBounds.right - margin,
            levelBounds.top - margin
        };
        if (!batch.safeRect.valid()) batch.safeRect = levelBounds;

        auto geometry = filterGeometry(levelBounds, layoutGeometry);
        batch.analyzedAnchors = static_cast<int>(geometry.size());
        if (geometry.size() < 3) {
            batch.error = "Сначала построй игровой layout из блоков и препятствий. Генератор оформляет готовую геометрию, а не создаёт случайный декор на пустом уровне.";
            return batch;
        }

        auto choices = choicesForStyle(plan.style);
        if (choices.empty()) {
            batch.error = "Для выбранного стиля не найден набор декоративных объектов.";
            return batch;
        }

        const int segmentWidth = segmentWidthForDensity(plan.density);
        auto slices = analyzeLayout(batch.safeRect, geometry, segmentWidth);
        batch.activeSegments = static_cast<int>(std::count_if(slices.begin(), slices.end(), [](LayoutSlice const& slice) {
            return slice.active;
        }));
        if (batch.activeSegments == 0) {
            batch.error = "Не удалось распознать геометрию layout. Добавь обычные блоки и попробуй снова.";
            return batch;
        }

        std::mt19937 rng(seed == 0 ? 0xA1DEC0DEu : seed);
        std::vector<Point> occupied;
        const auto estimate = std::min(static_cast<int>(MAX_DECOR_OBJECTS), batch.activeSegments * (plan.density == "ULTRA" ? 3 : 2) + 12);
        batch.objects.reserve(static_cast<size_t>(estimate));

        // Three intentional passes: silhouette tracing, structure highlights, then
        // restrained accents in gaps. Candidate positions are checked against the
        // original gameplay rectangles before being accepted.
        addSilhouetteDetails(batch, occupied, geometry, slices, choices, plan.layout, rng);

        // Ensure the result has a usable amount of decoration on small but real layouts.
        // This pass samples the largest unoccupied structural anchors, not random canvas points.
        if (batch.objects.size() < 4) {
            std::vector<GeometryAnchor> sorted = geometry;
            std::stable_sort(sorted.begin(), sorted.end(), [](GeometryAnchor const& a, GeometryAnchor const& b) {
                return a.width() * a.height() > b.width() * b.height();
            });
            for (size_t i = 0; i < sorted.size() && batch.objects.size() < 8; ++i) {
                auto const& anchor = sorted[i];
                const auto& choice = choices[i % choices.size()];
                float x = anchor.centerX();
                float y = anchor.top + 18.f;
                if (y > batch.safeRect.top - 8.f) y = anchor.bottom - 18.f;
                addPlannedObject(batch, occupied, geometry, choice.id, x, y,
                    clampf(choice.minScale + 0.08f, 0.20f, 0.44f),
                    choice.canRotate ? 0.f : 0.f, choice.opacity, choice.colorSlot);
            }
        }

        if (batch.objects.empty()) {
            batch.error = "План построен, но рядом с игровыми объектами не нашлось безопасных мест для декора. Попробуй изменить плотность или layout.";
            return batch;
        }

        // Final defensive cap: no caller can receive an oversized batch even if
        // future planning passes are added without using addPlannedObject().
        if (batch.objects.size() > MAX_DECOR_OBJECTS) batch.objects.resize(MAX_DECOR_OBJECTS);
        batch.valid = true;
        return batch;
    }
}
