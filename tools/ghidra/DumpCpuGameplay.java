// Dump only instructions proven live during the extended CPU-vs-CPU capture,
// centered on the player-coordinate, ball-coordinate, and AI dispatch paths.
// args: <outDir> <bankHex> <liveExecTrace>

import java.io.*;
import java.util.*;

import ghidra.app.decompiler.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;

public class DumpCpuGameplay extends GhidraScript {
    private Set<Long> loadTrace(File file, String bank) throws Exception {
        Set<Long> result = new TreeSet<>();
        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (!line.startsWith(bank) || line.length() < 13) continue;
                String[] halves = line.trim().split("-");
                long first = Long.parseLong(halves[0].substring(2), 16);
                long last = Long.parseLong(halves[1].substring(2), 16);
                for (long address = first; address <= last; ++address)
                    result.add(address);
            }
        }
        return result;
    }

    private long[][] focusRanges(String bank) {
        if (bank.equals("85")) return new long[][] {
            {0x8d00, 0xa7ff}, {0xaf00, 0xc6ff}, {0xf100, 0xf8ff}
        };
        if (bank.equals("86")) return new long[][] {
            {0x9800, 0xbaff}, {0xd300, 0xd6ff}, {0xe300, 0xf8ff}
        };
        if (bank.equals("87")) return new long[][] {
            {0x9200, 0xadff}, {0xb300, 0xb9ff}
        };
        return new long[0][0];
    }

    private long[] candidates(String bank) {
        if (bank.equals("85")) return new long[] {
            0x8d19, 0x8ee6, 0x9192, 0x963d, 0x9700, 0x9a24,
            0x9d40, 0x9f01, 0xa079, 0xa1e9, 0xa21f, 0xa357, 0xa518, 0xa5cc,
            0xab17, 0xaf5c, 0xb402, 0xb50e, 0xb60b, 0xb678, 0xb9d2,
            0xba1d, 0xbab7, 0xbae4, 0xbc07, 0xc37d, 0xc5fb, 0xef3a, 0xf02d,
            0xf1c1, 0xf34f, 0xf3c3, 0xf5e4, 0xf78b, 0xf7c9, 0xf867, 0xf8d9
        };
        if (bank.equals("86")) return new long[] {
            0x9846, 0x99c4, 0x9c45, 0x9cdb, 0x9d6e, 0x9ed8, 0xa110,
            0xa17d, 0xa1bd, 0xa561, 0xa5b0, 0xa6b3, 0xa7a8, 0xab2d,
            0xb00b, 0xb625, 0xb769, 0xb8ca, 0xbaa2, 0xbaee, 0xbf0b,
            0xc302, 0xc34c, 0xe39a, 0xe3cb, 0xe3e1, 0xe4a7, 0xe5ab,
            0xe635, 0xe7b3, 0xe7dc, 0xe8f7, 0xe923, 0xe96f, 0xec32, 0xecf9,
            0xef09, 0xf0b7, 0xf0fd, 0xf1b0, 0xf23f, 0xf2ca, 0xf34f,
            0xf3d2, 0xf43a, 0xf59f, 0xf64f, 0xf6cd, 0xf794, 0xf8cd,
            /* All 18 `$87:9BD0` behavior targets, including handlers outside
             * the otherwise shot/ball-oriented candidate set. */
            0x994c, 0xc6ad, 0xa7da, 0xb154, 0xb0f7, 0xb979
        };
        if (bank.equals("87")) return new long[] {
            0x9244, 0x98ea, 0x996a, 0x9a03, 0x9a73, 0x9b0d, 0x9b30, 0x9bd0,
            0xa2ce, 0xa357, 0xa846, 0xa9d0, 0xaa02, 0xaab2,
            0xad5b, 0xaec3, 0xb37c, 0xb47a, 0xb4db, 0xb538, 0xb555,
            0xb649, 0xb66a, 0xb832, 0xb953, 0xb572
        };
        return new long[0];
    }

    private boolean focused(long address, long[][] ranges) {
        for (long[] range : ranges)
            if (address >= range[0] && address <= range[1]) return true;
        return false;
    }

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 3)
            throw new IllegalArgumentException("Expected outDir, bank, live trace");
        File outDir = new File(args[0]); outDir.mkdirs();
        String bank = args[1].toUpperCase();
        Set<Long> trace = loadTrace(new File(args[2]), bank);
        long[][] ranges = focusRanges(bank);
        for (long address : trace)
            if (focused(address, ranges)) disassemble(toAddr(address));

        Listing listing = currentProgram.getListing();
        if (bank.equals("85")) {
            addEntryPoint(toAddr(0x8ee6)); disassemble(toAddr(0x8ee6));
            addEntryPoint(toAddr(0x9192)); disassemble(toAddr(0x9192));
        }
        File listingFile = new File(outDir, "cpu_gameplay_bank" + bank + "_listing.txt");
        try (PrintWriter out = new PrintWriter(listingFile, "UTF-8")) {
            out.printf("NBA Live '95 extended CPU gameplay, bank $%s%n", bank);
            out.println("Only instructions executed during frames 400+ are included.\n");
            for (long address : trace) {
                if (!focused(address, ranges)) continue;
                Instruction instruction = listing.getInstructionAt(toAddr(address));
                if (instruction != null)
                    out.printf("$%s:%04X  %s%n", bank, address,
                        instruction.toString());
            }
            if (bank.equals("85")) {
                out.println("\n--- Complete camera/court-streaming disassembly ---");
                for (long address = 0x8ee6; address <= 0x93f4; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$85:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete fine-direction/distance helper ---");
                addEntryPoint(toAddr(0xf3c3)); disassemble(toAddr(0xf3c3));
                for (long address = 0xf3c3; address <= 0xf472; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$85:%04X  %s%n", address, instruction.toString());
                }
            }
            if (bank.equals("86")) {
                out.println("\n--- Complete CPU pass initialization and release ---");
                addEntryPoint(toAddr(0xa6b3)); disassemble(toAddr(0xa6b3));
                addEntryPoint(toAddr(0xab2d)); disassemble(toAddr(0xab2d));
                for (long address = 0xa6b3; address <= 0xa7d9; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
                for (long address = 0xab2d; address <= 0xb04a; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
            }
        }

        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        File decompFile = new File(outDir, "cpu_gameplay_bank" + bank + "_functions.c");
        try (PrintWriter out = new PrintWriter(decompFile, "UTF-8")) {
            for (long target : candidates(bank)) {
                Address entry = toAddr(target);
                addEntryPoint(entry); disassemble(entry);
                Function function = getFunctionAt(entry);
                String name = String.format("cpu_gameplay_%s_%04X", bank, target);
                if (function == null) function = createFunction(entry, name);
                else function.setName(name, SourceType.USER_DEFINED);
                out.printf("/* ===== $%s:%04X %s ===== */%n", bank, target, name);
                DecompileResults result = function == null ? null :
                    decompiler.decompileFunction(function, 90, monitor);
                if (result != null && result.decompileCompleted() &&
                    result.getDecompiledFunction() != null)
                    out.println(result.getDecompiledFunction().getC());
                else out.println("/* decompilation failed */");
                out.println();
            }
        }
    }
}
