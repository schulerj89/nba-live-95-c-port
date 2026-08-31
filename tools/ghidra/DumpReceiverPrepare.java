import java.io.*;
import java.math.BigInteger;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.lang.Register;
import ghidra.program.model.listing.Instruction;
public class DumpReceiverPrepare extends GhidraScript {
 public void run() throws Exception {
  int bank=Integer.parseInt(getScriptArgs()[1],16);
  int[][] ranges=bank==0x80?new int[][]{{0xcee7,0xcefc},{0xad57,0xad85},{0xb79e,0xb7c5}}:
   bank==0x85?new int[][]{{0xf867,0xf8d8},{0xf8d9,0xf928}}:
   bank==0x86?new int[][]{{0xaa6a,0xab0c},{0xaf66,0xafa5},{0xb0e2,0xb0f6},{0xb468,0xb624}}:
   new int[][]{{0xb7d8,0xb952}};
  for(int[] range:ranges){
   clearListing(toAddr(range[0]),toAddr(range[1]));
   for(String name:new String[]{"M","X"}){
    Register r=currentProgram.getRegister(name);
    if(r!=null)currentProgram.getProgramContext().setValue(r,toAddr(range[0]),toAddr(range[1]),BigInteger.ZERO);
   }
   disassemble(toAddr(range[0]));
  }
  if(bank==0x87)disassemble(toAddr(0xb832));
  try(PrintWriter out=new PrintWriter(new File(getScriptArgs()[0],String.format("receiver_prepare_bank%02x.txt",bank)),"UTF-8")){
   for(int[] range:ranges)for(int pc=range[0];pc<=range[1];++pc){
    Instruction ins=currentProgram.getListing().getInstructionAt(toAddr(pc));
    if(ins!=null)out.printf("$%02X:%04X [%d] %s%n",bank,pc,ins.getLength(),ins);
   }
  }
 }
}
