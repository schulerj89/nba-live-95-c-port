// Dump bank $87 routines used by the post-EA title sequence and its audio commands.

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;

public class DumpBank87Title extends GhidraScript {
    private void dumpInstructions(PrintWriter writer, long first, long last) {
        Listing listing = currentProgram.getListing();
        Address start = toAddr(first);
        Address end = toAddr(last);
        writer.printf("Instructions $87:%04X-$87:%04X:%n", first, last);
        for (Instruction instruction = listing.getInstructionAt(start);
             instruction != null && instruction.getAddress().compareTo(end) <= 0;
             instruction = instruction.getNext()) {
            writer.printf("87:%s  %-12s %s%n", instruction.getAddress(),
                instruction.getMnemonicString(),
                instruction.toString().substring(instruction.getMnemonicString().length()).trim());
        }
        writer.println();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File outputDirectory = new File(args.length > 0 ? args[0] : ".");
        outputDirectory.mkdirs();
        File output = new File(outputDirectory, "post_ea_bank87.txt");

        long[] entries = { 0x8000, 0x80CB, 0x8C1D, 0x8C6B };
        String[] names = { "title_timeout", "title_frame_update", "pre_ea_audio_command", "post_title_audio_command" };
        for (int i = 0; i < entries.length; ++i) {
            Address address = toAddr(entries[i]);
            addEntryPoint(address);
            disassemble(address);
            if (getFunctionAt(address) == null) createFunction(address, names[i]);
        }

        try (PrintWriter writer = new PrintWriter(output, "UTF-8")) {
            writer.println("NBA Live '95 (USA) - bank $87 post-EA title/audio routines");
            writer.println();
            dumpInstructions(writer, 0x8000, 0x8400);
            dumpInstructions(writer, 0x8C1D, 0x8D20);
        }
        println("Wrote bank $87 title dump: " + output.getAbsolutePath());
    }
}
