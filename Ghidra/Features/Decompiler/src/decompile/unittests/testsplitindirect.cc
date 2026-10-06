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
#include "double.hh"
#include "funcdata.hh"
#include "test.hh"
#include <iostream>

namespace ghidra {

TEST(split_indirect_reuses_whole) {
  DocumentStorage store;
  istringstream image("<binaryimage arch=\"x86:LE:32:default:windows\"></binaryimage>");
  store.registerTag(store.parseDocument(image)->getRoot());
  unique_ptr<Architecture> arch(ArchitectureCapability::getCapability("xml")->buildArchitecture("", "", &cout));
  arch->init(store);
  Address pc(arch->getDefaultCodeSpace(),0x1000);
  Funcdata *fd = arch->symboltab->getGlobalScope()->addFunction(pc,"split_effect")->getFunction();
  fd->getScopeLocal()->addSymbol("value",arch->types->getBase(8,TYPE_FLOAT),Address(arch->getStackSpace(),0xfffffff0),Address());
  fd->getScopeLocal()->addSymbol("other",arch->types->getBase(8,TYPE_FLOAT),Address(arch->getStackSpace(),0xffffffe0),Address());
  arch->symboltab->setPropertyRange(Varnode::addrtied,Range(arch->getStackSpace(),0xffffffe0,0xfffffff7));
  BlockGraph blocks;
  BlockBasic *block = blocks.newBlockBasic(fd);
  PcodeOp *call = fd->newOp(1,pc);
  fd->opSetOpcode(call,CPUI_CALL);
  fd->opSetInput(call,fd->newConstant(4,0x2000),0);
  fd->opInsertBegin(call,block);

  Varnode *input = fd->setInputVarnode(fd->newUnique(8));
  SplitVarnode source;
  source.initAll(input,fd->newUnique(4),fd->newUnique(4));
  Varnode *whole = (Varnode *)0;
  for(int i=0;i<2;++i) {
    Address addr(arch->getStackSpace(),0xfffffff0);
    PcodeOp *loOp = fd->newIndirectOp(call,addr,4,0);
    PcodeOp *hiOp = fd->newIndirectOp(call,addr+4,4,0);
    SplitVarnode output(loOp->getOut(),hiOp->getOut());
    SplitVarnode::replaceIndirectOp(*fd,output,source,call);
    if(i==0)
      whole = output.getWhole();
    else
      ASSERT(output.getWhole()==whole);
    ASSERT(loOp->code()==CPUI_SUBPIECE);
    ASSERT(hiOp->code()==CPUI_SUBPIECE);
    ASSERT(loOp->getIn(0)==whole);
    ASSERT(hiOp->getIn(0)==whole);
  }
  int count = 0;
  for(auto iter=input->beginDescend();iter!=input->endDescend();++iter)
    if((*iter)->code()==CPUI_INDIRECT) ++count;
  ASSERT_EQUALS(count,1);

  // An effect on different storage must retain its own whole value.
  Address other(arch->getStackSpace(),0xffffffe0);
  PcodeOp *loOp = fd->newIndirectOp(call,other,4,0);
  PcodeOp *hiOp = fd->newIndirectOp(call,other+4,4,0);
  SplitVarnode output(loOp->getOut(),hiOp->getOut());
  SplitVarnode::replaceIndirectOp(*fd,output,source,call);
  ASSERT(output.getWhole()!=whole);
}

} // End namespace ghidra
