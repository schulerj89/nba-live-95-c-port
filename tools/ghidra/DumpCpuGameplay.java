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
        if (bank.equals("83")) return new long[][] {
            {0xce00, 0xd2ff}, {0xeb00, 0xefff}
        };
        if (bank.equals("85")) return new long[][] {
            {0x8d00, 0xaeff}, {0xaf00, 0xc6ff}, {0xf100, 0xf8ff}
        };
        if (bank.equals("86")) return new long[][] {
            {0x9800, 0xbaff}, {0xc400, 0xddff}, {0xe300, 0xf8ff}
        };
        if (bank.equals("87")) return new long[][] {
            {0x9200, 0xadff}, {0xb300, 0xbaff}
        };
        return new long[0][0];
    }

    private long[] candidates(String bank) {
        if (bank.equals("83")) return new long[] {
            0xce36, 0xcfe8, 0xebd8, 0xebdb
        };
        if (bank.equals("85")) return new long[] {
            0x8d19, 0x8ee6, 0x9192, 0x963d, 0x9700, 0x9a24,
            0x9d40, 0x9f01, 0xa079, 0xa1e9, 0xa21f, 0xa357, 0xa518, 0xa5cc,
            0xa3b7, 0xa5f4, 0xa656, 0xa755,
            0xab17, 0xae3b, 0xaf5c, 0xb402, 0xb50e, 0xb60b, 0xb678,
            0xb83e, 0xb88d, 0xb9d2,
            0xba1d, 0xbab7, 0xbae4, 0xbc07, 0x93f5,
            0x9530, 0xc37d, 0xc5fb, 0xef3a, 0xf02d,
            0xf1c1, 0xf34f, 0xf3c3, 0xf5e4, 0xf78b, 0xf7c9, 0xf867, 0xf8d9
        };
        if (bank.equals("86")) return new long[] {
            0x9846, 0x99c4, 0x9c45, 0x9cdb, 0x9d6e, 0x9ed8, 0xa110,
            0xa17d, 0xa1bd, 0xa561, 0xa5b0, 0xa613, 0xa6b3, 0xa7a8, 0xab2d,
            0xb00b, 0xb34f, 0xb625, 0xb769, 0xb8ca, 0xbaa2, 0xbaee, 0xbf0b,
            0xc302, 0xc34c, 0xc493, 0xc4fe, 0xcccd, 0xccfc, 0xd12d,
            0xd1d9, 0xd43e, 0xd549, 0xd5db, 0xd652, 0xdd1e,
            0xe39a, 0xe3cb, 0xe3e1, 0xe4a7, 0xe5ab,
            0xe635, 0xe7b3, 0xe7dc, 0xe8f7, 0xe923, 0xe96f, 0xec32, 0xecf9,
            0xef09, 0xf0b7, 0xf0fd, 0xf1b0, 0xf23f, 0xf2ca, 0xf34f,
            0xf3d2, 0xf43a, 0xf56e, 0xf59f, 0xf64f, 0xf6cd, 0xf794, 0xf8cd,
            /* All 18 `$87:9BD0` behavior targets, including handlers outside
             * the otherwise shot/ball-oriented candidate set. */
            0x994c, 0xc6ad, 0xa7da, 0xb154, 0xb0f7, 0xb979, 0xbc9b
        };
        if (bank.equals("87")) return new long[] {
            0x9244, 0x92a5, 0x98ea, 0x996a, 0x9a03, 0x9a73,
            0x9b0d, 0x9b30, 0x9b41, 0x9bd0, 0x9cbf, 0x9cdb,
            0x9e39, 0x9e88, 0x9f11, 0x9f60, 0x9f76, 0x9ff3, 0xa017,
            0xa2ce, 0xa357, 0xa846, 0xa9d0, 0xaa02, 0xaab2,
            0xad5b, 0xaec3, 0xb37c, 0xb47a, 0xb4db, 0xb538, 0xb555,
            0xb649, 0xb66a, 0xb832, 0xb953, 0xb572, 0xbacb
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
                out.println("\n--- Complete ownerless bounce and shared court clamp ---");
                addEntryPoint(toAddr(0xa3b7)); disassemble(toAddr(0xa3b7));
                addEntryPoint(toAddr(0xa5f4)); disassemble(toAddr(0xa5f4));
                addEntryPoint(toAddr(0xa656)); disassemble(toAddr(0xa656));
                for (long address = 0xa3b7; address <= 0xa755; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$85:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete dead-ball formation override ---");
                addEntryPoint(toAddr(0xad6b)); disassemble(toAddr(0xad6b));
                addEntryPoint(toAddr(0xae3b)); disassemble(toAddr(0xae3b));
                for (long address = 0xad6b; address <= 0xaebb; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$85:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete mode-11 shot/formation/pass dispatcher ---");
                addEntryPoint(toAddr(0xb50e)); disassemble(toAddr(0xb50e));
                addEntryPoint(toAddr(0xb678)); disassemble(toAddr(0xb678));
                addEntryPoint(toAddr(0xb83e)); disassemble(toAddr(0xb83e));
                addEntryPoint(toAddr(0xb88d)); disassemble(toAddr(0xb88d));
                for (long address = 0xb50e; address <= 0xb8ca; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$85:%04X  %s%n", address, instruction.toString());
                }
                out.print("Shot-decision distance table $85:B804 =");
                for (long address = 0xb804; address < 0xb80a; ++address)
                    out.printf(" %02X", getByte(toAddr(address)) & 0xff);
                out.println();
                out.println("\n--- Complete post-score inbound placement/steering ---");
                addEntryPoint(toAddr(0xc37d)); disassemble(toAddr(0xc37d));
                addEntryPoint(toAddr(0xc450)); disassemble(toAddr(0xc450));
                addEntryPoint(toAddr(0xc49e)); disassemble(toAddr(0xc49e));
                addEntryPoint(toAddr(0xc4d4)); disassemble(toAddr(0xc4d4));
                addEntryPoint(toAddr(0xc50b)); disassemble(toAddr(0xc50b));
                addEntryPoint(toAddr(0xc548)); disassemble(toAddr(0xc548));
                addEntryPoint(toAddr(0xc579)); disassemble(toAddr(0xc579));
                addEntryPoint(toAddr(0xc5c1)); disassemble(toAddr(0xc5c1));
                for (long address = 0xc37d; address <= 0xc600; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$85:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete pending-event whistle consumer ---");
                addEntryPoint(toAddr(0x93f5)); disassemble(toAddr(0x93f5));
                for (long address = 0x93f5; address <= 0x945e; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$85:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete free-throw presentation timer gate ---");
                addEntryPoint(toAddr(0x9530)); disassemble(toAddr(0x9530));
                for (long address = 0x9530; address <= 0x9597; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$85:%04X  %s%n", address, instruction.toString());
                }
            }
            if (bank.equals("83")) {
                out.println("\n--- Complete gameplay whistle presentation control ---");
                addEntryPoint(toAddr(0xebd8)); disassemble(toAddr(0xebd8));
                addEntryPoint(toAddr(0xebdb)); disassemble(toAddr(0xebdb));
                for (long address = 0xebd8; address <= 0xee4f; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$83:%04X  %s%n", address, instruction.toString());
                }
            }
            if (bank.equals("86")) {
                out.println("\n--- Complete CPU clear-lane layup/dunk initializer ---");
                addEntryPoint(toAddr(0xb34f)); disassemble(toAddr(0xb34f));
                addEntryPoint(toAddr(0xb424)); disassemble(toAddr(0xb424));
                addEntryPoint(toAddr(0xb468)); disassemble(toAddr(0xb468));
                for (long address = 0xb34f; address <= 0xb624; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete mode-13 layup/dunk executor ---");
                addEntryPoint(toAddr(0xa7da)); disassemble(toAddr(0xa7da));
                addEntryPoint(toAddr(0xa82b)); disassemble(toAddr(0xa82b));
                addEntryPoint(toAddr(0xa9d0)); disassemble(toAddr(0xa9d0));
                for (long address = 0xa7da; address <= 0xaa69; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete boundary-state cancellation helper ---");
                addEntryPoint(toAddr(0xa613)); disassemble(toAddr(0xa613));
                for (long address = 0xa613; address <= 0xa628; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete mode-12 shot release and flight initialization ---");
                addEntryPoint(toAddr(0x9d6e)); disassemble(toAddr(0x9d6e));
                addEntryPoint(toAddr(0xb625)); disassemble(toAddr(0xb625));
                for (long address = 0x9d6e; address <= 0xa45e; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
                for (long address = 0xb625; address <= 0xb978; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
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
                out.println("\n--- Complete ball acquisition/possession boundary ---");
                addEntryPoint(toAddr(0xbaa2)); disassemble(toAddr(0xbaa2));
                addEntryPoint(toAddr(0xbaee)); disassemble(toAddr(0xbaee));
                addEntryPoint(toAddr(0xbf0b)); disassemble(toAddr(0xbf0b));
                for (long address = 0xbaa2; address <= 0xbf0b; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete player/player contact and response ---");
                long[] contactEntries = {
                    0xbd41, 0xbf0b, 0xbf17, 0xbf1a, 0xbf22, 0xbf35,
                    0xbf48, 0xbf5b, 0xbf74, 0xbf96, 0xbfa2, 0xbfaf,
                    0xbfba, 0xbfc6, 0xbfde, 0xbfea, 0xc002, 0xc012,
                    0xc03a, 0xc063, 0xc076, 0xc0a2, 0xc0c3, 0xc0df,
                    0xc101, 0xc109, 0xc12f, 0xc15a, 0xc189, 0xc1a4,
                    0xc1c1, 0xc1ee, 0xc201, 0xc21b, 0xc239, 0xc244,
                    0xc254, 0xc261, 0xc279, 0xc28a, 0xc29d, 0xc2ad,
                    0xc2c1, 0xc2d8, 0xc2eb, 0xc2f1, 0xc302, 0xc34c,
                    0xc3ad, 0xc3ef, 0xc42d, 0xc454, 0xc476, 0xc88f, 0xc8c9,
                    0xc8d2, 0xc8da, 0xc8e7, 0xc8ef, 0xc8f7, 0xc908,
                    0xc91e, 0xc92a, 0xc93e, 0xc94b, 0xc959, 0xc968,
                    0xc97c, 0xc984, 0xc995, 0xc9a1, 0xc9b2, 0xc9c3,
                    0xc9ea, 0xca0c, 0xca28, 0xca36, 0xca4a, 0xca73,
                    0xca89, 0xcaa4, 0xcaba, 0xcac8, 0xcad6, 0xcaec,
                    0xcaf6, 0xcb27, 0xcb33, 0xcb46, 0xcb51, 0xcb62,
                    0xcb72, 0xcb7a, 0xcb82, 0xcbc4, 0xcbd3, 0xcbec,
                    0xcc00, 0xcc10, 0xcc25, 0xcc7d, 0xcc94, 0xcca8,
                    0xccb8, 0xccca, 0xd5db, 0xd652
                };
                for (long entry : contactEntries) {
                    addEntryPoint(toAddr(entry)); disassemble(toAddr(entry));
                }
                for (long address = 0xbd41; address <= 0xc492; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
                for (long address = 0xc88f; address <= 0xcccc; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete mode-8 knockdown recovery ---");
                long[] recoveryEntries = {
                    0xc6ad, 0xc6c1, 0xc6df, 0xc701, 0xc70f, 0xc728, 0xc73c
                };
                for (long entry : recoveryEntries) {
                    addEntryPoint(toAddr(entry)); disassemble(toAddr(entry));
                }
                for (long address = 0xc6ad; address <= 0xc74d; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete foul classification/bookkeeping boundary ---");
                addEntryPoint(toAddr(0xc493)); disassemble(toAddr(0xc493));
                addEntryPoint(toAddr(0xc4fe)); disassemble(toAddr(0xc4fe));
                addEntryPoint(toAddr(0xd12d)); disassemble(toAddr(0xd12d));
                for (long address = 0xc493; address <= 0xc6ac; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
                for (long address = 0xd12d; address <= 0xd1d0; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete player/ball contact orchestration ---");
                addEntryPoint(toAddr(0xcccd)); disassemble(toAddr(0xcccd));
                addEntryPoint(toAddr(0xccfc)); disassemble(toAddr(0xccfc));
                addEntryPoint(toAddr(0xd1d9)); disassemble(toAddr(0xd1d9));
                addEntryPoint(toAddr(0xd43e)); disassemble(toAddr(0xd43e));
                addEntryPoint(toAddr(0xd549)); disassemble(toAddr(0xd549));
                addEntryPoint(toAddr(0xd5db)); disassemble(toAddr(0xd5db));
                addEntryPoint(toAddr(0xd652)); disassemble(toAddr(0xd652));
                for (long address = 0xcccd; address <= 0xd728; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete inbound receiver/pass executor ---");
                addEntryPoint(toAddr(0xf3d2)); disassemble(toAddr(0xf3d2));
                addEntryPoint(toAddr(0xf43a)); disassemble(toAddr(0xf43a));
                addEntryPoint(toAddr(0xf59f)); disassemble(toAddr(0xf59f));
                addEntryPoint(toAddr(0xf64f)); disassemble(toAddr(0xf64f));
                addEntryPoint(toAddr(0xf654)); disassemble(toAddr(0xf654));
                for (long address = 0xf3d2; address <= 0xf669; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete whistle-latch clear sites ---");
                addEntryPoint(toAddr(0xdd1e)); disassemble(toAddr(0xdd1e));
                addEntryPoint(toAddr(0xf56e)); disassemble(toAddr(0xf56e));
                for (long address = 0xdcf0; address <= 0xdd50; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
                for (long address = 0xf540; address <= 0xf5a0; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$86:%04X  %s%n", address, instruction.toString());
                }
            }
            if (bank.equals("87")) {
                out.println("\n--- Complete gameplay violation/event dispatch ---");
                addEntryPoint(toAddr(0x92a5)); disassemble(toAddr(0x92a5));
                for (long address = 0x92a5; address <= 0x95e6; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$87:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete shared dead-ball initializer and whistle clear ---");
                addEntryPoint(toAddr(0x9b41)); disassemble(toAddr(0x9b41));
                addEntryPoint(toAddr(0x9cdb)); disassemble(toAddr(0x9cdb));
                for (long address = 0x9b30; address <= 0x9d20; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$87:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete free-throw actor state machine ---");
                addEntryPoint(toAddr(0x9cbf)); disassemble(toAddr(0x9cbf));
                addEntryPoint(toAddr(0x9e39)); disassemble(toAddr(0x9e39));
                addEntryPoint(toAddr(0x9e88)); disassemble(toAddr(0x9e88));
                addEntryPoint(toAddr(0x9f11)); disassemble(toAddr(0x9f11));
                addEntryPoint(toAddr(0x9f60)); disassemble(toAddr(0x9f60));
                addEntryPoint(toAddr(0x9f76)); disassemble(toAddr(0x9f76));
                addEntryPoint(toAddr(0x9ff3)); disassemble(toAddr(0x9ff3));
                addEntryPoint(toAddr(0xa017)); disassemble(toAddr(0xa017));
                for (long address = 0x9cbf; address <= 0xa017; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$87:%04X  %s%n", address, instruction.toString());
                }
                out.println("\n--- Complete whistle presentation-object scheduler ---");
                addEntryPoint(toAddr(0xbacb)); disassemble(toAddr(0xbacb));
                for (long address = 0xbacb; address <= 0xbaf4; ++address) {
                    Instruction instruction = listing.getInstructionAt(toAddr(address));
                    if (instruction != null)
                        out.printf("$87:%04X  %s%n", address, instruction.toString());
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
