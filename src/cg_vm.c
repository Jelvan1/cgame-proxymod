/*
  ==============================
  Written by id software, nightmare and hk of mdd
  This file is part of mdd client proxymod.

  mdd client proxymod is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  mdd client proxymod is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with mdd client proxymod.  If not, see <http://www.gnu.org/licenses/>.
  ==============================
  Note: mdd client proxymod contains large quantities from the quake III arena source code
  Note: mdd client proxymod contains code from Kevin Masterson a.k.a. CyberMind <kevinm@planetquake.com>
    of the QMM - Q3 MultiMod
*/
#include "cg_vm.h"

#include "assert.h"
#include "cg_ammo.h"
#include "cg_cgaz.h"
#include "cg_hud.h"
#include "cg_jump.h"
#include "cg_local.h"
#include "cg_snap.h"
#include "cg_syscall.h"
#include "cg_timer.h"
#include "compass.h"
#include "defrag.h"

#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_VMPATH "vm/cgame.qvm"

/* VM_Run, VM_Exec, VM_Create, VM_Destroy, and VM_Restart
 * originally from Q3Fusion (http://www.sourceforge.net/projects/q3fusion/)
 */

// executes the VM (only entry point = vmMain, start of codeSegment)
// all the opStack, opPointer, opBase, etc initialization has been done in VM_Exec
// modified to include real (non-VM) pointer support
//---
// vm = pointer to VM

#define byteswap int_byteswap

