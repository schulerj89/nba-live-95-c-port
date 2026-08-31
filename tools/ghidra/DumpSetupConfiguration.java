// Dedicated bounded configuration decode. New private project, no shared edits.
// args: output directory, optional bank hex (81 or82)
import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

public class DumpSetupConfiguration extends GhidraScript {
    private void decode(int first,int last,int...seeds)throws Exception{
        clearListing(toAddr(first),toAddr(last));
        for(String name:new String[]{"M","X"}){
            Register reg=currentProgram.getRegister(name);
            if(reg!=null)currentProgram.getProgramContext().setValue(
                reg,toAddr(first),toAddr(last),BigInteger.ZERO);
        }
        for(int seed:seeds)disassemble(toAddr(seed));
    }
    public void run()throws Exception{
        File dir=new File(getScriptArgs()[0]);dir.mkdirs();
        int bank=getScriptArgs().length>1?Integer.parseInt(getScriptArgs()[1],16):0x81;
        if(bank==0x81){
            decode(0xab58,0xac03,0xab58);
            decode(0xbfaa,0xc00a,0xbfaa);
            decode(0xc19a,0xc41d,0xc19a,0xc24b,0xc398,0xc3d5);
            decode(0xd318,0xd59a,0xd318,0xd446,0xd47a,0xd4c0,0xd516);
        }else if(bank==0x82){
            decode(0x8cd1,0x8f9b,0x8cd1,0x8d3c,0x8d92,0x8e5f,0x8eab);
        }else throw new Exception("unsupported bank");
        try(PrintWriter out=new PrintWriter(new File(dir,String.format("setup_config_bank%02x.txt",bank)),"UTF-8")){
            for(int pc=0x8000;pc<=0xffff;++pc){
                Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
                if(ins!=null)out.printf("$%02X:%04X [%d] %s%n",bank,pc,ins.getLength(),ins);
            }
        }
    }
}
