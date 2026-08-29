// Dump the Bank $80 host-service families selected by the gameplay 45% plan.
// args: <outDir> <liveExecTrace>

import java.io.*;
import java.util.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;

public class DumpGameplay45Services extends GhidraScript {
    private static final long[][] RANGES = {
        {0x815a, 0x8626}, {0x8627, 0x8bf2}, {0xa9b3, 0xab05},
        {0xc5ab, 0xcdcc}, {0xce33, 0xce8d}, {0xda72, 0xe95a}
    };
    private static final String[] NAMES = {
        "NmiOamVramService", "PpuPaletteVramService", "ApuCommandService",
        "FrameAndInputService", "FrameTimingInit", "ControllerScanService"
    };

    private Set<Long> loadTrace(File file) throws Exception {
        Set<Long> result = new TreeSet<>();
        try (BufferedReader reader = new BufferedReader(new FileReader(file))) {
            String line;
            while ((line = reader.readLine()) != null) {
                String text = line.trim();
                if (!text.startsWith("80") || text.length() < 13) continue;
                String[] halves = text.split("-");
                long first = Long.parseLong(halves[0].substring(2), 16);
                long last = Long.parseLong(halves[1].substring(2), 16);
                for (long[] range : RANGES)
                    for (long at = Math.max(first, range[0]);
                         at <= Math.min(last, range[1]); ++at) result.add(at);
            }
        }
        return result;
    }

    @Override public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) throw new IllegalArgumentException("Expected outDir and trace");
        File outDir = new File(args[0]); outDir.mkdirs();
        Set<Long> trace = loadTrace(new File(args[1]));
        Listing listing = currentProgram.getListing();
        for (int i = 0; i < RANGES.length; ++i) {
            Address entry = toAddr(RANGES[i][0]);
            disassemble(entry);
            if (getFunctionAt(entry) == null) createFunction(entry, NAMES[i]);
            else getFunctionAt(entry).setName(NAMES[i], SourceType.USER_DEFINED);
            for (long at : trace)
                if (at >= RANGES[i][0] && at <= RANGES[i][1]) disassemble(toAddr(at));
            File target = new File(outDir, String.format("service_%04x_%04x.txt",
                RANGES[i][0], RANGES[i][1]));
            try (PrintWriter out = new PrintWriter(target, "UTF-8")) {
                out.printf("%s $80:%04X-$80:%04X%n", NAMES[i], RANGES[i][0], RANGES[i][1]);
                int observed = 0;
                for (long at : trace) if (at >= RANGES[i][0] && at <= RANGES[i][1]) ++observed;
                out.printf("Observed address positions: %d%n%n", observed);
                for (long at = RANGES[i][0]; at <= RANGES[i][1]; ++at) {
                    Instruction instruction = listing.getInstructionAt(toAddr(at));
                    if (instruction != null)
                        out.printf("%c $80:%04X  %s%n", trace.contains(at) ? '*' : ' ', at,
                                   instruction.toString());
                }
            }
        }
    }
}
