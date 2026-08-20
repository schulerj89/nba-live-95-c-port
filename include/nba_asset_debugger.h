#ifndef NBA_ASSET_DEBUGGER_H
#define NBA_ASSET_DEBUGGER_H

#include "nba_types.h"
#include "nba_assets.h"
#include "nba_renderer.h"

typedef struct {
    bool is_active;
    int selected_index;
    int tile_page;
} NbaAssetDebugger;

void nba_asset_debugger_init(NbaAssetDebugger *dbg);
void nba_asset_debugger_toggle(NbaAssetDebugger *dbg);
void nba_asset_debugger_update(NbaAssetDebugger *dbg,
                               const NbaAssetPack *assets,
                               const NbaInput *input);
void nba_asset_debugger_render(const NbaAssetDebugger *dbg,
                               const NbaAssetPack *assets,
                               NbaRenderer *ren);

#endif
