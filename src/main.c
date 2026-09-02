#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "nba_game.h"
#include "nba_audio.h"
#include "nba_spc.h"
#include "nba_snes_ppu.h"
#include "nba_menu_input.h"

extern int win32_run_game(const char *rom_path, const char *assets_path,
                          bool title_only, bool setup_only, bool team_only,
                          bool player_setup_only);

typedef struct {
    unsigned frames;
    uint32_t buttons;
} HeadlessInputWord;

/* Input evidence uses native twelve-button words, not host enum values.
 * Each line is a duration and a hexadecimal held word. Adjacent identical
 * words remain held: only an explicit zero word produces a release. */
static bool load_headless_input(const char *path, HeadlessInputWord *words,
                                unsigned capacity, unsigned *count) {
    FILE *file = NULL;
#ifdef _MSC_VER
    if (fopen_s(&file, path, "rb") != 0) file = NULL;
#else
    file = fopen(path, "rb");
#endif
    if (!file) {
        fprintf(stderr, "[HEADLESS] Failed to open input script: %s\n", path);
        return false;
    }
    char line[128];
    unsigned line_number = 0, total = 0;
    bool valid = true;
    *count = 0;
    while (fgets(line, sizeof(line), file)) {
        char duration[32], value[32], extra, *end;
        ++line_number;
        if (!strchr(line, '\n') && !feof(file)) { valid = false; break; }
        const char *start = line + strspn(line, " \t\r\n");
        if (!*start || *start == '#') continue;
#ifdef _MSC_VER
        int fields = sscanf_s(start, "%31s %31s %c", duration, (unsigned)sizeof(duration),
                              value, (unsigned)sizeof(value), &extra, 1u);
#else
        int fields = sscanf(start, "%31s %31s %c", duration, value, &extra);
#endif
        if (fields != 2 ||
            duration[0] < '0' || duration[0] > '9' ||
            !strchr("0123456789abcdefABCDEF", value[0])) { valid = false; break; }
        unsigned long frames = strtoul(duration, &end, 10);
        if (*end || !frames || frames > 2000000u || total + frames > 2000000u) {
            valid = false; break;
        }
        unsigned long native = strtoul(value, &end, 16);
        if (*end || native > 0xFFF0u || (native & 15u) || *count == capacity) {
            valid = false; break;
        }
        uint32_t host = 0;
        for (unsigned bit = 0; bit < 12u; ++bit)
            if (native & (0x8000u >> bit)) host |= 1u << bit;
        words[*count].frames = (unsigned)frames;
        words[(*count)++].buttons = host;
        total += (unsigned)frames;
    }
    if (ferror(file) || !*count) valid = false;
    if (fclose(file) != 0) valid = false;
    if (!valid) fprintf(stderr, "[HEADLESS] Invalid input script %s at line %u.\n", path, line_number);
    return valid;
}

static uint64_t trace_fnv1a64(const void *data, size_t size) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static void write_setup_transition_trace_row(FILE *file, int stepped_frame,
                                              NbaSetupTransitionRoute route,
                                              const NbaGame *game) {
    const NbaSetupScreen *s = &game->scene.setup;
    uint64_t vram_hash = trace_fnv1a64(s->transition_vram,
                                      sizeof(s->transition_vram));
    uint64_t cgram_hash = trace_fnv1a64(s->transition_cgram,
                                       sizeof(s->transition_cgram));
    uint64_t rgb_hash = trace_fnv1a64(game->renderer.pixels,
                                     (size_t)NBA_SNES_WIDTH * NBA_SNES_HEIGHT *
                                         sizeof(game->renderer.pixels[0]));
    fprintf(file,
            "%d,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,"
            "%016llx,%016llx,%016llx\n",
            stepped_frame, game->state_frame, (int)route, s->transition_frame,
            s->active_transition_decoded_frame, s->active_transition_frame_count,
            (int)s->page, (int)s->transition_target,
            s->transition_blank ? 1 : 0, s->brightness,
            s->main_screen, s->sub_screen,
            s->bg1_hscroll, s->bg1_vscroll, s->bg2_hscroll, s->bg2_vscroll,
            s->bg3_hscroll, s->bg3_vscroll,
            s->layer_tilemap[0], s->layer_tilemap[1], s->layer_tilemap[2],
            s->layer_chr[0], s->layer_chr[1], s->layer_chr[2],
            s->layer_double_width[0] ? 1 : 0,
            s->layer_double_width[1] ? 1 : 0,
            s->layer_double_width[2] ? 1 : 0,
            s->layer_double_height[0] ? 1 : 0,
            s->layer_double_height[1] ? 1 : 0,
            s->layer_double_height[2] ? 1 : 0,
            (unsigned long long)vram_hash, (unsigned long long)cgram_hash,
            (unsigned long long)rgb_hash);
}

/**
 * Offset/Address/Size: N/A | Application Entry Point / CLI Dispatcher | size: N/A
 * Purpose: Parses CLI flags, executes headless frame verifications, or launches Win32 desktop application.
 */
