#ifndef NBA_AUDIO_DEBUGGER_H
#define NBA_AUDIO_DEBUGGER_H

#include "nba_types.h"
#include "nba_assets.h"
#include "nba_renderer.h"

typedef struct {
    bool is_active;
    int selected_index;
    int scroll_offset;
    int total_audio_items;
    uint32_t audio_item_indices[NBA_ASSET_MAX];
} NbaAudioDebugger;

void nba_audio_debugger_init(NbaAudioDebugger *dbg);
void nba_audio_debugger_toggle(NbaAudioDebugger *dbg);
void nba_audio_debugger_update(NbaAudioDebugger *dbg, const NbaAssetPack *assets, const NbaInput *input);
void nba_audio_debugger_render(const NbaAudioDebugger *dbg, const NbaAssetPack *assets, NbaRenderer *ren);

#endif /* NBA_AUDIO_DEBUGGER_H */
