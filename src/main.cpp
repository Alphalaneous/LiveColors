#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>

using namespace geode::prelude;

class ColorVisualButton : public CCMenuItemSpriteExtra {

public:
	EditorUI* m_editorUI;
	int m_currentColorID = -1;
	ColorChannelSprite* m_colorChannelSprite;
	ColorAction* m_action;
	CCLabelBMFont* m_IDLabel;
	GJColorSetupLayer* m_colorSetupLayer;

	static ColorVisualButton* create(EditorUI* editorUI, GJColorSetupLayer* colorSetupLayer) {
		auto ret = new ColorVisualButton();
		if (ret->init(editorUI, colorSetupLayer)) {
			ret->autorelease();
			return ret;
		}
		delete ret;
		return nullptr;
	};

	bool init(EditorUI* editorUI, GJColorSetupLayer* colorSetupLayer) {
		m_colorChannelSprite = ColorChannelSprite::create();
		CCMenuItemSpriteExtra::init(m_colorChannelSprite, nullptr, this, menu_selector(ColorVisualButton::openColorPicker));
		m_editorUI = editorUI;
		m_colorSetupLayer = colorSetupLayer;
		m_IDLabel = CCLabelBMFont::create("", "bigFont.fnt");
		m_IDLabel->setScale(0.4f);
		m_IDLabel->setPositionX(getContentSize().width/2);
		m_IDLabel->setPositionY(getContentSize().height/2 + 1);

		addChild(m_IDLabel);

		setVisible(false);
		return true;
	}

	void setColorData(int id, const ccColor3B& color, bool blending, float opacity, ColorAction* action) {
		m_currentColorID = id;
		m_IDLabel->setString(idToString(id).c_str());
		m_IDLabel->setScale(0.4f);
		m_action = action;
		m_colorChannelSprite->setColor(color);
		m_colorChannelSprite->updateBlending(blending);
		m_colorChannelSprite->updateOpacity(opacity);
		if (action) {
			if (action->m_copyID > 0) {
				m_IDLabel->setString(fmt::format("{}>{}", idToString(id), idToString(action->m_copyID)).c_str());
				m_IDLabel->setScale(0.3f);
			}
		}

		setVisible(id != -1);
	}

	void openColorPicker(CCObject* obj) {
		m_colorSetupLayer->m_page = (m_currentColorID - 1) / m_colorSetupLayer->m_colorsPerPage;
		int idx = ((m_currentColorID - 1) % m_colorSetupLayer->m_colorsPerPage) + 1;
		obj->setTag(idx);
		m_colorSetupLayer->onColor(obj);
	}

	std::string idToString(int ID) {
		switch (ID) {
			case 1000: return "BG";
			case 1001: return "G1";
			case 1002: return "L";
			case 1003: return "3DL";
			case 1004: return "Obj";
			case 1005: return "P1";
			case 1006: return "P2";
			case 1007: return "LBG";
			case 1009: return "G2";
			case 1010: return "B";
			case 1011: return "W";
			case 1012: return "LTR";
			case 1013: return "MG";
			case 1014: return "MG2";
			default: return utils::numToString(ID);
		}
	}
};

class $modify(MyEditorUI, EditorUI) {

	static void onModify(auto& self) {
        (void) self.setHookPriorityAfterPost("EditorUI::init", "hjfod.betteredit");
    }

	struct Fields {
		CCMenu* m_colorsMenu;
		std::vector<Ref<ColorVisualButton>> m_buttons;
		int m_lastBtnCount = 0;
		Ref<GJColorSetupLayer> m_evilColorSetupLayer;
		int m_availableBtnCount = 0;
	};

    void showUI(bool show) {
		EditorUI::showUI(show);
		float scale = m_positionSlider->getScale();
		bool isLowScale = scale <= 0.925;
		CCSize winSize = CCDirector::get()->getWinSize();
		auto fields = m_fields.self();
		fields->m_colorsMenu->setPositionY(show ? m_toolbarHeight + 20 * scale : 5);
		fields->m_colorsMenu->setEnabled(show);
	}

