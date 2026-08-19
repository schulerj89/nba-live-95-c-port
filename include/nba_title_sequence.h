#ifndef NBA_TITLE_SEQUENCE_H
#define NBA_TITLE_SEQUENCE_H

#include "nba_types.h"
#include "nba_assets.h"
#include "nba_renderer.h"

#define SNES_ADDR_POST_EA_ASSET_SETUP  0x82AC0E
#define SNES_ADDR_TITLE_SCENE          0x80E01E
#define SNES_ADDR_ATTRACT_FRAME_UPDATE 0x8780CB
#define NBA_TITLE_SEQUENCE_FRAMES      1000
#define NBA_TITLE_FADE_FRAMES          15

typedef struct {
    bool audio_started;
} NbaTitleSequence;

void nba_title_sequence_init(NbaTitleSequence *sequence);
void nba_title_sequence_render(const NbaTitleSequence *sequence,
                               const NbaAssetPack *assets,
                               NbaRenderer *renderer,
                               float timer);

#endif /* NBA_TITLE_SEQUENCE_H */
