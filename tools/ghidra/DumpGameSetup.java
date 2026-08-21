// Disassemble and decompile the routines that the ROM actually executes on the
// Game Setup screen. Ranges are read straight from the live Mesen execution
// trace (.analysis/setup_capture/setup_exec_addrs.txt), so nothing is guessed.
//
// args: <analysisDir> <bankHex>

import java.io.File;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.listing.CodeUnit;

public class DumpGameSetup extends GhidraScript {

    private static class Range {
        long start, end;
        Range(long s, long e) { start = s; end = e; }
    }

    private List<Range> readTrace(File traceFile, int bank) throws Exception {
        List<Range> out = new ArrayList<>();
        if (!traceFile.exists()) return out;
        for (String line : Files.readAllLines(traceFile.toPath(), StandardCharsets.UTF_8)) {
            line = line.trim();
            if (line.isEmpty() || line.startsWith("#")) continue;
            String[] parts = line.split("-");
            if (parts.length != 2) continue;
            long a = Long.parseLong(parts[0].trim(), 16);
            long b = Long.parseLong(parts[1].trim(), 16);
            int ba = (int) ((a >> 16) & 0xFF);
            // bank $00 mirrors $80 in LoROM
            if (ba != bank && !(bank == 0x80 && ba == 0x00)) continue;
            out.add(new Range(a & 0xFFFF, b & 0xFFFF));
        }
        return out;
    }

    private void addMenuRanges(List<Range> ranges, int bank) {
        // Entry points below came from tools/mesen_setup_menus_capture.lua.
        // Keeping them explicit makes the submenu analysis reproducible even
        // when the older settled-screen trace is regenerated independently.
        if (bank == 0x80) {
            ranges.add(new Range(0x9DEA, 0x9E61)); // shared input dispatch
        } else if (bank == 0x81) {
            ranges.add(new Range(0x9756, 0x9FD3)); // shared proportional BG3 glyph renderer
            ranges.add(new Range(0x9FD4, 0xA1ED)); // proportional BG3 text wrapper
            ranges.add(new Range(0xA1EE, 0xA2D2)); // BG3 upload/clear helper
            ranges.add(new Range(0xA2D3, 0xA35F)); // slider/OAM helper
            ranges.add(new Range(0xD318, 0xD3B0)); // Rules frame dispatcher
            ranges.add(new Range(0xD3B1, 0xD445)); // Rules cursor movement
            ranges.add(new Range(0xD446, 0xD479)); // Rules decrement
            ranges.add(new Range(0xD47A, 0xD4BF)); // Rules common commit
            ranges.add(new Range(0xD4C0, 0xD50D)); // Rules increment
            ranges.add(new Range(0xD50E, 0xD59A)); // Rules confirm/copy
            ranges.add(new Range(0xD59B, 0xD674)); // Rules redraw
            ranges.add(new Range(0xD675, 0xD6A0)); // Rules bar/text helper
        } else if (bank == 0x82) {
            ranges.add(new Range(0x8CD1, 0x8D3B)); // Options frame dispatcher
            ranges.add(new Range(0x8D3C, 0x8DA5)); // Options cursor movement
            ranges.add(new Range(0x8DA6, 0x8DC5)); // Options decrement
            ranges.add(new Range(0x8DC6, 0x8E72)); // Options common commit
            ranges.add(new Range(0x8E73, 0x8F9B)); // Options increment
            ranges.add(new Range(0x8F9C, 0x9027)); // Options redraw
            ranges.add(new Range(0x9028, 0x9075)); // Options text/bar helper
            ranges.add(new Range(0x902F, 0x90A5)); // Options slider-object setup
        } else if (bank == 0x87) {
            ranges.add(new Range(0x8C2D, 0x8C80)); // live volume apply helper
        }
    }

