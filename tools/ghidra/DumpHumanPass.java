import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

public class DumpHumanPass extends GhidraScript {
 public void run() throws Exception {
  int bank=Integer.parseInt(getScriptArgs()[1],16);
  int start=bank==0x84?0xdf7a:0xf1c1, end=bank==0x84?0xe140:0xf3ba;
  clearListing(toAddr(start),toAddr(end));
  for(String name:new String[]{"M","X"}){
   Register r=currentProgram.getRegister(name);
   if(r!=null)currentProgram.getProgramContext().setValue(r,toAddr(start),toAddr(end),BigInteger.ZERO);
  }
  int[] seeds=bank==0x84?new int[]{0xdf7a,0xe03c,0xe0b5}:new int[]{0xf1c1,0xf347,0xf34f};
  for(int seed:seeds)disassemble(toAddr(seed));
  try(PrintWriter out=new PrintWriter(new File(getScriptArgs()[0],String.format("human_pass_bank%02x.txt",bank)),"UTF-8")){
   for(int pc=start;pc<=end;++pc){
    Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
    if(ins!=null)out.printf("$%02X:%04X [%d] %s%n",bank,pc,ins.getLength(),ins);
   }
  }
 }
}