static void VM_Run(vm_t* vm)
{
  vmOps_t op;
  int32_t param;

  // local registers
  int32_t* opStack;
  int32_t* opPointer;

  // constants /not changed during execution/
  byte*    dataSegment;
  uint32_t dataSegmentMask;

  opStack   = vm->opStack;
  opPointer = vm->opPointer;

  dataSegment     = vm->dataSegment;
  dataSegmentMask = vm->dataSegmentMask;

  // keep going until opPointer is NULL
  // opPointer is set in OP_LEAVE, stored in the function stack
  // VM_Exec sets this to NULL before calling so that as soon as vmMain is done, execution stops

#ifndef NDEBUG
  int32_t nbfunc = 0;
#endif
  do
  {
    // fetch opcode
    op = opPointer[0];
    // get the param
    param = opPointer[1];
    // move to the next opcode
    opPointer += 2;

    // here's the magic
    switch (op)
    {
      //
      // aux
      //
      // undefined
    case OP_UNDEF:
    // no op?
    case OP_NOP:
    // break to debugger?
    case OP_BREAK:
    // anything else
    default:
      trap_Error(vaf("ERROR: VM_Run: Unhandled opcode(%i)", op));
      break;

//
// subroutines
//
// jumps to a specific opcode
#define GOTO(x)                                                                                                        \
  {                                                                                                                    \
    opPointer = vm->codeSegment + (x)*2;                                                                               \
  }

    // enter a function, assign function parameters (length=param) from stack
    case OP_ENTER:
#ifndef NDEBUG
      // trap_Print(vaf("OP_ENTER: %d\n", nbfunc));
      ++nbfunc;
#endif
      {
        intptr_t const subFunc = (opPointer - 2 - vm->codeSegment) / 2;
        if (subFunc == DF_CG_DRAW2D_DEFRAG || subFunc == DF_CG_DRAW2D_VANILLA)
        {
          draw_hud();
        }
      }

      vm->opBase -= param;
      *((int32_t*)(dataSegment + vm->opBase) + 1) = *opStack++;
      break;

    // leave a function, move opcode pointer to previous function
    case OP_LEAVE:
#ifndef NDEBUG
      --nbfunc;
      // trap_Print(vaf("OP_LEAVE: %d\n", nbfunc));
#endif
      opPointer = vm->codeSegment + *((int32_t*)(dataSegment + vm->opBase) + 1);
      vm->opBase += param;
      break;

    // call a function at address stored in opStack[0]
    case OP_CALL:
      param = opStack[0];

      // CyberMind - param(opStack[0]) is the function address, negative means a engine trap
      // added fix for external function pointers
      // if param is greater than the memorySize, it's a real function pointer, so call it
      if (param < 0 || param >= vm->memorySize)
      {
        int32_t  ret  = 0;
        int32_t* args = NULL;
        // int      *fp;

        // system trap or real system function

        // save local registers for recursive execution
        // vm->opBase = opBase;
        vm->opStack   = opStack;
        vm->opPointer = opPointer;

        // clear hook var
        vm->hook_realfunc = 0;

        args = (int32_t*)(dataSegment + vm->opBase) + 2;

        // if a trap function, call our local syscall, which parses each message
        if (param < 0)
        {
          ret = (int32_t)CG_SysCalls(dataSegment, -param - 1, args);
          // otherwise it's a real function call, grab args and call function
        }
        else
        {
          // cdecl calling convention says caller (us) cleans stack
          // so we can stuff the args without worry of stack corruption
          typedef uint32_t (*pfn_t)(
            int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
          ret = ((pfn_t)(intptr_t)param)(
            args[0],
            args[1],
            args[2],
            args[3],
            args[4],
            args[5],
            args[6],
            args[7],
            args[8],
            args[9],
            args[10],
            args[11]);
        }

        // restore local registers
        // opBase = vm->opBase;
        opStack   = vm->opStack;
        opPointer = vm->opPointer;

        // if we are running a VM function due to hook
        // and we have a real VM func to call, call it
        if (vm->hook_realfunc && param >= vm->memorySize)
        {
          // replace func address with return address
          opStack[0] = (int32_t)(opPointer - vm->codeSegment);
          GOTO(vm->hook_realfunc)
          // otherwise we use the syscall/hook func return value
        }
        else
        {
          opStack[0] = ret;
        }
        break;
      }
      // replace func address with return address
      opStack[0] = (int32_t)(opPointer - vm->codeSegment); // push pc /return address/
      // jump to VM function at address
      GOTO(param)
      break;

      //
      // stack
      //
      // pushes a 0 onto the end of the stack
    case OP_PUSH:
      opStack--;
      opStack[0] = 0;
      break;
    // pops the last value off the end of the stack
    case OP_POP:
      opStack++;
      break;
    // pushes a specified value onto the end of the stack
    case OP_CONST:
      opStack--;
      opStack[0] = param;
      break;
    // pushes a specified
    case OP_LOCAL:
      opStack--;
      opStack[0] = param + vm->opBase;
      break;

//
// branching
//
#define SOP(operation)                                                                                                 \
  {                                                                                                                    \
    if (opStack[1] operation opStack[0]) GOTO(param);                                                                  \
    opStack += 2;                                                                                                      \
  }
#define UOP(operation)                                                                                                 \
  {                                                                                                                    \
    if (*(uint32_t*)&opStack[1] operation * (uint32_t*)&opStack[0]) GOTO(param);                                       \
    opStack += 2;                                                                                                      \
  }
#define FOP(operation)                                                                                                 \
  {                                                                                                                    \
    if (*(float*)&opStack[1] operation * (float*)&opStack[0]) GOTO(param);                                             \
    opStack += 2;                                                                                                      \
  }

    // jump to address in opStack[0], and pop
    case OP_JUMP:
      GOTO(*opStack++) break;
    // if opStack[1] == opStack[0], goto address in param
    case OP_EQ:
      SOP(==) break;
    // if opStack[1] != opStack[0], goto address in param
    case OP_NE:
      SOP(!=) break;
    // if opStack[1] < opStack[0], goto address in param
    case OP_LTI:
      SOP(<) break;
    // if opStack[1] <= opStack[0], goto address in param
    case OP_LEI:
      SOP(<=) break;
    // if opStack[1] > opStack[0], goto address in param
    case OP_GTI:
      SOP(>) break;
    // if opStack[1] >= opStack[0], goto address in param
    case OP_GEI:
      SOP(>=) break;
    // if opStack[1] < opStack[0], goto address in param (uint32_t)
    case OP_LTU:
      UOP(<) break;
    // if opStack[1] <= opStack[0], goto address in param (uint32_t)
    case OP_LEU:
      UOP(<=) break;
    // if opStack[1] > opStack[0], goto address in param (uint32_t)
    case OP_GTU:
      UOP(>) break;
    // if opStack[1] >= opStack[0], goto address in param (uint32_t)
    case OP_GEU:
      UOP(>=) break;
    // if opStack[1] == opStack[0], goto address in param (float)
    case OP_EQF:
      FOP(==) break;
    // if opStack[1] != opStack[0], goto address in param (float)
    case OP_NEF:
      FOP(!=) break;
    // if opStack[1] < opStack[0], goto address in param (float)
    case OP_LTF:
      FOP(<) break;
    // if opStack[1] <= opStack[0], goto address in param (float)
    case OP_LEF:
      FOP(<=) break;
    // if opStack[1] > opStack[0], goto address in param (float)
    case OP_GTF:
      FOP(>) break;
    // if opStack[1] >= opStack[0], goto address in param (float)
    case OP_GEF:
      FOP(>=) break;

      //
      // memory I/O: masks protect main memory
      //

    // get value at address stored in opStack[0], store in opStack[0]
    //(do necessary conversions)
    // this is essentially the 'dereferencing' opcode set
    // 1-byte
    case OP_LOAD1:
      if (opStack[0] >= vm->memorySize)
        opStack[0] = *(byte*)(intptr_t)(opStack[0]);
      else
        opStack[0] = dataSegment[opStack[0] & dataSegmentMask];

      break;

    // 2-byte
    case OP_LOAD2:
      if (opStack[0] >= vm->memorySize)
        opStack[0] = *(uint16_t*)(intptr_t)(opStack[0]);
      else
        opStack[0] = *(uint16_t*)&dataSegment[opStack[0] & dataSegmentMask];

      break;

    // 4-byte
    case OP_LOAD4:
      if (opStack[0] >= vm->memorySize)
        opStack[0] = *(int32_t*)(intptr_t)(opStack[0]);
      else
        opStack[0] = *(int32_t*)&dataSegment[opStack[0] & dataSegmentMask];

      break;

    // store a value from opStack[0] into address stored in opStack[1]
    // 1-byte
    case OP_STORE1:
      if (opStack[1] >= vm->memorySize)
        *(byte*)(intptr_t)(opStack[1]) = (byte)(opStack[0] & 0xFF);
      else
        dataSegment[opStack[1] & dataSegmentMask] = (byte)(opStack[0] & 0xFF);

      opStack += 2;
      break;
    // 2-byte
    case OP_STORE2:
      if (opStack[1] >= vm->memorySize)
        *(uint16_t*)(intptr_t)(opStack[1]) = (uint16_t)(opStack[0] & 0xFFFF);
      else
        *(uint16_t*)&dataSegment[opStack[1] & dataSegmentMask] = (uint16_t)(opStack[0] & 0xFFFF);

      opStack += 2;
      break;
    // 4-byte
    case OP_STORE4:
      if (opStack[1] >= vm->memorySize)
        *(int32_t*)(intptr_t)(opStack[1]) = opStack[0];
      else
        *(int32_t*)&dataSegment[opStack[1] & dataSegmentMask] = opStack[0];

      opStack += 2;
      break;

    // set a function-call arg (offset = param) to the value in opStack[0]
    case OP_ARG:
      *(int32_t*)&dataSegment[(param + vm->opBase) & dataSegmentMask] = opStack[0];
      opStack++;
      break;

    // copy mem at address pointed to by opStack[0] to address pointed to by opStack[1]
    // for 'param' number of bytes
    case OP_BLOCK_COPY:
    {
      int32_t* from = (int32_t*)&dataSegment[opStack[0] & dataSegmentMask];
      int32_t* to   = (int32_t*)&dataSegment[opStack[1] & dataSegmentMask];

      if (param & 3)
      {
        trap_Error("[QMMVM] VM_Run: OP_BLOCK_COPY not dword aligned");
      }

      // FIXME: assume pointers don't overlap?
      param >>= 2;
      do
      {
        *to++ = *from++;
      } while (--param);

      opStack += 2;
    }
    break;

//
// arithmetic and logic
//
#undef SOP
#undef UOP
#undef FOP
// signed arithmetic
#define SOP(operation)                                                                                                 \
  {                                                                                                                    \
    opStack[1] operation opStack[0];                                                                                   \
    opStack++;                                                                                                         \
  }
// unsigned arithmetic
#define UOP(operation)                                                                                                 \
  {                                                                                                                    \
    *(uint32_t*)&opStack[1] operation*(uint32_t*)&opStack[0];                                                          \
    opStack++;                                                                                                         \
  }
// floating point arithmetic
#define FOP(operation)                                                                                                 \
  {                                                                                                                    \
    *(float*)&opStack[1] operation*(float*)&opStack[0];                                                                \
    opStack++;                                                                                                         \
  }
// signed arithmetic (on self)
#define SSOP(operation)                                                                                                \
  {                                                                                                                    \
    opStack[0] = operation opStack[0];                                                                                 \
  }
// floating point arithmetic (on self)
#define SFOP(operation)                                                                                                \
  {                                                                                                                    \
    *(float*)&opStack[0] = operation * (float*)&opStack[0];                                                            \
  }

    // sign extensions
    case OP_SEX8:
      if (opStack[0] & 0x80) opStack[0] |= 0xFFFFFF00;
      break;
    case OP_SEX16:
      if (opStack[0] & 0x8000) opStack[0] |= 0xFFFF0000;
      break;
    // make negative
    case OP_NEGI:
      SSOP(-) break;
    // add opStack[0] to opStack[1], store in opStack[1]
    case OP_ADD:
      SOP(+=) break;
    // subtract opStack[0] from opStack[1], store in opStack[1]
    case OP_SUB:
      SOP(-=) break;
    // divide opStack[0] into opStack[1], store in opStack[1]
    case OP_DIVI:
      SOP(/=) break;
    // divide opStack[0] into opStack[1], store in opStack[1] (unsigned)
    case OP_DIVU:
      UOP(/=) break;
    // modulus opStack[0] into opStack[1], store in opStack[1]
    case OP_MODI:
      SOP(%=) break;
    // modulus opStack[0] into opStack[1], store in opStack[1] (unsigned)
    case OP_MODU:
      UOP(%=) break;
    // multiply opStack[0] and opStack[1], store in opStack[1]
    case OP_MULI:
      SOP(*=) break;
    // multiply opStack[0] and opStack[1], store in opStack[1] (unsigned)
    case OP_MULU:
      UOP(*=) break;
    // bitwise AND opStack[0] and opStack[1], store in opStack[1]
    case OP_BAND:
      SOP(&=) break;
    // bitwise OR opStack[0] and opStack[1], store in opStack[1]
    case OP_BOR:
      SOP(|=) break;
    // bitwise XOR opStack[0] and opStack[1], store in opStack[1]
    case OP_BXOR:
      SOP(^=) break;
    // bitwise one's compliment opStack[0], store in opStack[1]
    case OP_BCOM:
      SSOP(~) break;
    // bitwise LEFTSHIFT opStack[1] by opStack[0] bits, store in opStack[1]
    case OP_LSH:
      UOP(<<=) break;
    // bitwise RIGHTSHIFT opStack[1] by opStack[0] bits, store in opStack[1]
    case OP_RSHI:
      SOP(>>=) break;
    // bitwise RIGHTSHIFT opStack[1] by opStack[0] bits, store in opStack[1] (unsigned)
    case OP_RSHU:
      UOP(>>=) break;
    // make negative (float)
    case OP_NEGF:
      SFOP(-) break;
    // add opStack[0] to opStack[1], store in opStack[1] (float)
    case OP_ADDF:
      FOP(+=) break;
    // subtract opStack[0] from opStack[1], store in opStack[1] (float)
    case OP_SUBF:
      FOP(-=) break;
    // divide opStack[0] into opStack[1], store in opStack[1] (float)
    case OP_DIVF:
      FOP(/=) break;
    // multiply opStack[0] and opStack[1], store in opStack[1] (float)
    case OP_MULF:
      FOP(*=) break;

      //
      // format conversion
      //
      // convert opStack[0] int32_t->float
    case OP_CVIF:
      *(float*)&opStack[0] = (float)opStack[0];
      break;
    // convert opStack[0] float->int32_t
    case OP_CVFI:
      opStack[0] = (int32_t)(*(float*)&opStack[0]);
      break;
    }
  } while ((int32_t)(intptr_t)opPointer);
  ASSERT_EQ(nbfunc, 0);

  //  vm->opBase = opBase;
  vm->opStack = opStack;
  //  vm->opPointer = opPointer;
}

// public function to begin the process of executing a VM
//----
// stuff args into the VM stack
// begin VM with VM_Run
//---
// vm = pointer to to VM
// command = GAME instruction to run
// arg# = args to command
intptr_t QDECL VM_Exec(
  vm_t*   vm,
  int32_t command,
  int32_t arg0,
  int32_t arg1,
  int32_t arg2,
  int32_t arg3,
  int32_t arg4,
  int32_t arg5,
  int32_t arg6,
  int32_t arg7,
  int32_t arg8,
  int32_t arg9,
  int32_t arg10,
  int32_t arg11)
{
  // intptr_t QDECL VM_Exec(vm_t *vm, int32_t command, ...) {
  int32_t* args;

  // prepare local stack
  vm->opBase -= 15 * sizeof(int32_t);
  args = (int32_t*)(vm->dataSegment + vm->opBase);

  // push all params
  args[0]  = 0;
  args[1]  = (int32_t)(vm->opPointer - vm->codeSegment); // save pc
  args[2]  = command;
  args[3]  = arg0;
  args[4]  = arg1;
  args[5]  = arg2;
  args[6]  = arg3;
  args[7]  = arg4;
  args[8]  = arg5;
  args[9]  = arg6;
  args[10] = arg7;
  args[11] = arg8;
  args[12] = arg9;
  args[13] = arg10;
  args[14] = arg11;

  vm->opPointer = NULL; // VM_Run stops execution when opPointer is NULL

  //(ready) move back in stack to save pc
  vm->opStack--;
  vm->opStack[0] = (int32_t)(vm->opPointer - vm->codeSegment);
  //(set) move opPointer to start of opcodes
  vm->opPointer = vm->codeSegment;

  // GO!
  VM_Run(vm);

  // restore previous state
  vm->opPointer = vm->codeSegment + args[1];
  vm->opBase += 15 * sizeof(int32_t);

  // pick return value from stack
  return *vm->opStack++;
}

/*
==================
crc32_buffer
==================
*/
static uint32_t crc32_reflect(byte const* buf, int32_t len)
{
  // clang-format off
  static uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, //   0 [0x00 .. 0x07]
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, //   8 [0x08 .. 0x0F]
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, //  16 [0x10 .. 0x17]
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, //  24 [0x18 .. 0x1F]
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, //  32 [0x20 .. 0x27]
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, //  40 [0x28 .. 0x2F]
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, //  48 [0x30 .. 0x37]
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, //  56 [0x38 .. 0x3F]
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, //  64 [0x40 .. 0x47]
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01, //  72 [0x48 .. 0x4F]
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, //  80 [0x50 .. 0x57]
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, //  88 [0x58 .. 0x5F]
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, //  96 [0x60 .. 0x67]
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, // 104 [0x68 .. 0x6F]
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, // 112 [0x70 .. 0x77]
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, // 120 [0x78 .. 0x7F]
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, // 128 [0x80 .. 0x87]
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, // 136 [0x88 .. 0x8F]
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, // 144 [0x90 .. 0x97]
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, // 152 [0x98 .. 0x9F]
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B, // 160 [0xA0 .. 0xA7]
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79, // 168 [0xA8 .. 0xAF]
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, // 176 [0xB0 .. 0xB7]
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, // 184 [0xB8 .. 0xBF]
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, // 192 [0xC0 .. 0xC7]
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, // 200 [0xC8 .. 0xCF]
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, // 208 [0xD0 .. 0xD7]
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45, // 216 [0xD8 .. 0xDF]
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, // 224 [0xE0 .. 0xE7]
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9, // 232 [0xE8 .. 0xEF]
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, // 240 [0xF0 .. 0xF7]
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D  // 248 [0xF8 .. 0xFF]
  };
  // clang-format on

  uint32_t crc = 0xFFFFFFFFUL;
  while (len--) crc = crc32_table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
  return crc ^ 0xFFFFFFFFUL;
}

