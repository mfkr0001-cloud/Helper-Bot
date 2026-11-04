#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/ui/TextInput.hpp>

#include "LocalGenerator.hpp"
#include "DecorationPlacer.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace {
    constexpr float HEADER_HEIGHT = 34.f;
    constexpr float RESIZE_HIT_SIZE = 24.f;
    constexpr float MIN_PANEL_WIDTH = 340.f;
    constexpr float MIN_PANEL_HEIGHT = 300.f;

    float clampFloat(float value, float minimum, float maximum) {
        if (maximum < minimum) maximum = minimum;
        return std::max(minimum, std::min(value, maximum));
    }

    class EditorFloatingPanel final : public CCLayer {
    protected:
        CCSize m_panelSize = {460.f, 320.f};
        CCPoint m_restorePosition = CCPointZero;
        CCSize m_restoreSize = {460.f, 320.f};
        CCPoint m_touchStartWorld = CCPointZero;
        CCPoint m_startPosition = CCPointZero;
        CCSize m_startSize = {460.f, 320.f};
        bool m_dragging = false;
        bool m_resizing = false;
        bool m_maximized = false;

        CCLayerColor* m_background = nullptr;
        CCLayerColor* m_header = nullptr;
        CCLabelBMFont* m_title = nullptr;
        CCLabelBMFont* m_promptLabel = nullptr;
        CCLabelBMFont* m_colorLabel = nullptr;
        CCLabelBMFont* m_footer = nullptr;
        CCLabelBMFont* m_resizeHint = nullptr;
        CCLabelBMFont* m_styleLabel = nullptr;
        CCLabelBMFont* m_densityLabel = nullptr;
        CCLabelBMFont* m_accentLabel = nullptr;
        CCLabelBMFont* m_layoutLabel = nullptr;
        CCLabelBMFont* m_shuffleLabel = nullptr;
        CCLabelBMFont* m_resetLabel = nullptr;
        CCLabelBMFont* m_generateLabel = nullptr;
        float m_promptBaseWidth = 1.f;
        EditorUI* m_editorUI = nullptr;
        LevelEditorLayer* m_editorLayer = nullptr;
        std::uint32_t m_generationSeed = 0xA1DEC0DEu;
        std::vector<CCLayerColor*> m_buttonTiles;

        geode::TextInput* m_promptInput = nullptr;
        geode::TextInput* m_colorInput = nullptr;

        CCMenuItemLabel* m_maximizeItem = nullptr;
        CCMenuItemLabel* m_hideItem = nullptr;
        CCMenuItemLabel* m_styleItem = nullptr;
        CCMenuItemLabel* m_densityItem = nullptr;
        CCMenuItemLabel* m_accentItem = nullptr;
        CCMenuItemLabel* m_layoutItem = nullptr;
        CCMenuItemLabel* m_shuffleItem = nullptr;
        CCMenuItemLabel* m_resetItem = nullptr;
        CCMenuItemLabel* m_generateItem = nullptr;

        std::vector<std::string> m_styles = aideco::availableStyles();
        std::vector<std::string> m_densities = {"LOW", "MEDIUM", "HIGH", "ULTRA"};
        std::vector<std::string> m_accentModes = {"AUTO", "MONO", "DUO", "TRIAD"};
        std::vector<std::string> m_layouts = {"BALANCED", "ASYMMETRIC", "CENTERED", "RHYTHM", "MIRRORED"};
        size_t m_styleIndex = 0;
        size_t m_densityIndex = 1;
        size_t m_accentIndex = 0;
        size_t m_layoutIndex = 0;
        size_t m_promptPresetIndex = 0;

        std::string m_promptValue = "Blue neon futuristic city with glowing details";
        std::string m_colorValue = "#28A9FF";

        void loadGeometry() {
            auto win = CCDirector::sharedDirector()->getWinSize();
            m_panelSize.width = Mod::get()->getSavedValue<float>("panel-width", 460.f);
            m_panelSize.height = Mod::get()->getSavedValue<float>("panel-height", 320.f);
            m_panelSize.width = clampFloat(m_panelSize.width, 300.f, std::max(300.f, win.width - 16.f));
            float maxHeight = std::max(220.f, win.height - 16.f);
            float minHeight = std::min(MIN_PANEL_HEIGHT, maxHeight);
            m_panelSize.height = clampFloat(m_panelSize.height, minHeight, maxHeight);

            float defaultX = std::max(8.f, win.width - m_panelSize.width - 18.f);
            float defaultY = std::max(8.f, win.height - m_panelSize.height - 54.f);
            float x = Mod::get()->getSavedValue<float>("panel-x", defaultX);
            float y = Mod::get()->getSavedValue<float>("panel-y", defaultY);
            m_maximized = Mod::get()->getSavedValue<bool>("panel-maximized", false);
            m_restorePosition.x = Mod::get()->getSavedValue<float>("restore-panel-x", defaultX);
            m_restorePosition.y = Mod::get()->getSavedValue<float>("restore-panel-y", defaultY);
            m_restoreSize.width = Mod::get()->getSavedValue<float>("restore-panel-width", 460.f);
            m_restoreSize.height = Mod::get()->getSavedValue<float>("restore-panel-height", 320.f);
            if (m_maximized) {
                m_panelSize = CCSize(
                    std::max(300.f, win.width - 16.f),
                    std::max(220.f, win.height - 16.f)
                );
                setPosition({8.f, 8.f});
            } else {
                setPosition({x, y});
            }
            clampToParent();
        }

        CCSize availableSize() {
            if (auto parent = getParent()) {
                auto size = parent->getContentSize();
                if (size.width > 0.f && size.height > 0.f) return size;
            }
            return CCDirector::sharedDirector()->getWinSize();
        }

        void clampToParent() {
            auto bounds = availableSize();
            float maxWidth = std::max(300.f, bounds.width - 16.f);
            float maxHeight = std::max(220.f, bounds.height - 16.f);
            m_panelSize.width = clampFloat(m_panelSize.width, std::min(MIN_PANEL_WIDTH, maxWidth), maxWidth);
            m_panelSize.height = clampFloat(m_panelSize.height, std::min(MIN_PANEL_HEIGHT, maxHeight), maxHeight);
            setPosition({
                clampFloat(getPositionX(), 8.f, std::max(8.f, bounds.width - m_panelSize.width - 8.f)),
                clampFloat(getPositionY(), 8.f, std::max(8.f, bounds.height - m_panelSize.height - 8.f))
            });
        }

        void saveGeometry() {
            Mod::get()->setSavedValue<float>("panel-x", getPositionX());
            Mod::get()->setSavedValue<float>("panel-y", getPositionY());
            Mod::get()->setSavedValue<float>("panel-width", m_panelSize.width);
            Mod::get()->setSavedValue<float>("panel-height", m_panelSize.height);
            Mod::get()->setSavedValue<bool>("panel-maximized", m_maximized);
        }

        CCLabelBMFont* makeLabel(std::string const& text, float scale) {
            auto label = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
            if (label) label->setScale(scale);
            return label;
        }

        CCMenuItemLabel* makeMenuItem(
            std::string const& text,
            float scale,
            SEL_MenuHandler callback,
            CCLabelBMFont** outLabel = nullptr
        ) {
            auto label = makeLabel(text, scale);
            if (!label) return nullptr;
            if (outLabel) *outLabel = label;
            return CCMenuItemLabel::create(label, this, callback);
        }

        void fitLabel(CCLabelBMFont* label, float maxWidth, float maxHeight, float maxScale = 0.34f) {
            if (!label) return;
            auto size = label->getContentSize();
            if (size.width <= 0.f || size.height <= 0.f) return;
            float scale = std::min(maxScale, std::min(maxWidth / size.width, maxHeight / size.height));
            label->setScale(std::max(0.18f, scale));
        }

        CCLayerColor* addButtonTile(float width, float height) {
            auto tile = CCLayerColor::create(ccc4(30, 48, 72, 235), width, height);
            if (!tile) return nullptr;
            tile->setAnchorPoint({0.5f, 0.5f});
            addChild(tile, 2);
            m_buttonTiles.push_back(tile);
            return tile;
        }

        void buildInterface() {
            removeAllChildrenWithCleanup(true);
            m_background = nullptr;
            m_header = nullptr;
            m_title = nullptr;
            m_promptLabel = nullptr;
            m_colorLabel = nullptr;
            m_footer = nullptr;
            m_resizeHint = nullptr;
            m_styleLabel = nullptr;
            m_densityLabel = nullptr;
            m_accentLabel = nullptr;
            m_layoutLabel = nullptr;
            m_shuffleLabel = nullptr;
            m_resetLabel = nullptr;
            m_generateLabel = nullptr;
            m_promptBaseWidth = 1.f;
            m_promptInput = nullptr;
            m_colorInput = nullptr;
            m_maximizeItem = nullptr;
            m_hideItem = nullptr;
            m_styleItem = nullptr;
            m_densityItem = nullptr;
            m_accentItem = nullptr;
            m_layoutItem = nullptr;
            m_shuffleItem = nullptr;
            m_resetItem = nullptr;
            m_generateItem = nullptr;
            m_buttonTiles.clear();

            setContentSize(m_panelSize);

            m_background = CCLayerColor::create(ccc4(13, 18, 29, 242), m_panelSize.width, m_panelSize.height);
            m_background->setAnchorPoint({0.f, 0.f});
            m_background->setPosition(CCPointZero);
            addChild(m_background, 0);

            m_header = CCLayerColor::create(ccc4(34, 69, 112, 255), m_panelSize.width, HEADER_HEIGHT);
            m_header->setAnchorPoint({0.f, 0.f});
            m_header->setPosition({0.f, m_panelSize.height - HEADER_HEIGHT});
            addChild(m_header, 1);

            m_title = makeLabel("AI DECORATOR  /  REMASTER TEST 10", 0.42f);
            if (m_title) {
                m_title->setAnchorPoint({0.f, 0.5f});
                m_title->setPosition({14.f, m_panelSize.height - HEADER_HEIGHT / 2.f});
                addChild(m_title, 3);
            }

            auto headerMenu = CCMenu::create();
            headerMenu->setPosition(CCPointZero);
            m_maximizeItem = makeMenuItem("MAX", 0.34f, menu_selector(EditorFloatingPanel::onMaximize));
            m_hideItem = makeMenuItem("X", 0.42f, menu_selector(EditorFloatingPanel::onHide));
            if (m_maximizeItem) headerMenu->addChild(m_maximizeItem);
            if (m_hideItem) headerMenu->addChild(m_hideItem);
            addChild(headerMenu, 5);

            m_promptLabel = makeLabel("PROMPT / DESCRIBE THE DECOR", 0.34f);
            if (m_promptLabel) {
                m_promptLabel->setAnchorPoint({0.f, 0.5f});
                m_promptLabel->setPosition({18.f, m_panelSize.height - 58.f});
                addChild(m_promptLabel, 3);
            }

            float promptWidth = std::max(250.f, m_panelSize.width - 38.f);
            m_promptBaseWidth = promptWidth;
            m_promptInput = geode::TextInput::create(promptWidth, "Describe your decoration...");
            m_promptInput->setMaxCharCount(220);
            m_promptInput->setString(m_promptValue);
            m_promptInput->setPosition({m_panelSize.width / 2.f, m_panelSize.height - 88.f});
            addChild(m_promptInput, 4);

            m_colorLabel = makeLabel("MAIN COLOR", 0.34f);
            if (m_colorLabel) {
                m_colorLabel->setAnchorPoint({0.f, 0.5f});
                m_colorLabel->setPosition({18.f, m_panelSize.height - 132.f});
                addChild(m_colorLabel, 3);
            }

            m_colorInput = geode::TextInput::create(112.f, "#28A9FF");
            m_colorInput->setMaxCharCount(7);
            m_colorInput->setString(m_colorValue);
            m_colorInput->setFilter("#0123456789abcdefABCDEF");
            m_colorInput->setPosition({88.f, m_panelSize.height - 158.f});
            addChild(m_colorInput, 4);

            // Separate square backplates make each setting a distinct touch target.
            for (int i = 0; i < 4; ++i) addButtonTile(142.f, 24.f);

            auto settingsMenu = CCMenu::create();
            settingsMenu->setPosition(CCPointZero);
            m_styleLabel = makeLabel("STYLE: AUTO", 0.34f);
            m_densityLabel = makeLabel("DENSITY: MEDIUM", 0.34f);
            m_accentLabel = makeLabel("COLORS: AUTO", 0.34f);
            m_layoutLabel = makeLabel("LAYOUT: BALANCED", 0.34f);
            m_styleItem = m_styleLabel ? CCMenuItemLabel::create(m_styleLabel, this, menu_selector(EditorFloatingPanel::onStyle)) : nullptr;
            m_densityItem = m_densityLabel ? CCMenuItemLabel::create(m_densityLabel, this, menu_selector(EditorFloatingPanel::onDensity)) : nullptr;
            m_accentItem = m_accentLabel ? CCMenuItemLabel::create(m_accentLabel, this, menu_selector(EditorFloatingPanel::onAccentMode)) : nullptr;
            m_layoutItem = m_layoutLabel ? CCMenuItemLabel::create(m_layoutLabel, this, menu_selector(EditorFloatingPanel::onLayout)) : nullptr;
            if (m_styleItem) settingsMenu->addChild(m_styleItem);
            if (m_densityItem) settingsMenu->addChild(m_densityItem);
            if (m_accentItem) settingsMenu->addChild(m_accentItem);
            if (m_layoutItem) settingsMenu->addChild(m_layoutItem);
            addChild(settingsMenu, 5);

            // Primary actions get larger, evenly spaced square tiles.
            for (int i = 0; i < 3; ++i) addButtonTile(126.f, 30.f);

            auto actionMenu = CCMenu::create();
            actionMenu->setPosition(CCPointZero);
            m_shuffleItem = makeMenuItem("EXAMPLE", 0.34f, menu_selector(EditorFloatingPanel::onShuffle), &m_shuffleLabel);
            m_resetItem = makeMenuItem("RESET", 0.36f, menu_selector(EditorFloatingPanel::onReset), &m_resetLabel);
            m_generateItem = makeMenuItem("GENERATE", 0.36f, menu_selector(EditorFloatingPanel::onGenerate), &m_generateLabel);
            if (m_shuffleItem) actionMenu->addChild(m_shuffleItem);
            if (m_resetItem) actionMenu->addChild(m_resetItem);
            if (m_generateItem) actionMenu->addChild(m_generateItem);
            addChild(actionMenu, 5);

            m_footer = makeLabel("V1.0.9 REMASTER TEST 10  |  50 STYLES  |  LAYOUT-AWARE DECOR", 0.27f);
            if (m_footer) {
                m_footer->setOpacity(175);
                m_footer->setPosition({m_panelSize.width / 2.f, 20.f});
                addChild(m_footer, 3);
            }

            m_resizeHint = makeLabel("///", 0.33f);
            if (m_resizeHint) {
                m_resizeHint->setOpacity(190);
                m_resizeHint->setPosition({m_panelSize.width - 14.f, 13.f});
                addChild(m_resizeHint, 6);
            }

            updateControlLabels();
            layoutControls();
        }

        void updateControlLabels() {
            if (m_styleLabel) {
                auto text = std::string("STYLE ") + std::to_string(m_styleIndex + 1) + "/50: " + m_styles[m_styleIndex];
                m_styleLabel->setString(text.c_str());
            }
            if (m_densityLabel) {
                auto text = std::string("DENSITY: ") + m_densities[m_densityIndex];
                m_densityLabel->setString(text.c_str());
            }
            if (m_accentLabel) {
                auto text = std::string("COLORS: ") + m_accentModes[m_accentIndex];
                m_accentLabel->setString(text.c_str());
            }
            if (m_layoutLabel) {
                auto text = std::string("LAYOUT: ") + m_layouts[m_layoutIndex];
                m_layoutLabel->setString(text.c_str());
            }
            // Fit every settings label inside its tile after text changes. The style
            // control can contain long names, so its scale is recalculated every time.
            float settingsWidth = std::min(142.f, m_panelSize.width - 190.f);
            fitLabel(m_styleLabel, settingsWidth - 12.f, 18.f, 0.34f);
            fitLabel(m_densityLabel, settingsWidth - 12.f, 18.f, 0.34f);
            fitLabel(m_accentLabel, settingsWidth - 12.f, 18.f, 0.34f);
            fitLabel(m_layoutLabel, settingsWidth - 12.f, 18.f, 0.34f);
        }

        void layoutControls() {
            if (m_title) m_title->setPosition({14.f, m_panelSize.height - HEADER_HEIGHT / 2.f});
            if (m_promptLabel) m_promptLabel->setPosition({18.f, m_panelSize.height - 58.f});
            if (m_colorLabel) m_colorLabel->setPosition({18.f, m_panelSize.height - 132.f});
            if (m_maximizeItem) m_maximizeItem->setPosition({m_panelSize.width - 58.f, m_panelSize.height - 17.f});
            if (m_hideItem) m_hideItem->setPosition({m_panelSize.width - 21.f, m_panelSize.height - 17.f});
            // Responsive two-row control grid: every option stays centred on its own tile.
            const float settingsTileWidth = std::min(142.f, m_panelSize.width - 190.f);
            const float settingsX = m_panelSize.width - settingsTileWidth * 0.5f - 14.f;
            const float settingY[4] = {
                m_panelSize.height - 132.f, m_panelSize.height - 162.f,
                m_panelSize.height - 192.f, m_panelSize.height - 222.f
            };
            if (m_styleItem) m_styleItem->setPosition({settingsX, settingY[0]});
            if (m_densityItem) m_densityItem->setPosition({settingsX, settingY[1]});
            if (m_accentItem) m_accentItem->setPosition({settingsX, settingY[2]});
            if (m_layoutItem) m_layoutItem->setPosition({settingsX, settingY[3]});
            const float actionTileWidth = std::min(126.f, (m_panelSize.width - 36.f) / 3.f - 8.f);
            const float actionX[3] = {
                m_panelSize.width / 6.f,
                m_panelSize.width / 2.f,
                m_panelSize.width * 5.f / 6.f
            };
            if (m_shuffleItem) m_shuffleItem->setPosition({actionX[0], 51.f});
            if (m_resetItem) m_resetItem->setPosition({actionX[1], 51.f});
            if (m_generateItem) m_generateItem->setPosition({actionX[2], 51.f});
            fitLabel(m_shuffleLabel, actionTileWidth - 12.f, 22.f, 0.36f);
            fitLabel(m_resetLabel, actionTileWidth - 12.f, 22.f, 0.36f);
            fitLabel(m_generateLabel, actionTileWidth - 12.f, 22.f, 0.36f);
            for (size_t i = 0; i < 4 && i < m_buttonTiles.size(); ++i) {
                m_buttonTiles[i]->changeWidthAndHeight(settingsTileWidth, 25.f);
                m_buttonTiles[i]->setPosition({settingsX, settingY[i]});
            }
            for (size_t i = 0; i < 3 && i + 4 < m_buttonTiles.size(); ++i) {
                m_buttonTiles[i + 4]->changeWidthAndHeight(actionTileWidth, 30.f);
                m_buttonTiles[i + 4]->setPosition({actionX[i], 51.f});
            }
            if (m_footer) m_footer->setPosition({m_panelSize.width / 2.f, 20.f});
            if (m_resizeHint) m_resizeHint->setPosition({m_panelSize.width - 14.f, 13.f});
        }

        void refreshFrame() {
            setContentSize(m_panelSize);
            if (m_background) m_background->changeWidthAndHeight(m_panelSize.width, m_panelSize.height);
            if (m_header) {
                m_header->changeWidthAndHeight(m_panelSize.width, HEADER_HEIGHT);
                m_header->setPosition({0.f, m_panelSize.height - HEADER_HEIGHT});
            }
            if (m_promptInput) {
                m_promptValue = m_promptInput->getString();
                float targetWidth = std::max(250.f, m_panelSize.width - 38.f);
                float scale = m_promptBaseWidth > 0.f ? targetWidth / m_promptBaseWidth : 1.f;
                m_promptInput->setScaleX(scale);
                m_promptInput->setPosition({m_panelSize.width / 2.f, m_panelSize.height - 88.f});
            }
            if (m_colorInput) m_colorInput->setPosition({88.f, m_panelSize.height - 158.f});
            layoutControls();
        }

        void captureInputs() {
            if (m_promptInput) m_promptValue = m_promptInput->getString();
            if (m_colorInput) m_colorValue = m_colorInput->getString();
        }

        void setGeometry(CCPoint position, CCSize size) {
            m_panelSize = size;
            setPosition(position);
            clampToParent();
            refreshFrame();
            saveGeometry();
        }

    public:
        static EditorFloatingPanel* create(EditorUI* editorUI, LevelEditorLayer* editorLayer) {
            auto ret = new EditorFloatingPanel();
            if (ret->init(editorUI, editorLayer)) {
                ret->autorelease();
                return ret;
            }
            delete ret;
            return nullptr;
        }

        bool init(EditorUI* editorUI, LevelEditorLayer* editorLayer) {
            if (!CCLayer::init()) return false;
            m_editorUI = editorUI;
            m_editorLayer = editorLayer;
            setAnchorPoint({0.f, 0.f});
            setTouchEnabled(true);
            setTouchMode(kCCTouchesOneByOne);
            loadGeometry();
            buildInterface();
            setVisible(false);
            return true;
        }

        void registerWithTouchDispatcher() override {
            CCDirector::sharedDirector()->getTouchDispatcher()->addTargetedDelegate(this, -500, true);
        }

        bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
            if (!isVisible() || !touch) return false;
            auto local = convertToNodeSpace(touch->getLocation());
            if (local.x < 0.f || local.y < 0.f ||
                local.x > m_panelSize.width || local.y > m_panelSize.height) {
                return false;
            }

            if (local.x >= m_panelSize.width - RESIZE_HIT_SIZE && local.y <= RESIZE_HIT_SIZE) {
                m_resizing = true;
                m_dragging = false;
                m_touchStartWorld = touch->getLocation();
                m_startPosition = getPosition();
                m_startSize = m_panelSize;
                return true;
            }

            if (local.y >= m_panelSize.height - HEADER_HEIGHT && local.x < m_panelSize.width - 88.f) {
                m_dragging = true;
                m_resizing = false;
                m_touchStartWorld = touch->getLocation();
                m_startPosition = getPosition();
                return true;
            }

            return false;
        }

        void ccTouchMoved(CCTouch* touch, CCEvent*) override {
            if (!touch || (!m_dragging && !m_resizing)) return;
            auto delta = touch->getLocation() - m_touchStartWorld;

            if (m_dragging) {
                setPosition(m_startPosition + delta);
                clampToParent();
                return;
            }

            auto bounds = availableSize();
            float maxWidth = std::max(300.f, bounds.width - 16.f);
            float maxHeight = std::max(220.f, bounds.height - 16.f);
            float minWidth = std::min(MIN_PANEL_WIDTH, maxWidth);
            float minHeight = std::min(MIN_PANEL_HEIGHT, maxHeight);

            m_panelSize.width = clampFloat(m_startSize.width + delta.x, minWidth, maxWidth);
            m_panelSize.height = clampFloat(m_startSize.height - delta.y, minHeight, maxHeight);
            float anchoredY = m_startPosition.y + m_startSize.height - m_panelSize.height;
            setPosition({m_startPosition.x, anchoredY});
            clampToParent();
            refreshFrame();
        }

        void ccTouchEnded(CCTouch*, CCEvent*) override {
            finishPointerAction();
        }

        void ccTouchCancelled(CCTouch*, CCEvent*) override {
            finishPointerAction();
        }

        void finishPointerAction() {
            bool wasResizing = m_resizing;
            m_dragging = false;
            m_resizing = false;
            if (wasResizing) refreshFrame();
            saveGeometry();
        }

        void onHide(CCObject*) {
            captureInputs();
            saveGeometry();
            setVisible(false);
        }

        void onMaximize(CCObject*) {
            captureInputs();
            if (!m_maximized) {
                m_restorePosition = getPosition();
                m_restoreSize = m_panelSize;
                Mod::get()->setSavedValue<float>("restore-panel-x", m_restorePosition.x);
                Mod::get()->setSavedValue<float>("restore-panel-y", m_restorePosition.y);
                Mod::get()->setSavedValue<float>("restore-panel-width", m_restoreSize.width);
                Mod::get()->setSavedValue<float>("restore-panel-height", m_restoreSize.height);
                auto bounds = availableSize();
                setGeometry(
                    {8.f, 8.f},
                    {std::max(300.f, bounds.width - 16.f), std::max(220.f, bounds.height - 16.f)}
                );
                m_maximized = true;
                saveGeometry();
            } else {
                m_maximized = false;
                setGeometry(m_restorePosition, m_restoreSize);
                saveGeometry();
            }
        }

        void onStyle(CCObject*) {
            m_styleIndex = (m_styleIndex + 1) % m_styles.size();
            updateControlLabels();
        }

        void onDensity(CCObject*) {
            m_densityIndex = (m_densityIndex + 1) % m_densities.size();
            updateControlLabels();
        }

        void onAccentMode(CCObject*) {
            m_accentIndex = (m_accentIndex + 1) % m_accentModes.size();
            updateControlLabels();
        }

        void onLayout(CCObject*) {
            m_layoutIndex = (m_layoutIndex + 1) % m_layouts.size();
            updateControlLabels();
        }

        void onShuffle(CCObject*) {
            static std::array<std::string, 7> examples = {
                "Blue neon futuristic city with glowing details",
                "Dark mechanical factory with orange warning lights",
                "Floating space station, stars, purple energy lines",
                "Green forest ruins with leaves and natural shapes",
                "Underwater temple, cyan waves and glowing crystals",
                "Minimal white architecture with gold accents",
                "Red volcano base with lava cracks and sharp forms"
            };
            m_promptValue = examples[m_promptPresetIndex % examples.size()];
            ++m_promptPresetIndex;
            if (m_promptInput) m_promptInput->setString(m_promptValue);
            m_styleIndex = (m_styleIndex + 1) % m_styles.size();
            updateControlLabels();
        }

        void onReset(CCObject*) {
            m_promptValue = "Blue neon futuristic city with glowing details";
            m_colorValue = "#28A9FF";
            m_styleIndex = 0;
            m_densityIndex = 1;
            m_accentIndex = 0;
            m_layoutIndex = 0;
            if (m_promptInput) m_promptInput->setString(m_promptValue);
            if (m_colorInput) m_colorInput->setString(m_colorValue);
            updateControlLabels();
        }

        bool getAutomaticLevelBounds(
            aideco::SelectionRect& outBounds,
            int& objectCount,
            std::vector<aideco::GeometryAnchor>& layoutGeometry
        ) const {
            objectCount = 0;
            layoutGeometry.clear();
            if (!m_editorLayer) return false;

            float minX = std::numeric_limits<float>::max();
            float maxX = std::numeric_limits<float>::lowest();
            auto allObjects = m_editorLayer->getAllObjects();
            if (allObjects) {
                for (unsigned int i = 0; i < allObjects->count(); ++i) {
                    auto object = static_cast<GameObject*>(allObjects->objectAtIndex(i));
                    if (!object) continue;
                    auto rect = m_editorLayer->getObjectRect(object, true, false);
                    float left = rect.getMinX();
                    float right = rect.getMaxX();
                    float bottom = rect.getMinY();
                    float top = rect.getMaxY();
                    if (right - left < 1.f || top - bottom < 1.f) {
                        auto position = object->getPosition();
                        left = position.x - 15.f;
                        right = position.x + 15.f;
                        bottom = position.y - 15.f;
                        top = position.y + 15.f;
                    }

                    minX = std::min(minX, left);
                    maxX = std::max(maxX, right);
                    ++objectCount;

                    const float objectWidth = right - left;
                    const float objectHeight = top - bottom;
                    const float centerY = (bottom + top) * 0.5f;
                    // Keep plausible layout geometry only. Oversized triggers/background
                    // rectangles are ignored as decoration anchors, but still count for bounds.
                    if (objectWidth >= 6.f && objectHeight >= 6.f &&
                        objectWidth <= 360.f && objectHeight <= 220.f &&
                        centerY >= -24.f && centerY <= 384.f) {
                        layoutGeometry.push_back({left, bottom, right, top});
                    }
                }
            }

            float lastObjectX = m_editorLayer->getLastObjectX();
            if (objectCount == 0 || minX == std::numeric_limits<float>::max()) {
                minX = 0.f;
                maxX = std::max(1200.f, lastObjectX + 240.f);
            } else {
                maxX = std::max(maxX, lastObjectX);
                minX = std::max(0.f, minX - 60.f);
                maxX += 180.f;
                if (maxX - minX < 900.f) maxX = minX + 900.f;
            }

            // The whole current level is scanned; no block selection is requested.
            outBounds = {minX, 0.f, maxX, 360.f};
            return outBounds.valid();
        }

        static cocos2d::ccColor3B parseColor(std::string const& hex) {
            if (hex.size() != 7 || hex[0] != '#') return {40, 169, 255};
            auto parsePair = [&](size_t offset) -> unsigned char {
                try {
                    return static_cast<unsigned char>(std::stoi(hex.substr(offset, 2), nullptr, 16));
                } catch (...) {
                    return 0;
                }
            };
            return {parsePair(1), parsePair(3), parsePair(5)};
        }

        void onGenerate(CCObject*) {
            captureInputs();

            if (!m_editorLayer || !m_editorUI) {
                FLAlertLayer::create("AI DECORATOR", "Не удалось получить доступ к редактору уровня.", "OK")->show();
                return;
            }

            aideco::SelectionRect selection;
            int levelObjectCount = 0;
            std::vector<aideco::GeometryAnchor> layoutGeometry;
            // Generation scans the full open level and measures its existing layout.
            // Manual selection is not required; the layout itself is the source geometry.
            if (!getAutomaticLevelBounds(selection, levelObjectCount, layoutGeometry)) {
                FLAlertLayer::create(
                    "AI DECORATOR",
                    "Не удалось определить длину уровня. Открой уровень в редакторе и попробуй снова.",
                    "OK"
                )->show();
                return;
            }

            auto plan = aideco::makePlan(
                m_promptValue,
                m_styles[m_styleIndex],
                m_densities[m_densityIndex],
                m_colorValue,
                m_accentModes[m_accentIndex],
                m_layouts[m_layoutIndex]
            );

            auto placements = aideco::makePlacements(plan, selection, layoutGeometry, m_generationSeed);
            m_generationSeed += 0x9E3779B9u;
            if (!placements.valid) {
                auto message = placements.error.empty() ? "Не удалось создать план расстановки." : placements.error;
                FLAlertLayer::create("AI DECORATOR", message.c_str(), "OK")->show();
                return;
            }

            auto primary = parseColor(plan.primaryColor);
            auto accent = parseColor(plan.accentColor);
            auto secondary = parseColor(plan.secondaryColor);
            int created = 0;

            for (auto const& placement : placements.objects) {
                auto object = m_editorLayer->createObject(
                    placement.objectID,
                    {placement.x, placement.y},
                    true // register generated objects in the editor Undo history
                );
                if (!object) continue;

                cocos2d::ccColor3B color = primary;
                if (placement.colorSlot == 1) color = accent;
                else if (placement.colorSlot == 2) color = secondary;

                object->setScale(placement.scale);
                object->setRotation(placement.rotation);
                object->setObjectColor(color);
                object->setChildColor(color);
                object->setOpacity(placement.opacity);
                if (placement.objectID == 1754 ||
                    placement.objectID == 1886 || placement.objectID == 1887 || placement.objectID == 1888) {
                    object->setGlowColor(color);
                }
                ++created;
            }

            if (created == 0) {
                FLAlertLayer::create(
                    "AI DECORATOR / ERROR",
                    "Редактор не создал ни одного объекта. Проверь свободное место в уровне и повтори попытку.",
                    "OK"
                )->show();
                return;
            }

            std::string message = "Готово! Создано объектов декора: " + std::to_string(created) +
                " из " + std::to_string(placements.objects.size()) +
                "\nОбъектов, обнаруженных в уровне: " + std::to_string(levelObjectCount) +
                "\nЭлементов layout проанализировано: " + std::to_string(placements.analyzedAnchors) +
                "\nАктивных участков геометрии: " + std::to_string(placements.activeSegments) +
                "\nРежим: анализ всего открытого уровня" +
                "\nСтиль: " + plan.style + "\nТема: " + plan.detectedThemes +
                "\nЦвета: " + plan.primaryColor + " / " + plan.accentColor +
                "\nКомпозиция: " + plan.layout + "\nПлотность: " + plan.density +
                "\n\nКаждый созданный объект добавлен в обычную историю Undo редактора.";
            FLAlertLayer::create("AI DECORATOR / DONE", message.c_str(), "OK")->show();
        }
    };
}

