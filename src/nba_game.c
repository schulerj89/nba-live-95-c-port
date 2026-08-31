#include "nba_game.h"
#include "nba_intro_text.h"
#include "nba_font.h"
#include "nba_audio.h"
#include "nba_snes_ppu.h"
#include <stdio.h>
#include <string.h>

#define NBA_DEBUG_MAX_LINES 8
#define NBA_DEBUG_LINE_SIZE 80

typedef struct {
    char line[NBA_DEBUG_MAX_LINES][NBA_DEBUG_LINE_SIZE];
    int count;
} NbaDebugLines;

static void nba_debug_add_line(NbaDebugLines *out, const char *text) {
    if (out->count >= NBA_DEBUG_MAX_LINES) return;
    snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE, "%s", text);
}

const char *nba_game_state_name(NbaGameState state) {
    static const char *const names[] = {
        "BOOT_RESET", "NINTENDO_LICENSE", "NBA_LEGAL", "EA_INTRO",
        "TITLE", "GAME_SETUP", "TEAM_SELECT", "PLAYER_SETUP", "PLAYER_INTRO",
        "TIPOFF", "POSTGAME"
    };
    return (unsigned)state < sizeof(names) / sizeof(names[0]) ?
           names[state] : "UNKNOWN";
}

static const char *nba_debug_setup_page_name(NbaSetupPage page) {
    static const char *const names[] = { "MAIN", "RULES", "OPTIONS" };
    return (unsigned)page < sizeof(names) / sizeof(names[0]) ? names[page] : "?";
}

static const char *nba_debug_setup_row_name(NbaSetupRow row) {
    static const char *const names[] = {
        "MODE", "STYLE", "LEVEL", "QUARTER", "RULES", "OPTIONS"
    };
    return (unsigned)row < sizeof(names) / sizeof(names[0]) ? names[row] : "?";
}

static const char *nba_debug_transition_name(NbaSetupTransition transition) {
    static const char *const names[] = { "NONE", "OPEN", "RETURN" };
    return (unsigned)transition < sizeof(names) / sizeof(names[0]) ?
           names[transition] : "?";
}

static const char *nba_debug_setup_action_name(NbaSetupAction action) {
    static const char *const names[] = { "NONE", "RULES", "OPTIONS", "MAIN", "MODE" };
    return (unsigned)action < sizeof(names) / sizeof(names[0]) ? names[action] : "?";
}

static const char *nba_debug_audio_name(uint8_t track) {
    static const char *const names[] = {
        "NONE", "WAV", "TITLE_SPC", "SETUP_SPC", "PLAYER_INTRO_SPC"
    };
    return track < sizeof(names) / sizeof(names[0]) ? names[track] : "?";
}

static const char *nba_debug_audio_status_name(uint8_t status) {
    static const char *const names[] = {
        "IDLE", "READY", "PLAYING", "HOST_FAIL", "SYNTH_FAIL"
    };
    return status < sizeof(names) / sizeof(names[0]) ? names[status] : "?";
}