    private void dumpRange(PrintWriter w, String bank, long first, long last) {
        Listing listing = currentProgram.getListing();
        Address end = toAddr(last);
        w.printf("--- $%s:%04X-$%s:%04X ---%n", bank, first, bank, last);
        Instruction ins = listing.getInstructionAt(toAddr(first));
        if (ins == null) {
            w.printf("(no instructions decoded at $%s:%04X)%n%n", bank, first);
            return;
        }
        for (; ins != null && ins.getAddress().compareTo(end) <= 0; ins = ins.getNext()) {
            String m = ins.getMnemonicString();
            String rest = ins.toString();
            rest = rest.length() > m.length() ? rest.substring(m.length()).trim() : "";
            w.printf("%s:%04X  %-8s %s%n", bank, ins.getAddress().getOffset(), m, rest);
        }
        w.println();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File outDir = new File(args.length > 0 ? args[0] : ".");
        outDir.mkdirs();
        String bankStr = args.length > 1 ? args[1] : "80";
        int bank = Integer.parseInt(bankStr, 16);

        List<Range> ranges = readTrace(new File(outDir, "setup_exec_addrs.txt"), bank);
        addMenuRanges(ranges, bank);
        println("bank $" + bankStr + ": " + ranges.size() + " traced ranges");
        if (ranges.isEmpty()) return;

        for (Range r : ranges) {
            Address a = toAddr(r.start);
            addEntryPoint(a);
            disassemble(a);
            if (getFunctionAt(a) == null) {
                createFunction(a, String.format("sub_%s_%04X", bankStr, r.start));
            }
        }

        Listing listing = currentProgram.getListing();
        if (bank == 0x80) {
            createLabel(toAddr(0xA2BF), "game_setup_build_layers", true);
            listing.setComment(toAddr(0xA2BF), CodeUnit.PLATE_COMMENT,
                "Shared Game Setup/Rules/Options layer builder. Runs during the forced-blank " +
                "load interval after the outgoing BG1/BG2 slide, then releases the new page " +
                "with both horizontal scrolls at 768.");
            createLabel(toAddr(0xA3B8), "game_setup_frame_update", true);
            listing.setComment(toAddr(0xA3B8), CodeUnit.PLATE_COMMENT,
                "Shared page transition/frame sequencer: scrolls BG3 away, moves BG1/BG2 in " +
                "opposite directions by 8 pixels/frame under the 15-step INIDISP fade, then " +
                "runs the 32-frame entrance and delayed BG3 vertical staging. Also advances " +
                "the steady BG2 backdrop scroll.");
            createLabel(toAddr(0xA9E3), "game_setup_apu_command", true);
            listing.setComment(toAddr(0xA9E3), CodeUnit.PLATE_COMMENT,
                "Game Setup CPU-side music command producer. Writes the $2140-$2143 protocol " +
                "that keeps the resident SPC700 driver sequencing BRR samples.");
            createLabel(toAddr(0xAA7B), "game_setup_apu_handshake", true);
            listing.setComment(toAddr(0xAA7B), CodeUnit.PLATE_COMMENT,
                "Waits for and acknowledges the SPC700 command-port handshake.");
            createLabel(toAddr(0xAACD), "game_setup_apu_queue", true);
            listing.setComment(toAddr(0xAACD), CodeUnit.PLATE_COMMENT,
                "Queues per-voice parameters and command $0B through $2140-$2143.");
            createLabel(toAddr(0x9DEA), "setup_menu_input_dispatch", true);
            listing.setComment(toAddr(0x9DEA), CodeUnit.PLATE_COMMENT,
                "Shared Game Setup controller dispatch reached before main-page and Rules/Options " +
                "row/value handlers. Main-page Left/Right emits command $49 and updates the " +
                "16-bit working values at $7E:16FB/$16FD/$16FF/$1701.");
            createLabel(toAddr(0xA62D), "game_setup_main_row_state", true);
            listing.setComment(toAddr(0xA62D), CodeUnit.PLATE_COMMENT,
                "Selects the six-row main Game Setup cursor state. Up/Down wraps across Mode, " +
                "Style, Level, Quarter, Set Rules, and Set Options and emits command $4A.");
            createLabel(toAddr(0xA77C), "game_setup_main_value_dispatch", true);
            listing.setComment(toAddr(0xA77C), CodeUnit.PLATE_COMMENT,
                "Dispatches the selected main-page value for the BG3 proportional-glyph writer. " +
                "Mesen confirms cycles Exhibition/Season/Playoffs/Load Series; " +
                "Arcade/Simulation/Custom; Rookie/Starter/All-Star; and 3/5/8/12 Minutes.");
        } else if (bank == 0x81) {
            createLabel(toAddr(0xF9F1), "game_setup_hdma_window_init", true);
            listing.setComment(toAddr(0xF9F1), CodeUnit.PLATE_COMMENT,
                "Initializes HDMA channel 7 for the selected-row color-math window.");
            createLabel(toAddr(0xD318), "set_rules_frame", true);
            createLabel(toAddr(0xD3B1), "set_rules_move_cursor", true);
            createLabel(toAddr(0xD446), "set_rules_decrement", true);
            createLabel(toAddr(0xD47A), "set_rules_write_working_value", true);
            createLabel(toAddr(0xD491), "set_rules_mark_dirty", true);
            createLabel(toAddr(0xD4C0), "set_rules_increment", true);
            createLabel(toAddr(0xD516), "set_rules_confirm_copy", true);
            createLabel(toAddr(0xD59B), "set_rules_redraw", true);
            createLabel(toAddr(0xD60E), "set_rules_redraw_discrete_value", true);
            createLabel(toAddr(0xD675), "set_rules_draw_value", true);
            createLabel(toAddr(0x9756), "setup_draw_proportional_text", true);
            createLabel(toAddr(0x9FD4), "setup_draw_proportional_text_wrapper", true);
            createLabel(toAddr(0xA1EE), "setup_upload_redrawn_bg3", true);
            createLabel(toAddr(0xA28E), "setup_queue_redrawn_bg3", true);
            createLabel(toAddr(0xA2D3), "setup_draw_slider_objects", true);
            listing.setComment(toAddr(0xD47A), CodeUnit.PLATE_COMMENT,
                "Common Rules value path stores the selected 16-bit value at the working array $7E:16FB + row*2. $81:D491 marks $7E:17AD = 2; Start at $81:D516 copies all 26 bytes to committed Rules $7E:17D1.");
            listing.setComment(toAddr(0xD491), CodeUnit.PLATE_COMMENT,
                "Marks Rules state 2; the common dispatcher then reaches the shared $80:A3B8 " +
                "exit/build/entrance transition instead of swapping pages in one frame.");
            listing.setComment(toAddr(0xD59B), CodeUnit.PLATE_COMMENT,
                "Rules redraw/viewport dispatcher. $81:D59B-$D5AB compares the logical row " +
                "$1693 with 2 and clears slider-object enable $1639 for every discrete row. " +
                "Only rows 0/1 may retain the two foul-meter objects while visible.");
            listing.setComment(toAddr(0xD60E), CodeUnit.PLATE_COMMENT,
                "Rules rows >=2 select their ON/OFF string, call $81:9FD4, set transfer " +
                "length $0800 at $196E, and call $81:A28E. This replaces the BG3 text " +
                "canvas; it does not paint a short value over the old one.");
            listing.setComment(toAddr(0x9756), CodeUnit.PLATE_COMMENT,
                "Shared proportional menu-text renderer used by Game Setup, Rules, and " +
                "Options. It consumes the selected ROM string and writes its complete 2bpp " +
                "glyph output into the mutable BG3 canvas before the caller uploads it.");
        } else if (bank == 0x82) {
            createLabel(toAddr(0x8CD1), "set_options_frame", true);
            createLabel(toAddr(0x8CD9), "set_options_confirm_copy", true);
            createLabel(toAddr(0x8D0A), "set_options_commit_copy", true);
            createLabel(toAddr(0x8D3C), "set_options_move_cursor", true);
            createLabel(toAddr(0x8DA6), "set_options_decrement", true);
            createLabel(toAddr(0x8DC6), "set_options_write_working_value", true);
            createLabel(toAddr(0x8E73), "set_options_increment", true);
            createLabel(toAddr(0x8F9C), "set_options_redraw", true);
            createLabel(toAddr(0x9028), "set_options_draw_value", true);
            createLabel(toAddr(0x902F), "set_options_prepare_slider_objects", true);
            listing.setComment(toAddr(0x8DC6), CodeUnit.PLATE_COMMENT,
                "Common Options value path stores the selected 16-bit value at working array $7E:16FB + row*2. Start through $82:8CD9/$82:8D0A copies all 14 bytes to committed Options $7E:17B5.");
            listing.setComment(toAddr(0x8CD1), CodeUnit.PLATE_COMMENT,
                "Options frame dispatcher. Open/Start state changes return through the shared " +
                "$80:A3B8 page exit/build/entrance sequence; this is not an immediate page swap.");
            listing.setComment(toAddr(0x8F9C), CodeUnit.PLATE_COMMENT,
                "Options redraw dispatcher. Rows 0/1 branch to $82:902F for slider objects. " +
                "Rows >=2 select a string, call the proportional-text wrapper $81:9FD4, then " +
                "call $81:A1EE with length $0800 to upload the redrawn BG3 canvas as a unit.");
            listing.setComment(toAddr(0x902F), CodeUnit.PLATE_COMMENT,
                "Options rows 0/1 only: seeds the slider object's rectangle and invokes " +
                "$87:8A62. Discrete text rows do not use this helper; they flow through " +
                "$81:9FD4 and the $0800-byte BG3 upload at $81:A1EE.");
        } else if (bank == 0x87) {
            createLabel(toAddr(0x8C2D), "set_options_apply_live_volume", true);
            listing.setComment(toAddr(0x8C2D), CodeUnit.PLATE_COMMENT,
                "Called from $82:8DDC for Options rows 0 and 1 after a changed slider value; applies the live Music/SFX volume setting.");
        }

        File listingOut = new File(outDir, "setup_bank" + bankStr + "_listing.txt");
        try (PrintWriter w = new PrintWriter(listingOut, "UTF-8")) {
            w.printf("NBA Live '95 (USA) - bank $%s routines executing on the Game Setup screen%n", bankStr);
            w.println("Ranges captured from a live Mesen exec trace; entry points are real, not inferred.");
            w.println();
            for (Range r : ranges) {
                // extend a little past the traced end to capture the tail of each block
                dumpRange(w, bankStr, r.start, Math.min(0xFFFF, r.end + 0x30));
            }
        }
        println("Wrote listing: " + listingOut.getAbsolutePath());

        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        File cOut = new File(outDir, "setup_bank" + bankStr + "_decomp.c");
        try (PrintWriter w = new PrintWriter(cOut, "UTF-8")) {
            w.printf("/* NBA Live '95 (USA) - decompiled bank $%s Game Setup routines */%n%n", bankStr);
            for (Range r : ranges) {
                Function f = getFunctionAt(toAddr(r.start));
                if (f == null) continue;
                DecompileResults res = decomp.decompileFunction(f, 60, monitor);
                w.printf("/* ===== $%s:%04X ===== */%n", bankStr, r.start);
                if (res != null && res.decompileCompleted() && res.getDecompiledFunction() != null) {
                    w.println(res.getDecompiledFunction().getC());
                } else {
                    w.printf("/* decompilation failed: %s */%n",
                        res == null ? "no result" : res.getErrorMessage());
                }
                w.println();
            }
        }
        decomp.dispose();
        println("Wrote decompilation: " + cOut.getAbsolutePath());
    }
}
