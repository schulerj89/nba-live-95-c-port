import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;
public class DumpHumanPassCatch extends GhidraScript {
 public void run() throws Exception {
  int bank=Integer.parseInt(getScriptArgs()[1],16);
  int[][] ranges=bank==0x80?new int[][]{{0xcee7,0xcefc}}:bank==0x85?
   new int[][]{{0xf02d,0xf099},{0xf347,0xf3ba},{0xf5e4,0xf727}}:new int[][]{{0xad3d,0xae0f},{0xaf66,0xafa5}};
  for(int[] range:ranges){
   clearListing(toAddr(range[0]),toAddr(range[1]));
   for(String name:new String[]{"M","X"}){
    Register r=currentProgram.getRegister(name);
    if(r!=null)currentProgram.getProgramContext().setValue(r,toAddr(range[0]),toAddr(range[1]),BigInteger.ZERO);
   }
   disassemble(toAddr(range[0]));
  }
  if(bank==0x85)disassemble(toAddr(0xf34f));
  try(PrintWriter out=new PrintWriter(new File(getScriptArgs()[0],String.format("human_pass_catch_bank%02x.txt",bank)),"UTF-8")){
   for(int[] range:ranges)for(int pc=range[0];pc<=range[1];++pc){
    Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
    if(ins!=null)out.printf("$%02X:%04X [%d] %s%n",bank,pc,ins.getLength(),ins);
   }
  }
 }
}
