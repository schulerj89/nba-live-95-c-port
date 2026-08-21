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
            new Range(0xDBE8, 0xDC05, "main_dispatch_team_select",
                "Main scene dispatcher branch observed after Exhibition is confirmed. " +
                "$80:DBF6 calls $82:809A, the Team Select scene entry/update routine.")
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
                "Rebuilds the selected team name, ROM logo objects, AI marker, and ranking columns after a change."),
            new Range(0x88D9, 0x8967, "team_select_draw_matchup",
                "Draws both matchup sides by temporarily selecting side 1 or 2. Live WRAM snapshots show " +
                "team IDs at $7E:16FB/$16FD and the active side at $7E:16B5.")
        };
        return new Range[0];
    }

    private void dumpRange(PrintWriter out, String bank, Range range) {
        Listing listing = currentProgram.getListing();
        Address end = toAddr(range.end);
        out.printf("--- $%s:%04X-$%s:%04X %s ---%n", bank, range.start, bank, range.end, range.name);
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
                "27 records x 5 bytes: Scoring, Rebounds, Ball Control, Defense, Overall. " +
                "Team IDs are alphabetical and each value is the displayed 1-based league rank.");
            String[] teams = { "Atlanta", "Boston", "Charlotte", "Chicago", "Cleveland",
                "Dallas", "Denver", "Detroit", "Golden State", "Houston", "Indiana",
                "L.A. Clippers", "L.A. Lakers", "Miami", "Milwaukee", "Minnesota",
                "New Jersey", "New York", "Orlando", "Philadelphia", "Phoenix", "Portland",
                "Sacramento", "San Antonio", "Seattle", "Utah", "Washington" };
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
