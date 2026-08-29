// Dump the observed Bank $80 gameplay metasprite/OAM parent family.
// args: <outDir> <liveExecTrace>

import java.io.*;
import java.util.*;

import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;

public class DumpGameplay45 extends GhidraScript {
    private static final long START = 0x9c75;
    private static final long END = 0xa7c5;

    private Set<Long> loadTrace(File file) throws Exception {
        Set<Long> result = new TreeSet<>();
        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            String line;
            while ((line = reader.readLine()) != null) {
                String trimmed = line.trim();
                if (!trimmed.startsWith("80") || trimmed.length() < 13)
                    continue;
                String[] halves = trimmed.split("-");
                long first = Long.parseLong(halves[0].substring(2), 16);
                long last = Long.parseLong(halves[1].substring(2), 16);
                for (long address = Math.max(first, START);
                     address <= Math.min(last, END); ++address)
                    result.add(address);
            }
        }
        return result;
    }

    private void seed(long address, String name) throws Exception {
        Address entry = toAddr(address);
        disassemble(entry);
        if (getFunctionAt(entry) == null)
            createFunction(entry, name);
        else
            getFunctionAt(entry).setName(name, SourceType.USER_DEFINED);
        createLabel(entry, name, true, SourceType.USER_DEFINED);
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2)
            throw new IllegalArgumentException("Expected outDir and live trace");
        File outDir = new File(args[0]); outDir.mkdirs();
        Set<Long> trace = loadTrace(new File(args[1]));

        long[] knownEntries = {
            0x9c75, 0xa4fc, 0xa732, 0xa75e, 0xa781
        };
        String[] names = {
            "GameplayMetaspriteParent",
            "GameplayMetaspriteProjection",
            "GameplayOamQueueCommit",
            "GameplayOamQueueTail",
            "GameplayOamHighTablePack"
        };
        for (int i = 0; i < knownEntries.length; ++i)
            seed(knownEntries[i], names[i]);
        for (long address : trace)
            disassemble(toAddr(address));

        Listing listing = currentProgram.getListing();
        Set<Long> callTargets = new TreeSet<>();
        File listingFile = new File(outDir, "gameplay45_bank80_listing.txt");
        try (PrintWriter out = new PrintWriter(listingFile, "UTF-8")) {
            out.println("NBA Live '95 observed gameplay metasprite/OAM family");
            out.printf("Selected range $80:%04X-$80:%04X%n", START, END);
            out.printf("Observed address positions: %d%n%n", trace.size());
            for (long address = START; address <= END; ++address) {
                Instruction instruction = listing.getInstructionAt(toAddr(address));
                if (instruction == null) continue;
                boolean observed = trace.contains(address);
                out.printf("%c $80:%04X  %s%n", observed ? '*' : ' ', address,
                           instruction.toString());
                if (instruction.getFlowType().isCall()) {
                    for (Address target : instruction.getFlows()) {
                        long offset = target.getOffset();
                        callTargets.add(offset);
                    }
                }
            }
            out.println("\n--- Static call targets ---");
            for (long target : callTargets)
                out.printf("$80:%04X%s%n", target,
                    target >= START && target <= END ? " (internal)" : "");
        }

        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        try (PrintWriter out = new PrintWriter(
                new File(outDir, "gameplay45_bank80_decomp.c"), "UTF-8")) {
            for (long address : knownEntries) {
                Function function = getFunctionAt(toAddr(address));
                if (function == null) continue;
                DecompileResults result = decompiler.decompileFunction(
                    function, 120, monitor);
                out.printf("/* $80:%04X %s */%n", address, function.getName());
                if (result.decompileCompleted())
                    out.println(result.getDecompiledFunction().getC());
                else
                    out.println("/* decompilation failed */");
                out.println();
            }
        }
        decompiler.dispose();
    }
}
