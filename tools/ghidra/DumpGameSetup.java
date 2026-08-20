// Disassemble and decompile the routines that the ROM actually executes on the
// Game Setup screen. Ranges are read straight from the live Mesen execution
// trace (.analysis/setup_capture/setup_exec_addrs.txt), so nothing is guessed.
//
// args: <analysisDir> <bankHex>

import java.io.File;
import java.io.PrintWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.List;

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.Listing;

public class DumpGameSetup extends GhidraScript {

    private static class Range {
        long start, end;
        Range(long s, long e) { start = s; end = e; }
    }

    private List<Range> readTrace(File traceFile, int bank) throws Exception {
        List<Range> out = new ArrayList<>();
        if (!traceFile.exists()) return out;
        for (String line : Files.readAllLines(traceFile.toPath(), StandardCharsets.UTF_8)) {
            line = line.trim();
            if (line.isEmpty() || line.startsWith("#")) continue;
            String[] parts = line.split("-");
            if (parts.length != 2) continue;
            long a = Long.parseLong(parts[0].trim(), 16);
            long b = Long.parseLong(parts[1].trim(), 16);
            int ba = (int) ((a >> 16) & 0xFF);
            // bank $00 mirrors $80 in LoROM
            if (ba != bank && !(bank == 0x80 && ba == 0x00)) continue;
            out.add(new Range(a & 0xFFFF, b & 0xFFFF));
        }
        return out;
    }

    private void dumpRange(PrintWriter w, String bank, long first, long last) {
        Listing listing = currentProgram.getListing();
        Address end = toAddr(last);
        w.printf("--- $%s:%04X-$%s:%04X ---%n", bank, first, bank, last);
        Instruction ins = listing.getInstructionAt(toAddr(first));
        if (ins == null) {
            w.printf("(no instructions decoded at $%s:%04X)%n%n", bank, first);
            return;
        }
        for (; ins != null && ins.getAddress().compareTo(end) <= 0; ins = ins.getNext()) {
            String m = ins.getMnemonicString();
            String rest = ins.toString();
            rest = rest.length() > m.length() ? rest.substring(m.length()).trim() : "";
            w.printf("%s:%04X  %-8s %s%n", bank, ins.getAddress().getOffset(), m, rest);
        }
        w.println();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File outDir = new File(args.length > 0 ? args[0] : ".");
        outDir.mkdirs();
        String bankStr = args.length > 1 ? args[1] : "80";
        int bank = Integer.parseInt(bankStr, 16);

        List<Range> ranges = readTrace(new File(outDir, "setup_exec_addrs.txt"), bank);
        println("bank $" + bankStr + ": " + ranges.size() + " traced ranges");
        if (ranges.isEmpty()) return;

        for (Range r : ranges) {
            Address a = toAddr(r.start);
            addEntryPoint(a);
            disassemble(a);
            if (getFunctionAt(a) == null) {
                createFunction(a, String.format("sub_%s_%04X", bankStr, r.start));
            }
        }

        File listingOut = new File(outDir, "setup_bank" + bankStr + "_listing.txt");
        try (PrintWriter w = new PrintWriter(listingOut, "UTF-8")) {
            w.printf("NBA Live '95 (USA) - bank $%s routines executing on the Game Setup screen%n", bankStr);
            w.println("Ranges captured from a live Mesen exec trace; entry points are real, not inferred.");
            w.println();
            for (Range r : ranges) {
                // extend a little past the traced end to capture the tail of each block
                dumpRange(w, bankStr, r.start, Math.min(0xFFFF, r.end + 0x30));
            }
        }
        println("Wrote listing: " + listingOut.getAbsolutePath());

        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        File cOut = new File(outDir, "setup_bank" + bankStr + "_decomp.c");
        try (PrintWriter w = new PrintWriter(cOut, "UTF-8")) {
            w.printf("/* NBA Live '95 (USA) - decompiled bank $%s Game Setup routines */%n%n", bankStr);
            for (Range r : ranges) {
                Function f = getFunctionAt(toAddr(r.start));
                if (f == null) continue;
                DecompileResults res = decomp.decompileFunction(f, 60, monitor);
                w.printf("/* ===== $%s:%04X ===== */%n", bankStr, r.start);
                if (res != null && res.decompileCompleted() && res.getDecompiledFunction() != null) {
                    w.println(res.getDecompiledFunction().getC());
                } else {
                    w.printf("/* decompilation failed: %s */%n",
                        res == null ? "no result" : res.getErrorMessage());
                }
                w.println();
            }
        }
        decomp.dispose();
        println("Wrote decompilation: " + cOut.getAbsolutePath());
    }
}
