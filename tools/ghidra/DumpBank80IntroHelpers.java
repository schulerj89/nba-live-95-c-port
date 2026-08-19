// Dump bank $80 helpers called by the EA Sports intro.

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.listing.CodeUnit;

public class DumpBank80IntroHelpers extends GhidraScript {
    private void dumpInstructions(PrintWriter writer, long first, long last) {
        Listing listing = currentProgram.getListing();
        Address start = toAddr(first);
        Address end = toAddr(last);
        writer.printf("Instructions $80:%04X-$80:%04X:%n", first, last);
        for (Instruction instruction = listing.getInstructionAt(start);
             instruction != null && instruction.getAddress().compareTo(end) <= 0;
             instruction = instruction.getNext()) {
            writer.printf("80:%s  %-12s %s%n",
                instruction.getAddress(), instruction.getMnemonicString(),
                instruction.toString().substring(instruction.getMnemonicString().length()).trim());
        }
        writer.println();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File outputDirectory = new File(args.length > 0 ? args[0] : ".");
        outputDirectory.mkdirs();
        File output = new File(outputDirectory, "ea_intro_bank80_helpers.txt");

        long[] entries = { 0x800D, 0x8020, 0x8640, 0x8677, 0x869B, 0x86B0, 0x86DA, 0x893F, 0x8968, 0x898F, 0x8A02, 0x8AD2,
                           0x8FA3, 0x9DF3, 0xAC1B, 0xAC89, 0xB344, 0xDA72, 0xDA91,
                           0xCF1B, 0xCF3B, 0xDD36, 0xDD50, 0xE01E, 0xE8CF, 0xE8D9, 0xFD9E, 0xFEE6 };
        String[] names = { "reset_entry", "startup", "configure_hirq_callback", "set_irq_callback", "set_frame_callback", "wait_frame", "wait_vblank", "display_force_blank", "video_reset", "video_enable",
                           "vram_copy", "vram_fill", "tilegroup_draw", "stage_commit",
                           "oam_build_begin", "oam_build_finish", "oam_group_draw",
                           "main_state_init", "main_state_loop", "master_brightness_fade_out", "master_brightness_fade_in",
                           "title_state_init", "title_fade_in", "title_scene_init", "title_enable_window_mask", "title_95_light_palette_cycle", "nintendo_license",
                           "nba_legal_notice" };
        for (int i = 0; i < entries.length; ++i) {
            Address address = toAddr(entries[i]);
            addEntryPoint(address);
            disassemble(address);
            if (getFunctionAt(address) == null) createFunction(address, names[i]);
        }

        Listing listing = currentProgram.getListing();
        listing.setComment(toAddr(0xE01E), CodeUnit.PLATE_COMMENT,
            "Post-EA title scene. Loads gym/logo graphics, starts a 1000-frame title window, " +
            "and consumes music-synchronization cues from $064A.");
        listing.setComment(toAddr(0xE381), CodeUnit.PRE_COMMENT,
            "Consume and clear $064A. Cue blocks 1..6 construct N, B, A, LIVE, 95, then enable the light cycle; " +
            "7..11 are alternate/repeated transition cues.");
        listing.setComment(toAddr(0xE391), CodeUnit.PRE_COMMENT,
            "Cue 1/7: first staged title element (observed N entry).");
        listing.setComment(toAddr(0xE3DB), CodeUnit.PRE_COMMENT,
            "Cue 2/8: second staged title element (observed B entry).");
        listing.setComment(toAddr(0xE432), CodeUnit.PRE_COMMENT,
            "Cue 3/9: third staged title element (observed A entry).");
        listing.setComment(toAddr(0xE489), CodeUnit.PRE_COMMENT,
            "Cue 4/10/11: LIVE word/reveal stage.");
        listing.setComment(toAddr(0xE4E0), CodeUnit.PRE_COMMENT,
            "Cue 5: 95 logo stage.");
        listing.setComment(toAddr(0xE548), CodeUnit.PRE_COMMENT,
            "Cue 6: enable $80:E8D9 palette cycling for the lights surrounding 95.");
        listing.setComment(toAddr(0xE8CF), CodeUnit.PLATE_COMMENT,
            "HIRQ callback that enables the title window mask at the configured scanline.");
        listing.setComment(toAddr(0xE8D9), CodeUnit.PLATE_COMMENT,
            "Per-frame callback for the 95 lights. Every eight frames it rotates three colors at CGRAM index $79.");

        try (PrintWriter writer = new PrintWriter(output, "UTF-8")) {
            writer.println("NBA Live '95 (USA) - bank $80 EA intro helpers");
            writer.println();
            dumpInstructions(writer, 0x800D, 0x86AF);
            dumpInstructions(writer, 0x86B0, 0x8720);
            dumpInstructions(writer, 0x893F, 0x8B30);
            dumpInstructions(writer, 0x8FA3, 0x9120);
            dumpInstructions(writer, 0x9DF3, 0x9F20);
            dumpInstructions(writer, 0xAC1B, 0xAD20);
            dumpInstructions(writer, 0xB344, 0xB440);
            dumpInstructions(writer, 0xCF1B, 0xCF80);
            dumpInstructions(writer, 0xDA72, 0xDE40);
            dumpInstructions(writer, 0xE01E, 0xE7D0);
            dumpInstructions(writer, 0xE8CF, 0xE95A);
            dumpInstructions(writer, 0xFD9E, 0xFFFF);
        }

        println("Wrote bank $80 helper dump: " + output.getAbsolutePath());
    }
}
