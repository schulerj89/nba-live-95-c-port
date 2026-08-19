// Dump the EA Sports intro's code and backing bytes after headless analysis.

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;

public class DumpEaIntro extends GhidraScript {
    private void dumpInstructions(PrintWriter writer, long first, long last) {
        Listing listing = currentProgram.getListing();
        Address start = toAddr(first);
        Address end = toAddr(last);
        writer.printf("Instructions $82:%04X-$82:%04X:%n", first, last);
        for (Instruction instruction = listing.getInstructionAt(start);
             instruction != null && instruction.getAddress().compareTo(end) <= 0;
             instruction = instruction.getNext()) {
            writer.printf("82:%s  %-12s %s%n",
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
        File output = new File(outputDirectory, "ea_intro_ghidra.txt");

        try (PrintWriter writer = new PrintWriter(output, "UTF-8")) {
            writer.println("NBA Live '95 (USA) - bank $82 EA Sports intro");
            writer.println("SNES range $82:F15C-$82:F620 (LoROM file $01715C-$017620)");
            writer.println();

            dumpInstructions(writer, 0x94D5, 0x96B0);
            dumpInstructions(writer, 0xA9D1, 0xAA80);
            dumpInstructions(writer, 0xAB69, 0xAC0D);
            dumpInstructions(writer, 0xABCE, 0xAC0D);
            dumpInstructions(writer, 0xAC0E, 0xAF80);
            dumpInstructions(writer, 0xF15C, 0xF6C0);

            writer.println();
            writer.println("Raw bytes:");
            byte[] row = new byte[16];
            for (long offset = 0xF15C; offset <= 0xF6C0; offset += row.length) {
                Address address = toAddr(offset);
                int count = currentProgram.getMemory().getBytes(address, row);
                writer.printf("82:%04X ", offset);
                for (int i = 0; i < count; ++i) {
                    writer.printf("%02X ", row[i] & 0xFF);
                }
                writer.println();
            }
        }

        println("Wrote EA intro dump: " + output.getAbsolutePath());
    }
}
