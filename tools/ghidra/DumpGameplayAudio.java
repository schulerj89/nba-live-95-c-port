// Labels and dumps the gameplay presentation-sound dispatcher and command tables.

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;
import ghidra.program.model.symbol.SourceType;

public class DumpGameplayAudio extends GhidraScript {
    private void function(long offset, String name, String comment) throws Exception {
        Address address = toAddr(offset);
        addEntryPoint(address);
        disassemble(address);
        if (getFunctionAt(address) == null) createFunction(address, name);
        currentProgram.getListing().setComment(address, CodeUnit.PLATE_COMMENT, comment);
    }

    private void data(long offset, String name, String comment) throws Exception {
        Address address = toAddr(offset);
        createLabel(address, name, true, SourceType.USER_DEFINED);
        currentProgram.getListing().setComment(address, CodeUnit.PLATE_COMMENT, comment);
    }

    private void instructions(PrintWriter writer, long first, long last) {
        Listing listing = currentProgram.getListing();
        Address end = toAddr(last);
        writer.printf("Instructions $82:%04X-$82:%04X:%n", first, last);
        for (Instruction instruction = listing.getInstructionAt(toAddr(first));
             instruction != null && instruction.getAddress().compareTo(end) <= 0;
             instruction = instruction.getNext()) {
            String rendered = instruction.toString();
            String mnemonic = instruction.getMnemonicString();
            String operands = rendered.length() > mnemonic.length() ?
                rendered.substring(mnemonic.length()).trim() : "";
            writer.printf("82:%s  %-8s %s%n", instruction.getAddress(), mnemonic, operands);
        }
        writer.println();
    }

    private void words(PrintWriter writer, long address, String name, int count)
            throws Exception {
        writer.printf("%s $82:%04X (%d words):", name, address, count);
        for (int index = 0; index < count; ++index) {
            int lo = getByte(toAddr(address + index * 2L)) & 0xff;
            int hi = getByte(toAddr(address + index * 2L + 1L)) & 0xff;
            writer.printf(" %04X", lo | (hi << 8));
        }
        writer.println();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File directory = new File(args.length > 0 ? args[0] : ".");
        directory.mkdirs();

        function(0xFD65, "gameplay_audio_event_dispatch",
            "$82:FD65-$FF08 consumes $13E7/$13E9 presentation bits. Each family calls " +
            "$80:8930, masks the random word, indexes a ROM command table, then calls $80:9DF3.");
        data(0xF822, "gameplay_audio_bounce_commands",
            "Four ball/floor bounce commands selected by RNG & 3.");
        data(0xF82A, "gameplay_audio_inner_rim_commands",
            "Sixteen inner-rim commands selected by RNG & $0F.");
        data(0xF84A, "gameplay_audio_make_commands",
            "Four made-basket commands selected by RNG & 3.");
        data(0xF852, "gameplay_audio_outer_rim_commands",
            "Four outer-rim commands selected by RNG & 3.");
        data(0xF85A, "gameplay_audio_catch_commands",
            "Four catch/acquisition commands selected by RNG & 3.");
        data(0xF862, "gameplay_audio_contact_commands",
            "Four close-contact commands selected by RNG & 3.");
        data(0xF86A, "gameplay_audio_shoe_commands",
            "Four direction-change/shoe commands selected by RNG & 3.");
        data(0xF872, "gameplay_audio_collision_commands",
            "Eight collision commands selected by RNG & 7.");
        data(0xF882, "gameplay_audio_landing_commands",
            "Four knockdown-landing commands selected by RNG & 3.");

        try (PrintWriter writer = new PrintWriter(
                new File(directory, "gameplay_audio_ghidra.txt"), "UTF-8")) {
            writer.println("NBA Live '95 (USA) - gameplay audio dispatcher proof");
            writer.println();
            words(writer, 0xF822, "bounce", 4);
            words(writer, 0xF82A, "inner_rim", 16);
            words(writer, 0xF84A, "made_basket", 4);
            words(writer, 0xF852, "outer_rim", 4);
            words(writer, 0xF85A, "catch", 4);
            words(writer, 0xF862, "contact", 4);
            words(writer, 0xF86A, "shoe", 4);
            words(writer, 0xF872, "collision", 8);
            words(writer, 0xF882, "landing", 4);
            writer.println();
            instructions(writer, 0xFD65, 0xFF08);
        }
    }
}