static void nba_game_debug_lines(const NbaGame *game, NbaDebugLines *out) {
    static const char *const modes[] = { "EXHIB", "SEASON", "PLAYOFF", "LOAD" };
    static const char *const styles[] = { "ARCADE", "SIM", "CUSTOM" };
    static const char *const levels[] = { "ROOKIE", "STARTER", "ALLSTAR" };
    static const char *const quarters[] = { "3MIN", "5MIN", "8MIN", "12MIN" };
    const NbaAudio *audio = &game->audio;
    memset(out, 0, sizeof(*out));
    snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
             "DEBUG [F10] SCN:%s", nba_game_state_name(game->state));
    snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
             "GF:%06u SF:%05u T:%6.2f", game->frame_count,
             game->state_frame, game->state_timer);
    snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
             "IN P:%04X H:%04X R:%04X", game->input.pressed,
             game->input.held, game->input.released);

    if (game->state == NBA_STATE_GAME_SETUP && game->scene.setup.is_initialized) {
        const NbaSetupScreen *s = &game->scene.setup;
        uint16_t mode = s->config->main_values[0];
        uint16_t style = s->config->main_values[1];
        uint16_t level = s->config->main_values[2];
        uint16_t quarter = s->config->main_values[3];
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "PG:%s ROW:%s MR:%02d", nba_debug_setup_page_name(s->page),
                 nba_debug_setup_row_name(s->row), s->menu_row);
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "TR:%s TF:%03d BLK:%d ACT:%s",
                 nba_debug_transition_name(s->transition), s->transition_frame,
                 s->transition_blank ? 1 : 0,
                 nba_debug_setup_action_name(game->last_setup_action));
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "CFG M:%s S:%s L:%s Q:%s",
                 mode < 4u ? modes[mode] : "?", style < 3u ? styles[style] : "?",
                 level < 3u ? levels[level] : "?",
                 quarter < 4u ? quarters[quarter] : "?");
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "PPU B:%02d X1:%03d X2:%03d Y2:%03d Y3:%03d", s->brightness,
                 s->bg1_hscroll, s->bg2_hscroll, s->bg2_vscroll, s->bg3_vscroll);
    } else if (game->state == NBA_STATE_TITLE_SEQUENCE) {
        static const char *const phases[] = { "BUILD", "HOLD", "FADE" };
        unsigned phase = (unsigned)game->scene.title.phase;
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "TITLE PH:%s B:%02d HOLD:%03d SNAP:%d",
                 phase < 3u ? phases[phase] : "?", game->scene.title.brightness,
                 game->scene.title.hold_frames_left,
                 game->scene.title.snap_frame);
    } else if (game->state == NBA_STATE_TEAM_SELECT &&
               game->scene.team_select.is_initialized) {
        const NbaTeamSelect *s = &game->scene.team_select;
        static const char *const selector_names[] = {
            "LEFT NAME", "RIGHT NAME", "SCORING", "REBOUNDS",
            "BALL CONTROL", "DEFENSE", "OVERALL"
        };
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "SIDE:%s SEL:%u %-12s TF:%03d",
                 s->active_side == NBA_TEAM_SIDE_LEFT ? "LEFT" : "RIGHT",
                 (unsigned)s->selector,
                 s->selector < NBA_TEAM_SELECT_POSITION_COUNT ?
                     selector_names[s->selector] : "?",
                 s->transition_frame);
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "TEAM L:%02u %-12s", s->session->left_team,
                 nba_team_records[s->session->left_team].name);
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "TEAM R:%02u %-12s", s->session->right_team,
                 nba_team_records[s->session->right_team].name);
    } else if (game->state == NBA_STATE_PLAYER_SETUP &&
               game->scene.player_setup.is_initialized) {
        const NbaPlayerSetup *s = &game->scene.player_setup;
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "P1:%s TF:%03d STEADY:%04d",
                 s->player_one_side == NBA_TEAM_SIDE_LEFT ? "LEFT" : "RIGHT",
                 s->transition_frame, s->steady_frame);
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "TEAM L:%02u %-12s", s->session->left_team,
                 nba_team_records[s->session->left_team].name);
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "TEAM R:%02u %-12s", s->session->right_team,
                 nba_team_records[s->session->right_team].name);
    } else if (game->state == NBA_STATE_PLAYER_INTRO &&
               game->scene.player_intro.is_initialized) {
        const NbaPlayerIntro *s = &game->scene.player_intro;
        static const char *const phases[] = {
            "TRANSITION", "MATCHUP", "RATINGS", "LINEUPS", "COMPLETE"
        };
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "INTRO PH:%s PF:%04d CARD:%02d/10",
                 (unsigned)s->phase < 5u ? phases[s->phase] : "?",
                 s->phase_frame, s->lineup_card + 1);
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "ROM LOOP:$87:BE92 DEC:$80:BD1B PAL:$81:A1E7");
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "TEAM L:%02u R:%02u ASSET:COURT+PORTRAIT",
                 s->session->left_team, s->session->right_team);
    } else if (game->state == NBA_STATE_TIPOFF &&
               game->scene.tipoff.is_initialized) {
        const NbaTipoff *s = &game->scene.tipoff;
        static const char *const phases[] = {
            "FORMATION", "JUMP BALL", "POSSESSION", "LIVE"
        };
        static const char *const cpu_phases[] = {
            "BREAK", "DRIVE", "PASS", "ATTACK", "SHOT", "REBOUND"
        };
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "TIP PH:%s F:%03d HOLD:%d HIT:%u HOME:%02u",
                 (unsigned)s->phase < 4u ? phases[s->phase] : "?", s->frame,
                 (int16_t)s->tip_toss_countdown_raw_09f2, s->tip_contact_frame,
                 s->session->right_team);
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "CPU %s P:%u PF:%u H:%u R:%u RNG:$%04X INT:$85:963D",
                 s->cpu_play_state < 6u ? cpu_phases[s->cpu_play_state] : "?",
                 s->possession_number, s->possession_frame,
                 s->handler_actor, s->receiver_actor, s->rng.state);
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "BALL M:%u O:%d W:%d,%d,%d V:%d,%d,%d PLAY:$%02X/%d T:%d W:%u",
                 s->ball.state, s->ball.owner_actor,
                 (int)(s->ball.x_fp / 256), (int)(s->ball.y_fp / 256),
                 (int)(s->ball.z_fp / 256), s->ball.velocity_x / 256,
                 s->ball.velocity_y / 256, s->ball.velocity_z / 256,
                 s->play_code, s->play_step_raw, s->play_countdown_raw,
                 s->play_event_wait_raw);
        if (s->pass_active_raw && s->pass_actor_raw >= 0 &&
            s->pass_actor_raw < NBA_GAMEPLAY_ACTOR_COUNT) {
            const NbaTipoffActor *passer = &s->actors[s->pass_actor_raw];
            snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                     "PASS $0942:%d $0946:%d D:%u B:%u DIR:%u F:%d PH:%u/%u R:%u",
                     s->pass_actor_raw, s->pass_receiver_raw,
                     s->pass_distance_raw, passer->pass_band_raw,
                     passer->pass_direction_raw, passer->pass_family_raw,
                     passer->upper_animation_phase_raw,
                     passer->pass_release_threshold_raw,
                     passer->pass_released_raw ? 1u : 0u);
        }
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "SCORE %u-%u SC:$%04X VAL:%u CH:%u MISS:%u/%u IN:%u/%u T:%u A2:%02X ACT:%u RC:$%04X EV:$%04X",
                 s->session->score[0], s->session->score[1],
                 s->rim_raw_092c,
                 s->shot_value_raw, s->shot_chance_raw,
                 s->shot_inner_veto_raw ? 1u : 0u, s->shot_miss_index_raw,
                 s->inbound_state_raw, s->inbound_actor_raw,
                 s->inbound_timer_raw, s->special_actor_raw & 0xFFu,
                  s->ball_activity_raw, s->rim_raw_097c, s->rim_raw_13e7);
        if (s->live_state_raw == 0x82u &&
            out->count < NBA_DEBUG_MAX_LINES)
            snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                     "INBOUND L:%d TARGET:%d,%d D:%u READY:%u TIMER:%u",
                     s->inbound_layout_raw, s->inbound_target_x_raw,
                     s->inbound_target_y_raw, s->inbound_direction_raw,
                     s->inbound_ready_raw, s->inbound_timer_raw);
    }

    if (out->count < NBA_DEBUG_MAX_LINES) {
        char srcn[4] = "--";
        if (audio->last_setup_sfx_srcn != 0xFFu)
            snprintf(srcn, sizeof(srcn), "%02X", audio->last_setup_sfx_srcn);
        snprintf(out->line[out->count++], NBA_DEBUG_LINE_SIZE,
                 "AUD:%s ST:%s MV:%02u SV:%02u SRC:%s",
                 nba_debug_audio_name(audio->active_track),
                 nba_debug_audio_status_name(audio->status), audio->setup_music_volume,
                 audio->setup_sfx_volume, srcn);
    }
}

