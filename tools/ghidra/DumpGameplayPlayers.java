// Disassemble/decompile gameplay player construction reached by
// tools/mesen_gameplay_player_capture.lua.
// args: <analysisDir> <bankHex>

import java.io.File;
import java.io.BufferedReader;
import java.io.FileReader;
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

public class DumpGameplayPlayers extends GhidraScript {
    private static class Range {
        long start, end;
        String name, comment;
        Range(long start, long end, String name, String comment) {
            this.start = start; this.end = end; this.name = name; this.comment = comment;
        }
    }

    private Range[] rangesFor(int bank) {
        if (bank == 0x80) return new Range[] {
            new Range(0x82A3, 0x83F0, "ppu_stream_queued_vram",
                "Live gameplay writes to $2118/$2119 execute here. The player routines prepare " +
                "dynamic sprite tiles before this shared PPU upload path runs."),
            new Range(0x8A02, 0x8BD0, "gameplay_copy_rom_palette_to_cgram",
                "Live Mesen proves $80:8A3F writes CGRAM $2122 while $80:8BCC reads " +
                "the selected ROM palette stream. This is the gameplay player/court palette loader."),
            new Range(0x985F, 0x99A0, "gameplay_decompress_player_graphics",
                "Live Mesen reads the compressed player graphics streams at $80:985F, " +
                "$80:986F, $80:98B2, and $80:98BB before the gameplay OBJ upload."),
            new Range(0xACC2, 0xAF1D, "gameplay_resolve_player_animation_parts",
                "Live pre-tip execution reaches this immediately before the queue writer. It " +
                "walks the selected player's animation/resource descriptors and supplies the " +
                "ROM part pointer consumed by $80:B348/$B679."),
            new Range(0xB300, 0xB7D0, "gameplay_queue_player_sprite_parts",
                "Live Mesen proves this routine reads the ROM sprite-part descriptors and " +
                "writes queued source pointer, byte count, and VRAM destination records. " +
                "$80:B736-$B7BA writes the exact head/body sources consumed at $80:82A3.")
        };
        if (bank == 0x85) return new Range[] {
            new Range(0x8C4F, 0x8DAF, "gameplay_build_player_and_number_palettes",
                "Live Mesen reaches this routine immediately before player OBJ loading. " +
                "$85:8DFB reads the first five-color ingredient at $AF:F022 and $85:8E10 " +
                "reads the uniform ingredient at $AF:F042. $85:8CAE-$8CF7 copies the " +
                "64-byte $AF:E99F block, patches team colors, and uploads OBJ palettes 6/7; " +
                "palette 7 is selected by the jersey overlay at $80:AE86."),
            new Range(0x8DB0, 0x8E1B, "gameplay_expand_player_palette_set",
                "Called once for each side from $85:8C6E/$8C95. It resolves the selected " +
                "team/player palette resource, expands three 16-color palettes to WRAM, then " +
                "overlays the proven skin/hair and uniform ingredients from $AF:F01C/F03C."),
            new Range(0xBC07, 0xBD80, "gameplay_expand_player_sprite_resources",
                "Called for both five-player source lists after appearance sorting."),
            new Range(0xC37D, 0xC500, "gameplay_configure_controlled_player",
                "Called from $86:E102 while configuring the controlled-player resource slot.")
        };
        if (bank == 0x86) return new Range[] {
            new Range(0xBC9B, 0xBD30, "gameplay_bind_player_sprite_resources",
                "Called with the left/right player-record tables and sorted appearance lists; " +
                "binds each player slot to its sprite resource descriptor."),
            new Range(0xD5DB, 0xD650, "gameplay_prepare_player_sprite_sources",
                "Executed immediately before the appearance-key sort during live player loading; " +
                "prepares source descriptors consumed by the later graphics DMA path."),
            new Range(0xD723, 0xD73D, "gameplay_sort_player_appearance_wrapper",
                "Initializes the destination list and invokes the five-player appearance sort."),
            new Range(0xD73E, 0xD7B7, "gameplay_resolve_player_sprite_parts",
                "Consumes five (appearance-key, player-slot) pairs prepared at $86:D8F1-$D975 " +
                "or $86:D97E-$DA07 and resolves the player graphics used by the gameplay loader."),
            new Range(0xD7B8, 0xD85D, "gameplay_build_player_record_pointers",
                "Prepares the ten active player record pointers later dereferenced at +$36/+$37."),
            new Range(0xD85E, 0xDA20, "gameplay_build_player_appearance",
                "Executed during the pre-game load. Live Mesen reads player-record appearance " +
                "selectors at +$36/+$37 from $86:D924/$D92A for the left team and " +
                "$86:D9B1/$D9B7 for the right team."),
            new Range(0xE0B0, 0xE207, "gameplay_build_player_sprite_resource_lists",
                "Called immediately after both five-player appearance lists are sorted. It " +
                "builds/deduplicates the sprite resource descriptors before the PPU upload."),
            new Range(0xE208, 0xE389, "gameplay_initialize_player_appearance_tables",
                "Initializes the ten-player appearance/resource tables during the live pre-game load.")
        };
        if (bank == 0x87) return new Range[] {
            new Range(0xA47A, 0xA98D, "gameplay_prepare_player_sprite_draw",
                "Live Mesen reaches $87:A6A4 immediately before $80:AD92. It supplies " +
                "the OBJ attribute word whose palette bits select one of the three " +
                "team/player palettes for the current active player. $87:A4E1-$A6A4 " +
                "also selects the direction-specific jersey overlay resource from $87:A98E."),
            new Range(0xAB38, 0xADA0, "gameplay_advance_player_animation_resources",
                "The $87:AB38 prefix selects the lower-body animation descriptor. " +
                "Live Mesen WRAM write tracing proves $87:AC3D writes the current " +
                "lower-body resource to player +$2C and $87:AC98 writes the upper-body " +
                "resource to +$2A. $87:AD5A applies a special upper-body override. " +
                "This is the resource-frame updater used to reconstruct Player Lab animations."),
            new Range(0xAF75, 0xB450, "gameplay_assign_player_appearance_resources",
                "Live Mesen reaches this while constructing each active-player WRAM record. " +
                "$87:B032/$B03D write the 16-bit sprite/head resource-set base to player " +
                "record +$2E. Live ROM reads also prove $87:B36B reads roster byte +$00 " +
                "(jersey number), $87:B378 maps it through the BCD table, and " +
                "$87:B05B-$B354 composites the three number views from $A6:AFD6.")
        };
        return new Range[0];
    }

