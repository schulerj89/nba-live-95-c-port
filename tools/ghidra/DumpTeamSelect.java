// Disassemble and decompile the Team Select routines reached by the live
// Mesen capture in tools/mesen_team_select_capture.lua.
//
// args: <analysisDir> <bankHex>

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.symbol.SourceType;

public class DumpTeamSelect extends GhidraScript {
    private static class Range {
        long start, end;
        String name, comment;
        Range(long start, long end, String name, String comment) {
            this.start = start; this.end = end; this.name = name; this.comment = comment;
        }
    }

    private Range[] rangesFor(int bank) {
        if (bank == 0x80) return new Range[] {
            new Range(0xA3B8, 0xA495, "setup_shared_frame_sequencer",
                "Shared Game Setup frame sequencer. On Exhibition Start the live PPU trace " +
                "shows BG3 leaving at 14 pixels/frame, then BG1/BG2 separating at 8 " +
                "pixels/frame under the INIDISP ramp. It remains active through forced blank; " +
                "$80:DBF6 dispatches Team Select only afterward."),
            new Range(0xB344, 0xB480, "draw_rom_object_group",
                "Shared ROM object-group renderer called twice per Team Select side by $81:AA73: " +
                "A=$2800 draws the variable team logo, then object group $22 uses A=$2200 for " +
                "inactive silver or A=$2400 for selected gold."),
            new Range(0xDBE8, 0xDC05, "main_dispatch_team_select",
                "Main scene dispatcher branch observed after Exhibition is confirmed. " +
                "$80:DBF6 calls $82:809A, the Team Select scene entry/update routine.")
        };
        if (bank == 0x81) return new Range[] {
            new Range(0x9FDF, 0xA05E, "team_select_centered_object_text",
                "Shared text/object wrapper used during matchup construction."),
            new Range(0xA05F, 0xA1ED, "team_select_seed_object_grid",
                "Called twice by $82:81BD-$81D0 and $82:82AF-$82CC while Team Select " +
                "seeds the two matchup-side object grids."),
            new Range(0xA975, 0xA980, "team_select_object_list_begin",
                "Begins the object-list update around each $81:AA73 matchup draw."),
            new Range(0xA981, 0xAA33, "team_select_object_list_commit",
                "Commits the object-list update after each $81:AA73 matchup draw."),
            new Range(0xAA34, 0xAA72, "team_select_object_list_reset",
                "Clears/resets the shared object-list state before Team Select construction."),
            new Range(0xAA73, 0xAB80, "team_select_draw_object_list",
                "Called with A=3 by $82:834C and $82:88F4 to draw both matchup sides. Each " +
                "side calls $80:B344 with A=$2800 for its variable logo and again for object " +
                "group $22 using A=$2200 inactive silver or A=$2400 selected gold. Live OAM " +
                "proves group $22 is a fixed 15-piece OBJ grid.")
        };
        if (bank == 0x82) return new Range[] {
            new Range(0x809A, 0x838D, "team_select_scene",
                "Team Select scene entry, forced-blank graphics construction, and per-frame dispatch."),
            new Range(0x838E, 0x83BA, "team_select_frame",
                "Per-frame Team Select controller/render dispatcher observed in every settled frame."),
            new Range(0x83BC, 0x8405, "team_select_side_input",
                "Face/shoulder-button path. Live L-button proof switches the active side from the " +
                "default right team to the left team before the shared redraw."),
            new Range(0x8406, 0x84F6, "team_select_direction_input",
                "Directional controller handler. Live isolated traces branch through $840B for Up, " +
                "$845E for Down, and $8477 for Left."),
            new Range(0x84F7, 0x85B3, "team_select_right_confirm_input",
                "Right-direction and face/shoulder-button continuation reached from $82:8474."),
            new Range(0x85B4, 0x85D0, "team_select_select_button",
                "Select-button branch from the common controller handler."),
            new Range(0x85D1, 0x88D8, "team_select_redraw",
                "Rebuilds the selected team name, ROM logo objects, AI marker, and ranking columns after a change. " +
                "The right/home branch $863C-$8792 writes the name through $81:A01F from X=$00B4; " +
                "the left/visitor branch $8793-$88D8 uses $81:9FD4 from X=$0050."),
            new Range(0x88D9, 0x8903, "team_select_draw_matchup",
                "Draws both matchup sides by temporarily selecting side 1 or 2. Live WRAM snapshots show " +
                "team IDs at $7E:16FB/$16FD and the active side at $7E:16B5. The right team remains the " +
                "home-team source for the screen wallpaper/palette while the visitor changes."),
            new Range(0x8933, 0x8967, "team_select_animate_selected_plate",
                "Executed every Team Select frame. $1805 selects one of seven overlapping 14-byte " +
                "windows at $82:8968, changing once per eight frames; the window is queued to CGRAM " +
                "$A1-$A7 while OAM remains unchanged.")
        };
        if (bank == 0x87) return new Range[] {
            new Range(0x89D5, 0x89E8, "advance_mod3_frame_divider",
                "Shared per-frame divider observed in the Team Select call chain. It increments $168F " +
                "modulo three and advances $0613 on rollover. This is the separate background-motion " +
                "cadence; selected-plate palette timing comes directly from $82:8933/$1805.")
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
        String bankText = args.length > 1 ? args[1] : "82";
        int bank = Integer.parseInt(bankText, 16);
        Range[] ranges = rangesFor(bank);
        Listing listing = currentProgram.getListing();

        for (Range range : ranges) {
            Address entry = toAddr(range.start);
            addEntryPoint(entry);
            disassemble(entry);
            Function function = getFunctionAt(entry);
            if (function == null) function = createFunction(entry, range.name);
            else function.setName(range.name, ghidra.program.model.symbol.SourceType.USER_DEFINED);
            listing.setComment(entry, CodeUnit.PLATE_COMMENT, range.comment);
        }

        File listingFile = new File(outDir, "team_select_bank" + bankText + "_listing.txt");
        try (PrintWriter out = new PrintWriter(listingFile, "UTF-8")) {
            out.printf("NBA Live '95 (USA) Team Select bank $%s%n%n", bankText);
            for (Range range : ranges) dumpRange(out, bankText, range);
        }

        if (bank == 0x80) {
            createLabel(toAddr(0xD9AF), "team_ranking_table", true);
            listing.setComment(toAddr(0xD9AF), CodeUnit.PLATE_COMMENT,
                "34 records x 5 bytes: Scoring, Rebounds, Ball Control, Defense, Overall. " +
                "IDs 0-26 are ranked league teams; East/West are IDs 27/28 and their " +
                "out-of-league values render as dashes. Bonus teams follow at IDs 29-33.");
            String[] teams = { "Atlanta", "Boston", "Charlotte", "Chicago", "Cleveland",
                "Dallas", "Denver", "Detroit", "Golden State", "Houston", "Indiana",
                "L.A. Clippers", "L.A. Lakers", "Miami", "Milwaukee", "Minnesota",
                "New Jersey", "New York", "Orlando", "Philadelphia", "Phoenix", "Portland",
                "Sacramento", "San Antonio", "Seattle", "Utah", "Washington",
                "East", "West", "-Slammers-", "-Blockers-", "-Jammers-", "-Stealers-",
                "EAC Hitmen" };
            File rankingsFile = new File(outDir, "team_rankings.txt");
            try (PrintWriter out = new PrintWriter(rankingsFile, "UTF-8")) {
                out.println("ROM table $80:D9AF, records are scoring rebounds ball-control defense overall");
                for (int team = 0; team < teams.length; ++team) {
                    Address record = toAddr(0xD9AF + team * 5L);
                    out.printf("%02X %-14s", team, teams[team]);
                    for (int column = 0; column < 5; ++column)
                        out.printf(" %02d", currentProgram.getMemory().getByte(record.add(column)) & 0xFF);
                    out.println();
                }
            }
        }

        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        File decompFile = new File(outDir, "team_select_bank" + bankText + "_decomp.c");
        try (PrintWriter out = new PrintWriter(decompFile, "UTF-8")) {
            for (Range range : ranges) {
                Function function = getFunctionAt(toAddr(range.start));
                out.printf("/* ===== $%s:%04X %s ===== */%n", bankText, range.start, range.name);
                if (function != null) {
                    DecompileResults result = decompiler.decompileFunction(function, 60, monitor);
                    if (result != null && result.decompileCompleted() && result.getDecompiledFunction() != null)
                        out.println(result.getDecompiledFunction().getC());
                    else out.println("/* decompilation failed */");
                }
                out.println();
            }
        }
        decompiler.dispose();
        println("Wrote Team Select analysis to " + outDir.getAbsolutePath());
    }
}