static void nba_game_debug_hud_lines(const NbaGame *game, uint8_t page,
                                     NbaDebugLines *out) {
    NbaDebugLines full;
    nba_game_debug_lines(game, &full);
    memset(out, 0, sizeof(*out));
    if (page == 1u) {
        char title[NBA_DEBUG_LINE_SIZE];
        char audio_line[NBA_DEBUG_LINE_SIZE];
        char srcn[4] = "--";
        snprintf(title, sizeof(title), "DEBUG 1/2 [F10] %s",
                 nba_game_state_name(game->state));
        nba_debug_add_line(out, title);
        if (full.count > 1) nba_debug_add_line(out, full.line[1]);
        if (full.count > 2) nba_debug_add_line(out, full.line[2]);
        if (game->audio.last_setup_sfx_srcn != 0xFFu)
            snprintf(srcn, sizeof(srcn), "%02X", game->audio.last_setup_sfx_srcn);
        snprintf(audio_line, sizeof(audio_line), "AUD:%s %s MV:%02u SV:%02u FX:%s",
                 game->audio.active_track == NBA_AUDIO_TRACK_SETUP_SPC ? "SETUP" :
                 game->audio.active_track == NBA_AUDIO_TRACK_TITLE_SPC ? "TITLE" :
                 nba_debug_audio_name(game->audio.active_track),
                 nba_debug_audio_status_name(game->audio.status),
                 game->audio.setup_music_volume, game->audio.setup_sfx_volume, srcn);
        nba_debug_add_line(out, audio_line);
        return;
    }

    {
        char title[NBA_DEBUG_LINE_SIZE];
        snprintf(title, sizeof(title), "DEBUG 2/2 [F10] %s",
                 game->state == NBA_STATE_GAME_SETUP ? "SETUP" :
                 nba_game_state_name(game->state));
        nba_debug_add_line(out, title);
    }
    if (game->state == NBA_STATE_GAME_SETUP && full.count >= 7) {
        nba_debug_add_line(out, full.line[3]);
        nba_debug_add_line(out, full.line[4]);
        nba_debug_add_line(out, full.line[5]);
        {
            const NbaSetupScreen *s = &game->scene.setup;
            char ppu[NBA_DEBUG_LINE_SIZE];
            snprintf(ppu, sizeof(ppu), "PPU B:%d X1:%d X2:%d Y2:%d Y3:%d",
                     s->brightness, s->bg1_hscroll, s->bg2_hscroll,
                     s->bg2_vscroll, s->bg3_vscroll);
            nba_debug_add_line(out, ppu);
        }
    } else if (full.count > 3) {
        for (int index = 3; index < full.count; ++index)
            nba_debug_add_line(out, full.line[index]);
    }
}

void nba_game_debug_print(const NbaGame *game) {
    if (!game) return;
    NbaDebugLines lines;
    nba_game_debug_lines(game, &lines);
    for (int index = 0; index < lines.count; ++index)
        printf("[DEBUG STATE] %s\n", lines.line[index]);
}

