// Correlate Mesen's formation/jump-ball/possession/live execution windows with
// Ghidra instructions and local call targets. args: <outDir> <bankHex>
// <formationTrace> <jumpTrace> <possessionTrace> <liveTrace>

import java.io.*;
import java.util.*;

import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;

public class DumpTipoff extends GhidraScript {
    private long[] manualCandidates(String bank) {
        if (bank.equals("80")) return new long[] { 0x98e9, 0x9df3, 0x9f0f,
            0xa34e, 0xa444, 0xa4b5, 0xa732, 0xa75e, 0xa781, 0xa8fa,
            0xa9b3, 0xa9e3, 0xb348, 0xcee7 };
        if (bank.equals("82")) return new long[] { 0xf987, 0xfe00, 0xff00 };
        if (bank.equals("85")) return new long[] { 0x9700, 0xb100, 0xb359,
            0xb377, 0xb3aa, 0xb402, 0xb95c, 0xf1c1, 0xf34f, 0xf3c3,
            0xf78b, 0xf867, 0xf8d9 };
        if (bank.equals("86")) return new long[] { 0x9846, 0x99c4, 0x9c45,
            0xb04c, 0xbaa2, 0xd549, 0xdda7, 0xe054, 0xe3cb, 0xe5ab,
            0xe635, 0xf1b0 };
        if (bank.equals("87")) return new long[] { 0xb3bd, 0xb47a, 0xb538,
            0xb555, 0xb832 };
        return new long[0];
    }

    private Set<Long> loadTrace(File file, String bank) throws Exception {
        Set<Long> result = new TreeSet<>();
        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (!line.startsWith(bank) || line.length() < 13) continue;
                String[] halves = line.trim().split("-");
                long first = Long.parseLong(halves[0].substring(2), 16);
                long last = Long.parseLong(halves[1].substring(2), 16);
                for (long address = first; address <= last; ++address) result.add(address);
            }
        }
        return result;
    }

    private String membership(long address, Set<Long>[] traces) {
        String[] names = { "formation", "jump", "possession", "live" };
        StringBuilder out = new StringBuilder();
        for (int index = 0; index < traces.length; ++index) {
            if (traces[index].contains(address)) {
                if (out.length() > 0) out.append(',');
                out.append(names[index]);
            }
        }
        return out.toString();
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 6) throw new IllegalArgumentException(
            "Expected outDir, bank, and four Mesen trace paths");
        File outDir = new File(args[0]); outDir.mkdirs();
        String bank = args[1].toUpperCase();
        @SuppressWarnings("unchecked")
        Set<Long>[] traces = new Set[] {
            loadTrace(new File(args[2]), bank), loadTrace(new File(args[3]), bank),
            loadTrace(new File(args[4]), bank), loadTrace(new File(args[5]), bank)
        };
        Set<Long> all = new TreeSet<>();
        for (Set<Long> trace : traces) all.addAll(trace);
        for (long address : all) disassemble(toAddr(address));
        if (bank.equals("86")) {
            disassemble(toAddr(0xdda7));
            disassemble(toAddr(0xe054));
        }

        Listing listing = currentProgram.getListing();
        Set<Long> nonJump = new HashSet<>();
        nonJump.addAll(traces[0]); nonJump.addAll(traces[2]); nonJump.addAll(traces[3]);
        Set<Long> localCandidates = new TreeSet<>();
        for (long target : manualCandidates(bank)) localCandidates.add(target);
        File listingFile = new File(outDir, "tipoff_bank" + bank + "_trace.txt");
        try (PrintWriter out = new PrintWriter(listingFile, "UTF-8")) {
            out.printf("NBA Live '95 tip-off trace correlation bank $%s%n%n", bank);
            out.println("--- Calls executed in any window ---");
            for (long address : all) {
                Instruction ins = listing.getInstructionAt(toAddr(address));
                if (ins == null) continue;
                String mnemonic = ins.getMnemonicString().toUpperCase();
                if (!mnemonic.equals("JSR") && !mnemonic.equals("JSL")) continue;
                out.printf("$%s:%04X [%s] %s%n", bank, address,
                    membership(address, traces), ins.toString());
                if (traces[1].contains(address) && !nonJump.contains(address)) {
                    for (Address flow : ins.getFlows()) {
                        long target = flow.getOffset();
                        if (target >= 0x8000 && target <= 0xffff) localCandidates.add(target);
                    }
                }
            }

            out.println("\n--- Instructions unique to the jump-ball window ---");
            for (long address : traces[1]) {
                if (nonJump.contains(address)) continue;
                Instruction ins = listing.getInstructionAt(toAddr(address));
                if (ins != null) out.printf("$%s:%04X  %s%n", bank, address, ins.toString());
            }
            if (bank.equals("86")) {
                out.println("\n--- Player-pair and ball actor initialization ---");
                for (long address = 0xdda7; address <= 0xe0aa; ++address) {
                    Instruction ins = listing.getInstructionAt(toAddr(address));
                    if (ins != null)
                        out.printf("$86:%04X  %s%n", address, ins.toString());
                }
            }
        }

        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        File decompFile = new File(outDir, "tipoff_bank" + bank + "_candidates.c");
        try (PrintWriter out = new PrintWriter(decompFile, "UTF-8")) {
            for (long target : localCandidates) {
                Address entry = toAddr(target);
                addEntryPoint(entry); disassemble(entry);
                Function function = getFunctionAt(entry);
                String name = String.format("tipoff_candidate_%s_%04X", bank, target);
                if (function == null) function = createFunction(entry, name);
                else function.setName(name, SourceType.USER_DEFINED);
                out.printf("/* ===== $%s:%04X %s ===== */%n", bank, target, name);
                DecompileResults result = function == null ? null :
                    decompiler.decompileFunction(function, 60, monitor);
                if (result != null && result.decompileCompleted() &&
                    result.getDecompiledFunction() != null)
                    out.println(result.getDecompiledFunction().getC());
                else out.println("/* decompilation failed */");
                out.println();
            }
        }
    }
}
