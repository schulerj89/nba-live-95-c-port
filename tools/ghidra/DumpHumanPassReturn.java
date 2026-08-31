import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;
public class DumpHumanPassReturn extends GhidraScript {
 public void run() throws Exception {
  int bank=Integer.parseInt(getScriptArgs()[1],16);
  int[][] ranges=bank==0x84?new int[][]{{0xdf7a,0xdf89},{0xe09c,0xe0b4},{0xe2ac,0xe2ad},{0xe2e8,0xe2ea},{0xe3e6,0xe3e9}}:new int[][]{{0xab2d,0xab3c},{0xaf4d,0xaf65}};
  for(int[] range:ranges){
   clearListing(toAddr(range[0]),toAddr(range[1]));
   for(String name:new String[]{"M","X"}){
    Register r=currentProgram.getRegister(name);
    if(r!=null)currentProgram.getProgramContext().setValue(r,toAddr(range[0]),toAddr(range[1]),BigInteger.ZERO);
   }
   disassemble(toAddr(range[0]));
  }
  try(PrintWriter out=new PrintWriter(new File(getScriptArgs()[0],String.format("human_pass_return_bank%02x.txt",bank)),"UTF-8")){
   for(int[] range:ranges)for(int pc=range[0];pc<=range[1];++pc){
    Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
    if(ins!=null)out.printf("$%02X:%04X [%d] %s%n",bank,pc,ins.getLength(),ins);
   }
  }
 }
}