class $modify(AIDecoratorEditorUI, EditorUI) {
    struct Fields {
        EditorFloatingPanel* panel = nullptr;
    };

    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        // Both the launcher and the panel are attached only to EditorUI.
        // They never appear in the main menu or during normal gameplay.
        auto winSize = CCDirector::sharedDirector()->getWinSize();

        auto panel = EditorFloatingPanel::create(this, m_editorLayer);
        if (panel) {
            m_fields->panel = panel;
            this->addChild(panel, 9998);
        }

        auto launcherMenu = CCMenu::create();
        launcherMenu->setPosition({48.f, winSize.height - 72.f});
        auto launcherTile = CCLayerColor::create(ccc4(30, 48, 72, 235), 72.f, 26.f);
        launcherTile->setAnchorPoint({0.5f, 0.5f});
        launcherMenu->addChild(launcherTile, 0);
        auto launcherLabel = CCLabelBMFont::create("DECOR", "bigFont.fnt");
        launcherLabel->setScale(0.36f);
        auto launcherButton = CCMenuItemLabel::create(
            launcherLabel, this, menu_selector(AIDecoratorEditorUI::onToggleDecorator)
        );
        launcherMenu->addChild(launcherButton);
        this->addChild(launcherMenu, 9999);
        return true;
    }

    void onToggleDecorator(CCObject*) {
        if (!m_fields->panel) return;
        m_fields->panel->setVisible(!m_fields->panel->isVisible());
    }
};