// load the .qvm into the vm_t
//---
// this function opens the .qvm in a file stream, stores in dynamic mem
// reads header info, and loads vm into memory
//----
// vm = pointer to vm_t to load into
// path = filename to load
// oldmem = location to use for VM memory (default NULL)
qboolean VM_Create(vm_t* vm, char const* path, byte* oldmem)
{
  vmHeader_t*  header;
  byte*        vmBase;
  byte*        src;
  int32_t*     lsrc;
  int32_t*     dst;
  vmOps_t      op;
  int32_t      codeSegmentSize;
  fileHandle_t fvm;
  vm->swapped = qfalse;

  if (!vm || !path || !path[0]) return qfalse;

  // open VM file (use engine calls so we can easily read into .pk3)
  vm->fileSize = trap_FS_FOpenFile(path, &fvm, FS_READ);
  // allocate memory block the size of the file
  vmBase = (byte*)malloc(vm->fileSize);

  // malloc failed
  if (!vmBase)
  {
    memset(vm, 0, sizeof(vm_t));
    return qfalse;
  }

  // read VM file into memory block
  trap_FS_Read(vmBase, vm->fileSize, fvm);
  trap_FS_FCloseFile(fvm);

  header = (vmHeader_t*)vmBase;

  // if we are a big-endian machine, need to swap everything around
  if (header->vmMagic == VM_MAGIC_BIG)
  {
    // RS_Printf("WARNING: VM_Create: Big-endian magic number detected, will byteswap during load.\n");
    vm->swapped              = qtrue;
    header->vmMagic          = byteswap(header->vmMagic);
    header->instructionCount = byteswap(header->instructionCount);
    header->codeOffset       = byteswap(header->codeOffset);
    header->codeLength       = byteswap(header->codeLength);
    header->dataOffset       = byteswap(header->dataOffset);
    header->dataLength       = byteswap(header->dataLength);
    header->litLength        = byteswap(header->litLength);
    header->bssLength        = byteswap(header->bssLength);
  }
  vm->header = *header; // save header info in vm_t

  // check file
  if (header->vmMagic != VM_MAGIC || header->instructionCount <= 0 || header->codeLength <= 0)
  {
    free(vmBase);
    memset(vm, 0, sizeof(vm_t));
    return qfalse;
  }

  // check defrag version
  uint32_t const crc32sum = crc32_reflect((byte const*)header, vm->fileSize);
  if (crc32sum != DF_CRC32SUM)
  {
    free(vmBase);
    memset(vm, 0, sizeof(vm_t));
    trap_Error(vaf(
      "Proxymod only works with defrag %s. "
      "Download it from https://q3defrag.org/files/defrag/defrag_%s.zip.\n",
      DF_VERSION,
      DF_VERSION));
    return qfalse;
  }

  // setup segments
  vm->codeSegmentLen = header->instructionCount;
  vm->dataSegmentLen = header->dataLength + header->litLength + header->bssLength;

  // calculate memory protection mask (including the stack?)
  for (vm->dataSegmentMask = 1;; vm->dataSegmentMask <<= 1)
  {
    if (vm->dataSegmentMask > vm->dataSegmentLen + vm_stacksize)
    {
      vm->dataSegmentMask--;
      break;
    }
  }

  // each opcode is 2 ints long, calculate total size of opcodes
  codeSegmentSize = vm->codeSegmentLen * sizeof(int32_t) * 2;

  vm->memorySize = codeSegmentSize + vm->dataSegmentLen + vm_stacksize;
  // load memory code block (freed in VM_Destroy)
  // if we are reloading, we should keep the same memory location, otherwise, make more
  vm->memory = (oldmem ? oldmem : (byte*)malloc(vm->memorySize));
  // malloc failed
  if (!vm->memory)
  {
    // RS_Printf("Unable to allocate VM memory chunk (size=%i)\n", vm->memorySize);
    free(vmBase);
    memset(vm, 0, sizeof(vm_t));
    return qfalse;
  }
  // clear the memory
  memset(vm->memory, 0, vm->memorySize);

  // set pointers
  vm->codeSegment  = (int32_t*)vm->memory;
  vm->dataSegment  = (byte*)(vm->memory + codeSegmentSize);
  vm->stackSegment = (byte*)(vm->dataSegment + vm->dataSegmentLen);

  // setup registers
  vm->opPointer = NULL;
  vm->opStack   = (int32_t*)(vm->stackSegment + vm_stacksize);
  vm->opBase    = vm->dataSegmentLen + vm_stacksize / 2;

  // load instructions from file to memory
  src = vmBase + header->codeOffset;
  dst = vm->codeSegment;

  // loop through each instruction
  for (int32_t n = 0; n < header->instructionCount; ++n)
  {
    // get its opcode and move src to the parameter field
    op = (vmOps_t)*src++;
    // write opcode (as int32_t) and move dst to next int32_t
    *dst++ = (int32_t)op;

    switch (op)
    {
    // these ops all have full 4-byte 'param's, which may need to be byteswapped
    // remaining args are drawn from stack
    case OP_ENTER:
    case OP_LEAVE:
    case OP_CONST:
    case OP_LOCAL:
    case OP_EQ:
    case OP_NE:
    case OP_LTI:
    case OP_LEI:
    case OP_GTI:
    case OP_GEI:
    case OP_LTU:
    case OP_LEU:
    case OP_GTU:
    case OP_GEU:
    case OP_EQF:
    case OP_NEF:
    case OP_LTF:
    case OP_LEF:
    case OP_GTF:
    case OP_GEF:
    case OP_BLOCK_COPY:
      *dst = *(int32_t*)src;
      if (vm->swapped == qtrue) *dst = byteswap(*dst);
      dst++;
      src += 4;
      break;
    // this op has only a single byte 'param' (draws 1 arg from stack)
    case OP_ARG:
      *dst++ = (int32_t)*src++;
      break;
    // remaining ops require no 'param' (draw all, if any, args from stack)
    default:
      *dst++ = 0;
      break;
    }
  }

  // load data segment from file to memory
  lsrc = (int32_t*)(vmBase + header->dataOffset);
  dst  = (int32_t*)(vm->dataSegment);

  // loop through each 4-byte data block (even though data may be single bytes)
  for (uint32_t n = 0; n < header->dataLength / sizeof(int32_t); ++n)
  {
    *dst = *lsrc++;
    // swap if need-be
    if (vm->swapped == qtrue) *dst = byteswap(*dst);
    dst++;
  }

  // copy remaining data into the lit segment
  memcpy(dst, lsrc, header->litLength);

  // free file from memory
  free(vmBase);

  // a winner is us
  return qtrue;
}

