// Dump every captured address in the gameplay-85 selected ranges and retain
// static call ownership. Args: <outDir> <bankHex> <analysisRoot>

import java.io.*;
import java.nio.file.*;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;

public class DumpGameplay85Closure extends GhidraScript {
    private long[] selectedRange(String bank) {
        if (bank.equals("80")) return new long[] {0xa7c6, 0xbf00};
        if (bank.equals("85") || bank.equals("86"))
            return new long[] {0x8000, 0xffff};
        if (bank.equals("87")) return new long[] {0x8000, 0xbfff};
        throw new IllegalArgumentException("Unsupported bank " + bank);
    }

    private Set<Long> loadCaptured(Path root, String bank, long first,
                                   long last) throws Exception {
        Set<Long> result = new TreeSet<>();
        try (java.util.stream.Stream<Path> paths = Files.walk(root)) {
            Iterator<Path> iterator = paths.filter(path ->
                Files.isRegularFile(path) &&
                path.getFileName().toString().startsWith("exec_") &&
                path.getFileName().toString().endsWith(".txt")).iterator();
            while (iterator.hasNext()) {
                Path path = iterator.next();
                for (String line : Files.readAllLines(path)) {
                    String value = line.trim().toUpperCase();
                    if (!value.startsWith(bank) || value.length() < 13)
                        continue;
                    String[] halves = value.split("-");
                    if (halves.length != 2) continue;
                    long start = Long.parseLong(halves[0].substring(2), 16);
                    long end = Long.parseLong(halves[1].substring(2), 16);
                    for (long address = Math.max(first, start);
                         address <= Math.min(last, end); ++address)
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
                "Expected outDir, bank and analysis root");
        File outDir = new File(args[0]); outDir.mkdirs();
        String bank = args[1].toUpperCase();
        long[] range = selectedRange(bank);
        Set<Long> captured = loadCaptured(Paths.get(args[2]), bank,
                                          range[0], range[1]);

        // Captures contain address-position intervals, not only instruction
        // starts. Asking Ghidra at each still lets its instruction ownership
        // reject operand bytes already claimed by a decoded instruction.
        for (long address : captured)
            disassemble(toAddr(address));

        Listing listing = currentProgram.getListing();
        Set<Long> calls = new TreeSet<>();
        int decodedStarts = 0;
        File listingFile = new File(outDir,
            "gameplay85_bank" + bank + "_listing.txt");
        try (PrintWriter out = new PrintWriter(listingFile, "UTF-8")) {
            out.printf("NBA Live '95 gameplay-85 closure, bank $%s%n", bank);
            out.printf("Selected range $%s:%04X-$%s:%04X%n", bank,
                       range[0], bank, range[1]);
            out.printf("Captured address positions: %d%n%n", captured.size());
            for (long address = range[0]; address <= range[1]; ++address) {
                Instruction instruction = listing.getInstructionAt(toAddr(address));
                if (instruction == null) continue;
                ++decodedStarts;
                boolean observed = captured.contains(address);
                out.printf("%c $%s:%04X  %s%n", observed ? '*' : ' ', bank,
                           address, instruction.toString());
                String mnemonic = instruction.getMnemonicString();
                if (instruction.getFlowType().isCall() ||
                    mnemonic.equals("JSR") || mnemonic.equals("JSL")) {
                    for (Object object : instruction.getOpObjects(0))
                        if (object instanceof Address)
                            calls.add(((Address)object).getOffset());
                        else if (object instanceof Scalar)
                            calls.add(((Scalar)object).getUnsignedValue());
                    for (Address target : instruction.getFlows())
                        calls.add(target.getOffset());
                }
            }
            out.printf("%nDecoded instruction starts: %d%n", decodedStarts);
            out.println("Static call targets:");
            for (long target : calls)
                out.printf("$%s:%04X%s%n", bank, target,
                    target >= range[0] && target <= range[1]
                        ? " (selected family)" : " (external)");
        }

        println(String.format(
            "Gameplay85 bank $%s: captured=%d decoded=%d calls=%d",
            bank, captured.size(), decodedStarts, calls.size()));
    }
}
