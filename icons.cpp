
#include "game.h"
#include "render.h"

enum {
	kIconJump = 0,
	kIconRun,
	kIconGun,
	kIconLookFront,
	kIconLookRight,
	kIconAction,
	kIconWalk,
	kIconLoad,
	kIconLookBack,
	kIconLookLeft,
	kIconTurn180,
	kIconCameraFix,
	kIconDuck,
	kIconStepLeft,
	kIconStepBack,
	kIconStepRight,
	kIconCameraMotion,
	kIconCameraRotation,
	kIconInventory,
	kIconOptions,
	kIconMap,
	kIconMode,
	kIconStatic,
	kIconMapZoom,
	kIconMapUnzoom,
	kIconMapLeft,
	kIconMapUp,
	kIconMapRight,
	kIconMapDown,
	kIconConradGun,
	kIconConradRun,
	kIconConradJump,
	kIconConradWalk,
	kIconConradStatic,
	kIconConradDuck,
	kIconConradLoad,
	kIconInventoryLeft = 45,
	kIconInventoryUp,
	kIconInventoryRight,
	kIconInventoryDown,
	kIconInventoryHand,
	kIconConradHand = 60,
	kIconConradHandUse,

	kIconTableSize
};

struct IconsInitTable {
	int num;
	int x, y;
	int action;
};

static const IconsInitTable _iconsGroupMoveTable[] = {
	{ kIconConradRun, 0, 0, kIconActionRun },
	{ kIconConradJump, 18, 0, kIconActionJump },
	{ kIconConradDuck, 36, 0, kIconActionDuck },
	{ kIconConradLoad, 54, 0, kIconActionLoad },
	{ kIconConradGun, 72, 0, kIconActionGun },
	{ kIconConradHand, 90, 10, kIconActionHand },
	{ kIconConradHandUse, 90, -10, kIconActionHandUse },
};

static const IconsInitTable _iconsGroupStepTable[] = {
	{ kIconTurn180, 0, 0, kIconActionDirTurn180 },
	{ kIconMapUp, 0, -12, kIconActionDirUp },
	{ kIconMapDown, 0, 12, kIconActionDirDown },
	{ kIconMapLeft, -12, 0, kIconActionDirLeft },
	{ kIconMapRight, 12, 0, kIconActionDirRight },
};

static const IconsInitTable _iconsGroupToolsTable[] = {
	{ kIconInventory, 0, -12, kIconActionInventory },
	{ kIconOptions, 0, 0, kIconActionOptions },
};

static const IconsInitTable _iconsGroupCabTable[] = {
	{ kIconInventoryHand, 0, 0, kIconActionHand },
	{ kIconInventoryRight, 12, 0, kIconActionDirRight },
	{ kIconInventoryLeft, -12, 0, kIconActionDirLeft },
};

static const int kOptions_x0 = 296;
static const int kOptions_y0 = 4;

static const int kDpad_x0 = 24;
static const int kDpad_y0 = 150;

static const int kButtons_x0 = 200;
static const int kButtons_y0 = 172;

static const IconsInitTable _iconsTouchTable[] = {
	{ kIconInventory, kOptions_x0,      kOptions_y0,      kIconActionInventory },
	{ kIconOptions,   kOptions_x0,      kOptions_y0 + 24, kIconActionOptions },
	{ kIconTurn180,   kDpad_x0,         kDpad_y0,         kIconActionDirTurn180 },
	{ kIconMapUp,     kDpad_x0,         kDpad_y0 - 24,    kIconActionDirUp },
	{ kIconMapDown,   kDpad_x0,         kDpad_y0 + 24,    kIconActionDirDown },
	{ kIconMapLeft,   kDpad_x0 - 24,    kDpad_y0,         kIconActionDirLeft },
	{ kIconMapRight , kDpad_x0 + 24,    kDpad_y0,         kIconActionDirRight },
	{ kIconJump,      kButtons_x0,      kButtons_y0,      kIconActionJump },
	{ kIconGun,       kButtons_x0 + 32, kButtons_y0,      kIconActionGun },
	{ kIconAction,    kButtons_x0 + 64, kButtons_y0,      kIconActionHandUse },
	{ kIconRun,       kButtons_x0 + 96, kButtons_y0,      kIconActionRun },
};

// return false if the icon should not be presented
static bool canUseIcon(int level, int action) {
	if (level == 12) {
		switch (action) {
		case kIconActionRun:
		case kIconActionJump:
		case kIconActionDuck:
		case kIconActionLoad:
		case kIconActionGun:
		case kIconActionHand:
		case kIconActionHandUse:
		case kIconActionDirTurn180:
			return false;
		}
	}
	return true;
}

