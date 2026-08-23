// Disassemble and decompile the Player Setup paths reached by the live
// tools/mesen_gameplay_player_capture.lua trace.
//
// args: <analysisDir> <bankHex>

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.symbol.SourceType;

public class DumpPlayerSetup extends GhidraScript {
    private static class Range {
        long start, end;
        String name, comment;
        Range(long start, long end, String name, String comment) {
            this.start = start; this.end = end; this.name = name; this.comment = comment;
        }
    }

    private Range[] rangesFor(int bank) {
        if (bank == 0x80) return new Range[] {
            new Range(0xDB60, 0xDC88, "main_matchup_scene_dispatch",
                "Main dispatcher around Team Select completion and the following matchup screens."),
            new Range(0xE95B, 0xEA98, "run_menu_transition_script",
                "Shared transition-script interpreter called with the $81:B901 script when Team Select confirms.")
        };
        if (bank == 0x81) return new Range[] {
            new Range(0xA489, 0xA520, "player_setup_scene_dispatch",
                "Main dispatcher calls this immediately after the $81:B901 transition script; live trace frame 1701 then reaches $81:B404."),
            new Range(0xB404, 0xB422, "player_setup_patch_object_pool",
                "Player Setup construction helper first reached at live trace frame 1701; replaces $9147 object markers in the $70:0058 pool."),
            new Range(0xB493, 0xB544, "player_setup_position_controller",
                "Positions one controller assignment relative to its side; callers pass controller indices and this local helper returns with RTS."),
            new Range(0xB546, 0xB605, "player_setup_animate_panel_palette",
                "Five-frame palette cadence for the selected panel; writes the animated colors through CGADD/CGDATA ($2121/$2122)."),
            new Range(0xB62C, 0xB77A, "player_setup_redraw_assignment",
                "Synchronizes controller ownership and rebuilds the Player label/controller objects. Live construction reaches $B719 at frame 1795."),
            new Range(0xB7C1, 0xB7E0, "player_setup_vscroll_irq",
                "Player Setup vertical-scroll IRQ handler, first enabled at live trace frame 1816."),
            new Range(0xC41E, 0xC4C8, "menu_fade_out",
                "Fade/forced-blank helper called by Team Select confirmation before the $81:B901 transition script."),
        };
        return new Range[0];
    }

    private void dumpRange(PrintWriter out, String bank, Range range) {
        Listing listing = currentProgram.getListing();
        Address end = toAddr(range.end);
        out.printf("--- $%s:%04X-$%s:%04X %s ---%n", bank, range.start, bank, range.end, range.name);
        out.printf("; %s%n", range.comment);
        Instruction ins = listing.getInstructionAt(toAddr(range.start));
        if (ins == null) {
            out.println("(no instructions decoded)"); out.println(); return;
        }
        for (; ins != null && ins.getAddress().compareTo(end) <= 0; ins = ins.getNext()) {
            String mnemonic = ins.getMnemonicString();
            String rest = ins.toString();
            rest = rest.length() > mnemonic.length() ? rest.substring(mnemonic.length()).trim() : "";
            out.printf("%s:%04X  %-8s %s%n", bank, ins.getAddress().getOffset(), mnemonic, rest);
        }
        out.println();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File outDir = new File(args.length > 0 ? args[0] : ".");
        outDir.mkdirs();
        String bankText = args.length > 1 ? args[1] : "81";
        int bank = Integer.parseInt(bankText, 16);
        Range[] ranges = rangesFor(bank);
        Listing listing = currentProgram.getListing();

        for (Range range : ranges) {
            Address start = toAddr(range.start);
            Address end = toAddr(range.end);
            clearListing(start, end);
            disassemble(start);
            Function function = listing.getFunctionAt(start);
            if (function == null) function = createFunction(start, range.name);
            if (function != null) {
                function.setName(range.name, SourceType.USER_DEFINED);
                function.setComment(range.comment);
            }
        }

        File listingFile = new File(outDir, "player_setup_bank" + bankText + "_listing.txt");
        try (PrintWriter out = new PrintWriter(listingFile)) {
            out.printf("NBA Live '95 (USA) Player Setup bank $%s%n%n", bankText);
            for (Range range : ranges) dumpRange(out, bankText, range);
        }

        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        File decompFile = new File(outDir, "player_setup_bank" + bankText + "_decomp.c");
        try (PrintWriter out = new PrintWriter(decompFile)) {
            for (Range range : ranges) {
                Function function = listing.getFunctionAt(toAddr(range.start));
                out.printf("/* ===== $%s:%04X %s ===== */%n", bankText, range.start, range.name);
                if (function != null) {
                    DecompileResults result = decompiler.decompileFunction(function, 60, monitor);
                    if (result.decompileCompleted()) out.println(result.getDecompiledFunction().getC());
                    else out.println("/* decompilation unavailable */");
                }
                out.println();
            }
        }
        decompiler.dispose();
    }
}
