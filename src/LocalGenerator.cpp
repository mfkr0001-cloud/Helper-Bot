#include "LocalGenerator.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace aideco {
    namespace {
        // Lowercase ASCII and the common Cyrillic alphabet in UTF-8.
        // This avoids requiring a process-wide locale on Windows or Android.
        std::string lowerAscii(std::string value) {
            for (size_t i = 0; i < value.size(); ++i) {
                auto ch = static_cast<unsigned char>(value[i]);
                if (ch == 0xD0 && i + 1 < value.size()) {
                    auto next = static_cast<unsigned char>(value[i + 1]);
                    if (next >= 0x90 && next <= 0x9F) {
                        value[i + 1] = static_cast<char>(next + 0x20); // А-П -> а-п
                        ++i;
                        continue;
                    }
                    if (next >= 0xA0 && next <= 0xAF) {
                        value[i] = static_cast<char>(0xD1);
                        value[i + 1] = static_cast<char>(next - 0x20); // Р-Я -> р-я
                        ++i;
                        continue;
                    }
                    if (next == 0x81) {
                        value[i] = static_cast<char>(0xD1);
                        value[i + 1] = static_cast<char>(0x91); // Ё -> ё
                        ++i;
                        continue;
                    }
                }
                if (ch < 0x80) value[i] = static_cast<char>(std::tolower(ch));
            }
            return value;
        }

        bool containsAny(std::string const& text, std::vector<std::string> const& words) {
            return std::any_of(words.begin(), words.end(), [&](std::string const& word) {
                return !word.empty() && text.find(word) != std::string::npos;
            });
        }

        bool isHexColor(std::string const& value) {
            if (value.size() != 7 || value[0] != '#') return false;
            for (size_t i = 1; i < value.size(); ++i) {
                if (!std::isxdigit(static_cast<unsigned char>(value[i]))) return false;
            }
            return true;
        }

        std::string normalizedColor(std::string const& value) {
            if (!isHexColor(value)) return "#28A9FF";
            std::string result = value;
            std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
                return static_cast<char>(std::toupper(ch));
            });
            return result;
        }

        std::string detectAccent(std::string const& prompt, std::string const& primary) {
            if (containsAny(prompt, {"pink", "розов", "magenta", "пурпур", "розовый"})) return "#FF55C8";
            if (containsAny(prompt, {"green", "зелён", "зелен", "nature", "природ", "мятн"})) return "#54E28A";
            if (containsAny(prompt, {"gold", "золот", "yellow", "жёлт", "желт", "золото"})) return "#FFD34E";
            if (containsAny(prompt, {"purple", "фиолет", "violet", "лаванд"})) return "#A66BFF";
            if (containsAny(prompt, {"red", "красн", "crimson", "алый"})) return "#FF5A62";
            if (containsAny(prompt, {"cyan", "бирюз", "голуб", "teal"})) return "#28E6D5";
            if (primary == "#28A9FF") return "#A66BFF";
            if (primary == "#FF55C8") return "#FFD34E";
            return "#FFFFFF";
        }

        std::string chooseSecondary(
            std::string const& primary,
            std::string const& accent,
            std::string const& mode
        ) {
            if (mode == "MONO") return primary;
            if (mode == "DUO") return accent;
            if (mode == "TRIAD") return "#FFFFFF";
            return accent;
        }

        void addLayer(
            GenerationPlan& plan,
            std::string name,
            std::string objectHint,
            std::string colorRole,
            std::string purpose,
            int count
        ) {
            if (count <= 0) return;
            plan.layers.push_back({
                std::move(name),
                std::move(objectHint),
                std::move(colorRole),
                std::move(purpose),
                count
            });
        }
    }

    std::vector<std::string> availableStyles() {
        return {
            "AUTO", "NEON", "TECHNO", "FUTURISM", "CYBERPUNK", "SYNTHWAVE",
            "GALAXY", "SPACE", "CELESTIAL", "STARS", "NATURE", "FOREST",
            "JUNGLE", "ORGANIC", "OCEAN", "UNDERWATER", "CORAL", "SKY",
            "CLOUD", "DARK", "GOTHIC", "SHADOW", "ABYSS", "HAUNTED",
            "VOLCANIC", "LAVA", "FIRE", "DESERT", "ANCIENT", "RUINS",
            "TEMPLE", "EGYPTIAN", "ICE", "FROST", "SNOW", "CRYSTAL",
            "CANDY", "TOY", "CARTOON", "PIXEL", "ARCADE", "RETRO",
            "GEOMETRIC", "ABSTRACT", "INDUSTRIAL", "FACTORY", "MACHINE",
            "HOLOGRAM", "LASER", "DIGITAL"
        };
    }

    GenerationPlan makePlan(
        std::string const& prompt,
        std::string const& chosenStyle,
        std::string const& chosenDensity,
        std::string const& chosenColor,
        std::string const& chosenAccentMode,
        std::string const& chosenLayout
    ) {
        GenerationPlan plan;
        auto text = lowerAscii(prompt);

        plan.style = chosenStyle;
        plan.density = chosenDensity;
        plan.layout = chosenLayout;
        plan.accentMode = chosenAccentMode;
        plan.primaryColor = normalizedColor(chosenColor);
        plan.accentColor = detectAccent(text, plan.primaryColor);
        plan.secondaryColor = chooseSecondary(plan.primaryColor, plan.accentColor, chosenAccentMode);

        std::vector<std::string> themes;
        bool city = containsAny(text, {"city", "город", "urban", "метропол", "улиц"});
        bool space = containsAny(text, {"space", "космос", "галакт", "star", "звезд", "планет"});
        bool nature = containsAny(text, {"nature", "лес", "природ", "plant", "растен", "листв"});
        bool tech = containsAny(text, {"tech", "техно", "robot", "робот", "cyber", "кибер", "digital"});
        bool glow = containsAny(text, {"glow", "свеч", "неон", "neon", "light", "свет", "сиян"});
        bool dark = containsAny(text, {"dark", "тёмн", "темн", "shadow", "тень", "мрачн"});
        bool water = containsAny(text, {"water", "вода", "океан", "море", "подвод"});
        bool fire = containsAny(text, {"fire", "огонь", "пламя", "лава", "volcano"});
        bool crystal = containsAny(text, {"crystal", "кристалл", "diamond", "алмаз", "gem"});
        bool industrial = containsAny(text, {"industrial", "индустр", "завод", "механ", "factory"});
        bool ice = containsAny(text, {"ice", "лед", "лёд", "снеж", "зим", "frost", "frozen"});
        bool desert = containsAny(text, {"desert", "пустын", "песок", "sand", "dune"});
        bool ancient = containsAny(text, {"ancient", "древн", "руин", "temple", "храм", "египет"});
        bool sky = containsAny(text, {"sky", "небо", "облак", "cloud", "heaven", "небес"});
        bool music = containsAny(text, {"music", "музык", "sound", "звук", "ритм", "rhythm"});

        if (city) themes.push_back("городские силуэты");
        if (space) themes.push_back("космические мотивы");
        if (nature) themes.push_back("органические формы");
        if (tech) themes.push_back("технологические детали");
        if (glow) themes.push_back("светящиеся акценты");
        if (dark) themes.push_back("тёмные контрастные формы");
        if (water) themes.push_back("волны и плавные линии");
        if (fire) themes.push_back("динамичные языки пламени");
        if (crystal) themes.push_back("кристаллические силуэты");
        if (industrial) themes.push_back("механические конструкции");
        if (ice) themes.push_back("ледяные кристаллы и холодные узоры");
        if (desert) themes.push_back("песчаные формы и слоистые силуэты");
        if (ancient) themes.push_back("древние арки и рунические мотивы");
        if (sky) themes.push_back("облака и воздушные конструкции");
        if (music) themes.push_back("ритмичные повторяющиеся узоры");

        if (themes.empty()) themes.push_back("геометрические детали и рамки");

        std::ostringstream themeStream;
        for (size_t i = 0; i < themes.size(); ++i) {
            if (i) themeStream << ", ";
            themeStream << themes[i];
        }
        plan.detectedThemes = themeStream.str();

        if (chosenStyle == "AUTO") {
            if (tech || industrial) plan.style = "TECHNO";
            else if (nature || water) plan.style = "NATURE";
            else if (space || glow) plan.style = "NEON";
            else if (dark || fire || ancient) plan.style = "GOTHIC";
            else if (city) plan.style = "FUTURISM";
            else plan.style = "FUTURISM";
        }

        if (chosenLayout == "ASYMMETRIC") {
            plan.placementStrategy = "Асимметричная композиция: крупные элементы смещены от центра, мелкие детали балансируют пустые зоны.";
        } else if (chosenLayout == "CENTERED") {
            plan.placementStrategy = "Центральная композиция: главный мотив в центре выделения, рамки и детали расходятся наружу.";
        } else if (chosenLayout == "RHYTHM") {
            plan.placementStrategy = "Ритмичная композиция: повторяющиеся элементы, чередование крупных и мелких форм, равные интервалы.";
        } else if (chosenLayout == "MIRRORED") {
            plan.placementStrategy = "Зеркальная композиция: парные элементы располагаются симметрично относительно вертикальной оси области.";
        } else {
            plan.placementStrategy = "Сбалансированная композиция: равномерное распределение деталей с сохранением свободного пространства.";
        }

        int base = 0;
        if (chosenDensity == "LOW") base = 18;
        else if (chosenDensity == "HIGH") base = 54;
        else if (chosenDensity == "ULTRA") base = 76;
        else base = 36;

        plan.suggestedObjectCount = base;
        plan.suggestedLayerCount = chosenDensity == "LOW" ? 3 : (chosenDensity == "ULTRA" ? 6 : 5);

        addLayer(plan, "01 / STRUCTURE", "крупные блоки, контуры и угловые элементы", "PRIMARY", "Основной каркас и границы композиции", base / 5);
        addLayer(plan, "02 / SECONDARY", "короткие линии, небольшие панели и повторяющиеся формы", "SECONDARY", "Заполнение крупных пустот без перегруза", base / 4);
        addLayer(plan, "03 / ACCENTS", "маленькие геометрические объекты и маркеры", "ACCENT", "Визуальные акценты и ритм", base / 4);
        if (chosenDensity != "LOW") {
            addLayer(plan, "04 / HIGHLIGHTS", "световые полосы, тонкие контуры и блики", "ACCENT", "Подчёркивание ключевых границ", base / 6);
        }
        if (chosenDensity == "HIGH" || chosenDensity == "ULTRA") {
            addLayer(plan, "05 / MICRO DETAILS", "точки, короткие штрихи и мелкие вставки", "SECONDARY", "Мелкие детали на свободных участках", base / 7);
        }
        if (chosenDensity == "ULTRA") {
            addLayer(plan, "06 / EXTRA", "дополнительные орнаменты и повторяющиеся элементы", "PRIMARY", "Дополнительная глубина и насыщенность", base / 8);
        }

        std::ostringstream summary;
        summary
            << "ЛОКАЛЬНЫЙ ПЛАН ОФОРМЛЕНИЯ\n\n"
            << "Стиль: " << plan.style << "\n"
            << "Основной цвет: " << plan.primaryColor << "\n"
            << "Акцентный цвет: " << plan.accentColor << "\n"
            << "Дополнительный цвет: " << plan.secondaryColor << "\n"
            << "Цветовая схема: " << plan.accentMode << "\n"
            << "Плотность: " << plan.density << "\n"
            << "Композиция: " << plan.layout << "\n"
            << "Распознано: " << plan.detectedThemes << "\n\n"
            << "СЛОИ ОФОРМЛЕНИЯ\n";

        for (auto const& layer : plan.layers) {
            summary << "\n" << layer.name << " — около " << layer.count << " объектов\n"
                    << "  Типы: " << layer.objectHint << "\n"
                    << "  Назначение: " << layer.purpose << "\n";
        }

        summary
            << "\nТактика размещения: " << plan.placementStrategy << "\n\n"
            << "Ориентир плана: " << plan.suggestedObjectCount << " декоративных элементов в "
            << plan.suggestedLayerCount << " слоях.\n"
            << "Фактическое количество зависит от размера выделенной области и выбранной плотности.";

        plan.summary = summary.str();
        return plan;
    }
}
