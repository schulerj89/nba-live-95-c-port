import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;
public class DumpHumanPassLaunch extends GhidraScript {
 public void run() throws Exception {
  int bank=Integer.parseInt(getScriptArgs()[1],16);
  int[][] ranges=bank==0x85?new int[][]{{0xf78b,0xf820},{0xf867,0xf8d8},{0xf8d9,0xf928}}:new int[][]{{0x9846,0x986c},{0x99c4,0x9c6e}};
  for(int[] range:ranges){
   clearListing(toAddr(range[0]),toAddr(range[1]));
   for(String name:new String[]{"M","X"}){
    Register r=currentProgram.getRegister(name);
    if(r!=null)currentProgram.getProgramContext().setValue(r,toAddr(range[0]),toAddr(range[1]),BigInteger.ZERO);
   }
   disassemble(toAddr(range[0]));
  }
  if(bank==0x86)for(int pc:new int[]{0x9bb1,0x9bb8,0x9bfb,0x9c02,0x9c45})disassemble(toAddr(pc));
  try(PrintWriter out=new PrintWriter(new File(getScriptArgs()[0],String.format("human_pass_launch_bank%02x.txt",bank)),"UTF-8")){
   for(int[] range:ranges)for(int pc=range[0];pc<=range[1];++pc){
    Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
    if(ins!=null)out.printf("$%02X:%04X [%d] %s%n",bank,pc,ins.getLength(),ins);
   }
  }
 }
}
