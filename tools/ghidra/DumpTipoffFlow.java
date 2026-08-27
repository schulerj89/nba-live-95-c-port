// Fresh tip contact/receiver/deflection/possession census. args: outDir
import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;
public class DumpTipoffFlow extends GhidraScript {
    public void run() throws Exception {
        int bank=currentProgram.getName().contains("85")?0x85:0x86;
        int[][] ranges=bank==0x86?new int[][]{
            {0x99c4,0x9c44},{0x9c45,0x9c6e},{0xb04c,0xb0e1},{0xccfc,0xd43d},
            {0xe054,0xe0ab},{0xec32,0xecf8}}:
            new int[][]{{0xb100,0xb28b}};
        for(int[] r:ranges) {
            clearListing(toAddr(r[0]),toAddr(r[1]));
            for(String n:new String[]{"M","X"}) {
                Register reg=currentProgram.getRegister(n);
                if(reg!=null)currentProgram.getProgramContext().setValue(reg,toAddr(r[0]),toAddr(r[1]),BigInteger.ZERO);
            }
            disassemble(toAddr(r[0]));
        }
        if(bank==0x86)for(int p:new int[]{0xd25a,0xd3c6,0xd3fd,0xb0d8,0xcf01,0xcf9d,0xcfa0})disassemble(toAddr(p));
        // These selected intervals are code-only. Decode branch alternatives
        // missed by the older trace-only listing, and reject partial tails.
        for(int[] r:ranges){int p=r[0];while(p<=r[1]){
            Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(p));
            if(ins==null){disassemble(toAddr(p));ins=currentProgram.getListing().getInstructionAt(toAddr(p));}
            if(ins==null)throw new Exception("decode gap at "+Integer.toHexString(p));
            p+=ins.getLength();
        }if(p!=r[1]+1)throw new Exception("range ends inside instruction "+Integer.toHexString(r[1]));}
        String[][] labels=bank==0x86?new String[][]{
            {"d3c6","TipContactState81","Tip-state collision branch. Establish branch census and native inputs before replacing frame198/200 bridge."},
            {"b04c","TipSelectReceiverAndLaunch","Native tip receiver uses RNG low bit plus team base. Calls99C4 for launch; not a fixed actor8."}
        }:new String[][]{};
        for(String[] row:labels){createLabel(toAddr(Integer.parseInt(row[0],16)),row[1],true);setPlateComment(toAddr(Integer.parseInt(row[0],16)),row[2]);}
        File dir=new File(getScriptArgs()[0]);dir.mkdirs();
        try(PrintWriter out=new PrintWriter(new File(dir,String.format("tip_flow_bank%02x.txt",bank)),"UTF-8")) {
            for(int[] r:ranges){int count=0;for(int p=r[0];p<=r[1];p++){
                Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(p));
                if(ins!=null){out.printf("$%02X:%04X [%d] %s%n",bank,p,ins.getLength(),ins);count++;}
            }out.printf("# %04X-%04X count=%d (contiguous code)%n",r[0],r[1],count);}
        }
    }
}
