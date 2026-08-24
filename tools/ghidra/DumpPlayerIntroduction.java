// Disassemble and decompile the post-Player Setup matchup/lineup paths proven
// by tools/mesen_gameplay_player_capture.lua with NBA95_PLAYER_INTRO_TRACE=1.
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

public class DumpPlayerIntroduction extends GhidraScript {
    private static class Range {
        long start, end;
        String name, comment;
        Range(long start, long end, String name, String comment) {
            this.start = start; this.end = end;
            this.name = name; this.comment = comment;
        }
    }

    private Range[] rangesFor(int bank) {
        if (bank == 0x80) return new Range[] {
            new Range(0xB344, 0xB378, "intro_build_screen_object",
                "Shared object builder reached while the matchup comparison and lineup labels are assembled."),
            new Range(0xBBA8, 0xBD1A, "intro_decode_planar_graphics",
                "Planar graphics decoder active immediately before each Starting Lineup portrait swap."),
            new Range(0xBD1B, 0xBE10, "expand_fb46_graphics_command",
                "FB46 compressed-graphics command handler used by the lineup palette/tile construction stream."),
            new Range(0xC62B, 0xC67D, "dispatch_compressed_graphics_stream",
                "Compressed graphics dispatcher. At each lineup swap it reaches $80:C633 then dispatches FB46 to $80:BD1B; source is $AE:DB76 for the visitor and $AB:FDE2 for the home team."),
            new Range(0xE95B, 0xEA98, "run_menu_transition_script",
                "Shared PPU transition-script interpreter used when leaving Player Setup.")
        };
        if (bank == 0x81) return new Range[] {
            new Range(0x9756, 0x979C, "intro_update_object_position",
                "Shared post-setup presentation positioning helper reached by matchup/lineup object scripts."),
            new Range(0x9F54, 0xA03C, "intro_object_motion_wrappers",
                "Object-script wrappers that feed the presentation position helper."),
            new Range(0xA1E7, 0xA241, "intro_copy_portrait_palette",
                "Copies the three portrait palette groups; live at the first card and every starter swap."),
            new Range(0xA489, 0xA520, "post_setup_scene_dispatch",
                "Dispatcher shared by Player Setup and the following matchup presentation states.")
        };
        if (bank == 0x83) return new Range[] {
            new Range(0xF790, 0xF858, "intro_court_presentation_update",
                "Court-backed matchup and lineup presentation controller observed in the broad post-setup trace."),
            new Range(0xF891, 0xF8FD, "intro_draw_matchup_panels",
                "Builds both team-logo objects and their gold plates for the matchup/ratings presentation."),
            new Range(0xF901, 0xFA90, "intro_build_rating_balls",
                "Builds the five team-comparison rows. Live OAM proves ranks 1-8 produce three basketballs, 9-18 two, and 19-27 one; the six ball tiles advance on the $F7B3/$F7BE twelve-frame divider.")
        };
        if (bank == 0x87) return new Range[] {
            new Range(0xBD7F, 0xC0AB, "run_player_introduction",
                "Starting Lineup presentation. Initialization reaches the internal $87:BE92 loop; the function ends at the $87:C0AB RTL. Live card changes are 434 frames apart and the visitor/home boundary follows the fifth starter.")
        };
        return new Range[0];
    }

    private void dumpRange(PrintWriter out, String bank, Range range) {
        Listing listing = currentProgram.getListing();
        Address end = toAddr(range.end);
        out.printf("--- $%s:%04X-$%s:%04X %s ---%n",
            bank, range.start, bank, range.end, range.name);
        out.printf("; %s%n", range.comment);
        Instruction ins = listing.getInstructionAt(toAddr(range.start));
        if (ins == null) {
            out.println("(no instructions decoded)"); out.println(); return;
        }
        for (; ins != null && ins.getAddress().compareTo(end) <= 0;
             ins = ins.getNext()) {
            String mnemonic = ins.getMnemonicString();
            String rest = ins.toString();
            rest = rest.length() > mnemonic.length() ?
                rest.substring(mnemonic.length()).trim() : "";
            out.printf("%s:%04X  %-8s %s%n", bank,
                ins.getAddress().getOffset(), mnemonic, rest);
        }
        out.println();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File outDir = new File(args.length > 0 ? args[0] : ".");
        outDir.mkdirs();
        String bankText = args.length > 1 ? args[1] : "87";
        int bank = Integer.parseInt(bankText, 16);
        Range[] ranges = rangesFor(bank);
        Listing listing = currentProgram.getListing();

        for (Range range : ranges) {
            Address start = toAddr(range.start), end = toAddr(range.end);
            clearListing(start, end);
            disassemble(start);
            Function function = listing.getFunctionAt(start);
            if (function == null) function = createFunction(start, range.name);
            if (function != null) {
                function.setName(range.name, SourceType.USER_DEFINED);
                function.setComment(range.comment);
            }
        }

        File listingFile = new File(outDir,
            "player_intro_bank" + bankText + "_listing.txt");
        try (PrintWriter out = new PrintWriter(listingFile)) {
            out.printf("NBA Live '95 (USA) player introduction bank $%s%n%n", bankText);
            for (Range range : ranges) dumpRange(out, bankText, range);
        }

        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        File decompFile = new File(outDir,
            "player_intro_bank" + bankText + "_decomp.c");
        try (PrintWriter out = new PrintWriter(decompFile)) {
            for (Range range : ranges) {
                Function function = listing.getFunctionAt(toAddr(range.start));
                out.printf("/* ===== $%s:%04X %s ===== */%n",
                    bankText, range.start, range.name);
                if (function != null) {
                    DecompileResults result = decompiler.decompileFunction(function, 60, monitor);
                    if (result.decompileCompleted())
                        out.println(result.getDecompiledFunction().getC());
                    else out.println("/* decompilation unavailable */");
                }
                out.println();
            }
        }
        decompiler.dispose();
    }
}
