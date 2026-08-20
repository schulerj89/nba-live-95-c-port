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
                "Builds the Game Setup BG1/BG2/BG3 VRAM state while forced blank is active; " +
                "releases the first visible entrance frame with BG1/BG2 scrolls at 768.");
            createLabel(toAddr(0xA3B8), "game_setup_frame_update", true);
            listing.setComment(toAddr(0xA3B8), CodeUnit.PLATE_COMMENT,
                "Drives the 32-frame BG1/BG2 slide, 1..15 INIDISP ramp, delayed BG3 vertical " +
                "entrance, and steady BG2 scroll.");
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
                "Shared Game Setup controller dispatch reached before Rules/Options row and value handlers.");
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
            createLabel(toAddr(0xD675), "set_rules_draw_value", true);
            listing.setComment(toAddr(0xD47A), CodeUnit.PLATE_COMMENT,
                "Common Rules value path stores the selected 16-bit value at the working array $7E:16FB + row*2. $81:D491 marks $7E:17AD = 2; Start at $81:D516 copies all 26 bytes to committed Rules $7E:17D1.");
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
            listing.setComment(toAddr(0x8DC6), CodeUnit.PLATE_COMMENT,
                "Common Options value path stores the selected 16-bit value at working array $7E:16FB + row*2. Start through $82:8CD9/$82:8D0A copies all 14 bytes to committed Options $7E:17B5.");
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