bool nba_game_enter_state(NbaGame *game, NbaGameState state) {
    if (!game) return false;
    NbaGameState previous_state = game->state;
    bool keep_setup_audio =
        (previous_state == NBA_STATE_GAME_SETUP && state == NBA_STATE_TEAM_SELECT) ||
        (previous_state == NBA_STATE_TEAM_SELECT && state == NBA_STATE_PLAYER_SETUP);
    if (previous_state == NBA_STATE_TEAM_SELECT)
        nba_team_select_shutdown(&game->scene.team_select);
    if (previous_state == NBA_STATE_PLAYER_SETUP)
        nba_player_setup_shutdown(&game->scene.player_setup);
    if (previous_state == NBA_STATE_PLAYER_INTRO)
        nba_player_intro_shutdown(&game->scene.player_intro);
    if (!keep_setup_audio && game->audio.active_track != NBA_AUDIO_TRACK_NONE)
        nba_audio_stop(&game->audio);
    memset(&game->scene, 0, sizeof(game->scene));
    game->state = state;
    game->state_frame = 0;
    game->state_timer = 0.0f;
    game->ea_intro_audio_started = false;
    if (state != NBA_STATE_TEAM_SELECT)
        game->last_setup_action = NBA_SETUP_ACTION_NONE;

    if (state == NBA_STATE_TITLE_SEQUENCE) {
        nba_title_sequence_init(&game->scene.title);
        game->scene.title.audio_started =
            nba_audio_play_title_spc(&game->audio, &game->assets);
        if (!game->scene.title.audio_started) {
            game->audio.status = NBA_AUDIO_STATUS_SYNTH_FAILED;
            fprintf(stderr, "[AUDIO] Title synthesis failed; continuing silently.\n");
            return false;
        }
    } else if (state == NBA_STATE_GAME_SETUP) {
        nba_setup_screen_init(&game->scene.setup, &game->assets,
                              &game->session.config);
        if (!game->scene.setup.is_initialized || !game->scene.setup.has_gfx) {
            fprintf(stderr, "[GAME] Game Setup graphics initialization failed.\n");
            return false;
        }
        if (!nba_audio_play_setup_dsp(&game->audio, &game->assets)) {
            game->audio.status = NBA_AUDIO_STATUS_SYNTH_FAILED;
            fprintf(stderr, "[AUDIO] Game Setup synthesis failed; continuing silently.\n");
            return false;
        }
    } else if (state == NBA_STATE_TEAM_SELECT) {
        if (!nba_team_select_init(&game->scene.team_select, &game->assets,
                                  &game->session)) {
            fprintf(stderr, "[GAME] Team Select asset initialization failed.\n");
            return false;
        }
        if (previous_state == NBA_STATE_GAME_SETUP)
            game->scene.team_select.transition_frame = NBA_TEAM_SCENE_ENTRY_FRAME;
    } else if (state == NBA_STATE_PLAYER_SETUP) {
        if (!nba_player_setup_init(&game->scene.player_setup, &game->assets,
                                   &game->session, game->renderer.pixels)) {
            fprintf(stderr, "[GAME] Player Setup asset initialization failed.\n");
            return false;
        }
    } else if (state == NBA_STATE_PLAYER_INTRO) {
        if (!nba_player_intro_init(&game->scene.player_intro, &game->assets,
                                   &game->session, game->renderer.pixels)) {
            fprintf(stderr, "[GAME] Player Introduction asset initialization failed.\n");
            return false;
        }
        if (!nba_audio_play_player_intro_dsp(&game->audio, &game->assets)) {
            game->audio.status = NBA_AUDIO_STATUS_SYNTH_FAILED;
            fprintf(stderr, "[AUDIO] Player Introduction synthesis failed; "
                            "continuing silently.\n");
            return false;
        }
    } else if (state == NBA_STATE_TIPOFF) {
        if (!nba_tipoff_init(&game->scene.tipoff, &game->assets, &game->session)) {
            fprintf(stderr, "[GAME] Tip-off asset initialization failed.\n");
            return false;
        }
        if (!nba_audio_start_gameplay(&game->audio, &game->assets)) {
            fprintf(stderr, "[GAMEPLAY AUDIO] ROM gameplay bank failed to start; "
                            "continuing silently.\n");
        }
    }
    return true;
}

static int nba_game_license_brightness(uint32_t state_frame) {
    int fade_frame = (int)state_frame - NBA_LICENSE_FRAMES;
    return fade_frame <= 0 ? 15 : 15 - fade_frame;
}

static int nba_game_legal_brightness(uint32_t state_frame) {
    int frame = (int)state_frame;
    int fade_out_start = NBA_SCREEN_FADE_FRAMES + NBA_LEGAL_FRAMES;
    if (frame < NBA_SCREEN_FADE_FRAMES) return frame;
    if (frame <= fade_out_start) return 15;
    return 15 - (frame - fade_out_start);
}

/**
 * Offset/Address/Size: 0x000020 | $80:8020 | size: 0x80
 * Purpose: Game engine cold-boot initialization (loads ROM, assets, video/audio subsystems, enters initial state).
 */
bool nba_game_init(NbaGame *game, const char *rom_path, const char *assets_path) {
    if (!game) return false;
    memset(game, 0, sizeof(NbaGame));

    printf("[GAME] Initializing NBA Live '95 Native C Port...\n");

    nba_renderer_init(&game->renderer);
    nba_font_init();
    nba_audio_init(&game->audio);
    nba_session_init(&game->session);

    /* Load asset pack if provided via parameter */
    if (assets_path && assets_path[0] != '\0' &&
        !nba_assets_load(&game->assets, assets_path)) {
        nba_audio_shutdown(&game->audio);
        return false;
    }

    if (!nba_assets_get(&game->assets, NBA_ASSET_EA_INDEXED) ||
        !nba_assets_get(&game->assets, NBA_ASSET_INTRO_TEXT)) {
        fprintf(stderr, "[GAME] Asset pack lacks indexed intro resources; rebuild it with tools/extract_assets.py.\n");
        nba_assets_free(&game->assets);
        nba_audio_shutdown(&game->audio);
        return false;
    }

    if (rom_path && rom_path[0] != '\0') {
        if (!nba_rom_load_file(&game->rom, rom_path)) {
            nba_assets_free(&game->assets);
            nba_audio_shutdown(&game->audio);
            return false;
        }
    }

    /* Setup initial game state */
    nba_game_enter_state(game, NBA_STATE_NINTENDO_LICENSE);
    game->frame_count = 0;
    nba_audio_debugger_init(&game->audio_debugger);
    nba_asset_debugger_init(&game->asset_debugger);
    nba_player_lab_init(&game->player_lab, &game->assets);
    nba_gameplay_debugger_init(&game->gameplay_debugger);
    game->is_initialized = true;

    printf("[GAME] Initialization complete. Entering state NBA_STATE_NINTENDO_LICENSE.\n");
    return true;
}

/**
 * Offset/Address/Size: N/A | Host Game Shutdown | size: N/A
 * Purpose: Gracefully releases audio, asset pack, and ROM cartridge buffers.
 */
