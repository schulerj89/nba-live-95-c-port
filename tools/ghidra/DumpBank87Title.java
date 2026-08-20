// Dump bank $87 routines used by the title/attract transition and animation.

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.listing.CodeUnit;

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

        long[] entries = { 0x8000, 0x80CB, 0x8211, 0x8230, 0x8C1D, 0x8C6B };
        String[] names = { "title_timeout_to_attract", "title_frame_update",
                           "title_credit_scanline_irq",
                           "title_credit_scroll_irq",
                           "derive_team_runtime_values", "gameplay_scene_setup" };
        String[] comments = {
            "Called when $80:E01E's 1000-frame title timer expires; prepares attract-mode state.",
            "Called once per title frame after attract setup. Drives credit motion with $0615 (X), $1872 (Y), " +
            "$186C (credit index) and $186E (delay); it does not construct the initial N/B/A/LIVE/95 build.",
            "H/V IRQ installed by $87:8009 through $80:8640. Splits BG3 credit scroll state within the frame; " +
            "the port must use this boundary rather than inferring an X correction from $186E.",
            "Second IRQ armed for scanline $BE by $87:8211. Switches BG3 from the horizontally moving role band " +
            "to the vertically moving names band using $0615/$1872 state.",
            "Derives capped runtime values from team selections; this is not a title-music command.",
            "Initializes the later gameplay/transition scene; this is not the title-song start."
        };
        for (int i = 0; i < entries.length; ++i) {
            Address address = toAddr(entries[i]);
            addEntryPoint(address);
            disassemble(address);
            if (getFunctionAt(address) == null) createFunction(address, names[i]);
            currentProgram.getListing().setComment(address, CodeUnit.PLATE_COMMENT, comments[i]);
        }
        currentProgram.getListing().setComment(toAddr(0x80DC), CodeUnit.PRE_COMMENT,
            "Slide the active credit: $0615 moves by four pixels and $1872 by two until the hold/exit bounds are reached.");
        currentProgram.getListing().setComment(toAddr(0x8114), CodeUnit.PRE_COMMENT,
            "Select the next credit motion direction from table $87:825C using $186C.");

        try (PrintWriter writer = new PrintWriter(output, "UTF-8")) {
            writer.println("NBA Live '95 (USA) - bank $87 title/attract routines");
            writer.println();
            dumpInstructions(writer, 0x8000, 0x8400);
            dumpInstructions(writer, 0x8C1D, 0x8D20);
        }
        println("Wrote bank $87 title dump: " + output.getAbsolutePath());
    }
}
