/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "threads/system.hh"

#include <string.h>


/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *executable_file)
{
    ASSERT(executable_file != nullptr);

    exe = new Executable(executable_file);
    // esto verifica que no estemos tratando de ejecutar un archivo q no sea de Nachos
    ASSERT(exe->CheckMagic());

    // How big is address space?

    size = exe->GetSize() + USER_STACK_SIZE;
      // We need to increase the size to leave room for the stack.
    numPages = DivRoundUp(size, PAGE_SIZE);
    size = numPages * PAGE_SIZE;

    // ASSERT(numPages <= NUM_PHYS_PAGES);
    // ahora nos fijamos paginas libres, puesto algunas pueden estar siendo usadas por otros procesos
    ASSERT(numPages <= pagesInUse->CountClear());

    // Check we are not trying to run anything too big -- at least until we
    // have virtual memory.

    DEBUG('a', "Initializing address space, num pages %u, size %u\n",
          numPages, size);

    pageTable = new TranslationEntry[numPages];

    for (unsigned i = 0; i < numPages; i++) {
        pageTable[i].virtualPage  = i;
      #ifndef DEMAND_LOADING
        unsigned FPNumber = pagesInUse->Find();
        ASSERT(FPNumber >= 0);
        pageTable[i].physicalPage = FPNumber;
        pageTable[i].valid        = true;
      #else
        pageTable[i].physicalPage = -1;
        pageTable[i].valid        = false;
      #endif
        pageTable[i].use          = false;
        pageTable[i].dirty        = false;
        pageTable[i].readOnly     = false;
    }

    #ifndef DEMAND_LOADING // cargamos solo si no estamos utilizando demand_loading
    char *mainMemory = machine->GetMMU()->mainMemory;
    for (unsigned i = 0; i < numPages; i++) {
        memset(&mainMemory[pageTable[i].physicalPage * PAGE_SIZE], 0, PAGE_SIZE);
    }
    
    // Then, copy in the code and data segments into memory.
    uint32_t codeSize = exe->GetCodeSize();
    uint32_t initDataSize = exe->GetInitDataSize();
    if (codeSize > 0) {
        uint32_t virtualAddr = exe->GetCodeAddr();
        DEBUG('a', "Initializing code segment, at 0x%X, size %u\n",
              virtualAddr, codeSize);
        for (uint32_t i = 0; i < codeSize; i++) {
            uint32_t frame = pageTable[DivRoundDown(virtualAddr + i, PAGE_SIZE)].physicalPage;
            uint32_t offset = (virtualAddr + i) % PAGE_SIZE;
            uint32_t physicalAddr = frame * PAGE_SIZE + offset;
            exe->ReadCodeBlock(&mainMemory[physicalAddr], 1, i);
        }
    }
    if (initDataSize > 0) {
        uint32_t virtualAddr = exe->GetInitDataAddr();
        DEBUG('a', "Initializing data segment, at 0x%X, size %u\n",
              virtualAddr, initDataSize);
        // exe.ReadDataBlock(&mainMemory[virtualAddr], initDataSize, 0);
        for (uint32_t i = 0; i < initDataSize; i++) {
          uint32_t frame = pageTable[DivRoundDown(virtualAddr + i, PAGE_SIZE)].physicalPage;
          uint32_t offset = (virtualAddr + i) % PAGE_SIZE;
          uint32_t physicalAddr = frame * PAGE_SIZE + offset;
          exe->ReadDataBlock(&mainMemory[physicalAddr], 1, i);
        }
    }
    #endif
}
/// Deallocate an address space.
///
/// Nothing for now!
AddressSpace::~AddressSpace()
{
  for (unsigned i = 0; i < numPages; i++) {
    pagesInUse->Clear(pageTable[i].physicalPage);
  }
  delete [] pageTable;
}

