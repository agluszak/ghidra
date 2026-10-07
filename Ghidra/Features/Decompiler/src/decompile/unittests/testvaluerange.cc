/* ###
 * IP: GHIDRA
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "architecture.hh"
#include "funcdata.hh"
#include "rangeutil.hh"
#include "test.hh"

namespace ghidra {

TEST(valueset_constant_mask_bounds) {
  DocumentStorage store;
  istringstream image("<binaryimage arch=\"x86:LE:32:default:windows\"></binaryimage>");
  store.registerTag(store.parseDocument(image)->getRoot());
  unique_ptr<Architecture> arch(ArchitectureCapability::getCapability("xml")->buildArchitecture("", "", &cout));
  arch->init(store);
  const uintb masks[] = { 0, 1, 2, 6, 0xff, 0xffffffff };
  int4 index = 0;
  for(int4 slot=0;slot<2;++slot) {
    for(uintb mask : masks) {
      Address pc(arch->getDefaultCodeSpace(),0x1000 + index++*0x10);
      Funcdata *fd = arch->symboltab->getGlobalScope()->addFunction(pc,"mask")->getFunction();
      BlockGraph blocks;
      BlockBasic *block = blocks.newBlockBasic(fd);
      Varnode *input = fd->setInputVarnode(fd->newUnique(4));
      PcodeOp *op = fd->newOp(2,pc);
      fd->opSetOpcode(op,CPUI_INT_AND);
      fd->opSetInput(op,fd->newConstant(4,mask),slot);
      fd->opSetInput(op,input,1-slot);
      Varnode *output = fd->newUniqueOut(4,op);
      fd->opInsertBegin(op,block);
      vector<Varnode *> sinks(1,output);
      vector<PcodeOp *> reads;
      ValueSetSolver solver;
      solver.establishValueSets(sinks,reads,(Varnode *)0,false);
      WidenerFull widener;
      solver.solve(100,widener);
      ASSERT(solver.getNumIterations() <= 100);
      const CircleRange &range = output->getValueSet()->getRange();
      ASSERT(!range.isEmpty());
      ASSERT_EQUALS(range.getMin(),0);
      ASSERT_EQUALS(range.getEnd(),(mask+1)&0xffffffff);
      for(uintb value=0;value<256;++value)
        ASSERT(range.contains(value&mask));
      ASSERT(range.contains(0xffffffff&mask));
    }
  }
}

} // End namespace ghidra