void nba_game_shutdown(NbaGame *game) {
    if (!game) return;
    if (game->state == NBA_STATE_TEAM_SELECT)
        nba_team_select_shutdown(&game->scene.team_select);
    if (game->state == NBA_STATE_PLAYER_SETUP)
        nba_player_setup_shutdown(&game->scene.player_setup);
    if (game->state == NBA_STATE_PLAYER_INTRO)
        nba_player_intro_shutdown(&game->scene.player_intro);
    nba_snes_mode1_release(&game->renderer);
    nba_audio_shutdown(&game->audio);
    if (game->assets.is_loaded) {
        nba_assets_free(&game->assets);
    }
    if (game->rom.is_loaded) {
        nba_rom_free(&game->rom);
    }
    game->is_initialized = false;
    printf("[GAME] Shutdown complete.\n");
}

/* Captured-bank closure: `$00:8000-$00:FFFF` and `$80:8000-$80:FFFF`.
 * Only retained executed positions within those address windows are claimed.
 * `$00:8156` jumps into the Bank $80 bootstrap and `$00:8600-$861C` is the
 * native IRQ save/indirect-dispatch/restore trampoline. The host process and
 * fixed-step loop replace that CPU ABI boundary; gameplay100_closure_probe
 * exercises its first gameplay-visible publication here on every run.
 *
 * `$80:CE33-$CEFD`: the native routine serially reads both controller ports,
 * rejects the disconnected $FFFF signature and publishes current/edge state.
 * Win32 has already performed the serial read, so this is its exact portable
 * state-publication tail. Debug keys occupy host-only bits above the twelve
 * SNES buttons and intentionally use the same edge contract. */
void nba_game_input_update(NbaInput *input, uint32_t raw_buttons) {
    if (!input) return;
    input->pressed = raw_buttons & ~input->held;
    input->released = ~raw_buttons & input->held;
    input->held = raw_buttons;
}

/**
 * Offset/Address/Size: 0x005A91 | $80:DA91 | size: 0xC8
 * `$80:CF00-$F100` is the surrounding scene/resource service family. The
 * portable dispatcher adopts its gameplay-visible contract: fixed frame
 * waits, INIDISP fades, trace-backed PPU publication, resource handoffs and
 * forced-blank boundaries. It deliberately does not emulate DMA latency.
 * Purpose: Main game loop dispatcher and scene timer state machine.
 */
