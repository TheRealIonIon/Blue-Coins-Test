#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>

#define getFrameByName CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName

using namespace geode::prelude;

class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        std::array<bool, 3> m_coinPicks = {};
        std::vector<GameObject*> m_coinArr;
    };

    bool isRobtop(GJGameLevel* level) {
        int levelID = level->m_levelID.value();
        if (levelID > 0 && levelID < 50) return true;
        if (levelID >= 1001 && levelID <= 1003) return true;
        if (levelID >= 4001 && levelID <= 4003) return true;
        return levelID >= 5001 && levelID <= 5004 || levelID == 3001;
    }

    bool hasCoin(int n) {
        if (m_level->m_levelID == 0) {
            switch (n) {
                case 1: return m_level->m_firstCoinVerified;
                case 2: return m_level->m_secondCoinVerified;
                case 3: return m_level->m_thirdCoinVerified;
            }
        }

        auto coinKey = m_level->getCoinKey(n);
        auto statsMng = GameStatsManager::sharedState();
        if (!coinKey || !statsMng) return false;

        if (isRobtop(m_level)) return statsMng->hasSecretCoin(coinKey);
        return statsMng->hasUserCoin(coinKey) || statsMng->hasPendingUserCoin(coinKey);
    }

    void updateCoins() {
        std::string coinType = isRobtop(m_level) ? "Secret" : "User";
        int coinCount = m_fields->m_coinArr.size();

        for (int i = 0; i < coinCount; i++) {
            if (this->hasCoin(i + 1)) {
                auto coinSpr = typeinfo_cast<CCSprite*>(m_fields->m_coinArr[i]);
                
                if (!coinSpr) continue;
                coinSpr->stopAllActions();

                auto coinAnim = CCAnimation::create();
                CCSpriteFrame* firstFrame = nullptr;

                for (int j = 1; j <= 4; j++) {
                    std::string coinName = fmt::format("{}Coin{}.png"_spr, coinType, j);
                    if (auto coinFrame = getFrameByName(coinName.c_str())) {
                        if (!firstFrame) firstFrame = coinFrame;
                        coinAnim->addSpriteFrame(coinFrame);
                    }
                }

                if (coinAnim->getFrames() && coinAnim->getFrames()->count() > 0) {
                    coinAnim->setDelayPerUnit(0.1f);
                    if (firstFrame) coinSpr->setDisplayFrame(firstFrame);
                    coinSpr->runAction(CCRepeatForever::create(CCAnimate::create(coinAnim)));

                    coinSpr->setColor({ 255, 255, 255 });
                    coinSpr->setID("blue-coin"_spr);
                }
            }
        }
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        m_fields->m_coinPicks = {};
    }

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        for (auto* gameObj : CCArrayExt<GameObject*>(this->m_objects)) {
            if (gameObj->m_objectID == 142 || gameObj->m_objectID == 1329) {
                m_fields->m_coinArr.push_back(gameObj);
                gameObj->m_addToNodeContainer = true;
            }
        }

        std::sort(
            m_fields->m_coinArr.begin(), 
            m_fields->m_coinArr.end(),
            [](GameObject* a, GameObject* b) {
                float ax = a->getPositionX(), ay = a->getPositionY();
                float bx = b->getPositionX(), by = b->getPositionY();
                return (ax * ax + ay * ay) < (bx * bx + by * by);
            }
        );
        
        this->updateCoins();
    }
};

class $modify(EndLevelLayer) {
	void customSetup() {
		EndLevelLayer::customSetup();

        auto playLayer = static_cast<MyPlayLayer*>(PlayLayer::get());
        auto isRobtop = playLayer->isRobtop(playLayer->m_level);

		auto mainLayer = this->getChildByID("main-layer");
        playLayer->updateCoins();

        const char* frameName = isRobtop ? "SecretCoin1.png"_spr : "UserCoin1.png"_spr;

		for (int i = 1; i <= 3; i++) {
			auto coinNode = fmt::format("coin-{}-sprite", i);
			auto blueNode = fmt::format("blue-{}-sprite", i);
			
            if (playLayer->m_fields->m_coinPicks[i-1]) {
                if (auto oldSpr = mainLayer->getChildByID(coinNode)) {
                    typeinfo_cast<CCSprite*>(oldSpr)->setColor({ 255, 255, 255 });
                    auto newSpr = CCSprite::createWithSpriteFrameName(frameName);
                    newSpr->setPosition(oldSpr->getPosition());
                    mainLayer->addChild(newSpr, 10);
                    newSpr->setID(blueNode);
                }
            }
		}
	}
};

