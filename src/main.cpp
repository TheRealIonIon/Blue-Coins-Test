#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>

using namespace geode::prelude;
std::array<bool, 3> coins = {false, false, false};

$on_mod(Loaded) {
    auto path = Mod::get()->getResourcesDir();
    CCFileUtils::get()->addTexturePack({
        .m_id = "GJ_GameSheet02",
        .m_paths = {string::pathToString(path)}
    });
}

class $modify(PlayLayer) {
    void onEnter() {
        PlayLayer::onEnter();
        coins = {false, false, false};
    }

    void fullReset() {
        PlayLayer::fullReset();
        coins = {false, false, false};
    }

    void resetLevelFromStart() {
        PlayLayer::resetLevelFromStart();
        coins = {false, false, false};
    }

    void resumeAndRestart(bool fromStart) {
        PlayLayer::resumeAndRestart(fromStart);
        if (fromStart) coins = {false, false, false};
    }
};

class $modify(GameObject) {
    void playDestroyObjectAnim(GJBaseGameLayer* param) {
        if (this->m_objectID == 1329 || this->m_objectID == 142) {
            if (auto playLayer = PlayLayer::get()) {
                std::vector<GameObject*> allCoins;
                if (playLayer->m_objects) {
                    for (unsigned int i = 0; i < playLayer->m_objects->count(); i++) {
                        auto obj = static_cast<CCNode*>(playLayer->m_objects->objectAtIndex(i));
                        if (auto go = typeinfo_cast<GameObject*>(obj)) {
                            if (go->m_objectID == 1329 || go->m_objectID == 142) allCoins.push_back(go);
                        }
                    }
                }
                
                std::sort(allCoins.begin(), allCoins.end(), [](GameObject* a, GameObject* b) {
                    float ax = a->getPositionX();
                    float ay = a->getPositionY();
                    float bx = b->getPositionX();
                    float by = b->getPositionY();
                    return (ax * ax + ay * ay) < (bx * bx + by * by);
                });
                
                for (size_t i = 0; i < allCoins.size() && i < 3; i++) {
                    if (allCoins[i] == this) coins[i] = true;
                }
            }
        }
        
        GameObject::playDestroyObjectAnim(param);
    }
};

class $modify(PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto levelID = PlayLayer::get()->m_level->m_levelID;
		bool levelCond = levelID < 50 || levelID >= 5001 && levelID <= 5004;
		const char* dottedName = levelCond ? "DottedSecretCoin.png"_spr : "DottedUserCoin.png"_spr;

        if (auto compactMenu = this->getChildByID("xjotabelikex.compact-pause-menu/coins-menu")) {
            for (int i = 0; i < 3; i++) {
                if (!compactMenu->getChildByType<CCNode>(i)) continue;
                auto coinNode = compactMenu->getChildByType<CCNode>(i)->getChildByType<CCSprite>(0);

                if (!coinNode) continue;
                auto coinSpr = typeinfo_cast<CCSprite*>(coinNode);

                if (coinSpr) {
                    auto newFrame = CCSprite::createWithSpriteFrameName(dottedName);  
                    if (newFrame) coinSpr->setDisplayFrame(newFrame->displayFrame());
                }
            }            
        }

        auto bottomMenu = this->getChildByID("bottom-button-menu");
        if (!bottomMenu) return;

        for (int i = 1; i <= 3; i++) {
            if (!bottomMenu->getChildByTag(1000 + i)) continue;

            auto coinNode = bottomMenu->getChildByTag(1000 + i);
            auto coinSpr = typeinfo_cast<CCSprite*>(coinNode);

            if (coinSpr) {
                auto newFrame = CCSprite::createWithSpriteFrameName(dottedName);  
                if (newFrame) coinSpr->setDisplayFrame(newFrame->displayFrame());
            }
        }
    }
};

class $modify(EndLevelLayer) {
	void customSetup() {
		EndLevelLayer::customSetup();

		auto mainLayer = this->getChildByID("main-layer");
		auto levelID = PlayLayer::get()->m_level->m_levelID;
		bool levelCond = levelID < 50 || levelID >= 5001 && levelID <= 5004;
		const char* dottedName = levelCond ? "DottedSecretCoin.png"_spr : "DottedUserCoin.png"_spr;
        const char* blueName = levelCond ? "StaticSecretCoin.png"_spr : "StaticUserCoin.png"_spr;

        for (int i = 1; i <= 3; i++) {
            auto coinName = fmt::format("coin-{}-background", i);
            auto coinNode = mainLayer->getChildByID(coinName);
            auto coinSpr = typeinfo_cast<CCSprite*>(coinNode);

            if (coinSpr) {
                auto newFrame = CCSprite::createWithSpriteFrameName(dottedName);  
                if (newFrame) coinSpr->setDisplayFrame(newFrame->displayFrame());
            }
        }

		for (int i = 1; i <= 3; i++) {
			auto coinNode = fmt::format("coin-{}-sprite", i);
			auto blueNode = fmt::format("blue-{}-sprite", i);
			auto oldSpr = mainLayer->getChildByID(coinNode);

			if (oldSpr && coins[i-1]) {
                auto coinObj = typeinfo_cast<CCSprite*>(oldSpr);
                coinObj->setColor(ccColor3B({255, 255, 255}));

				auto newNode = CCNode::create();
				newNode->setID(blueNode);
				newNode->setPosition(oldSpr->getPosition());
				newNode->setScale(oldSpr->getScale());
				newNode->setZOrder(oldSpr->getZOrder());

				auto newSpr = CCSprite::createWithSpriteFrameName(blueName);
				newNode->addChild(newSpr);
				mainLayer->addChild(newNode);		
			}
		}
	}
};