    private void dumpRange(PrintWriter out, String bank, Range range) {
        Listing listing = currentProgram.getListing();
        Address end = toAddr(range.end);
        out.printf("--- $%s:%04X-$%s:%04X %s ---%n", bank, range.start, bank,
            range.end, range.name);
        out.printf("; %s%n", range.comment);
        Instruction ins = listing.getInstructionAt(toAddr(range.start));
        if (ins == null) { out.println("(no instructions decoded)\n"); return; }
        for (; ins != null && ins.getAddress().compareTo(end) <= 0; ins = ins.getNext()) {
            String mnemonic = ins.getMnemonicString();
            String rest = ins.toString();
            rest = rest.length() > mnemonic.length() ?
                rest.substring(mnemonic.length()).trim() : "";
            out.printf("%s:%04X  %-8s %s%n", bank, ins.getAddress().getOffset(),
                mnemonic, rest);
        }
        out.println();
    }

    private int readU16(long address) throws Exception {
        int lo = currentProgram.getMemory().getByte(toAddr(address)) & 0xff;
        int hi = currentProgram.getMemory().getByte(toAddr(address + 1)) & 0xff;
        return lo | (hi << 8);
    }

    private void dumpAnimationTables(PrintWriter out) throws Exception {
        long[] tables = { 0xC218, 0xC28A, 0xC2FC };
        String[] names = { "lower_home", "lower_away", "upper" };
        out.println("--- ROM player animation descriptor tables ---");
        out.println("; Consumed by $87:AB38-$AD5A; $80:AD92-$AEC1 composes lower, upper, head.");
        for (int tableIndex = 0; tableIndex < tables.length; ++tableIndex) {
            out.printf("%s table $84:%04X%n", names[tableIndex], tables[tableIndex]);
            for (int state = 0; state < 57; ++state) {
                int pointer = readU16(tables[tableIndex] + state * 2L);
                if (pointer < 0x8000) continue;
                int mode = readU16(pointer);
                int timing = readU16(pointer + 4);
                int frames = readU16(pointer + 6);
                out.printf("  state $%02X -> $84:%04X mode=$%04X timing=$%04X frames=%d dirs=",
                    state, pointer, mode, timing, frames);
                for (int direction = 0; direction < 8; ++direction) {
                    int list = readU16(pointer + 8 + direction * 2L);
                    out.printf("%s$%04X", direction == 0 ? "" : ",", list);
                }
                out.println();
            }
        }
        out.print("head direction offsets $84:C36E = ");
        for (int direction = 0; direction < 8; ++direction)
            out.printf("%s%d", direction == 0 ? "" : ",",
                readU16(0xC36E + direction * 2L));
        out.println("\n");
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File outDir = new File(args.length > 0 ? args[0] : ".");
        outDir.mkdirs();
        String bankText = args.length > 1 ? args[1] : "86";
        int bank = Integer.parseInt(bankText, 16);
        Range[] ranges = rangesFor(bank);
        Listing listing = currentProgram.getListing();

        // The preceding RTL and 65816 M/X state keep recursive flow analysis
        // from discovering this routine. Seed only addresses Mesen executed.
        if ((bank == 0x80 || bank == 0x85 || bank == 0x86 || bank == 0x87) && args.length > 2) {
            File trace = new File(args[2]);
            if (trace.exists()) {
                try (BufferedReader reader = new BufferedReader(new FileReader(trace))) {
                    String line;
                    while ((line = reader.readLine()) != null) {
                        String prefix = String.format("%02X", bank);
                        if (!line.startsWith(prefix) || line.length() < 8) continue;
                        long address = Long.parseLong(line.substring(2, 6), 16);
                        if ((bank == 0x85 && address >= 0xBC00 && address <= 0xC500) ||
                            (bank == 0x86 && ((address >= 0xBC9B && address <= 0xBD30) ||
                                             (address >= 0xD5DB && address <= 0xE400))) ||
                            (bank == 0x87 && ((address >= 0xA47A && address <= 0xA98D) ||
                                             (address >= 0xAB38 && address <= 0xADA0) ||
                                             (address >= 0xAF75 && address <= 0xB450))) ||
                            (bank == 0x80 && ((address >= 0x8A00 && address <= 0x8C20) ||
                                             (address >= 0x9800 && address <= 0x99A0) ||
                                             (address >= 0xACC2 && address <= 0xAF1D) ||
                                             (address >= 0xB300 && address <= 0xB7D0))) ||
                            (bank == 0x85 && address >= 0x8C4F && address <= 0x8E54))
                            disassemble(toAddr(address));
                    }
                }
            }
        }

        for (Range range : ranges) {
            Address entry = toAddr(range.start);
            addEntryPoint(entry); disassemble(entry);
            Function function = getFunctionAt(entry);
            if (function == null) function = createFunction(entry, range.name);
            else function.setName(range.name, SourceType.USER_DEFINED);
            listing.setComment(entry, CodeUnit.PLATE_COMMENT, range.comment);
        }

        if (bank == 0x84) {
            Address table = toAddr(0xE640);
            createLabel(table, "team_roster_pointer_table", true);
            listing.setComment(table, CodeUnit.PLATE_COMMENT,
                "Four-byte entries: reserved byte followed by a 24-bit roster-block pointer. " +
                "Each roster block begins with twelve 16-bit relative player-record offsets.");
            createLabel(toAddr(0xC218), "player_lower_animation_home_table", true);
            createLabel(toAddr(0xC28A), "player_lower_animation_away_table", true);
            createLabel(toAddr(0xC2FC), "player_upper_animation_state_table", true);
            createLabel(toAddr(0xC36E), "player_head_direction_resource_offsets", true);
            listing.setComment(toAddr(0xC218), CodeUnit.PLATE_COMMENT,
                "$87:AB5F/$87:AB6C select one of the two lower-body tables using player +$32.");
            listing.setComment(toAddr(0xC2FC), CodeUnit.PLATE_COMMENT,
                "$87:AC3D-$AD57 selects upper frames using player +$30 and advances ROM cadence.");
        }

        File listingFile = new File(outDir,
            "gameplay_players_bank" + bankText + "_listing.txt");
        try (PrintWriter out = new PrintWriter(listingFile, "UTF-8")) {
            out.printf("NBA Live '95 (USA) gameplay players bank $%s%n%n", bankText);
            for (Range range : ranges) dumpRange(out, bankText, range);
            if (bank == 0x84) dumpAnimationTables(out);
        }

        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        File decompFile = new File(outDir,
            "gameplay_players_bank" + bankText + "_decomp.c");
        try (PrintWriter out = new PrintWriter(decompFile, "UTF-8")) {
            for (Range range : ranges) {
                Function function = getFunctionAt(toAddr(range.start));
                out.printf("/* ===== $%s:%04X %s ===== */%n", bankText,
                    range.start, range.name);
                DecompileResults result = function == null ? null :
                    decompiler.decompileFunction(function, 60, monitor);
                if (result != null && result.decompileCompleted() &&
                    result.getDecompiledFunction() != null)
                    out.println(result.getDecompiledFunction().getC());
                else out.println("/* decompilation failed */");
                out.println();
            }
        }
    }
}