// frees used memory and clears vm_t
void VM_Destroy(vm_t* vm)
{
  if (vm->memory) free(vm->memory);
  memset(vm, 0, sizeof(vm_t));
}

// modified to save old memory pointer and pass it to VM_Create
// so we can keep the same memory location (might be desirable at
// some point in the future, but for now, we don't even reload)

// keep in mind...SHIT WILL BREAK IF YOU SWAP QVMS AND THEN RESTART
// do NOT blame me for assuming the file will stay the same
qboolean VM_Restart(vm_t* vm, qboolean savemem)
{
  char  name[MAX_QPATH];
  byte* oldmem = NULL;

  if (!vm) return qfalse;

  // save filename (we need this to reload the same file, obviously)
  strncpy(name, vm->name, sizeof(name) - 1);
  name[sizeof(name) - 1] = '\0';

  // save memory pointer or free it
  if (savemem == qtrue)
    oldmem = vm->memory;
  else
    free(vm->memory);

  // kill it!
  memset(vm, 0, sizeof(vm_t));

  // reload
  if (!VM_Create(vm, name, oldmem))
  {
    VM_Destroy(vm);
    return qfalse;
  }

  return qtrue;
}

void* VM_ArgPtr(int32_t intValue)
{
  // TODO: assert if intValue < g_VM.dataSegmentMask
  return (void*)(g_VM.dataSegment + (intValue & g_VM.dataSegmentMask));
}