void nba_game_tick(NbaGame *game, float delta_time) {
    if (game->input.pressed & NBA_BTN_DEBUG_F8) {
        if (game->state == NBA_STATE_TIPOFF) {
            nba_gameplay_debugger_toggle(&game->gameplay_debugger);
            if (game->gameplay_debugger.is_active) {
                game->audio_debugger.is_active = false;
                game->asset_debugger.is_active = false;
                game->player_lab.is_active = false;
                game->debug_hud_page = 0;
            }
        } else {
            printf("[GAMEPLAY LAB] F8 is available after gameplay begins.\n");
        }
    }

    if (game->input.pressed & NBA_BTN_DEBUG_F9) {
        nba_player_lab_toggle(&game->player_lab, &game->assets);
        if (game->player_lab.is_active) {
            game->gameplay_debugger.is_active = false;
            game->audio_debugger.is_active = false;
            game->asset_debugger.is_active = false;
            game->debug_hud_page = 0;
        }
    }
    /* Handle F10 Timing Debug overlay toggle */
    if (game->input.pressed & NBA_BTN_DEBUG_F10) {
        game->debug_hud_page = (uint8_t)((game->debug_hud_page + 1u) % 3u);
        printf("[DEBUG] State HUD %s\n", game->debug_hud_page == 0u ? "OFF" :
               game->debug_hud_page == 1u ? "OVERVIEW (1/2)" : "SCENE (2/2)");
    }

    /* Handle F11 Audio Debugger toggle */
    if (game->input.pressed & NBA_BTN_DEBUG_F11) {
        nba_audio_debugger_toggle(&game->audio_debugger);
    }
    if (game->input.pressed & NBA_BTN_DEBUG_F12) {
        nba_asset_debugger_toggle(&game->asset_debugger);
    }

    /* Update audio debugger navigation / playback */
    nba_audio_debugger_update(&game->audio_debugger, &game->audio,
                              &game->assets, &game->input);
    nba_asset_debugger_update(&game->asset_debugger, &game->assets, &game->input);
    nba_player_lab_update(&game->player_lab, &game->assets, &game->input);
    if (game->state == NBA_STATE_TIPOFF) {
        nba_tipoff_capture_telemetry(&game->scene.tipoff, &game->input,
                                     &game->gameplay_telemetry);
        game->gameplay_telemetry.global_frame = game->frame_count;
        nba_gameplay_debugger_update(&game->gameplay_debugger, &game->input);
    }

    /* If audio debugger is active, freeze game state progression */
    if (game->audio_debugger.is_active || game->asset_debugger.is_active ||
        game->player_lab.is_active) {
        return;
    }
    if (game->state == NBA_STATE_TIPOFF &&
        !nba_gameplay_debugger_should_advance(&game->gameplay_debugger)) return;

    game->state_timer += delta_time;
    game->state_frame++;
    game->frame_count++;

    switch (game->state) {
        case NBA_STATE_BOOT_RESET:
            if (game->state_frame >= 6) {
                nba_game_enter_state(game, NBA_STATE_NINTENDO_LICENSE);
            }
            break;

        case NBA_STATE_NINTENDO_LICENSE:
            /* $80:FD9E holds for $78 frames, then $80:CF1B fades brightness to zero. */
            if (game->input.pressed & (NBA_BTN_START | NBA_BTN_A | NBA_BTN_B)) {
                nba_game_enter_state(game, NBA_STATE_NBA_LEGAL_NOTICE);
            } else if (game->state_frame >=
                       NBA_LICENSE_FRAMES + NBA_SCREEN_FADE_FRAMES) {
                nba_game_enter_state(game, NBA_STATE_NBA_LEGAL_NOTICE);
            }
            break;

        case NBA_STATE_NBA_LEGAL_NOTICE:
            /* $80:CF3B fades in, $80:FEE6 holds for $B4 frames, then $80:CF1B fades out. */
            if ((game->input.pressed & (NBA_BTN_START | NBA_BTN_A | NBA_BTN_B)) ||
                game->state_frame >=
                    NBA_SCREEN_FADE_FRAMES + NBA_LEGAL_FRAMES + NBA_SCREEN_FADE_FRAMES) {
                nba_game_enter_state(game, NBA_STATE_EA_INTRO);
            }
            break;

        case NBA_STATE_EA_INTRO:
            /* Trigger EA intro voice/audio if present in asset pack and not yet played */
            if (!game->ea_intro_audio_started) {
                const NbaAssetItem *audio_item = nba_assets_get(&game->assets, NBA_ASSET_AUDIO_EA_INTRO);
                if (audio_item && audio_item->data && audio_item->size > 0) {
                    printf("[AUDIO] Playing EA Sports intro voice sequence (%u bytes, 5.05s total)...\n", audio_item->size);
                    nba_audio_play_wav(&game->audio, audio_item->data,
                                       (size_t)audio_item->size);
                    game->ea_intro_audio_started = true;
                }
            }

            /* Legacy dispatcher duration/input remains pending native hold,
             * audio and resource-handoff translation; renderer parity below
             * does not validate this timing contract. */
            if (game->input.pressed & (NBA_BTN_START | NBA_BTN_A | NBA_BTN_B)) {
                nba_game_enter_state(game, NBA_STATE_TITLE_SEQUENCE);
            } else if (game->state_frame >= NBA_INTRO_TOTAL_FRAMES) {
                nba_game_enter_state(game, NBA_STATE_TITLE_SEQUENCE);
            }
            break;

        case NBA_STATE_TITLE_SEQUENCE:
            /* $80:E01E enters the NBA shield/title scene immediately after $82:AC0E. */
            /* $80:E5C7 - dismissing the title. Bit 7 of $0A4C selects the path:
             * if the build is still running the ROM snaps it complete via
             * $80:F07E and holds 120 frames ($80:E5D3 #$0078); if it had already
             * finished it holds 40 ($80:E5D9 #$0028). Pressing again during the
             * hold does nothing - the count is fixed. */
            if (game->scene.title.phase == NBA_TITLE_PHASE_BUILD) {
                int title_frame = (int)game->state_frame;
                bool build_complete = title_frame >= NBA_TITLE_BUILD_COMPLETE_FRAMES;
                bool dismissed =
                    (game->input.pressed & (NBA_BTN_START | NBA_BTN_A | NBA_BTN_B)) != 0;

                if (dismissed || title_frame >= NBA_TITLE_SEQUENCE_FRAMES) {
                    if (!build_complete) {
                        nba_title_sequence_snap_complete(&game->scene.title);
                        /* $80:E5F9 DEC A / BPL runs the wait count+1 times. */
                        game->scene.title.hold_frames_left = NBA_TITLE_SNAP_HOLD_FRAMES + 1;
                        printf("[TITLE] Start during build: snapped complete ($80:F07E), "
                               "holding %d frames.\n", NBA_TITLE_SNAP_HOLD_FRAMES);
                    } else {
                        /* $80:E5F9 DEC A / BPL runs the wait count+1 times. */
                        game->scene.title.hold_frames_left = NBA_TITLE_COMPLETE_HOLD_FRAMES + 1;
                        printf("[TITLE] Start after build: holding %d frames.\n",
                               NBA_TITLE_COMPLETE_HOLD_FRAMES);
                    }
                    game->scene.title.phase = NBA_TITLE_PHASE_HOLD;
                }
            } else if (nba_title_sequence_advance(&game->scene.title)) {
                /* $80:CF1B finished ramping INIDISP to zero. */
                nba_game_enter_state(game, NBA_STATE_GAME_SETUP);
            }
            break;

        case NBA_STATE_GAME_SETUP:
            /* $80:A3B8 - per-frame Game Setup update: slide-in, backdrop
             * scroll and row cursor. $80:A9E3/$80:AA7B/$80:AACD feed the
             * cycle-timed SPC command path started at the title handoff. */
            {
                NbaSetupPage page_before = game->scene.setup.page;
                NbaSetupUpdateResult update =
                    nba_setup_screen_update(&game->scene.setup, &game->input);
                NbaSetupSound sound = update.sound;
                if (update.action != NBA_SETUP_ACTION_NONE)
                    game->last_setup_action = update.action;
                uint8_t srcn = 0xFFu;
                if (sound == NBA_SETUP_SOUND_ADJUST) srcn = 0x1Au;
                if (sound == NBA_SETUP_SOUND_MOVE) srcn = 0x1Bu;
                if (sound == NBA_SETUP_SOUND_CONFIRM) srcn = 0x1Cu;
                /* $82:8DDC calls $87:8C2D for the Options slider rows while the
                 * working value still lives at $7E:16FB.  Keep the independent
                 * waveOut music stream in sync without disturbing menu SFX. */
                if (page_before == NBA_SETUP_PAGE_OPTIONS &&
                    sound == NBA_SETUP_SOUND_ADJUST) {
                    if (game->scene.setup.menu_row == 0) {
                        nba_audio_set_setup_music_volume(
                            &game->audio, game->scene.setup.working_options[0], 45u);
                    } else if (game->scene.setup.menu_row == 1) {
                        nba_audio_set_setup_sfx_volume(
                            &game->audio, game->scene.setup.working_options[1], 45u);
                    }
                }
                if (srcn != 0xFFu) {
                    nba_audio_play_setup_sfx(&game->audio, &game->assets, srcn);
                }
                if (update.action == NBA_SETUP_ACTION_CONFIRM_MODE) {
                    static const char *const routes[] = {
                        "TEAM_SELECTION", "SEASON", "PLAYOFFS", "LOAD_SERIES"
                    };
                    uint16_t mode = game->session.config.main_values[0];
                    printf("[SETUP] Mode confirmed: mode=%u route=%s style=%u "
                           "level=%u quarter=%u.\n", mode,
                           mode < 4u ? routes[mode] : "UNKNOWN",
                           game->session.config.main_values[1],
                           game->session.config.main_values[2],
                           game->session.config.main_values[3]);
                    if (mode == 0u) {
                        /* A confirmed Exhibition starts a new match; period
                         * restarts enter Tipoff without clearing the session. */
                        nba_session_begin_match(&game->session);
                        if (!nba_game_enter_state(game, NBA_STATE_TEAM_SELECT))
                            fprintf(stderr, "[GAME] Could not enter Team Select.\n");
                    }
                }
            }
            break;

        case NBA_STATE_TEAM_SELECT:
            {
                NbaTeamSelectSound sound = nba_team_select_update(
                    &game->scene.team_select, &game->input);
                uint8_t srcn = 0xFFu;
                if (sound == NBA_TEAM_SOUND_SIDE) srcn = 0x19u;
                if (sound == NBA_TEAM_SOUND_CATEGORY) srcn = 0x1Bu;
                if (sound == NBA_TEAM_SOUND_CHANGE) srcn = 0x1Au;
                if (sound == NBA_TEAM_SOUND_CONFIRM) srcn = 0x1Cu;
                if (srcn != 0xFFu)
                    nba_audio_play_setup_sfx(&game->audio, &game->assets, srcn);
                if (game->scene.team_select.confirm_requested &&
                    !nba_game_enter_state(game, NBA_STATE_PLAYER_SETUP))
                    fprintf(stderr, "[GAME] Could not enter Player Setup.\n");
            }
            break;

        case NBA_STATE_PLAYER_SETUP:
            {
                NbaPlayerSetupSound sound = nba_player_setup_update(
                    &game->scene.player_setup, &game->input);
                uint8_t srcn = 0xFFu;
                if (sound == NBA_PLAYER_SETUP_SOUND_MOVE) srcn = 0x1Au;
                if (sound == NBA_PLAYER_SETUP_SOUND_CONFIRM) srcn = 0x1Cu;
                if (srcn != 0xFFu)
                    nba_audio_play_setup_sfx(&game->audio, &game->assets, srcn);
                if (game->scene.player_setup.confirm_requested &&
                    !nba_game_enter_state(game, NBA_STATE_PLAYER_INTRO))
                    fprintf(stderr, "[GAME] Could not enter Player Introduction.\n");
            }
            break;

        case NBA_STATE_PLAYER_INTRO:
            nba_player_intro_update(&game->scene.player_intro, &game->input);
            if (game->scene.player_intro.phase == NBA_PLAYER_INTRO_COMPLETE &&
                !nba_game_enter_state(game, NBA_STATE_TIPOFF))
                fprintf(stderr, "[GAME] Could not enter Tip-off.\n");
            break;

        case NBA_STATE_TIPOFF:
            {
                nba_tipoff_update(&game->scene.tipoff, &game->input);
                nba_audio_dispatch_gameplay_events(
                    &game->audio, game->scene.tipoff.rim_raw_13e7,
                    game->scene.tipoff.tip_event_bits_raw_13e9);
                nba_tipoff_capture_telemetry(&game->scene.tipoff, &game->input,
                                             &game->gameplay_telemetry);
                game->gameplay_telemetry.global_frame = game->frame_count;
                if (game->session.match.flow_state ==
                        NBA_MATCH_FLOW_POSTGAME_PRESENTATION_PENDING &&
                    !nba_game_enter_state(game, NBA_STATE_POSTGAME))
                    fprintf(stderr, "[GAME] Could not enter postgame.\n");
            }
            break;

        case NBA_STATE_POSTGAME:
            /* Exhibition `$87:97A0-$985C` returns after the captured final
             * summary/record-update pass. The host routes that boundary back
             * to Game Setup; the native callee beyond `$87:985C` is not yet
             * instruction-captured, so do not infer Season persistence here. */
            if (game->session.match.presentation_ticks_remaining > 0u)
                --game->session.match.presentation_ticks_remaining;
            if ((game->input.pressed & (NBA_BTN_START | NBA_BTN_A)) != 0u)
                game->session.match.presentation_ticks_remaining = 0u;
            if (game->session.match.presentation_ticks_remaining == 0u) {
                game->session.match.flow_state = NBA_MATCH_FLOW_FINAL;
                if (!nba_game_enter_state(game, NBA_STATE_GAME_SETUP))
                    fprintf(stderr, "[GAME] Could not return to Game Setup.\n");
            }
            break;

        default:
            break;
    }
}