static uint8_t *magnifyIcon2x(const uint8_t *data, int w, int h) {
	uint8_t *out = (uint8_t *)malloc(w * 2 * h * 2);
	if (out) {
		// simple point scaler
		int offset = 0;
		for (int y = 0; y < h; ++y) {
			const int offset2 = offset + 2 * w;
			for (int x = 0; x < w; ++x) {
				const uint8_t color = *data++;
				out[offset + 2 * x] = out[offset + 2 * x + 1] = color;
				out[offset2 + 2 * x] = out[offset2 + 2 * x + 1] = color;
			}
			offset = offset2 + 2 * w;
		}
	}
	return out;
}

void Game::loadIcon(int16_t key, int num, int x, int y, int action) {
	assert(_iconsCount < ARRAYSIZE(_iconsTable));
	Icon *icon = &_iconsTable[_iconsCount];

	const uint8_t *p_anifram = _res.getData(kResType_ANI, key, "ANIFRAM");
	assert(p_anifram);
	int16_t sprKey = READ_LE_UINT16(p_anifram);
	initSprite(kResType_SPR, sprKey, &icon->spr);
	icon->spr.data = _spriteCache.getData(sprKey, icon->spr.data);

	if (_params.touchMode) {
		icon->scaledSprData = icon->spr.data = magnifyIcon2x(icon->spr.data, icon->spr.w, icon->spr.h);
		icon->spr.w *= 2;
		icon->spr.h *= 2;
	}

	icon->x = x;
	icon->y = y;
	icon->xMargin = 2;
	icon->yMargin = 2;
	icon->action = action;
	++_iconsCount;
}

static void loadIconGroup(Game *g, int16_t *aniKeys, const IconsInitTable *icons, int iconsCount, int xPos, int yPos) {
	for (int i = 0; i < iconsCount; ++i) {
		const int16_t key = aniKeys[icons[i].num];
		if (key != 0 && canUseIcon(g->_level, icons[i].action)) {
			int x = icons[i].x + xPos;
			int y = icons[i].y + yPos;
			if (g->_params.touchMode) {
				switch (icons[i].num) {
				case kIconInventoryHand:
				case kIconInventoryRight:
				case kIconInventoryLeft:
					x = icons[i].x * 2 + xPos - 6;
					y = icons[i].y * 2 + yPos - 6;
					break;
				}
			}
			g->loadIcon(key, icons[i].num, x, y, icons[i].action);
		}
	}
}

void Game::initIcons(int iconMode) {
	_iconsCount = 0;
	GameObject *o = _objectsPtrTable[kObjPtrIconesSouris];
	if (o) {
		int16_t key = _res.getChild(kResType_ANI, o->anim.currentAnimKey);
		int16_t aniKeys[kIconTableSize];
		for (int i = 0; key != 0 && i < ARRAYSIZE(aniKeys); ++i) {
			aniKeys[i] = key;
			key = _res.getNext(kResType_ANI, key);
		}
		switch (iconMode) {
		case kIconModeCabinet:
			loadIconGroup(this, aniKeys, _iconsGroupCabTable, ARRAYSIZE(_iconsGroupCabTable), _res._userConfig.iconLrCabX, _res._userConfig.iconLrCabY);
			break;
		case kIconModeGame:
			if (_params.touchMode) {
				loadIconGroup(this, aniKeys, _iconsTouchTable, ARRAYSIZE(_iconsTouchTable), 0, 0);
				break;
			}
			loadIconGroup(this, aniKeys, _iconsGroupMoveTable, ARRAYSIZE(_iconsGroupMoveTable), _res._userConfig.iconLrMoveX, _res._userConfig.iconLrMoveY);
			loadIconGroup(this, aniKeys, _iconsGroupStepTable, ARRAYSIZE(_iconsGroupStepTable), _res._userConfig.iconLrStepX, _res._userConfig.iconLrStepY);
			loadIconGroup(this, aniKeys, _iconsGroupToolsTable, ARRAYSIZE(_iconsGroupToolsTable), _res._userConfig.iconLrToolsX, _res._userConfig.iconLrToolsY);
			break;
		}
	}
}

static bool iconPressed(const PlayerInput &inp, const Icon *icon) {
	for (int i = 0; i < kPlayerInputPointersCount; ++i) {
		if (inp.pointers[i][0].down && icon->isCursorOver(inp.pointers[i][0].x, inp.pointers[i][0].y)) {
			return true;
		}
	}
	return false;
}

void Game::drawIcons() {
	int transparentScale = 256;
	for (int i = 0; i < _iconsCount; ++i) {
		const Icon *icon = &_iconsTable[i];
		if (_params.touchMode) {
			transparentScale = iconPressed(inp, icon) ? 256 : 128;
		}
		_render->drawSprite(icon->x, icon->y, icon->spr.data, icon->spr.w, icon->spr.h, 0, icon->spr.key, transparentScale);
	}
}