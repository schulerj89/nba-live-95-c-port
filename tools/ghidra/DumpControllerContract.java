import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

public class DumpControllerContract extends GhidraScript {
    public void run() throws Exception {
        int bank = Integer.parseInt(getScriptArgs()[1], 16);
        int[][] ranges;
        if (bank == 0x81) ranges = new int[][]{{0xa1e7,0xa2d2},{0xa489,0xaeff},{0xb410,0xb7ff}};
        else if (bank == 0x84) ranges = new int[][]{{0xdf7a,0xe4b0}};
        else if (bank == 0x85) ranges = new int[][]{{0xef3a,0xefec}};
        else if (bank == 0x86) ranges = new int[][]{{0xbc9b,0xbd1e},{0xd25a,0xd34a},{0xe208,0xe389}};
        else if (bank == 0x87) ranges = new int[][]{{0x9075,0x92a4}};
        else throw new Exception("unexpected bank");
        for (int[] range : ranges) {
            clearListing(toAddr(range[0]), toAddr(range[1]));
            for (String name : new String[]{"M", "X"}) {
                Register reg = currentProgram.getRegister(name);
                if (reg != null) currentProgram.getProgramContext().setValue(reg,
                    toAddr(range[0]), toAddr(range[1]), BigInteger.ZERO);
            }
            disassemble(toAddr(range[0]));
        }
        int[] extra = bank == 0x81 ? new int[]{0xa2b8,0xa776,0xa7ab,0xa9a6,0xb493,0xb546,0xb62c,0xb719,0xb748} :
                      bank == 0x84 ? new int[]{0xe2ac,0xe3ea,0xe432} :
                      bank == 0x86 ? new int[]{0xe24c} : new int[]{};
        for(int pc : extra) disassemble(toAddr(pc));
        try(PrintWriter out = new PrintWriter(new File(getScriptArgs()[0],
                String.format("controller_contract_bank%02x.txt",bank)),"UTF-8")) {
            for(int[] range : ranges) for(int pc=range[0];pc<=range[1];++pc) {
                Instruction ins = currentProgram.getListing().getInstructionAt(toAddr(pc));
                if(ins!=null) out.printf("$%02X:%04X [%d] %s%n",bank,pc,ins.getLength(),ins);
            }
        }
    }
}
