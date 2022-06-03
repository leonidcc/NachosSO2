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
#include <iostream>
#include <climits>
#define NUM_ROUNDS 4
#endif

/// First, set up the translation from program memory to physical memory.
/// For now, this is really simple (1:1), since we are only uniprogramming,
/// and we have a single unsegmented page table.
AddressSpace::AddressSpace(OpenFile *executable_file, SpaceId id)
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
      DEBUG('e', "Error: Swap file not created.\n");
    }
    swap = fileSystem->Open(nombreSwap);
    if (swap == nullptr) {
      DEBUG('e', "Cannot open swap file!\n");
      ASSERT(false);
    }
    addressSpaceId = id;
    #endif

    // ASSERT(numPages <= NUM_PHYS_PAGES);
    // ahora nos fijamos paginas libres, puesto algunas pueden estar siendo usadas por otros procesos
    #ifndef SWAP
    ASSERT(numPages <= pagesInUse->CountClear());
    #endif

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
  #ifndef SWAP
  for (unsigned i = 0; i < numPages; i++) {
    pagesInUse->Clear(pageTable[i].physicalPage);
  }
  #endif
  delete [] pageTable;
  #ifdef SWAP
  fileSystem->Remove(nombreSwap);
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
      unsigned physicalPageToSave = machine->GetMMU()->tlb[i].physicalPage;
      TranslationEntry* entry = &GetPageTable()[pagesInUse[physicalPageToSave].virtualPage];
      machine->GetMMU()->tlb[i].valid = false;
      *entry = machine->GetMMU()->tlb[i];
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
void AddressSpace::LoadPage(unsigned vpn, unsigned phy)
{
  ASSERT(vpn >= 0);
  ASSERT(phy != INT_MAX); //i  have a valid frame

  uint32_t codeSize = exe->GetCodeSize();
  uint32_t initDataSize = exe->GetInitDataSize();
  uint32_t dataVirtualAddr = exe->GetInitDataAddr();

  DEBUG('e', "Loading page: physicalPage: %d, vpn: %d\n", phy, vpn);

  // Get the physical address to write into
  uint32_t physicalAddressToWrite = phy * PAGE_SIZE;

  // Clean the memory
  char *mainMemory = machine->GetMMU()->mainMemory;
  memset(&mainMemory[physicalAddressToWrite], 0, PAGE_SIZE);

  vpn = vpn / PAGE_SIZE;
  unsigned vpnAddressToRead = vpn * PAGE_SIZE;

  unsigned readed = 0; // I need to ensure that i have readed PAGE_SIZE bytes

  if(pageTable[vpn].dirty) {
    DEBUG('e',"Reading from swap at position %d...\n", vpn * PAGE_SIZE);
    swap->ReadAt(&mainMemory[physicalAddressToWrite], PAGE_SIZE, vpn * PAGE_SIZE);
  } else { //read from the exe file
    if (codeSize > 0 && vpnAddressToRead < codeSize) {
      uint32_t toRead = codeSize - vpnAddressToRead < PAGE_SIZE ? codeSize - vpnAddressToRead : PAGE_SIZE;

      exe->ReadCodeBlock(&mainMemory[physicalAddressToWrite], PAGE_SIZE, vpnAddressToRead);
      readed += toRead; //to check if there is some data left to read
    }

    if (initDataSize > 0 && vpnAddressToRead + readed < dataVirtualAddr + initDataSize &&
      readed != PAGE_SIZE) {
      uint32_t toRead = (dataVirtualAddr + initDataSize) - (vpnAddressToRead + readed) < (PAGE_SIZE - readed) ?
                              (dataVirtualAddr + initDataSize) - (vpnAddressToRead + readed) : PAGE_SIZE - readed;
      readed ? //if read any bytes in the code section and i have not completed the PAGE_SIZE
          exe->ReadDataBlock(&mainMemory[physicalAddressToWrite + readed], toRead,  0) :
          exe->ReadDataBlock(&mainMemory[physicalAddressToWrite], toRead, vpnAddressToRead - codeSize);
      readed += toRead;
    }
  }

  if(vpnAddressToRead > codeSize + initDataSize) { // We are reading from the stack
    readed = PAGE_SIZE; // memset already done previously
  };

  #ifdef SWAP

  CoreMapEntry* chosenCoreMapEntry = &pagesInUse[phy];
  chosenCoreMapEntry->spaceId = addressSpaceId;
  chosenCoreMapEntry->virtualPage = vpn;

  DEBUG('e',"Marking physical page %u, with virtualPage %u from process %d in the coremap\n", phy, vpn, addressSpaceId);

  DEBUG('e',"State of the coremap: \n");
  for(unsigned i = 0; i < NUM_PHYS_PAGES; i++){
    DEBUG('e',"Physical page: %u, spaceId: %d, virtualPage of the mentioned process: %u \n", i, pagesInUse[i].spaceId, pagesInUse[i].virtualPage);
  }
#endif
  DEBUG('e', "finished loading page! :) \n");
  return;
}
#endif

TranslationEntry *
AddressSpace::GetPageTable()
{
    return pageTable;
}

#ifdef PRPOLICY_FIFO
int nextVictim = 0;
#endif

#ifdef PRPOLICY_LRU
int nextVictim = 0;
#endif

#ifdef SWAP
unsigned
AddressSpace::EvacuatePage() {
    //search for a victim
    unsigned victim = PickVictim();
    SpaceId victimSpace = pagesInUse[victim].spaceId;

  if(runningProcesses->HasKey(victimSpace)) { // the victim process is alive
      TranslationEntry* entry = &runningProcesses->Get(victimSpace)->space->GetPageTable()[pagesInUse[victim].virtualPage];

    for(unsigned i = 0; i < TLB_SIZE; ++i) { // save the bits if the page is in the TLB
      if(machine->GetMMU()->tlb[i].physicalPage == victim && machine->GetMMU()->tlb[i].valid) {
        machine->GetMMU()->tlb[i].valid = false;
        *entry = machine->GetMMU()->tlb[i];
      }
    }

    //if dirty, we put the midified virtualPage into the N block of the swap file
    DEBUG('e', "In evacuate page, the entry is: \n dirty: %d\n valid: %d\n", entry->dirty, entry->valid);
    if(entry->dirty) {
      char *mainMemory = machine->GetMMU()->mainMemory;
      unsigned physicalAddressToWrite = victim * PAGE_SIZE;
      DEBUG('e',"Writing into swap...\n");
      runningProcesses->Get(pagesInUse[victim].spaceId)->space->swap->WriteAt(&mainMemory[physicalAddressToWrite], PAGE_SIZE, pagesInUse[victim].virtualPage * PAGE_SIZE);   //save the evacuated information in the N file block
    }

      entry->physicalPage = INT_MAX; // mark the entry out of the memory for the pageTable
      entry->valid = false; // mark the entry out of the memory for the machine
  }
  return victim;
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
  int victim;
  if(references_done == UINT_MAX) {
    references_done = 0;
    for(unsigned i = 0; i < NUM_PHYS_PAGES; i++)
      pagesInUse[i].last_use_counter = 0;
  }

  unsigned min = UINT_MAX;
  for(unsigned i = 0; i < NUM_PHYS_PAGES; i++) {
      if(pagesInUse[i].last_use_counter < min) {
          min = pagesInUse[i].last_use_counter;
          victim = i;
      }
  }
  return victim;
  #endif
  #ifdef PRPOLICY_RANDOM
  // politica aleatoria, por default
  return rand() % NUM_PHYS_PAGES;
  #endif
  return 0;
}