void* VM_ExplicitArgPtr(vm_t const* vm, int32_t intValue)
{
  // TODO: assert if intValue < vm->dataSegmentMask
  return (void*)(vm->dataSegment + (intValue & vm->dataSegmentMask));
}

vm_t    g_VM;
char    vmpath[MAX_QPATH];
char    vmbase[16];
int32_t vm_stacksize = 0;

/*
==========
initVM
==========
*/
int32_t initVM(void)
{
  strncpy(vmpath, DEFAULT_VMPATH, sizeof(vmpath) - 1);
  vmpath[sizeof(vmpath) - 1] = '\0';

  vm_stacksize = 1;
  vm_stacksize *= (1 << 20); // convert to MB

  // clear VM
  memset(&g_VM, 0, sizeof(vm_t));

  // if we can't load the VM, we pretty much have to exit
  // or we can fake it, and sell it to the Fox Network
  if (!VM_Create(&g_VM, vmpath, NULL))
  {
    trap_Error(vaf("FATAL ERROR: Unable to load VM \"%s\"\n", vmpath));
    return qfalse;
  }
  strncpy(vmbase, vaf("%u", g_VM.dataSegment), sizeof(vmbase) - 1);
  vmbase[sizeof(vmbase) - 1] = '\0';

  return qtrue;
}

