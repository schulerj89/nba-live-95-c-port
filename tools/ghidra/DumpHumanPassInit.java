import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;

public class DumpHumanPassInit extends GhidraScript {
 public void run() throws Exception {
  int bank=Integer.parseInt(getScriptArgs()[1],16);
  int start=bank==0x85?0xf3bb:bank==0x86?0xab2d:0xb538, end=bank==0x85?0xf5e3:bank==0x86?0xaf65:0xb554;
  clearListing(toAddr(start),toAddr(end));
  for(String name:new String[]{"M","X"}){
   Register r=currentProgram.getRegister(name);
   if(r!=null)currentProgram.getProgramContext().setValue(r,toAddr(start),toAddr(end),BigInteger.ZERO);
  }
  int[] seeds=bank==0x85?new int[]{0xf3bb,0xf3c3,0xf473}:bank==0x86?new int[]{0xab2d,0xac50,0xac5f,0xad0e,0xae10,0xae52}:new int[]{0xb538};
  for(int seed:seeds)disassemble(toAddr(seed));
  try(PrintWriter out=new PrintWriter(new File(getScriptArgs()[0],String.format("human_pass_init_bank%02x.txt",bank)),"UTF-8")){
   for(int pc=start;pc<=end;++pc){
    Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
    if(ins!=null)out.printf("$%02X:%04X [%d] %s%n",bank,pc,ins.getLength(),ins);
   }
  }
 }
}
