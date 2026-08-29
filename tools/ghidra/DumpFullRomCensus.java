// Conservative recursive instruction census for one canonical LoROM bank.
// Args: <outputDir> <bankHex> <seedFile>

import java.io.*;
import java.nio.file.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;

public class DumpFullRomCensus extends GhidraScript {
    private long targetValue(Instruction instruction) {
        for (Address flow : instruction.getFlows())
            return flow.getOffset();
        for (Object object : instruction.getOpObjects(0)) {
            if (object instanceof Address) return ((Address)object).getOffset();
            if (object instanceof Scalar) return ((Scalar)object).getUnsignedValue();
        }
        return -1;
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 3) throw new IllegalArgumentException(
            "Expected output directory, bank and seed file");
        File root = new File(args[0]);
        File listingDir = new File(root, "listings"); listingDir.mkdirs();
        File callsDir = new File(root, "calls"); callsDir.mkdirs();
        int bank = Integer.parseInt(args[1], 16);
        List<String> seeds = Files.readAllLines(Paths.get(args[2]));
        for (String text : seeds) {
            text = text.trim();
            if (text.length() != 6) continue;
            long address = Long.parseLong(text, 16);
            if ((address >>> 16) == bank && (address & 0xffff) >= 0x8000)
                disassemble(toAddr(address & 0xffff));
        }

        Listing listing = currentProgram.getListing();
        Set<Long> calls = new TreeSet<>();
        File instructions = new File(listingDir,
            String.format("bank_%02X_instructions.tsv", bank));
        try (PrintWriter writer = new PrintWriter(instructions, "UTF-8")) {
            for (Instruction instruction = listing.getInstructionAfter(toAddr(0x7fff));
                 instruction != null && instruction.getAddress().getOffset() <= 0xffff;
                 instruction = instruction.getNext()) {
                long full = ((long)bank << 16) | instruction.getAddress().getOffset();
                writer.printf("%06X\t%d\t%s%n", full, instruction.getLength(),
                    instruction.toString().replace('\t', ' '));
                String mnemonic = instruction.getMnemonicString().toUpperCase();
                if (mnemonic.equals("JSL") || mnemonic.equals("JML")) {
                    long target = targetValue(instruction);
                    if (target >= 0) calls.add(target <= 0xffff ?
                        (((long)bank << 16) | target) : target);
                } else if (mnemonic.equals("JSR") || mnemonic.equals("JMP")) {
                    long target = targetValue(instruction);
                    if (target >= 0 && target <= 0xffff)
                        calls.add(((long)bank << 16) | target);
                }
            }
        }
        try (PrintWriter writer = new PrintWriter(new File(callsDir,
                String.format("bank_%02X_calls.txt", bank)), "UTF-8")) {
            for (long address : calls) writer.printf("%06X%n", address & 0xffffff);
        }
        println(String.format("Full-ROM bank $%02X: seeds=%d instructions=%d calls=%d",
            bank, seeds.size(), listing.getNumInstructions(), calls.size()));
    }
}
