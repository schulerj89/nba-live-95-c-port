// args: output directory, bank hex. Program is one LoROM bank loaded at $8000.
// Read-only reverse-engineering aid for the foul-out/substitution continuation.
import java.io.*;
import java.math.BigInteger;
import java.util.*;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

public class DumpSubstitutionContinuation extends GhidraScript {
    private void decode(int lo, int hi, int... seeds) throws Exception {
        clearListing(toAddr(lo), toAddr(hi));
        for (String name : new String[]{"M", "X"}) {
            Register reg = currentProgram.getRegister(name);
            if (reg != null) currentProgram.getProgramContext().setValue(
                reg, toAddr(lo), toAddr(hi), BigInteger.ZERO);
        }
        for (int seed : seeds) disassemble(toAddr(seed));
    }

    public void run() throws Exception {
        String[] args = getScriptArgs();
        File dir = new File(args[0]); dir.mkdirs();
        int bank = Integer.parseInt(args[1], 16);
        if (bank == 0x83) {
            decode(0x939d, 0x98ef, 0x939d, 0x947d, 0x9549, 0x95db, 0x966d);
            decode(0xebd8, 0xefff, 0xebd8, 0xecb0, 0xed73, 0xee50);
        }
        else if (bank == 0x85) decode(0xc0f6, 0xc4ff,
            0xc0f6, 0xc224, 0xc2ce, 0xc36a, 0xc37d);
        else if (bank == 0x86) decode(0xf570, 0xf6ff, 0xf587, 0xf58c, 0xf65a);
        else if (bank == 0x87) {
            decode(0x8c66, 0x8fff, 0x8c66);
            decode(0xa357, 0xa47a, 0xa357);
            decode(0xaf75, 0xb0ff, 0xaf75, 0xaf95);
        } else throw new Exception("unsupported bank " + args[1]);

        if (bank == 0x83) {
            String[][] labels = {
                {"939d","RepairFirstLineupBySwap","Swap outgoing active player with an eligible bench player; prefers matching roster position."},
                {"947d","RepairSecondLineupBySwap","Second-team equivalent using $4779 and $495B."},
                {"9549","RebuildFirstLineupAutomatic","Rebuild twelve staged lineup entries then copy them to $46F9."},
                {"95db","RebuildSecondLineupAutomatic","Rebuild twelve staged lineup entries then copy them to $4779."},
                {"ebd8","FoulWhistlePresentationOwner","Presentation owner; $ECA5 enters substitution refresh when exactly one request class is pending."},
                {"ecb0","SubstitutionRefreshParent","Lineup transaction, actor/appearance/resource rebuild, draw preparation, request clear; common return at $ED46."},
                {"ed73","ApplyPendingLineupTransactions","Consumes $09CA/$492D and parallel $09CC/$09CE; repairs affected lineups and clears per-class request state."},
                {"ee50","BuildSubstitutionPresentation","Build presentation from selected roster record pointers; input/cancel semantics are outside this decoded boundary."}
            };
            for (String[] label : labels) {
                int pc = Integer.parseInt(label[0], 16);
                createLabel(toAddr(pc), label[1], true);
                setPlateComment(toAddr(pc), label[2] + " C port: src/nba_gameplay_foul.c.");
            }
        } else if (bank == 0x85) {
            createLabel(toAddr(0xc0f6), "RebuildTenCourtActorsAfterLineupChange", true);
            setPlateComment(toAddr(0xc0f6),
                "Reconstruct ten active actors from repaired lineups; returns at $C37C. C port: src/nba_gameplay_foul.c.");
        } else if (bank == 0x87) {
            createLabel(toAddr(0x8c66), "RebuildActiveAppearanceWrapper", true);
            createLabel(toAddr(0xaf75), "BindTenActivePlayerResources", true);
            createLabel(toAddr(0xaf95), "RebuildTenPlayerResourceMetadata", true);
        }

        try (PrintWriter out = new PrintWriter(
                new File(dir, "substitution_bank" + args[1] + ".txt"), "UTF-8")) {
            for (int pc = 0x8000; pc <= 0xffff; ++pc) {
                Instruction ins = currentProgram.getListing().getInstructionAt(toAddr(pc));
                if (ins != null) out.printf("$%02X:%04X [%d] %s%n",
                    bank, pc, ins.getLength(), ins);
            }
        }
    }
}