/*
    Code adapted from Rated Layouts mod, with ArcticWoof's approval:
    https://github.com/RatedLayouts/RatedLayouts/blob/main/src/hook/GameObjectCoin.cpp
*/

#include <Geode/modify/CCSprite.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/EffectGameObject.hpp>

#define getFrameByName CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName

class $modify(CCSprite) {
    void setDisplayFrame(CCSpriteFrame* newFrame) {
        if (this->getID() == "blue-coin"_spr) {
            auto playLayer = static_cast<MyPlayLayer*>(PlayLayer::get());
            bool isRobtop = playLayer->isRobtop(playLayer->m_level);
            std::string coinType = isRobtop ? "Secret" : "User";

            if (!newFrame) newFrame = getFrameByName(isRobtop ? "SecretCoin1.png"_spr : "UserCoin1.png"_spr);

            for (int i = 1; i <= 4; i++) {
                std::string coinName = fmt::format("{}Coin{}.png"_spr, coinType, i);
                auto coinFrame = getFrameByName(coinName.c_str());

                if (coinFrame && coinFrame == newFrame) {
                    CCSprite::setDisplayFrame(newFrame);
                    return;
                }
            }

            return;
        }

        CCSprite::setDisplayFrame(newFrame);
    }
};

class $modify(GameObject) {
    void playDestroyObjectAnim(GJBaseGameLayer* gameLayer) {
        if (m_objectID == 142 || m_objectID == 1329) {
            if (gameLayer->m_isEditor || !this->m_isUIObject) {
                GameObject::playDestroyObjectAnim(gameLayer);
                return;
            }

            auto playLayer = static_cast<MyPlayLayer*>(PlayLayer::get());
            size_t coinCount = playLayer->m_fields->m_coinArr.size();
            int coinIndex = -1;

            for (size_t i = 0; i < coinCount; i++) {
                if (playLayer->m_fields->m_coinArr[i] == this) {
                    playLayer->m_fields->m_coinPicks[i] = true;
                    coinIndex = static_cast<int>(i + 1);
                }
            }

            if (coinIndex == -1) coinIndex = 1;
            CCNode* parentNode = this->getParent();
            std::vector<CCNode*> prevChildren;

            for (auto childNode : parentNode->getChildrenExt()) {
                prevChildren.push_back(childNode);
            }

            GameObject::playDestroyObjectAnim(gameLayer);
            if (!playLayer->hasCoin(coinIndex)) return;

            for (auto childNode : parentNode->getChildrenExt()) {
                bool hasExisted = false;
                for (auto& n : prevChildren) {
                    if (n == childNode) {
                        hasExisted = true;
                        break;
                    }
                }

                if (hasExisted) continue;

                if (auto coinSpr = typeinfo_cast<CCSprite*>(childNode)) {
                    CCAnimation* coinAnim = CCAnimation::create();
                    CCSpriteFrame* firstFrame = nullptr;
                    std::string coinType = this->m_objectID == 142 ? "Secret" : "User";

                    for (int i = 1; i <= 4; i++) {
                        auto coinName = fmt::format("{}Coin{}.png"_spr, coinType, i);
                        if (auto coinFrame = getFrameByName(coinName.c_str())) {
                            if (!firstFrame) firstFrame = coinFrame;
                            coinAnim->addSpriteFrame(coinFrame);
                        }
                    }

                    if (coinAnim->getFrames() && coinAnim->getFrames()->count() > 0) {
                        coinAnim->setDelayPerUnit(0.05f);
                        if (firstFrame) coinSpr->setDisplayFrame(firstFrame);
                        coinSpr->runAction(CCRepeatForever::create(CCAnimate::create(coinAnim)));

                        coinSpr->setColor({ 255, 255, 255 });
                        coinSpr->setID("blue-coin"_spr);
                    }
                }
            }

            return;
        }

        GameObject::playDestroyObjectAnim(gameLayer);
    }
};