/*
==========
setVMPtr
==========
*/
int32_t setVMPtr(int32_t arg0)
{
  g_VM.hook_realfunc = arg0;
  return 0;
}

/*
==========
callVM_Exec
==========
*/
intptr_t callVM(
  int32_t cmd,
  int32_t arg0,
  int32_t arg1,
  int32_t arg2,
  int32_t arg3,
  int32_t arg4,
  int32_t arg5,
  int32_t arg6,
  int32_t arg7,
  int32_t arg8,
  int32_t arg9,
  int32_t arg10,
  int32_t arg11)
{
  if (g_VM.memory)
  {
    return VM_Exec(&g_VM, cmd, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
  }
  return 0; // dunno if this is OK
}

/*
==========
callVM_Destroy
==========
*/
intptr_t callVM_Destroy(void)
{
  VM_Destroy(&g_VM);
  return 0;
}

/*
============
int_byteswap
============
*/
// from sdk/game/q_shared.c
int32_t int_byteswap(int32_t i)
{
  byte b1, b2, b3, b4;

  b1 = i & 255;
  b2 = (i >> 8) & 255;
  b3 = (i >> 16) & 255;
  b4 = (i >> 24) & 255;

  return ((int32_t)b1 << 24) + ((int32_t)b2 << 16) + ((int32_t)b3 << 8) + b4;
}

/*
============
short_byteswap
============
*/
short short_byteswap(short s)
{
  byte b1, b2;

  b1 = s & 255;
  b2 = (s >> 8) & 255;

  return ((int32_t)b1 << 8) + b2;
}
