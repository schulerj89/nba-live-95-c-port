#ifndef NBA_INTRO_TEXT_H
#define NBA_INTRO_TEXT_H
#include "nba_assets.h"
#include "nba_renderer.h"

bool nba_intro_text_payload_valid(const uint8_t *data, size_t size);
bool nba_intro_text_render(const NbaAssetPack *assets, NbaRenderer *renderer,
                           bool legal, int brightness);
#endif