/**
 * Offset/Address/Size: 0x007EE6 | $80:FEE6 | size: N/A (timing routine)
 * Purpose: Renders original packed font/strings/palette through $81:9FDF/$A163.
 */
void nba_game_render_nba_legal_notice(NbaGame *game) {
    if (game) nba_intro_text_render(&game->assets, &game->renderer, true,
                                    nba_game_legal_brightness(game->state_frame));
}

/**
 * Offset/Address/Size: 0x01715C | $82:F15C | size: 0x4E0 (1248 bytes)
 * Subroutines: $82:94DF (Mode 7 scale init), $82:962D (Matrix A/D update), $82:F56D (22-frame zoom loop), $82:F4C4 (8-frame flash)
 * Purpose: Multi-stage EA Sports intro animation wrapper delegating to dedicated nba_ea_intro module.
 */
void nba_game_render_ea_intro(NbaGame *game) {
    if (!game) return;
    nba_ea_intro_render(&game->assets, &game->renderer, game->state_frame);
}

/**
 * Offset/Address/Size: 0x005A91 | $80:DA91 | size: 0x140
 * Purpose: Top-level scene rendering multiplexer and debug overlay compositor.
 */
void nba_game_render(NbaGame *game) {
    if (!game || !game->is_initialized) return;

    NbaRenderer *ren = &game->renderer;

    /* Palette colors */
    uint32_t col_black      = 0xFF000000;
    uint32_t col_white      = 0xFFFFFFFF;
    uint32_t col_shadow     = 0xFF202020;

    switch (game->state) {
        case NBA_STATE_BOOT_RESET:
            nba_renderer_clear(ren, col_black);
            break;

        case NBA_STATE_NINTENDO_LICENSE:
            nba_intro_text_render(&game->assets, ren, false,
                                  nba_game_license_brightness(game->state_frame));
            break;

        case NBA_STATE_NBA_LEGAL_NOTICE:
            nba_game_render_nba_legal_notice(game);
            break;

        case NBA_STATE_EA_INTRO:
            nba_game_render_ea_intro(game);
            break;

        case NBA_STATE_TITLE_SEQUENCE:
            nba_title_sequence_render(&game->scene.title, &game->assets, ren,
                                      (int)game->state_frame);
            break;

        case NBA_STATE_GAME_SETUP:
            nba_setup_screen_render(&game->scene.setup, ren);
            break;

        case NBA_STATE_TEAM_SELECT:
            nba_team_select_render(&game->scene.team_select, ren);
            break;

        case NBA_STATE_PLAYER_SETUP:
            nba_player_setup_render(&game->scene.player_setup, ren);
            break;

        case NBA_STATE_PLAYER_INTRO:
            nba_player_intro_render(&game->scene.player_intro, ren);
            break;

        case NBA_STATE_TIPOFF:
            nba_tipoff_render(&game->scene.tipoff, ren);
            break;

        case NBA_STATE_POSTGAME: {
            /* `$83:FA91`: final summary renders the right score at x=52 and
             * left score at x=204, both y=100, before Exhibition returns. */
            nba_renderer_clear(ren, 0xFF080C18u);
            nba_renderer_draw_rect(ren, 16, 24, 224, 176, 0xFF14243Cu);
            nba_renderer_draw_rect(ren, 16, 24, 224, 3, 0xFFFFD760u);
            nba_font_render_text_centered(ren->pixels, NBA_SNES_WIDTH, 42,
                "FINAL", 0xFFFFD760u, 0xFF14243Cu, 2);
            char left[16], right[16];
            snprintf(left, sizeof(left), "%u", game->session.score[1]);
            snprintf(right, sizeof(right), "%u", game->session.score[0]);
            nba_font_render_text_centered(ren->pixels, NBA_SNES_WIDTH, 84,
                "HOME        VISITOR", 0xFFFFFFFFu, 0xFF14243Cu, 1);
            nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 48, 100, left,
                0xFFFFFFFFu, 0xFF14243Cu, 2);
            nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, 196, 100, right,
                0xFFFFFFFFu, 0xFF14243Cu, 2);
            nba_font_render_text_centered(ren->pixels, NBA_SNES_WIDTH, 170,
                "START  GAME SETUP", 0xFF9EF7A9u, 0xFF14243Cu, 1);
            break;
        }
    }

    /* Render live state Debug Overlay [F10] if enabled. */
    if (game->debug_hud_page != 0u && !game->audio_debugger.is_active) {
        uint32_t hud_bg = 0xCC001020;
        uint32_t hud_border = 0xFF4A90E2;
        uint32_t col_cyan = 0xFF40C0FF;
        uint32_t col_yellow = 0xFFFFD700;
        NbaDebugLines lines;
        nba_game_debug_hud_lines(game, game->debug_hud_page, &lines);
        int bx = 4, by = 4, bw = 248, bh = 8 + lines.count * 10;
        for (int y = by; y < by + bh; y++) {
            for (int x = bx; x < bx + bw; x++) {
                if (x == bx || x == bx + bw - 1 || y == by || y == by + bh - 1) {
                    ren->pixels[y * NBA_SNES_WIDTH + x] = hud_border;
                } else {
                    ren->pixels[y * NBA_SNES_WIDTH + x] = hud_bg;
                }
            }
        }

        for (int index = 0; index < lines.count; ++index) {
            uint32_t color = index == 0 ? col_yellow :
                             index == lines.count - 1 ? col_cyan : col_white;
            nba_font_render_text(ren->pixels, NBA_SNES_WIDTH, bx + 6,
                                 by + 4 + index * 10, lines.line[index],
                                 color, col_shadow, 1);
        }
    }

    if (game->state == NBA_STATE_TIPOFF) {
        nba_gameplay_debugger_render(&game->gameplay_debugger,
                                     &game->gameplay_telemetry, ren);
    }

    /* Render Audio Debugger Overlay on top of any game screen if active */
    if (game->audio_debugger.is_active) {
        nba_audio_debugger_render(&game->audio_debugger, &game->assets, ren);
    }
    if (game->asset_debugger.is_active) {
        nba_asset_debugger_render(&game->asset_debugger, &game->assets, ren);
    }
    if (game->player_lab.is_active) {
        nba_player_lab_render(&game->player_lab, &game->assets, ren);
    }
}
