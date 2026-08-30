// args: output directory, bank hex. Program is one LoROM bank at $8000.
import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

public class DumpHumanFreeThrow extends GhidraScript {
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File directory = new File(args[0]); directory.mkdirs();
        int bank = Integer.parseInt(args[1], 16);
        if (bank != 0x87) throw new Exception("human free throw is in Bank $87");
        clearListing(toAddr(0x9cbf), toAddr(0xa045));
        for (String name : new String[]{"M", "X"}) {
            Register register = currentProgram.getRegister(name);
            if (register != null) currentProgram.getProgramContext().setValue(
                register, toAddr(0x9cbf), toAddr(0xa045), BigInteger.ZERO);
        }
        for (int seed : new int[]{0x9cbf, 0x9d25, 0x9d76, 0x9e39,
                                   0x9e88, 0x9eba, 0x9f11, 0xa018})
            disassemble(toAddr(seed));

        String[][] labels = {
            {"9cbf", "FreeThrowActorDispatcher",
             "Per-actor stripe dispatcher. State 3 selects CPU aim or controller-owned two-press aim from actor +$16 and team context +$3B."},
            {"9d25", "InitializeHumanFreeThrowAim",
             "Clear $0980/$0984 and derive oscillator quantum $0986 from roster free-throw rating byte +$38."},
            {"9e39", "HumanFreeThrowFirstAxis",
             "Advance $A018 oscillator; B/Y locks the first axis into $0982, clears $0980 and enters state 4."},
            {"9e88", "HumanFreeThrowWaitForRelease",
             "State 4 remains while B/Y is held; release enters state 5. Lost controller assignment falls back to state 3."},
            {"9eba", "HumanFreeThrowSecondAxis",
             "State 5 advances the second oscillator; the second B/Y press joins common launch state 9."},
            {"9f11", "FreeThrowLaunchCommit",
             "Common CPU/human launch commit also clears actor +$4A, cancels both animation channels, installs action 22 and falls through into state 9 in the same call; those wider effects are outside the exact oscillator oracle."},
            {"a018", "AdvanceFreeThrowAimOscillator",
             "Add $0986 to $0984, subtract 110 per cursor step and wrap $0980 at 112."}
        };
        for (String[] label : labels) {
            int address = Integer.parseInt(label[0], 16);
            createLabel(toAddr(address), label[1], true);
            setPlateComment(toAddr(address), label[2] +
                " See docs/human-free-throw-differential.md.");
        }

        try (PrintWriter output = new PrintWriter(
                new File(directory, "human_free_throw_bank87.txt"), "UTF-8")) {
            for (int pc = 0x9cbf; pc <= 0xa045; ++pc) {
                Instruction instruction =
                    currentProgram.getListing().getInstructionAt(toAddr(pc));
                if (instruction != null) output.printf("$87:%04X [%d] %s%n",
                    pc, instruction.getLength(), instruction);
            }
        }
    }
}