	bool init(LevelEditorLayer* editorLayer) {
		if (!EditorUI::init(editorLayer)) {
			return false;
		}

		auto fields = m_fields.self();

		CCSize winSize = CCDirector::get()->getWinSize();

		float scale = m_positionSlider->getScale();

		bool isLowScale = scale <= 0.925;

		fields->m_evilColorSetupLayer = GJColorSetupLayer::create(editorLayer->m_levelSettings);
		fields->m_colorsMenu = CCMenu::create();
		fields->m_colorsMenu->ignoreAnchorPointForPosition(false);
		fields->m_colorsMenu->setAnchorPoint({0.5f, 0});
		fields->m_colorsMenu->setScale(0.35f);
		if (isLowScale) {
			fields->m_colorsMenu->setPosition({winSize.width/2, m_toolbarHeight + 20 * scale});
		}
		else {
			fields->m_colorsMenu->setPosition({winSize.width/2 - 35 * scale, m_toolbarHeight + 20 * scale});
		}
		auto rowLayout = RowLayout::create();
		rowLayout->setAxisAlignment(AxisAlignment::Center);
		rowLayout->setCrossAxisAlignment(AxisAlignment::Center);
		rowLayout->ignoreInvisibleChildren(true);
		fields->m_colorsMenu->setLayout(rowLayout);

		float maxWidth = 0;
		if (isLowScale) {
			maxWidth = (winSize.width - 20 * scale) / fields->m_colorsMenu->getScale();
		}
		else {
			maxWidth = (winSize.width - 210 * scale) / fields->m_colorsMenu->getScale();
		}
		float btnWidth = CCSprite::createWithSpriteFrameName("GJ_colorBtn_001.png")->getContentWidth();
		fields->m_availableBtnCount = std::floor((maxWidth + 5)/ (btnWidth + 5));

		fields->m_colorsMenu->setContentSize({maxWidth, 30});

		for (int i = 0; i < fields->m_availableBtnCount; i++) {
			ColorVisualButton* btn = ColorVisualButton::create(this, fields->m_evilColorSetupLayer);
			fields->m_buttons.push_back(btn);
			fields->m_colorsMenu->addChild(btn);
		}

		fields->m_colorsMenu->updateLayout();

		addChild(fields->m_colorsMenu);

		schedule(schedule_selector(MyEditorUI::checkColors));

		return true;
	}

	void checkColors(float dt) {
		std::set<int> activeColors;

		auto fields = m_fields.self();

		for (ColorVisualButton* btn : fields->m_buttons) {
			btn->setColorData(-1, {255, 255, 255}, false, 1, nullptr);
		}

		forEachObject(m_editorLayer, [&activeColors, this](GameObject* object) {
			if (m_editorLayer->m_currentLayer != -1 
				&& object->m_editorLayer != m_editorLayer->m_currentLayer 
				&& object->m_editorLayer2 != m_editorLayer->m_currentLayer) return;
			if (auto base = object->m_baseColor) {
				activeColors.insert(base->m_colorID);
			}
			if (auto detail = object->m_detailColor) {
				activeColors.insert(detail->m_colorID);
			}
		});

		int count = 0;
		for (ColorActionSprite* action : m_editorLayer->m_effectManager->m_unkVector2d8) {
			static const std::unordered_set<int> invalidColorIDs = {
				1005, 1006, 1010, 1011, 1012
			};

			if (!action || action->m_colorID <= 0 || invalidColorIDs.count(action->m_colorID)) continue;
			if (activeColors.contains(action->m_colorID)) {
				auto btn = fields->m_buttons[count];
				ccColor3B color = action->m_color;
				for (PulseEffectAction& pulse : m_editorLayer->m_effectManager->m_pulseEffectVector) {
					if (pulse.m_targetGroupID == action->m_colorID) {
						color = m_editorLayer->m_effectManager->colorForPulseEffect(color, &pulse);
					}
				}
				bool blending = false;
				if (action->m_colorAction) blending = action->m_colorAction->m_blending;
				btn->setColorData(action->m_colorID, color, blending, action->m_opacity / 255.f, action->m_colorAction);
				count++;
			}
			if (count >= fields->m_availableBtnCount) break;
		}
		if (fields->m_lastBtnCount != count) {
			fields->m_colorsMenu->updateLayout();
		}
		fields->m_lastBtnCount = count;
	}

	//thanks eclipse
	static void forEachObject(GJBaseGameLayer const* game, std::function<void(GameObject*)> const& callback) {
        int count = game->m_sections.empty() ? -1 : game->m_sections.size();
        for (int i = game->m_leftSectionIndex; i <= game->m_rightSectionIndex && i < count; ++i) {
            auto leftSection = game->m_sections[i];
            if (!leftSection) continue;

            auto leftSectionSize = leftSection->size();
            for (int j = game->m_bottomSectionIndex; j <= game->m_topSectionIndex && j < leftSectionSize; ++j) {
                auto section = leftSection->at(j);
                if (!section) continue;

                auto sectionSize = game->m_sectionSizes[i]->at(j);
                for (int k = 0; k < sectionSize; ++k) {
                    auto obj = section->at(k);
                    if (!obj) continue;

                    callback(obj);
                }
            }
        }
    }
};