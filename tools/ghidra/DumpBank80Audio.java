// Dump and label the 65816-side loader/command routines used to prepare the SPC700.

import java.io.File;
import java.io.PrintWriter;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;

public class DumpBank80Audio extends GhidraScript {
    private void mark(long offset, String name, String comment) throws Exception {
        Address address = toAddr(offset);
        addEntryPoint(address);
        disassemble(address);
        if (getFunctionAt(address) == null) createFunction(address, name);
        currentProgram.getListing().setComment(address, CodeUnit.PLATE_COMMENT, comment);
    }

    private void dump(PrintWriter writer, long first, long last) {
        Listing listing = currentProgram.getListing();
        Address end = toAddr(last);
        writer.printf("Instructions $80:%04X-$80:%04X:%n", first, last);
        for (Instruction instruction = listing.getInstructionAt(toAddr(first));
             instruction != null && instruction.getAddress().compareTo(end) <= 0;
             instruction = instruction.getNext()) {
            writer.printf("80:%s  %-12s %s%n", instruction.getAddress(),
                instruction.getMnemonicString(),
                instruction.toString().substring(instruction.getMnemonicString().length()).trim());
        }
        writer.println();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File directory = new File(args.length > 0 ? args[0] : ".");
        directory.mkdirs();

        mark(0x972F, "apu_bootstrap_if_needed",
             "Checks/initializes the SNES APU before a new sound bank is transferred.");
        mark(0x9763, "apu_begin_transfer",
             "Begins the CPU-to-SPC transfer handshake used by the sound-bank loader.");
        mark(0x9829, "apu_transfer_table_item",
             "Transfers one sequence/instrument/sample item selected through the bank-$82 pointer table.");
        mark(0x985A, "apu_calculate_item_transfer_size",
             "Derives the SPC payload dimensions/counts from fields in the selected item's 0x40-byte header.");
        mark(0x987B, "apu_transfer_item_payload",
             "Sends the selected item's payload after its 0x40-byte header.");
        mark(0x9B73, "audio_engine_reset_state",
             "Resets the 65816-side music/voice bookkeeping before a new bank or sequence is selected.");
        mark(0x9C47, "apu_set_runtime_parameters",
             "Sends two runtime parameters to the resident SPC sound driver.");
        mark(0x9C75, "apu_commit_runtime_parameters",
             "Commits the current sound-driver parameters through the APU command queue.");
        mark(0x9CC8, "audio_sequence_load",
             "Resolves a selected music-sequence structure and initializes its CPU-side channel pointers.");
        mark(0x9D80, "audio_sequence_select",
             "Stores the requested music-sequence selector/pointer and starts or stops sequence processing.");

        try (PrintWriter writer = new PrintWriter(
                new File(directory, "bank80_apu_driver.txt"), "UTF-8")) {
            writer.println("NBA Live '95 (USA) - 65816 APU loader and command routines");
            writer.println();
            dump(writer, 0x972F, 0x9DDF);
        }
    }
}