/// Set the initial values for the user-level register set.
///
/// We write these directly into the “machine” registers, so that we can
/// immediately jump to user code.  Note that these will be saved/restored
/// into the `currentThread->userRegisters` when this thread is context
/// switched out.
void
AddressSpace::InitRegisters()
{
    for (unsigned i = 0; i < NUM_TOTAL_REGS; i++) {
        machine->WriteRegister(i, 0);
    }

    // Initial program counter -- must be location of `Start`.
    machine->WriteRegister(PC_REG, 0);

    // Need to also tell MIPS where next instruction is, because of branch
    // delay possibility.
    machine->WriteRegister(NEXT_PC_REG, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we do not
    // accidentally reference off the end!
    machine->WriteRegister(STACK_REG, numPages * PAGE_SIZE - 16);
    DEBUG('a', "Initializing stack register to %u\n",
          numPages * PAGE_SIZE - 16);
}

/// On a context switch, save any machine state, specific to this address
/// space, that needs saving.
///
/// For now, nothing!
void
AddressSpace::SaveState()
{}

/// On a context switch, restore the machine state so that this address space
/// can run.
///
/// For now, tell the machine where to find the page table.
void AddressSpace::RestoreState()
{
#ifndef USE_TLB
  machine->GetMMU()->pageTable = pageTable;
  machine->GetMMU()->pageTableSize = numPages;
#else
  for (unsigned i = 0; i < TLB_SIZE; i++) {
    machine->GetMMU()->tlb[i].valid = false;
  }
#endif
}

#ifdef DEMAND_LOADING
void AddressSpace::LoadPage(int vpn)
{
  char *mainMemory = machine->GetMMU()->mainMemory;
  int phy = pagesInUse->Find();
  if (phy == -1)
    ASSERT(false);
  pageTable[vpn].physicalPage = phy;
  pageTable[vpn].valid = true;
  pageTable[vpn].use = false;
  pageTable[vpn].dirty = false;
  pageTable[vpn].readOnly = false;

  memset(&mainMemory[pageTable[vpn].physicalPage * PAGE_SIZE], 0, PAGE_SIZE); //Solo cuando es la pila
  unsigned int vaddrinit = vpn * PAGE_SIZE;
  
  if (vaddrinit <= size - USER_STACK_SIZE) { // size es getSize + USER_STACK_SIZE
    uint32_t dataSizeI = exe->GetInitDataSize();
    uint32_t dataSizeU = exe->GetUninitDataSize();
    uint32_t dataAddr = exe->GetInitDataAddr();

    if (dataSizeI + dataSizeU == 0) { // No hay sección Data 
      exe->ReadCodeBlock(&mainMemory[pageTable[vpn].physicalPage * PAGE_SIZE], PAGE_SIZE, vaddrinit);
    }
    else if (vaddrinit < dataAddr && dataSizeI + dataSizeU > 0) { // Hay data y estamos en codeBlock 
      if (vaddrinit + PAGE_SIZE >= dataAddr) { // Arranca en code termina en Data 
        unsigned leercode = dataAddr - vaddrinit;
        unsigned leerdata = PAGE_SIZE - leercode;
        exe->ReadCodeBlock(&mainMemory[pageTable[vpn].physicalPage * PAGE_SIZE], leercode, vaddrinit);
        exe->ReadDataBlock(&mainMemory[pageTable[vpn].physicalPage * PAGE_SIZE] + leercode, leerdata, 0);
      } else { // Esta todo en Code 
        exe->ReadCodeBlock(&mainMemory[pageTable[vpn].physicalPage * PAGE_SIZE], PAGE_SIZE, vaddrinit);
      }
    }
    else if (vaddrinit < dataAddr + dataSizeI && vaddrinit >= dataAddr && dataSizeI + dataSizeU > 0) { // Estamos en data Init
      if (vaddrinit + PAGE_SIZE >= dataAddr + dataSizeI) { // Arranca en init, termina afuera
        unsigned leerInit = dataAddr + dataSizeI - vaddrinit;
        exe->ReadDataBlock(&mainMemory[pageTable[vpn].physicalPage * PAGE_SIZE], leerInit, vaddrinit - dataAddr);
      } else { // Esta todo en data Init 
        exe->ReadDataBlock(&mainMemory[pageTable[vpn].physicalPage * PAGE_SIZE], PAGE_SIZE, vaddrinit - dataAddr);
      }
    }
  }
}
#endif

TranslationEntry *
AddressSpace::GetPageTable()
{
    return pageTable;
}