int main(int argc, char *argv[]) {
    const char *rom_path = NULL;
    const char *assets_path = NULL;
    const char *dump_frame_path = NULL;
    const char *dump_audio_path = NULL;
    const char *dump_menu_sfx_path = NULL;
    const char *dump_gameplay_whistle_path = NULL;
    const char *setup_transition_trace_path = NULL;
    const char *gameplay_trace_path = NULL;
    const char *ppu_trace_path = NULL;
    const char *dump_sequence_dir = NULL;
    const char *input_script_path = NULL;
    const char *input_trace_path = NULL;
    int dump_sequence_from = 1;
    int menu_sfx_srcn = 0x1B;
    bool is_headless = false;
    bool audio_debug_test = false;
    int asset_debug_id = -1;
    bool player_lab = false;
    bool gameplay_lab = false;
    int gameplay_actor = 0;
    int gameplay_special_shot_at = 0;
    int gameplay_page = 1;
    bool gameplay_paused = false;
    int gameplay_step_count = 0;
    int player_lab_team = 3;
    int player_lab_roster = 0;
    int player_lab_team_right = 0;
    int player_lab_roster_down = 0;
    int player_lab_animation = 3;
    int player_lab_direction = 6;
    int player_lab_animation_right = 0;
    int player_lab_direction_right = 0;
    bool start_at_title = false;
    bool start_at_setup = false;
    bool start_at_team = false;
    bool start_at_player_setup = false;
    bool start_at_player_intro = false;
    bool start_at_tipoff = false;
    int player_intro_team = 18;
    int player_intro_slot = 0;
    bool spc_self_test = false;
    int step_frames = 30;
    double tick_rate = 60.0;
    const char *setup_menu = NULL;
    int setup_menu_row = 0;
    int setup_menu_right = 0;
    int setup_menu_delay = 0;
    int setup_menu_confirm_delay = 0;
    int setup_menu_visits = 1;
    int setup_menu_revisit_delay = 0;
    int setup_main_row = -1;
    int setup_main_right = 0;
    int setup_main_left = 0;
    bool setup_main_confirm = false;
    bool setup_main_a = false;
    bool setup_reenter = false;
    bool setup_menu_confirm = false;
    bool setup_menu_b = false;
    bool setup_simulation_three_minute = false;
    bool timing_debug = false;
    bool debug_state = false;
    int debug_every = 0;
    int debug_hud_page = 0;
    bool team_side_toggle = false;
    int team_category = -1;
    int team_up = 0;
    int team_down = 0;
    int team_right = 0;
    int team_left = 0;
    bool team_list = false;
    bool team_demo = false;
    int team_action_gap = 1;
    bool team_confirm = false;
    bool player_setup_left = false;
    bool player_setup_confirm = false;
    int tipoff_clock_override = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--rom") == 0 && i + 1 < argc) {
            rom_path = argv[++i];
        } else if (strcmp(argv[i], "--assets") == 0 && i + 1 < argc) {
            assets_path = argv[++i];
        } else if (strcmp(argv[i], "--headless") == 0) {
            is_headless = true;
        } else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            step_frames = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--tick-rate") == 0 && i + 1 < argc) {
            tick_rate = strtod(argv[++i], NULL);
        } else if (strcmp(argv[i], "--dump-frame") == 0 && i + 1 < argc) {
            dump_frame_path = argv[++i];
        } else if (strcmp(argv[i], "--dump-audio") == 0 && i + 1 < argc) {
            dump_audio_path = argv[++i];
        } else if (strcmp(argv[i], "--audio-debug") == 0) {
            audio_debug_test = true;
        } else if (strcmp(argv[i], "--asset-debug") == 0 && i + 1 < argc) {
            char *end = NULL;
            long value = strtol(argv[++i], &end, 10);
            if (!end || *end != '\0' || value <= 0 || value >= NBA_ASSET_MAX) {
                fprintf(stderr, "[HEADLESS] Invalid ROM asset ID: %s\n", argv[i]);
                return 1;
            }
            asset_debug_id = (int)value;
        } else if (strcmp(argv[i], "--player-lab") == 0) {
            player_lab = true;
        } else if (strcmp(argv[i], "--gameplay-lab") == 0) {
            gameplay_lab = true;
        } else if (strcmp(argv[i], "--gameplay-special-shot-at") == 0 && i + 1 < argc) {
            gameplay_special_shot_at = atoi(argv[++i]);
            if (gameplay_special_shot_at < 1) {
                fprintf(stderr, "[HEADLESS] --gameplay-special-shot-at must be positive.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--gameplay-actor") == 0 && i + 1 < argc) {
            gameplay_actor = atoi(argv[++i]);
            if (gameplay_actor < 0 || gameplay_actor >= NBA_GAMEPLAY_ACTOR_COUNT) {
                fprintf(stderr, "[HEADLESS] --gameplay-actor must be 0..9.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--gameplay-page") == 0 && i + 1 < argc) {
            gameplay_page = atoi(argv[++i]);
            if (gameplay_page < 1 || gameplay_page > 3) {
                fprintf(stderr, "[HEADLESS] --gameplay-page must be 1..3.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--gameplay-paused") == 0) {
            gameplay_paused = true;
        } else if (strcmp(argv[i], "--gameplay-step-count") == 0 && i + 1 < argc) {
            gameplay_step_count = atoi(argv[++i]);
            if (gameplay_step_count < 0) {
                fprintf(stderr, "[HEADLESS] --gameplay-step-count must be nonnegative.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--player-team") == 0 && i + 1 < argc) {
            player_lab_team = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--player-roster") == 0 && i + 1 < argc) {
            player_lab_roster = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--player-team-right") == 0 && i + 1 < argc) {
            player_lab_team_right = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--player-roster-down") == 0 && i + 1 < argc) {
            player_lab_roster_down = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--player-animation") == 0 && i + 1 < argc) {
            player_lab_animation = (int)strtol(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "--player-direction") == 0 && i + 1 < argc) {
            player_lab_direction = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--player-animation-right") == 0 && i + 1 < argc) {
            player_lab_animation_right = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--player-direction-right") == 0 && i + 1 < argc) {
            player_lab_direction_right = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--dump-menu-sfx") == 0 && i + 1 < argc) {
            dump_menu_sfx_path = argv[++i];
        } else if (strcmp(argv[i], "--dump-gameplay-whistle") == 0 &&
                   i + 1 < argc) {
            dump_gameplay_whistle_path = argv[++i];
        } else if (strcmp(argv[i], "--menu-sfx-srcn") == 0 && i + 1 < argc) {
            char *end = NULL;
            long value = strtol(argv[++i], &end, 0);
            if (!end || *end != '\0' || value < 0x1A || value > 0x1C) {
                fprintf(stderr, "[HEADLESS] Menu SFX SRCN must be 0x1A..0x1C.\n");
                return 1;
            }
            menu_sfx_srcn = (int)value;
        } else if (strcmp(argv[i], "--title-only") == 0) {
            start_at_title = true;
        } else if (strcmp(argv[i], "--setup-only") == 0) {
            start_at_setup = true;
        } else if (strcmp(argv[i], "--team-only") == 0) {
            start_at_team = true;
        } else if (strcmp(argv[i], "--player-setup-only") == 0) {
            start_at_player_setup = true;
        } else if (strcmp(argv[i], "--player-intro-only") == 0) {
            start_at_player_intro = true;
        } else if (strcmp(argv[i], "--player-intro-team") == 0 && i + 1 < argc) {
            player_intro_team = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--player-intro-slot") == 0 && i + 1 < argc) {
            player_intro_slot = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--tipoff-only") == 0) {
            start_at_tipoff = true;
        } else if (strcmp(argv[i], "--tipoff-clock") == 0 && i + 1 < argc) {
            tipoff_clock_override = atoi(argv[++i]);
            if (tipoff_clock_override < 0 || tipoff_clock_override > 0xFFFF) {
                fprintf(stderr, "[HEADLESS] --tipoff-clock must be 0..65535.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--team-confirm") == 0) {
            team_confirm = true;
        } else if (strcmp(argv[i], "--player-setup-left") == 0) {
            player_setup_left = true;
        } else if (strcmp(argv[i], "--player-setup-confirm") == 0) {
            player_setup_confirm = true;
        } else if (strcmp(argv[i], "--team-side-toggle") == 0) {
            team_side_toggle = true;
        } else if (strcmp(argv[i], "--team-category") == 0 && i + 1 < argc) {
            team_category = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--team-up") == 0 && i + 1 < argc) {
            team_up = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--team-down") == 0 && i + 1 < argc) {
            team_down = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--team-right") == 0 && i + 1 < argc) {
            team_right = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--team-left") == 0 && i + 1 < argc) {
            team_left = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--team-list") == 0) {
            team_list = true;
        } else if (strcmp(argv[i], "--team-demo") == 0) {
            team_demo = true;
        } else if (strcmp(argv[i], "--team-action-gap") == 0 && i + 1 < argc) {
            team_action_gap = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--dump-sequence-dir") == 0 && i + 1 < argc) {
            dump_sequence_dir = argv[++i];
        } else if (strcmp(argv[i], "--dump-sequence-from") == 0 && i + 1 < argc) {
            dump_sequence_from = atoi(argv[++i]);
            if (dump_sequence_from < 1) {
                fprintf(stderr, "[HEADLESS] --dump-sequence-from must be positive.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--spc-self-test") == 0) {
            spc_self_test = true;
        } else if (strcmp(argv[i], "--setup-menu") == 0 && i + 1 < argc) {
            setup_menu = argv[++i];
        } else if (strcmp(argv[i], "--input-script") == 0) {
            if (i + 1 >= argc || strncmp(argv[i + 1], "--", 2) == 0) {
                fprintf(stderr, "[HEADLESS] --input-script requires a file path.\n");
                return 1;
            }
            input_script_path = argv[++i];
        } else if (strcmp(argv[i], "--input-trace") == 0) {
            if (i + 1 >= argc || strncmp(argv[i + 1], "--", 2) == 0) {
                fprintf(stderr, "[HEADLESS] --input-trace requires a file path.\n");
                return 1;
            }
            input_trace_path = argv[++i];
        } else if (strcmp(argv[i], "--setup-simulation-three-minute") == 0) {
            setup_simulation_three_minute = true;
        } else if (strcmp(argv[i], "--setup-menu-row") == 0 && i + 1 < argc) {
            setup_menu_row = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--setup-menu-right") == 0 && i + 1 < argc) {
            setup_menu_right = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--setup-menu-delay") == 0 && i + 1 < argc) {
            const char *value_text = argv[++i];
            char *end = NULL;
            long value = strtol(value_text, &end, 10);
            if (end == value_text || !end || *end || value < 0 || value > 20000) {
                fprintf(stderr, "[HEADLESS] --setup-menu-delay must be 0..20000 frames.\n");
                return 1;
            }
            setup_menu_delay = (int)value;
        } else if (strcmp(argv[i], "--setup-menu-confirm-delay") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[HEADLESS] --setup-menu-confirm-delay requires a frame count.\n");
                return 1;
            }
            const char *value_text = argv[++i];
            char *end = NULL;
            long value = strtol(value_text, &end, 10);
            if (end == value_text || !end || *end || value < 0 || value > 20000) {
                fprintf(stderr, "[HEADLESS] --setup-menu-confirm-delay must be 0..20000 frames.\n");
                return 1;
            }
            setup_menu_confirm_delay = (int)value;
        } else if (strcmp(argv[i], "--setup-menu-revisit-delay") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[HEADLESS] --setup-menu-revisit-delay requires a frame count.\n");
                return 1;
            }
            const char *value_text = argv[++i];
            char *end = NULL;
            long value = strtol(value_text, &end, 10);
            if (end == value_text || !end || *end || value < 0 || value > 20000) {
                fprintf(stderr, "[HEADLESS] --setup-menu-revisit-delay must be 0..20000 frames.\n");
                return 1;
            }
            setup_menu_revisit_delay = (int)value;
        } else if (strcmp(argv[i], "--setup-menu-visits") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[HEADLESS] --setup-menu-visits requires a count.\n");
                return 1;
            }
            const char *value_text = argv[++i];
            char *end = NULL;
            long value = strtol(value_text, &end, 10);
            if (end == value_text || !end || *end || value < 1 || value > 3) {
                fprintf(stderr, "[HEADLESS] --setup-menu-visits must be 1..3.\n");
                return 1;
            }
            setup_menu_visits = (int)value;
        } else if (strcmp(argv[i], "--setup-main-row") == 0 && i + 1 < argc) {
            setup_main_row = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--setup-main-right") == 0 && i + 1 < argc) {
            setup_main_right = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--setup-main-left") == 0 && i + 1 < argc) {
            setup_main_left = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--setup-main-confirm") == 0) {
            setup_main_confirm = true;
        } else if (strcmp(argv[i], "--setup-main-a") == 0) {
            setup_main_a = true;
        } else if (strcmp(argv[i], "--setup-reenter") == 0) {
            setup_reenter = true;
        } else if (strcmp(argv[i], "--setup-menu-confirm") == 0) {
            setup_menu_confirm = true;
        } else if (strcmp(argv[i], "--setup-menu-b") == 0) {
            setup_menu_b = true;
        } else if (strcmp(argv[i], "--setup-transition-trace") == 0 &&
                   i + 1 < argc) {
            setup_transition_trace_path = argv[++i];
        } else if (strcmp(argv[i], "--gameplay-trace") == 0 && i + 1 < argc) {
            gameplay_trace_path = argv[++i];
        } else if (strcmp(argv[i], "--ppu-trace") == 0 && i + 1 < argc) {
            ppu_trace_path = argv[++i];
        } else if (strcmp(argv[i], "--timing-debug") == 0) {
            timing_debug = true;
        } else if (strcmp(argv[i], "--debug-hud-page") == 0 && i + 1 < argc) {
            char *end = NULL;
            long value = strtol(argv[++i], &end, 10);
            if (!end || *end != '\0' || value < 1 || value > 2) {
                fprintf(stderr, "[HEADLESS] --debug-hud-page must be 1 or 2.\n");
                return 1;
            }
            debug_hud_page = (int)value;
        } else if (strcmp(argv[i], "--debug-state") == 0) {
            debug_state = true;
        } else if (strcmp(argv[i], "--debug-every") == 0 && i + 1 < argc) {
            char *end = NULL;
            long value = strtol(argv[++i], &end, 10);
            if (!end || *end != '\0' || value <= 0 || value > 1000000) {
                fprintf(stderr, "[HEADLESS] --debug-every must be 1..1000000.\n");
                return 1;
            }
            debug_every = (int)value;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("NBA Live '95 Native C Port\n");
            printf("Usage: nba95_port.exe [options]\n\n");
            printf("Options:\n");
            printf("  --rom <path>          Path to SNES ROM file\n");
            printf("  --assets <path>       Path to extracted asset pack (.pak)\n");
            printf("  --headless            Run without opening GUI window\n");
            printf("  --frames <N>          Number of frames to step in headless mode (default: 30)\n");
            printf("  --tick-rate <Hz>      Headless host tick rate (default: 60.0)\n");
            printf("  --audio-debug         Activate audio sample debugger in headless render\n");
            printf("  --asset-debug <ID>    Render the F12 ROM asset browser at asset ID\n");
            printf("  --player-lab          Render the F9 Player Lab from packed ROM data\n");
            printf("  --gameplay-lab        Render the F8 gameplay telemetry overlay\n");
            printf("  --gameplay-special-shot-at N  Controlled rare-shot input at frame N (uses --gameplay-actor)\n");
            printf("  --gameplay-actor N    Select gameplay actor 0..9\n");
            printf("  --gameplay-page N     Select Gameplay Lab page 1..3\n");
            printf("  --gameplay-paused     Start Gameplay Lab with simulation paused\n");
            printf("  --gameplay-step-count N  Step N simulation frames while paused\n");
            printf("  --player-team N       Player Lab team 0..28 (default Chicago 3)\n");
            printf("  --player-roster N     Player Lab roster slot 0..11\n");
            printf("  --player-team-right N Apply N Player Lab Right presses\n");
            printf("  --player-roster-down N Apply N Player Lab Down presses\n");
            printf("  --player-animation N  Select ROM animation state 0x00..0x38\n");
            printf("  --player-direction N  Select ROM direction 0..7 (default 6)\n");
            printf("  --player-animation-right N Apply N Player Lab E presses\n");
            printf("  --player-direction-right N Apply N Player Lab I presses\n");
            printf("  --dump-menu-sfx FILE  Save a deterministic packed-SPC menu sound\n");
            printf("  --dump-gameplay-whistle FILE  Save ROM command-$44 whistle PCM\n");
            printf("  --menu-sfx-srcn N     Select menu SRCN 0x1A..0x1C (default 0x1B)\n");
            printf("  --title-only          Start at $80:E01E title state (headless tests)\n");
            printf("  --setup-only          Start at the $80:E600 -> $80:A2BF handoff\n");
            printf("  --team-only           Start at the $80:DBF6 -> $82:809A Team Select handoff\n");
            printf("  --player-setup-only   Start at the Team Select -> Player Setup handoff\n");
            printf("  --player-intro-only   Seed a home Starting Lineup card for visual tests\n");
            printf("  --player-intro-team N Select lineup team 0..28 (default 18)\n");
            printf("  --player-intro-slot N Select starter slot 0..4 (default 0)\n");
            printf("  --tipoff-only         Start at the ROM-matched center-court jump ball\n");
            printf("  --tipoff-clock N      Controlled raw clock seed for gameplay tests\n");
            printf("  --team-confirm        Press Start after Team Select settles\n");
            printf("  --player-setup-left   Assign Player 1 to the visitor/left team\n");
            printf("  --player-setup-confirm Press Start after Player Setup settles\n");
            printf("  --team-side-toggle    Toggle the active Team Select side once\n");
            printf("  --team-category N     Move from the name row to ranking category 0..4\n");
            printf("  --team-up N           Apply N raw Team Select Up presses\n");
            printf("  --team-down N         Apply N raw Team Select Down presses\n");
            printf("  --team-right N        Advance N teams alphabetically or by selected rank\n");
            printf("  --team-left N         Move back N teams alphabetically or by selected rank\n");
            printf("  --team-list           Print all 29 ROM team/ranking records and exit\n");
            printf("  --team-demo           Script right cycle, side toggle, then left cycle\n");
            printf("  --team-action-gap N   Frames between scripted Team Select inputs\n");
            printf("  --dump-sequence-dir D Save every rendered headless frame in directory D\n");
            printf("  --dump-sequence-from N  Begin sequence capture at stepped frame N\n");
            printf("  --setup-menu <name>   Open Rules or Options in headless mode\n");
            printf("  --setup-menu-row <N>  Move to submenu row N\n");
            printf("  --setup-menu-right N  Apply N right-value adjustments\n");
            printf("  --setup-menu-delay N  Wait N extra frames before scripted menu inputs\n");
            printf("  --setup-menu-confirm  Press Start to commit submenu values\n");
            printf("  --setup-menu-confirm-delay N  Idle N extra frames after edits before Start/B\n");
            printf("  --setup-menu-visits N  Repeat 1..3 normal submenu entry/commit/return journeys\n");
            printf("  --setup-menu-revisit-delay N  Idle N extra frames after return before reentry\n");
            printf("  --setup-menu-b        Press ignored B after scripted edits\n");
            printf("  --setup-transition-trace FILE\n");
            printf("                        Export every Setup transition PPU/render state as CSV\n");
            printf("  --setup-main-row N    Select main Setup row 0..3\n");
            printf("  --setup-main-right N  Apply N right adjustments on the main row\n");
            printf("  --setup-main-left N   Apply N left adjustments on the main row\n");
            printf("  --setup-main-confirm  Press Start and report the requested scene action\n");
            printf("  --setup-main-a        Press A on the selected main Setup row\n");
            printf("  --setup-reenter       Reinitialize Setup to verify session persistence\n");
            printf("  --timing-debug        Draw compact F10 overview page in a frame dump\n");
            printf("  --debug-hud-page N    Draw compact F10 page 1 or 2 in a frame dump\n");
            printf("  --debug-state         Print one expanded state snapshot after stepping\n");
            printf("  --gameplay-trace FILE Write per-frame gameplay telemetry as JSONL\n");
            printf("  --ppu-trace FILE      Write final Mode-1 pixel priority/palette provenance as JSONL\n");
            printf("  --debug-every N       Print an expanded state snapshot every N frames\n");
            printf("  --spc-self-test       Run deterministic SPC700/S-DSP core vectors\n");
            printf("  --dump-frame <file>   Save rendered frame to 24-bit BMP image\n");
            printf("  --dump-audio <file>   Save the active runtime-synthesized WAV\n");
            printf("  --help, -h            Show this help text\n");
            return 0;
        }
    }

    if (team_list) {
        for (int team = 0; team < NBA_TEAM_COUNT; ++team) {
            const NbaTeamRecord *record = &nba_team_records[team];
            char rank_text[NBA_TEAM_RANK_COUNT][3];
            for (int rank = 0; rank < NBA_TEAM_RANK_COUNT; ++rank) {
                if (record->rank[rank] > NBA_REGULAR_TEAM_COUNT)
                    snprintf(rank_text[rank], sizeof(rank_text[rank]), "-");
                else
                    snprintf(rank_text[rank], sizeof(rank_text[rank]), "%02u",
                             record->rank[rank]);
            }
            printf("[TEAM DATA] %02d %-14s %-14s S=%s R=%s B=%s D=%s O=%s\n",
                   team, record->name, record->nickname, rank_text[0], rank_text[1],
                   rank_text[2], rank_text[3], rank_text[4]);
        }
        return 0;
    }

    if (spc_self_test) {
        if (!nba_spc_self_test()) {
            fprintf(stderr, "[SPC TEST] FAIL\n");
            return 1;
        }
        printf("[SPC TEST] PASS: opcodes, timers, ports, BRR, and envelopes\n");
        return 0;
    }

    if (is_headless) {
        if (step_frames < 0 || tick_rate <= 0.0) {
            fprintf(stderr, "[HEADLESS] --frames must be non-negative and --tick-rate must be positive.\n");
            return 1;
        }
        if (setup_menu && strcmp(setup_menu, "rules") != 0 &&
            strcmp(setup_menu, "options") != 0) {
            fprintf(stderr, "[HEADLESS] --setup-menu must be rules or options.\n");
            return 1;
        }
        if (setup_menu_visits > 1 && (!setup_menu || !setup_menu_confirm || setup_menu_b)) {
            fprintf(stderr, "[HEADLESS] Repeated submenu visits require --setup-menu and --setup-menu-confirm without --setup-menu-b.\n");
            return 1;
        }
        if (setup_menu_row < 0 || setup_menu_row > 1000 ||
            setup_menu_right < 0 || setup_menu_right > 1000 ||
            setup_main_row < -1 || setup_main_row > 3 ||
            setup_main_right < 0 || setup_main_right > 1000 ||
            setup_main_left < 0 || setup_main_left > 1000) {
            fprintf(stderr, "[HEADLESS] Invalid Setup menu row or adjustment count.\n");
            return 1;
        }
        if (team_category < -1 || team_category >= NBA_TEAM_RANK_COUNT ||
            team_up < 0 || team_up > 1000 || team_down < 0 || team_down > 1000 ||
            team_right < 0 || team_right > 1000 || team_left < 0 || team_left > 1000 ||
            team_action_gap < 1 || team_action_gap > 1000) {
            fprintf(stderr, "[HEADLESS] Invalid Team Select category or adjustment count.\n");
            return 1;
        }
        if (player_lab_team < 0 || player_lab_team >= NBA_TEAM_COUNT ||
            player_lab_roster < 0 || player_lab_roster >= NBA_PLAYER_ROSTER_SIZE ||
            player_lab_team_right < 0 || player_lab_team_right > 1000 ||
            player_lab_roster_down < 0 || player_lab_roster_down > 1000 ||
            player_lab_animation < 0 || player_lab_animation >= NBA_PLAYER_ANIMATION_STATES ||
            player_lab_direction < 0 || player_lab_direction > 7 ||
            player_lab_animation_right < 0 || player_lab_animation_right > 1000 ||
            player_lab_direction_right < 0 || player_lab_direction_right > 1000) {
            fprintf(stderr, "[HEADLESS] Player Lab team must be 0..28 and roster must be 0..11.\n");
            return 1;
        }
        if (player_intro_team < 0 || player_intro_team >= NBA_TEAM_COUNT ||
            player_intro_slot < 0 ||
            player_intro_slot >= NBA_PLAYER_INTRO_STARTERS_PER_TEAM) {
            fprintf(stderr, "[HEADLESS] Player Introduction team must be 0..28 "
                            "and starter slot must be 0..4.\n");
            return 1;
        }
        printf("[HEADLESS] Starting headless verification (ROM: %s, Assets: %s, frames: %d)\n",
               rom_path ? rom_path : "(none)", assets_path ? assets_path : "(none)", step_frames);
        static NbaGame game; /* large renderer/active-scene buffers live off-stack */
        if (!nba_game_init(&game, rom_path, assets_path)) {
            fprintf(stderr, "[HEADLESS] Error: Failed to initialize game\n");
            return 1;
        }
        /* Synthesis and capture must not depend on a Windows output device. */
        nba_audio_set_host_playback_enabled(&game.audio, false);

        bool enter_setup = false;
        int title_press_frame = -1;
        int setup_down_count = 0;
        bool setup_menu_opened = false;
        bool setup_menu_done = false;
        int setup_menu_moves_done = 0;
        int setup_menu_right_done = 0;
        int setup_menu_confirm_waited = 0;
        int setup_menu_returns_completed = 0;
        int setup_menu_revisit_waited = 0;
        int setup_main_right_done = 0;
        int setup_main_left_done = 0;
        bool setup_main_confirm_done = false;
        bool setup_main_a_done = false;
        bool setup_main_done = setup_main_row < 0;
        unsigned setup_config_step = 0;
        static HeadlessInputWord input_words[8192];
        unsigned input_word_count = 0, input_word_index = 0, input_word_frame = 0;
        FILE *input_trace_file = NULL;
        bool team_toggle_done = false;
        bool team_category_done = team_category < 0;
        int team_up_done = 0;
        int team_down_done = 0;
        int team_right_done = 0;
        int team_left_done = 0;
        int team_action_wait = team_action_gap;
        FILE *setup_trace_file = NULL;
        int setup_trace_rows = 0;
        FILE *gameplay_trace_file = NULL;
        int gameplay_trace_rows = 0;
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "--enter-setup") == 0) enter_setup = true;
            if (strcmp(argv[i], "--title-press") == 0 && i + 1 < argc) title_press_frame = atoi(argv[++i]);
            if (strcmp(argv[i], "--setup-down") == 0 && i + 1 < argc) setup_down_count = atoi(argv[++i]);
        }

        bool automatic_navigation = enter_setup || setup_down_count || setup_menu ||
            setup_main_row >= 0 || setup_simulation_three_minute || team_demo ||
            team_up || team_down || team_right || team_left || team_category >= 0 ||
            team_side_toggle || team_confirm || player_setup_left || player_setup_confirm ||
            player_lab_team_right || player_lab_roster_down || player_lab_animation_right ||
            player_lab_direction_right || gameplay_step_count;
        if ((setup_menu && (setup_main_confirm || setup_main_a)) ||
            (setup_down_count && (setup_menu || setup_main_row >= 0 || setup_simulation_three_minute)) ||
            (title_press_frame >= 0 && automatic_navigation)) {
            fprintf(stderr, "[HEADLESS] Conflicting automatic button scripts; use one explicit --input-script for combined journeys.\n");
            nba_game_shutdown(&game);
            return 1;
        }

        if (input_script_path) {
            if (enter_setup || title_press_frame >= 0 || setup_down_count || setup_menu ||
                setup_main_row >= 0 || setup_simulation_three_minute || team_demo ||
                team_up || team_down || team_right || team_left || team_category >= 0 ||
                team_side_toggle || team_confirm || player_setup_left || player_setup_confirm ||
                player_lab_team_right || player_lab_roster_down || player_lab_animation_right ||
                player_lab_direction_right || gameplay_step_count) {
                fprintf(stderr, "[HEADLESS] --input-script cannot be mixed with automatic button scripts.\n");
                nba_game_shutdown(&game);
                return 1;
            }
            if (!load_headless_input(input_script_path, input_words, 8192u, &input_word_count)) {
                nba_game_shutdown(&game);
                return 1;
            }
        }
        if (setup_simulation_three_minute && !start_at_setup && !enter_setup) {
            fprintf(stderr, "[HEADLESS] Configuration navigation requires --setup-only or --enter-setup.\n");
            nba_game_shutdown(&game);
            return 1;
        }
        if (input_trace_path) {
#ifdef _MSC_VER
            if (fopen_s(&input_trace_file, input_trace_path, "wb") != 0) input_trace_file = NULL;
#else
            input_trace_file = fopen(input_trace_path, "wb");
#endif
            if (!input_trace_file) {
                fprintf(stderr, "[HEADLESS] Failed to open input trace: %s\n", input_trace_path);
                nba_game_shutdown(&game);
                return 1;
            }
            fprintf(input_trace_file, "step,held,pressed,released,native,state,page,row,working_mode,working_style,working_level,working_quarter,committed_mode,committed_style,committed_level,committed_quarter,previous,pending,delay,speed,fast");
            for (unsigned i = 0; i < NBA_SETUP_RULE_COUNT; ++i) fprintf(input_trace_file, ",rules%u", i);
            for (unsigned i = 0; i < NBA_SETUP_OPTION_COUNT; ++i) fprintf(input_trace_file, ",options%u", i);
            for (unsigned i = 0; i < NBA_SETUP_RULE_COUNT; ++i) fprintf(input_trace_file, ",custom%u", i);
            for (unsigned i = 0; i < NBA_SETUP_RULE_COUNT; ++i) fprintf(input_trace_file, ",working%u", i);
            fputc('\n', input_trace_file);
        }

        if (timing_debug && debug_hud_page == 0) debug_hud_page = 1;
        game.debug_hud_page = (uint8_t)debug_hud_page;

        if (setup_transition_trace_path) {
#ifdef _MSC_VER
            if (fopen_s(&setup_trace_file, setup_transition_trace_path, "wb") != 0)
                setup_trace_file = NULL;
#else
            setup_trace_file = fopen(setup_transition_trace_path, "wb");
#endif
            if (!setup_trace_file) {
                fprintf(stderr, "[HEADLESS] Failed to open Setup transition trace: %s\n",
                        setup_transition_trace_path);
                nba_game_shutdown(&game);
                return 1;
            }
            fprintf(setup_trace_file,
                    "step,state_frame,route,transition_frame,trace_frame,trace_frames,page,target,"
                    "forced_blank,brightness,main,sub,bg1h,bg1v,bg2h,bg2v,bg3h,bg3v,"
                    "bg1map,bg2map,bg3map,bg1chr,bg2chr,bg3chr,bg1wide,bg2wide,bg3wide,"
                    "bg1tall,bg2tall,bg3tall,"
                    "vram_fnv64,cgram_fnv64,rgb_fnv64\n");
        }

        if (gameplay_trace_path) {
#ifdef _MSC_VER
            if (fopen_s(&gameplay_trace_file, gameplay_trace_path, "wb") != 0)
                gameplay_trace_file = NULL;
#else
            gameplay_trace_file = fopen(gameplay_trace_path, "wb");
#endif
            if (!gameplay_trace_file) {
                fprintf(stderr, "[HEADLESS] Failed to open gameplay trace: %s\n",
                        gameplay_trace_path);
                if (setup_trace_file) fclose(setup_trace_file);
                nba_game_shutdown(&game);
                return 1;
            }
        }

        if (start_at_title) {
            if (!nba_game_enter_state(&game, NBA_STATE_TITLE_SEQUENCE)) {
                nba_game_shutdown(&game);
                return 1;
            }
        }

        if (start_at_setup) {
            if (!nba_game_enter_state(&game, NBA_STATE_GAME_SETUP)) {
                nba_game_shutdown(&game);
                return 1;
            }
        }
        if (start_at_team) {
            if (!nba_game_enter_state(&game, NBA_STATE_TEAM_SELECT)) {
                nba_game_shutdown(&game);
                return 1;
            }
        }
        if (start_at_player_setup) {
            if (!nba_game_enter_state(&game, NBA_STATE_PLAYER_SETUP)) {
                nba_game_shutdown(&game);
                return 1;
            }
        }
        if (start_at_player_intro) {
            /* This seed bypasses presentation timing only. It initializes the
             * production scene, assets, font and renderer, then selects one
             * home card so the visual smoke can inspect every ROM starter. */
            game.session.right_team = (uint8_t)player_intro_team;
            if (!nba_game_enter_state(&game, NBA_STATE_PLAYER_INTRO)) {
                nba_game_shutdown(&game);
                return 1;
            }
            game.scene.player_intro.phase = NBA_PLAYER_INTRO_LINEUPS;
            game.scene.player_intro.phase_frame = 0;
            game.scene.player_intro.lineup_card =
                NBA_PLAYER_INTRO_STARTERS_PER_TEAM + player_intro_slot;
        }
        if (start_at_tipoff) {
            if (!nba_game_enter_state(&game, NBA_STATE_TIPOFF)) {
                nba_game_shutdown(&game);
                return 1;
            }
            if (tipoff_clock_override >= 0)
                game.scene.tipoff.match_clock_raw_0928 =
                    (uint16_t)tipoff_clock_override;
        }

        if (audio_debug_test) {
            game.audio_debugger.is_active = true;
            nba_audio_debugger_update(&game.audio_debugger, &game.audio,
                                      &game.assets, &game.input);
        }
        if (player_lab) {
            game.player_lab.team = (uint8_t)player_lab_team;
            game.player_lab.player = (uint8_t)player_lab_roster;
            game.player_lab.animation_state = (uint8_t)player_lab_animation;
            game.player_lab.direction = (uint8_t)player_lab_direction;
            game.player_lab.is_active = true;
        }
        if (gameplay_lab) {
            if (game.state != NBA_STATE_TIPOFF) {
                fprintf(stderr, "[HEADLESS] --gameplay-lab requires --tipoff-only.\n");
                nba_game_shutdown(&game);
                return 1;
            }
            game.gameplay_debugger.is_active = true;
            game.gameplay_debugger.selected_actor = (uint8_t)gameplay_actor;
            game.gameplay_debugger.page = (uint8_t)(gameplay_page - 1);
            game.gameplay_debugger.is_paused = gameplay_paused;
        }
        int player_team_right_done = 0;
        int player_roster_down_done = 0;
        int player_animation_right_done = 0;
        int player_direction_right_done = 0;
        bool team_confirm_done = false;
        unsigned player_setup_left_steps = 0u;
        bool player_intro_confirm_done = false;
        int gameplay_steps_done = 0;

        /* Step frames to reach desired screen */
        for (int frame = 0; frame < step_frames; frame++) {
            if (frame+1 == gameplay_special_shot_at) {
                if (game.state != NBA_STATE_TIPOFF ||
                    !nba_tipoff_debug_special_shot(&game.scene.tipoff,(unsigned)gameplay_actor)) {
                    fprintf(stderr,"[SHOT DEBUG] Unable to seed controlled special-shot inputs.\n");
                    nba_game_shutdown(&game);
                    return 1;
                }
                printf("[SHOT DEBUG] CONTROLLED INPUT frame=%d actor=%d; native B625 -> B979 -> 9DA6\n",
                       frame+1,gameplay_actor);
            }
            uint32_t raw_buttons = 0u;
            /* The mandatory release is already an idle frame in Team Select's
             * --team-action-gap contract; do not count it twice. */
            if (!input_script_path && game.input.held != 0u &&
                game.state == NBA_STATE_TEAM_SELECT &&
                game.scene.team_select.transition_frame >= NBA_TEAM_TRANSITION_FRAMES &&
                team_action_wait > 0) --team_action_wait;
            /* Automatic commands are taps. Spend the next frame released before
             * advancing any command counter; otherwise repeated DOWN words are
             * a native hold, not multiple presses. Explicit scripts below retain
             * their exact whole-word durations, including simultaneous buttons. */
            if (!input_script_path && game.input.held == 0u) {
                if (setup_simulation_three_minute && setup_config_step < 8u &&
                    game.state == NBA_STATE_GAME_SETUP) {
                    static const uint32_t configure[] = { NBA_BTN_DOWN, NBA_BTN_RIGHT,
                        NBA_BTN_DOWN, NBA_BTN_DOWN, NBA_BTN_RIGHT,
                        NBA_BTN_UP, NBA_BTN_UP, NBA_BTN_UP };
                    if (game.scene.setup.frame >= NBA_SETUP_BG3_SETTLE_FRAME)
                        raw_buttons = configure[setup_config_step++];
                } else {
                    if (game.player_lab.is_active) {
                        if (player_team_right_done < player_lab_team_right) {
                            raw_buttons = NBA_BTN_RIGHT;
                            player_team_right_done++;
                        } else if (player_roster_down_done < player_lab_roster_down) {
                            raw_buttons = NBA_BTN_DOWN;
                            player_roster_down_done++;
                        } else if (player_animation_right_done < player_lab_animation_right) {
                            raw_buttons = NBA_BTN_R;
                            player_animation_right_done++;
                        } else if (player_direction_right_done < player_lab_direction_right) {
                            raw_buttons = NBA_BTN_X;
                            player_direction_right_done++;
                        }
                    }
                    if (game.gameplay_debugger.is_active &&
                        game.gameplay_debugger.is_paused &&
                        gameplay_steps_done < gameplay_step_count) {
                        raw_buttons = NBA_BTN_X;
                        gameplay_steps_done++;
                    }

                    if (enter_setup) {
                        if (game.state == NBA_STATE_TITLE_SEQUENCE) {
                            raw_buttons = NBA_BTN_START; /* skip the title video */
                        }
                    }

                    /* --title-press N: press Start once on frame N, mirroring the
                     * Mesen experiment used to time $80:E5C7's hold and fade. */
                    if (title_press_frame >= 0 && frame == title_press_frame) {
                        raw_buttons = NBA_BTN_START;
                    }

                    /* --setup-down N: step the Game Setup cursor down N rows, one press
                     * every 8 frames once the screen has settled. */
                    if (setup_down_count > 0 && game.state == NBA_STATE_GAME_SETUP &&
                        game.scene.setup.frame > NBA_SETUP_ENTER_FRAMES) {
                        int since = game.scene.setup.frame - NBA_SETUP_ENTER_FRAMES;
                        if (since % 8 == 1 && (since / 8) < setup_down_count) {
                            raw_buttons = NBA_BTN_DOWN;
                        }
                    }

                    /* Deterministic controller script for Rules/Options regressions.
                     * Each pulse has a real release frame, after $80:A3B8 has settled.
                     * Optional input delay aligns a declared native idle/background
                     * phase; it does not change production transition timing. */
                    if (setup_menu_done && setup_menu_returns_completed + 1 < setup_menu_visits &&
                        game.state == NBA_STATE_GAME_SETUP &&
                        game.scene.setup.page == NBA_SETUP_PAGE_MAIN &&
                        game.scene.setup.transition == NBA_SETUP_TRANSITION_NONE &&
                        !game.scene.setup.transition_release_pending) {
                        /* Reuse the real returned screen/configuration; never reinitialize
                         * the scene or patch its cursor/scroll/resource state for a test. */
                        if (setup_menu_revisit_waited < setup_menu_revisit_delay) {
                            setup_menu_revisit_waited++;
                        } else {
                            setup_menu_returns_completed++;
                            setup_menu_done = setup_menu_opened = false;
                            setup_menu_moves_done = setup_menu_right_done = 0;
                            setup_menu_confirm_waited = setup_menu_revisit_waited = 0;
                        }
                    }
                    if (setup_menu && setup_main_done && !setup_menu_done &&
                        game.state == NBA_STATE_GAME_SETUP &&
                        game.scene.setup.frame >= NBA_SETUP_BG3_SETTLE_FRAME + setup_menu_delay) {
                        NbaSetupRow target = strcmp(setup_menu, "rules") == 0 ?
                                             NBA_SETUP_ROW_RULES : NBA_SETUP_ROW_OPTIONS;
                        if (!setup_menu_opened) {
                            if (game.scene.setup.row != target) {
                                raw_buttons = NBA_BTN_DOWN;
                            } else {
                                raw_buttons = NBA_BTN_A;
                                setup_menu_opened = true;
                            }
                        } else if (game.scene.setup.page != NBA_SETUP_PAGE_MAIN &&
                                   game.scene.setup.transition == NBA_SETUP_TRANSITION_NONE) {
                            if (setup_menu_moves_done < setup_menu_row) {
                                raw_buttons = NBA_BTN_DOWN;
                                setup_menu_moves_done++;
                            } else if (setup_menu_right_done < setup_menu_right) {
                                raw_buttons = NBA_BTN_RIGHT;
                                setup_menu_right_done++;
                            } else if ((setup_menu_confirm || setup_menu_b) &&
                                       setup_menu_confirm_waited < setup_menu_confirm_delay) {
                                /* Input-script idle, not an added game transition wait. */
                                setup_menu_confirm_waited++;
                            } else if (setup_menu_b) {
                                raw_buttons = NBA_BTN_B;
                                setup_menu_done = true;
                            } else if (setup_menu_confirm) {
                                raw_buttons = NBA_BTN_START;
                                setup_menu_done = true;
                            } else {
                                setup_menu_done = true;
                            }
                        }
                    }
                    if (!setup_main_done && game.state == NBA_STATE_GAME_SETUP &&
                        game.scene.setup.page == NBA_SETUP_PAGE_MAIN &&
                        game.scene.setup.frame >= NBA_SETUP_BG3_SETTLE_FRAME) {
                        if ((int)game.scene.setup.row != setup_main_row) {
                            raw_buttons = NBA_BTN_DOWN;
                        } else if (setup_main_right_done < setup_main_right) {
                            raw_buttons = NBA_BTN_RIGHT;
                            setup_main_right_done++;
                        } else if (setup_main_left_done < setup_main_left) {
                            raw_buttons = NBA_BTN_LEFT;
                            setup_main_left_done++;
                        } else if (setup_main_confirm && !setup_main_confirm_done) {
                            raw_buttons = NBA_BTN_START;
                            setup_main_confirm_done = true;
                        } else if (setup_main_a && !setup_main_a_done) {
                            raw_buttons = NBA_BTN_A;
                            setup_main_a_done = true;
                        } else {
                            setup_main_done = true;
                        }
                    }
                    if (game.state == NBA_STATE_TEAM_SELECT &&
                        game.scene.team_select.transition_frame >= NBA_TEAM_TRANSITION_FRAMES) {
                        NbaTeamSelect *team = &game.scene.team_select;
                        if (team_demo) {
                            int at = team->steady_frame;
                            if (at == 30 || at == 60 || at == 130 || at == 160)
                                raw_buttons = NBA_BTN_RIGHT;
                            else if (at == 100)
                                raw_buttons = NBA_BTN_A;
                        } else if (team_action_wait > 0) {
                            team_action_wait--;
                        } else if (team_up_done < team_up) {
                            raw_buttons = NBA_BTN_UP;
                            team_up_done++;
                            team_action_wait = team_action_gap;
                        } else if (team_down_done < team_down) {
                            raw_buttons = NBA_BTN_DOWN;
                            team_down_done++;
                            team_action_wait = team_action_gap;
                        } else if (!team_category_done) {
                            if ((int)team->selector !=
                                (int)NBA_TEAM_SELECT_SCORING + team_category) {
                                raw_buttons = NBA_BTN_DOWN;
                                team_action_wait = team_action_gap;
                            } else {
                                team_category_done = true;
                            }
                        } else if (team_side_toggle && !team_toggle_done) {
                            raw_buttons = NBA_BTN_A;
                            team_toggle_done = true;
                            team_action_wait = team_action_gap;
                        } else if (team_right_done < team_right) {
                            raw_buttons = NBA_BTN_RIGHT;
                            team_right_done++;
                            team_action_wait = team_action_gap;
                        } else if (team_left_done < team_left) {
                            raw_buttons = NBA_BTN_LEFT;
                            team_left_done++;
                            team_action_wait = team_action_gap;
                        } else if (team_confirm && !team_confirm_done) {
                            raw_buttons = NBA_BTN_START;
                            team_confirm_done = true;
                        }
                    }
                    if (game.state == NBA_STATE_PLAYER_SETUP &&
                        game.scene.player_setup.transition_frame >=
                            NBA_PLAYER_SETUP_TRANSITION_FRAMES) {
                        /* Native selection walks RIGHT(2), NEUTRAL(1), LEFT(0).
                         * Keep the CLI's requested LEFT destination using two
                         * taps; the outer driver supplies their release frames. */
                        if (player_setup_left && player_setup_left_steps < 2u) {
                            raw_buttons = NBA_BTN_LEFT;
                            ++player_setup_left_steps;
                        } else if (player_setup_confirm &&
                                   !game.scene.player_setup.confirm_requested) {
                            raw_buttons = NBA_BTN_START;
                        }
                    }
                    if (player_setup_confirm && !player_intro_confirm_done &&
                        game.state == NBA_STATE_PLAYER_INTRO &&
                        game.scene.player_intro.phase == NBA_PLAYER_INTRO_LINEUPS &&
                        game.scene.player_intro.lineup_card == 9 &&
                        game.scene.player_intro.phase_frame >= NBA_PLAYER_INTRO_CARD_FRAMES) {
                        raw_buttons = NBA_BTN_START;
                        player_intro_confirm_done = true;
                    }
                }
            }
            if (input_script_path && input_word_index < input_word_count) {
                raw_buttons = input_words[input_word_index].buttons;
                if (++input_word_frame == input_words[input_word_index].frames) {
                    ++input_word_index;
                    input_word_frame = 0u;
                }
            }
            nba_game_input_update(&game.input, raw_buttons);
            NbaSetupTransitionRoute route_before =
                game.state == NBA_STATE_GAME_SETUP ?
                    game.scene.setup.transition_route :
                    NBA_SETUP_TRANSITION_ROUTE_NONE;
            bool transition_before = route_before != NBA_SETUP_TRANSITION_ROUTE_NONE;
            bool release_before = transition_before &&
                game.scene.setup.transition_release_pending;
            nba_game_tick(&game, (float)(1.0 / tick_rate));
            if (input_trace_file) {
                bool setup = game.state == NBA_STATE_GAME_SETUP;
                const NbaSetupScreen *s = setup ? &game.scene.setup : NULL;
                const NbaMenuControllerInput *c = setup ? &s->menu_input.controller[0] : NULL;
                fprintf(input_trace_file,
                        "%d,%u,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u,%d,%d,%d,%d,%d",
                        frame + 1, game.input.held, game.input.pressed, game.input.released,
                        nba_menu_input_native_buttons(game.input.held), (int)game.state,
                        setup ? (int)s->page : -1,
                        setup ? (s->page == NBA_SETUP_PAGE_MAIN ? (int)s->row : s->menu_row) : -1,
                        setup ? s->working_main[0] : -1, setup ? s->working_main[1] : -1,
                        setup ? s->working_main[2] : -1, setup ? s->working_main[3] : -1,
                        game.session.config.main_values[0], game.session.config.main_values[1],
                        game.session.config.main_values[2], game.session.config.main_values[3],
                        c ? c->previous : -1, c ? c->pending : -1,
                        c ? c->delay : -1, c ? c->speed : -1, c ? c->fast : -1);
                for (unsigned i = 0; i < NBA_SETUP_RULE_COUNT; ++i)
                    fprintf(input_trace_file, ",%u", game.session.config.rules[i]);
                for (unsigned i = 0; i < NBA_SETUP_OPTION_COUNT; ++i)
                    fprintf(input_trace_file, ",%u", game.session.config.options[i]);
                for (unsigned i = 0; i < NBA_SETUP_RULE_COUNT; ++i)
                    fprintf(input_trace_file, ",%u", game.session.config.custom_rules[i]);
                const uint16_t *working = setup ?
                    (s->page == NBA_SETUP_PAGE_MAIN ? s->working_main :
                     s->page == NBA_SETUP_PAGE_RULES ? s->working_rules : s->working_options) : NULL;
                unsigned working_count = setup ? (s->page == NBA_SETUP_PAGE_MAIN ? 4u :
                    s->page == NBA_SETUP_PAGE_RULES ? NBA_SETUP_RULE_COUNT : NBA_SETUP_OPTION_COUNT) : 0u;
                for (unsigned i = 0; i < NBA_SETUP_RULE_COUNT; ++i)
                    fprintf(input_trace_file, ",%d", i < working_count ? working[i] : -1);
                fputc('\n', input_trace_file);
            }
            if (gameplay_trace_file && game.state == NBA_STATE_TIPOFF) {
                nba_gameplay_telemetry_write_jsonl(gameplay_trace_file,
                                                   &game.gameplay_telemetry);
                gameplay_trace_rows++;
            }
            bool transition_after = game.state == NBA_STATE_GAME_SETUP &&
                game.scene.setup.transition_route != NBA_SETUP_TRANSITION_ROUTE_NONE;
            bool rendered_frame = false;
            if (setup_trace_file &&
                (transition_after || (transition_before && !release_before))) {
                NbaSetupTransitionRoute route = transition_after ?
                    game.scene.setup.transition_route : route_before;
                nba_game_render(&game);
                rendered_frame = true;
                write_setup_transition_trace_row(setup_trace_file, frame + 1,
                                                 route, &game);
                setup_trace_rows++;
            }
            if (debug_every > 0 && (frame + 1) % debug_every == 0) {
                printf("[DEBUG SAMPLE] stepped=%d\n", frame + 1);
                nba_game_debug_print(&game);
            }
            /* Some source presentation paths intentionally retain the prior
             * rendered framebuffer while changing brightness or adding the
             * next layer.  Render every stepped frame whenever a sequence is
             * requested, including the unsaved warmup before `from`; starting
             * the renderer at the first saved frame otherwise turns a valid
             * mid-transition image into black. */
            if (dump_sequence_dir) {
                char sequence_path[1024];
                if (!rendered_frame)
                    nba_game_render(&game);
                if (frame + 1 >= dump_sequence_from) {
                    snprintf(sequence_path, sizeof(sequence_path), "%s/frame_%04d.bmp",
                             dump_sequence_dir, frame + 1);
                    if (!nba_renderer_save_bmp(&game.renderer, sequence_path)) {
                        fprintf(stderr, "[HEADLESS] Failed sequence frame: %s\n", sequence_path);
                        if (setup_trace_file) fclose(setup_trace_file);
                        if (gameplay_trace_file) fclose(gameplay_trace_file);
                        nba_game_shutdown(&game);
                        return 1;
                    }
                }
            }
        }
        if (setup_trace_file) {
            if (fclose(setup_trace_file) != 0) {
                fprintf(stderr, "[HEADLESS] Failed to finish Setup transition trace.\n");
                nba_game_shutdown(&game);
                return 1;
            }
            setup_trace_file = NULL;
            printf("[HEADLESS] Wrote %d Setup transition rows to: %s\n",
                   setup_trace_rows, setup_transition_trace_path);
        }

        if (input_trace_file && fclose(input_trace_file) != 0) {
            fprintf(stderr, "[HEADLESS] Failed to finish input trace.\n");
            nba_game_shutdown(&game);
            return 1;
        }
        if (gameplay_trace_file) {
            if (fclose(gameplay_trace_file) != 0) {
                fprintf(stderr, "[HEADLESS] Failed to finish gameplay trace.\n");
                nba_game_shutdown(&game);
                return 1;
            }
            gameplay_trace_file = NULL;
            printf("[HEADLESS] Wrote %d gameplay JSONL rows to: %s\n",
                   gameplay_trace_rows, gameplay_trace_path);
        }
        if (dump_sequence_dir)
            printf("[HEADLESS] Wrote %d rendered sequence frames to: %s\n",
                   step_frames >= dump_sequence_from ? step_frames - dump_sequence_from + 1 : 0,
                   dump_sequence_dir);
        if (asset_debug_id >= 0) {
            game.asset_debugger.is_active = true;
            bool found = false;
            for (uint32_t index = 0; index < game.assets.item_count; ++index) {
                if (game.assets.items[index].id == (uint32_t)asset_debug_id) {
                    found = true;
                    game.asset_debugger.selected_index = (int)index;
                    if (game.assets.items[index].size == 0x10000u)
                        game.asset_debugger.tile_page = 16;
                    break;
                }
            }
            if (!found) {
                fprintf(stderr, "[HEADLESS] ROM asset ID %d is not present in this pack.\n",
                        asset_debug_id);
                nba_game_shutdown(&game);
                return 1;
            }
        }
        if (player_lab) {
            nba_player_lab_print(&game.player_lab, &game.assets);
        }

        if (dump_menu_sfx_path) {
            nba_audio_play_setup_sfx(&game.audio, &game.assets,
                                     (uint8_t)menu_sfx_srcn);
            if (!nba_audio_save_setup_sfx_wav(&game.audio, dump_menu_sfx_path)) {
                fprintf(stderr, "[HEADLESS] Failed to write menu SFX WAV.\n");
                nba_game_shutdown(&game);
                return 1;
            }
        }
        if (dump_gameplay_whistle_path) {
            if (!nba_audio_play_gameplay_whistle(&game.audio, &game.assets) ||
                !nba_audio_save_setup_sfx_wav(
                    &game.audio, dump_gameplay_whistle_path)) {
                fprintf(stderr, "[HEADLESS] Failed to write gameplay whistle WAV.\n");
                nba_game_shutdown(&game);
                return 1;
            }
        }

        if (setup_menu) {
            if (game.state != NBA_STATE_GAME_SETUP) {
                fprintf(stderr, "[HEADLESS] Cannot report requested submenu after leaving Game Setup (state %d).\n", (int)game.state);
                nba_game_shutdown(&game);
                return 1;
            }
            const NbaSetupScreen *s = &game.scene.setup;
            int menu_count = strcmp(setup_menu, "rules") == 0 ?
                             NBA_SETUP_RULE_COUNT : NBA_SETUP_OPTION_COUNT;
            int report_row = strcmp(setup_menu, "rules") == 0 ?
                (setup_menu_row < menu_count ? setup_menu_row : menu_count - 1) :
                setup_menu_row % menu_count;
            printf("[SETUP TEST] page=%d menu_row=%d transition=%d/%d blank=%d gfx=%d "
                   "rules0=%u/%u options0=%u/%u "
                   "option_row=%d working=%u committed=%u main_row=%d main=%u/%u/%u/%u",
                   (int)s->page, s->menu_row, (int)s->transition,
                   s->transition_frame, s->transition_blank, s->has_gfx,
                   s->working_rules[0], s->config->rules[0],
                   s->working_options[0], s->config->options[0],
                   report_row,
                   strcmp(setup_menu, "rules") == 0 ?
                       s->working_rules[report_row] : s->working_options[report_row],
                   strcmp(setup_menu, "rules") == 0 ?
                       s->config->rules[report_row] : s->config->options[report_row],
                   (int)s->row, s->working_main[0],
                   s->working_main[1], s->working_main[2],
                   s->working_main[3]);
            printf(" rules=");
            for (int row = 0; row < NBA_SETUP_RULE_COUNT; row++) {
                printf("%s%u", row ? "/" : "", s->config->rules[row]);
            }
            printf("\n");
        }
        if (setup_main_row >= 0) {
            const uint16_t *main = game.state == NBA_STATE_GAME_SETUP ?
                game.scene.setup.working_main : game.session.config.main_values;
            printf("[SETUP MAIN TEST] row=%d mode=%u style=%u level=%u quarter=%u action=%d\n",
                   game.state == NBA_STATE_GAME_SETUP ? (int)game.scene.setup.row :
                       setup_main_row,
                   main[0], main[1], main[2], main[3],
                   (int)game.last_setup_action);
            printf("[SETUP COMMITTED] main=%u/%u/%u/%u\n",
                   game.session.config.main_values[0], game.session.config.main_values[1],
                   game.session.config.main_values[2], game.session.config.main_values[3]);
        }
        if (setup_reenter) {
            if (!nba_game_enter_state(&game, NBA_STATE_GAME_SETUP)) {
                nba_game_shutdown(&game);
                return 1;
            }
            printf("[SETUP REENTER] mode=%u style=%u level=%u quarter=%u\n",
                   game.session.config.main_values[0],
                   game.session.config.main_values[1],
                   game.session.config.main_values[2],
                   game.session.config.main_values[3]);
        }
        if (game.state == NBA_STATE_TEAM_SELECT) {
            const NbaTeamSelect *team = &game.scene.team_select;
            int category = team->selector >= NBA_TEAM_SELECT_SCORING ?
                           (int)team->selector - NBA_TEAM_SELECT_SCORING : -1;
            printf("[TEAM SELECT TEST] active=%s selector=%u category=%d left=%u:%s right=%u:%s "
                   "transition=%d\n",
                   team->active_side == NBA_TEAM_SIDE_LEFT ? "LEFT" : "RIGHT",
                   (unsigned)team->selector, category, team->session->left_team,
                   nba_team_records[team->session->left_team].name,
                   team->session->right_team,
                   nba_team_records[team->session->right_team].name,
                   team->transition_frame);
        }
        if (game.state == NBA_STATE_PLAYER_SETUP) {
            const NbaPlayerSetup *player_setup = &game.scene.player_setup;
            unsigned selection = player_setup->controller_selection & 0x7fffu;
            printf("[PLAYER SETUP TEST] p1=%s left=%u:%s right=%u:%s "
                   "transition=%d steady=%d confirm=%d\n",
                   selection == 0u ? "LEFT" : selection == 1u ? "NEUTRAL" : "RIGHT",
                   player_setup->session->left_team,
                   nba_team_records[player_setup->session->left_team].name,
                   player_setup->session->right_team,
                   nba_team_records[player_setup->session->right_team].name,
                   player_setup->transition_frame, player_setup->steady_frame,
                   player_setup->confirm_requested ? 1 : 0);
        }
        if (debug_state) nba_game_debug_print(&game);

        nba_game_render(&game);

        if (ppu_trace_path) {
            if (game.state != NBA_STATE_TIPOFF) {
                fprintf(stderr, "[PPU TRACE] --ppu-trace requires --tipoff-only or a run ending in gameplay.\n");
                nba_game_shutdown(&game);
                return 1;
            }
            FILE *ppu_trace_file = NULL;
#ifdef _MSC_VER
            if (fopen_s(&ppu_trace_file, ppu_trace_path, "wb") != 0)
                ppu_trace_file = NULL;
#else
            ppu_trace_file = fopen(ppu_trace_path, "wb");
#endif
            bool ppu_trace_ok = ppu_trace_file && nba_snes_mode1_write_jsonl(
                ppu_trace_file, &game.renderer, game.frame_count,
                game.state_frame);
            if (ppu_trace_file && fclose(ppu_trace_file) != 0)
                ppu_trace_ok = false;
            if (!ppu_trace_ok) {
                fprintf(stderr, "[PPU TRACE] Failed to write: %s\n", ppu_trace_path);
                nba_game_shutdown(&game);
                return 1;
            }
            NbaSnesMode1Stats ppu_stats;
            nba_snes_mode1_stats(&game.renderer, &ppu_stats);
            printf("[PPU TRACE] BG1=%u BG2=%u BG3=%u OBJ=%u BACKDROP=%u -> %s\n",
                   ppu_stats.visible[NBA_SNES_LAYER_BG1],
                   ppu_stats.visible[NBA_SNES_LAYER_BG2],
                   ppu_stats.visible[NBA_SNES_LAYER_BG3],
                   ppu_stats.visible[NBA_SNES_LAYER_OBJ],
                   ppu_stats.visible[NBA_SNES_LAYER_BACKDROP], ppu_trace_path);
        }

        if (dump_frame_path) {
            printf("[HEADLESS] Saving frame capture to: %s\n", dump_frame_path);
            if (nba_renderer_save_bmp(&game.renderer, dump_frame_path)) {
                printf("[HEADLESS] BMP frame written successfully (%dx%d).\n",
                       game.renderer.width, game.renderer.height);
            } else {
                fprintf(stderr, "[HEADLESS] Failed to write BMP file: %s\n", dump_frame_path);
                nba_game_shutdown(&game);
                return 1;
            }
        }

        if (dump_audio_path) {
            printf("[HEADLESS] Saving synthesized audio to: %s\n", dump_audio_path);
            if (!nba_audio_save_generated_wav(&game.audio, dump_audio_path)) {
                fprintf(stderr, "[HEADLESS] Failed to write synthesized audio.\n");
                nba_game_shutdown(&game);
                return 1;
            }
        }

        nba_game_shutdown(&game);
        printf("[HEADLESS] Headless execution completed successfully.\n");
        return 0;
    }

    /* Normal Win32 graphical execution */
    return win32_run_game(rom_path, assets_path, start_at_title, start_at_setup,
                          start_at_team, start_at_player_setup);
}
