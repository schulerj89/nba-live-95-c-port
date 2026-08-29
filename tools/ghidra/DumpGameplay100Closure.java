// Dump all retained captured addresses for one remaining-closure bank and
// retain static call ownership. Args: <outDir> <bankHex> <analysisRoot>

import java.io.*;
import java.nio.file.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;

public class DumpGameplay100Closure extends GhidraScript {
    private Set<Long> loadCaptured(Path root, String bank) throws Exception {
        Set<Long> result = new TreeSet<>();
        try (java.util.stream.Stream<Path> paths = Files.walk(root)) {
            Iterator<Path> iterator = paths.filter(path ->
                Files.isRegularFile(path) &&
                path.getFileName().toString().startsWith("exec_") &&
                path.getFileName().toString().endsWith(".txt")).iterator();
            while (iterator.hasNext()) {
                for (String line : Files.readAllLines(iterator.next())) {
                    String value = line.trim().toUpperCase();
                    if (!value.startsWith(bank) || value.length() < 13)
                        continue;
                    String[] halves = value.split("-");
                    if (halves.length != 2) continue;
                    long first = Long.parseLong(halves[0].substring(2), 16);
                    long last = Long.parseLong(halves[1].substring(2), 16);
                    for (long address = Math.max(0x8000, first);
                         address <= Math.min(0xffff, last); ++address)
                        result.add(address);
                }
            }
        }
        return result;
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 3)
            throw new IllegalArgumentException(
                "Expected output directory, bank and analysis root");
        File outDir = new File(args[0]); outDir.mkdirs();
        String bank = args[1].toUpperCase();
        Set<Long> captured = loadCaptured(Paths.get(args[2]), bank);
        for (long address : captured) disassemble(toAddr(address));

        Listing listing = currentProgram.getListing();
        Set<Long> calls = new TreeSet<>();
        int decodedStarts = 0;
        File output = new File(outDir,
            "gameplay100_bank" + bank + "_listing.txt");
        try (PrintWriter out = new PrintWriter(output, "UTF-8")) {
            out.printf("NBA Live '95 exact captured-address closure, bank $%s%n", bank);
            out.printf("Captured address positions: %d%n%n", captured.size());
            for (long address = 0x8000; address <= 0xffff; ++address) {
                Instruction instruction = listing.getInstructionAt(toAddr(address));
                if (instruction == null) continue;
                ++decodedStarts;
                out.printf("%c $%s:%04X  %s%n",
                    captured.contains(address) ? '*' : ' ', bank, address,
                    instruction.toString());
                String mnemonic = instruction.getMnemonicString();
                if (instruction.getFlowType().isCall() ||
                    mnemonic.equals("JSR") || mnemonic.equals("JSL")) {
                    for (Object object : instruction.getOpObjects(0)) {
                        if (object instanceof Address)
                            calls.add(((Address)object).getOffset());
                        else if (object instanceof Scalar)
                            calls.add(((Scalar)object).getUnsignedValue());
                    }
                    for (Address target : instruction.getFlows())
                        calls.add(target.getOffset());
                }
            }
            out.printf("%nDecoded instruction starts: %d%n", decodedStarts);
            out.println("Static call targets:");
            for (long target : calls) out.printf("$%06X%n", target);
        }
        println(String.format(
            "Gameplay100 bank $%s: captured=%d decoded=%d calls=%d",
            bank, captured.size(), decodedStarts, calls.size()));
    }
}
