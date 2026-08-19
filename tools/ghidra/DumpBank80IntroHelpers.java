// Dump bank $80 helpers called by the EA Sports intro.

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;

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

        long[] entries = { 0x86B0, 0x86DA, 0x8968, 0x898F, 0x8A02, 0x8AD2,
                           0x8FA3, 0x9DF3, 0xAC1B, 0xAC89, 0xB344 };
        String[] names = { "wait_frame", "wait_vblank", "video_reset", "video_enable",
                           "vram_copy", "vram_fill", "tilegroup_draw", "stage_commit",
                           "oam_build_begin", "oam_build_finish", "oam_group_draw" };
        for (int i = 0; i < entries.length; ++i) {
            Address address = toAddr(entries[i]);
            addEntryPoint(address);
            disassemble(address);
            if (getFunctionAt(address) == null) createFunction(address, names[i]);
        }

        try (PrintWriter writer = new PrintWriter(output, "UTF-8")) {
            writer.println("NBA Live '95 (USA) - bank $80 EA intro helpers");
            writer.println();
            dumpInstructions(writer, 0x86B0, 0x8720);
            dumpInstructions(writer, 0x8968, 0x8B30);
            dumpInstructions(writer, 0x8FA3, 0x9120);
            dumpInstructions(writer, 0x9DF3, 0x9F20);
            dumpInstructions(writer, 0xAC1B, 0xAD20);
            dumpInstructions(writer, 0xB344, 0xB440);
        }

        println("Wrote bank $80 helper dump: " + output.getAbsolutePath());
    }
}
