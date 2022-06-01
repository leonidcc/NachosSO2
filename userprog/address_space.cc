/// Routines to manage address spaces (memory for executing user programs).
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include "address_space.hh"
#include "threads/system.hh"
#include <string.h>

#ifdef SWAP
#include <stdlib.h>
#include "vmem/swappedlist.hh"
#include <iostream>
#define NUM_ROUNDS 4
#endif

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

    #ifdef SWAP
    // ver si se copia el nombre del archvivo en nombreSwap
    sprintf(nombreSwap, "userprog/swap/SWAP.%d", currentThread->Pid);

    if (!fileSystem->Create(nombreSwap, 0)) {
      // error
    }
    swap = fileSystem->Open(nombreSwap);
    if (swap == nullptr) {
      // error
    }
    swapped = new SwappedList(numPages);
    #endif

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
  #ifdef SWAP
  fileSystem->Remove(swap);
  delete swap;
  #endif
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
{
  #ifdef SWAP
  TranslationEntry *tlb = machine->GetMMU()->tlb;
  for (unsigned i = 0; i < TLB_SIZE; i++)
  {
    if (tlb[i].valid)
    {
      unsigned vpn = tlb[i].virtualPage;
      pageTable[vpn] = tlb[i];
    }
  }
  #endif
}

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
  #ifndef SWAP
  int phy = pagesInUse->Find();
  if (phy == -1)
    ASSERT(false);
  #else
  int phy = pagesInUse->Find(vpn, currentThread->pid);
  if (phy == -1)
  {
    phy = PickVictim();
    int *tokill = pagesInUse->GetOwner(phy);
    int killPID = tokill[1];                                         // PID del Proceso
    int killVPN = tokill[0];                                         // VPN para de la pagina fisica para el proceso
    if (killPID == -1 && killVPN == -1) {
      // hacer algo
    } else {
      runningThreads->Get(killPID)->space->WriteToSwap(killVPN, phy); 
      pagesInUse->Mark(phy, vpn, currentThread->pid);
    }
  }
  #endif

  pageTable[vpn].physicalPage = phy;
  pageTable[vpn].valid = true;
  unsigned int vaddrinit = vpn * PAGE_SIZE;

  #ifdef SWAP
  int whereswap = swapped->Find(vpn);
  if (whereswap == -1) {
  #endif
    uint32_t dataSize = exe->GetInitDataSize();
    uint32_t dataAddr = exe->GetInitDataAddr();
    uint32_t codeSize = exe->GetCodeSize();
    uint32_t codeAddr = exe->GetCodeAddr();

    unsigned int codeCopy = 0;
    if (codeSize > 0 && codeSize + codeAddr > vaddrinit) { // leer algo en Code
      codeCopy = codeSize - vaddrinit;
      codeCopy = codeCopy > PAGE_SIZE ? PAGE_SIZE : codeCopy; // Chequeo que no se pase de una página
      exe->ReadCodeBlock(&mainMemory[phy * PAGE_SIZE], codeCopy, vaddrinit - codeAddr);
    }
    unsigned int dataCopy = 0;
    if (codeCopy < PAGE_SIZE && dataSize > 0 && vaddrinit < dataAddr + dataSize) { // leer data inicializada
      int offset = codeCopy > 0 ? 0 : vaddrinit - dataAddr;
      dataCopy = PAGE_SIZE - codeCopy;
      dataCopy = dataCopy + vaddrinit + codeCopy > dataSize + dataAddr ? dataSize + dataAddr - (vaddrinit + codeCopy) : dataCopy; // Si lo que hay que leer se pasa de data inicializada, achico la lectura
      exe->ReadDataBlock(&mainMemory[phy * PAGE_SIZE + codeCopy], dataCopy, offset);
    }
    unsigned int copied = codeCopy + dataCopy;
    if (copied < PAGE_SIZE) { // Pongo en 0 la memoria para data no inicializada o STACK
      memset(&mainMemory[phy * PAGE_SIZE + copied], 0, PAGE_SIZE - copied);
    }
#ifdef SWAP
  }
  else {
    swap->ReadAt(&mainMemory[phy * PAGE_SIZE], PAGE_SIZE, whereswap * PAGE_SIZE);
    stats->fromSwap++;
  }
#endif
}
#endif

TranslationEntry *
AddressSpace::GetPageTable()
{
    return pageTable;
}

#ifdef PRPOLICY_FIFO || PRPOLICY_LRU
int nextVictim = 0;
#endif

#ifdef SWAP
void AddressSpace::WriteToSwap(unsigned vpn, int phy)
{
  if (currentThread->space == this)
  {
    TranslationEntry *tlb = machine->GetMMU()->tlb;
    for (unsigned i = 0; i < TLB_SIZE; i++)
    {
      if (tlb[i].physicalPage == phy && tlb[i].valid)
      {
        pageTable[vpn] = tlb[i];
        tlb[i].valid = false;
      }
    }
  }
  if (pageTable[vpn].dirty)
  {
    int whereswap = swapped->Find(vpn);
    if (whereswap == -1)
      whereswap = swapped->Add(vpn);

    char *mainMemory = machine->GetMMU()->mainMemory;
    swap->WriteAt(&mainMemory[pageTable[vpn].physicalPage * PAGE_SIZE], PAGE_SIZE, whereswap * PAGE_SIZE);
    pageTable[vpn].dirty = false;
    stats->toSwap++;
  }
  pageTable[vpn].valid = false;
}
#endif

int
PickVictim()
{
  #ifdef PRPOLICY_FIFO
  // politica fifo
  int i = nextVictim;
  nextVictim++ % NUM_PHYS_PAGES;
  return i;
  #endif
  #ifdef PRPOLICY_LRU
  // politica reloj mejorada
  currentThread->space->SaveState();
  for (int i = 1; i < NUM_ROUNDS+1; i++) {
    for (unsigned pos = nextVictim; pos < nextVictim + NUM_PHYS_PAGES; pos++) {
      int *check = pagesInUse->GetOwner(pos % NUM_PHYS_PAGES);
      int checkPID = check[1];
      int checkVPN = check[0];
      TranslationEntry *entry = &(runningThreads->Get(checkPID)->space->pageTable[checkVPN]);
      switch (i) {
      case 1:
        if (!entry->use && !entry->dirty) {
          nextVictim = (pos + 1) % NUM_PHYS_PAGES;
          return pos % NUM_PHYS_PAGES;
        }
        break;
      case 2:
        if (!entry->use && entry->dirty) {
          nextVictim = (pos + 1) % NUM_PHYS_PAGES;
          return pos % NUM_PHYS_PAGES;
        }
        else {entry->use = false;}
        break;
      case 3:
        if (!entry->use && !entry->dirty) {
          nextVictim = (pos + 1) % NUM_PHYS_PAGES;
          return pos % NUM_PHYS_PAGES;
        }
        break;
      case 4:
        if (!entry->use && entry->dirty) {
          nextVictim = (pos + 1) % NUM_PHYS_PAGES;
          return pos % NUM_PHYS_PAGES;
        }
        break;
      }
    }
  }
  nextVictim = (nextVictim + 1) % NUM_PHYS_PAGES;
  return nextVictim;
  #endif
  #ifdef PRPOLICY_RANDOM
  // politica aleatoria
  srand(1);
  return rand() % NUM_PHYS_PAGES;
  #endif
  return 0;
}
