import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

public class DumpHumanPassAligned extends GhidraScript {
 public void run() throws Exception {
  int bank=Integer.parseInt(getScriptArgs()[1],16);
  int start=bank==0x85?0xf473:bank==0x86?0xad0e:0xb3bd, end=bank==0x85?0xf5e3:bank==0x86?0xaf2f:0xb4da;
  clearListing(toAddr(start),toAddr(end));
  for(String name:new String[]{"M","X"}){
   Register r=currentProgram.getRegister(name);
   if(r!=null)currentProgram.getProgramContext().setValue(r,toAddr(start),toAddr(end),BigInteger.ZERO);
  }
  int[] seeds=bank==0x85?new int[]{0xf473}:bank==0x86?new int[]{0xad0e,0xae10,0xae52,0xaed9}:new int[]{0xb3bd,0xb47a};
  for(int seed:seeds)disassemble(toAddr(seed));
  try(PrintWriter out=new PrintWriter(new File(getScriptArgs()[0],String.format("human_pass_aligned_bank%02x.txt",bank)),"UTF-8")){
   for(int pc=start;pc<=end;++pc){
    Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
    if(ins!=null)out.printf("$%02X:%04X [%d] %s%n",bank,pc,ins.getLength(),ins);
   }
  }
 }
}
