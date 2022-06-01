/// Entry points into the Nachos kernel from user programs.
///
/// There are two kinds of things that can cause control to transfer back to
/// here from user code:
///
/// * System calls: the user code explicitly requests to call a procedure in
///   the Nachos kernel.  Right now, the only function we support is `Halt`.
///
/// * Exceptions: the user code does something that the CPU cannot handle.
///   For instance, accessing memory that does not exist, arithmetic errors,
///   etc.
///
/// Interrupts (which can also cause control to transfer from user code into
/// the Nachos kernel) are handled elsewhere.
///
/// For now, this only handles the `Halt` system call.  Everything else core-
/// dumps.
///
/// Copyright (c) 1992-1993 The Regents of the University of California.
///               2016-2021 Docentes de la Universidad Nacional de Rosario.
/// All rights reserved.  See `copyright.h` for copyright notice and
/// limitation of liability and disclaimer of warranty provisions.


#include <stdio.h>
#include <stdlib.h>
#include "transfer.hh"
#include "syscall.h"
#include "args.hh"
#include "filesys/directory_entry.hh"
#include "filesys/open_file.hh"
#include "threads/system.hh"
#include "exception.hh"

static void
IncrementPC()
{
    unsigned pc;

    pc = machine->ReadRegister(PC_REG);
    machine->WriteRegister(PREV_PC_REG, pc);
    pc = machine->ReadRegister(NEXT_PC_REG);
    machine->WriteRegister(PC_REG, pc);
    pc += 4;
    machine->WriteRegister(NEXT_PC_REG, pc);
}

/// Do some default behavior for an unexpected exception.
///
/// NOTE: this function is meant specifically for unexpected exceptions.  If
/// you implement a new behavior for some exception, do not extend this
/// function: assign a new handler instead.
///
/// * `et` is the kind of exception.  The list of possible exceptions is in
///   `machine/exception_type.hh`.
static void
DefaultHandler(ExceptionType et)
{
    int exceptionArg = machine->ReadRegister(2);

    fprintf(stderr, "Unexpected user mode exception: %s, arg %d.\n",
            ExceptionTypeToString(et), exceptionArg);
    ASSERT(false);
}

/// Runs an user program.
/// Opens the exec, loads it into the memory, and jumps into it.

static void
StartProcess(void * voidargv)
{

     char** argv = (char**)voidargv;

    currentThread->space->InitRegisters();  // Set the initial register values.


    currentThread->space->RestoreState();   // Load page table register.


     unsigned argc = 0;

     if(argv != nullptr) {
         argc = WriteArgs(argv);// libera la memoria de argv
         DEBUG('e', "argv is not null and argc is: %d \n", argc);

         if(argc) {
             //Load a1 register (5)
             int argvaddr = machine->ReadRegister(STACK_REG);
             //Lo ultimo que hace WriteArgs es dejar el sp dentro de STACK_REG
             //luego de haber cargado todos los argumentos.
             machine->WriteRegister(STACK_REG, argvaddr - 24);// convencion de MIPS, restandole 24 seteo el stack pointer 24 bytes más abajo.

             DEBUG('e', "argvaddr is: %d\n", argvaddr);
             machine->WriteRegister(5, argvaddr);
         }
     } else {
         DEBUG('e', "argvaddr is null!: %p\n", argv);
     }

     machine->WriteRegister(4, argc);
     machine->Run();  // Jump to the user progam
     ASSERT(false); //   `machine->Run` never returns; the address space
                     //exits by doing the system call `Exit`.
}
// Handle a system call exception.

// * `et` is the kind of exception.  The list of possible exceptions is in
//   `machine/exception_type.hh`.
//
/// The calling convention is the following:
///
/// * system call identifier in `r2`;
/// * 1st argument in `r4`;
/// * 2nd argument in `r5`;
/// * 3rd argument in `r6`;
/// * 4th argument in `r7`;
/// * the result of the system call, if any, must be put back into `r2`.
///
/// And do not forget to increment the program counter before returning. (Or
/// else you will loop making the same system call forever!)
static void
SyscallHandler(ExceptionType _et)
{
    int scid = machine->ReadRegister(2);

    switch (scid) {

        case SC_HALT:
            DEBUG('e', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
            break;

        case SC_CREATE: {
            int filenameAddr = machine->ReadRegister(4);
            if (filenameAddr == 0) {
                DEBUG('e', "Error: address to filename string is null.\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char filename[FILE_NAME_MAX_LEN + 1];
            if (!ReadStringFromUser(filenameAddr,
                                    filename, sizeof filename)) {
                DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                      FILE_NAME_MAX_LEN);
                machine->WriteRegister(2, -1);
                  break;
            }

            if (!fileSystem->Create(filename, 100)) {
              DEBUG('e', "File creation failed. \n");
              machine->WriteRegister(2, 1);
            } else {
              DEBUG('e', "`Create` requested for file `%s`.\n", filename);
              machine->WriteRegister(2, 0);
            }
            break;
        }

        case SC_REMOVE: {
             int filenameAddr = machine->ReadRegister(4);
             if (filenameAddr == 0) {
                 DEBUG('e', "Error: address to filename string is null.\n");
                 machine->WriteRegister(2, -1);
                 break;
             }

             char filename[FILE_NAME_MAX_LEN + 1];
             if (!ReadStringFromUser(filenameAddr,
                                     filename, sizeof filename)) {
                 DEBUG('e', "Error: filename string too long (maximum is %u bytes).\n",
                       FILE_NAME_MAX_LEN);
                 machine->WriteRegister(2, -1);
                 break;
             }

             if (!fileSystem->Remove(filename)) {
                 DEBUG('e', "File deletion failed. \n");
                 machine->WriteRegister(2, -1);
             } else {
                 DEBUG('e', "File removed succesfully. \n");
                 machine->WriteRegister(2, 0);
             }
             break;
         }

         case SC_WRITE: {
             int usrStringAddr = machine->ReadRegister(4);
             if (usrStringAddr == 0) {
                 DEBUG('e', "User string address is null\n");
                 machine->WriteRegister(2, -1);
                 break;
             }
             int size = machine->ReadRegister(5);
             if (size <= 0) {
                 DEBUG('e', "Invalid size\n");
                 machine->WriteRegister(2, -1);
                 break;
             }
             int id = machine->ReadRegister(6);
             if (id < 0) {
                 DEBUG('e', "Invalid file descriptor id\n");
                 machine->WriteRegister(2, -1);
                 break;
             }

             char buffer[size+1];
             ReadBufferFromUser(usrStringAddr, buffer, size);
             buffer[size] = '\0';
             int counter = 0;
             if (id == CONSOLE_OUTPUT) {
                 DEBUG('e', "`Write` requested to console output.\n");
                 ReadBufferFromUser(usrStringAddr, buffer, size);
                 for (; counter < size; counter++) {
                     synchConsole->WriteChar(buffer[counter]);
                 }
                 machine->WriteRegister(2, counter);
                 break;
             } else {
                 OpenFile* file = currentThread->files->Get(id);
                 if (file != nullptr) {
                     counter = file->Write(buffer, size);
                     machine->WriteRegister(2, counter);
                     break;
                 } else {
                     DEBUG('e', "No matching file with id %d\n", id);
                     machine->WriteRegister(2, -1);
                     break;
                 }
             }
             if (counter == size)
                    machine->WriteRegister(2, 0);
             else
                  machine->WriteRegister(2, -1);
             break;
         }

         case SC_READ: {
            int usrStringAddr = machine->ReadRegister(4);
            if (usrStringAddr == 0) {
                DEBUG('e', "User string address is null\n");
                machine->WriteRegister(2, -1);
                break;
            }
            int size = machine->ReadRegister(5);
            if (size <= 0) {
                DEBUG('e', "Invalid size\n");
                machine->WriteRegister(2, -1);
                break;
            }
            int id = machine->ReadRegister(6);
            if (id < 0) {
                DEBUG('e', "Invalid file descriptor id\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char buffer[size+1];
            int counter = 0;

            if (id == CONSOLE_INPUT) {
                DEBUG('e', "`Read` requested from console input.\n");
                for (;counter < size; counter++) {
                    char c = synchConsole->ReadChar();
                    if (c == '\n') {
                        break;
                    }
                    buffer[counter] = c;
                }
                buffer[counter] = '\n';
                WriteStringToUser(buffer, usrStringAddr);
                machine->WriteRegister(2, counter);
                break;
            } else {
                DEBUG('e', "`Read` requested from file with id %u.\n", id);
                OpenFile* file = currentThread->files->Get(id);
                if (file != nullptr) {
                    counter = file->Read(buffer, size);
                    buffer[counter] = '\0';
                    WriteStringToUser(buffer, usrStringAddr);
                    machine->WriteRegister(2, counter);
                    break;
                } else {
                    DEBUG('e', "No matching file with id %d\n", id);
                    machine->WriteRegister(2, -1);
                    break;
                }
            }

            break;
        }


        case SC_EXIT: {
            int status = machine->ReadRegister(4);

            DEBUG('e', "Program exited with status %d \n", status);

            currentThread->Finish();

            break;
        }

        case SC_OPEN: {
            int nameAddr = machine->ReadRegister(4);
            if (nameAddr == 0) {
                DEBUG('e', "Invalid address\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char buffer[FILE_NAME_MAX_LEN+1];
            if(!ReadStringFromUser(nameAddr, buffer, sizeof buffer)) {
                DEBUG('e', "Read string error\n");
                machine->WriteRegister(2, -1);
                break;
            }

            OpenFile *file = fileSystem->Open(buffer);

            if (file == nullptr) {
                DEBUG('e', "Invalid file name\n");
                machine->WriteRegister(2, -1);
                break;
            }

            OpenFileId openFileId = currentThread->files->Add(file);

            if (openFileId == -1) {
                DEBUG('e', "Full table\n");
                delete file;
                machine->WriteRegister(2, -1);
                break;
            }

            machine->WriteRegister(2, openFileId);
            break;
        }

        case SC_CLOSE: {
            int fid = machine->ReadRegister(4);
            DEBUG('e', "`Close` requested for id %u.\n", fid);

            if (fid < 2) {
                DEBUG('e', "Invalid file descriptor id\n");
                machine->WriteRegister(2, -1);
                break;
            }

            OpenFile *file = currentThread->files->Remove(fid);

            if (file != nullptr) {
                delete file;
                machine->WriteRegister(2, 0);
            } else {
                DEBUG('e', "File doesn't match this id: %d\n", fid);
                machine->WriteRegister(2, -1);
            }
            break;
        }

        case SC_JOIN: {
            SpaceId spaceId = machine->ReadRegister(4);

            if(spaceId < 0){
                DEBUG('e',"Error en Join: id del proceso inexistente");
                machine->WriteRegister(2, -1);
                break;
            }

            if(!runningProcesses->HasKey(spaceId)){
                DEBUG('e',"Error en Join: id del proceso inexistente");
                machine->WriteRegister(2, -1);
                break;
            }

            Thread* pr = runningProcesses->Get(spaceId);
            pr->Join();

            machine->WriteRegister(2, 0);
            break;
        }

        case SC_EXEC: {

            int nameAddr = machine->ReadRegister(4);
            int argvAddr = machine->ReadRegister(5);
            bool joinable = machine->ReadRegister(6);
            char **argv = nullptr;

            if (nameAddr == 0) {
                DEBUG('e', "Invalid address\n");
                machine->WriteRegister(2, -1);
                break;
            }

            char buffer[FILE_NAME_MAX_LEN+1];
            if(!ReadStringFromUser(nameAddr, buffer, sizeof buffer )) {
                DEBUG('e', "Read string error\n");
                machine->WriteRegister(2, -1);
                break;
            }

            if (argvAddr) {
                 DEBUG('e', "argvAddr distinto de cero, con direccion: %d \n", argvAddr);
                 argv = SaveArgs(argvAddr);
            }

            OpenFile *executable = fileSystem->Open(buffer);

            if (executable == nullptr) {
                DEBUG('e', "Unable to open file %s\n", buffer);
                printf("Che master el programa que intenta ejecutar no existe \n");
                machine->WriteRegister(2, -1);
                break;
            }

            // creamos el proceso
            Thread *newThread = new Thread(buffer,  joinable , currentThread->GetPriority());

            // añadimos el proceso a las tabla de procesos
            SpaceId id = (SpaceId)runningProcesses->Add(newThread);
            newThread->Pid = id;

            // cargamos el programa a memoria
            AddressSpace *space = new AddressSpace(executable);
            newThread->space = space;

            delete executable;

            //ejecutamos el proceso
            newThread->Fork(StartProcess, (void *) argv);
            machine->WriteRegister(2, id);
            break;
        }

        default:
            fprintf(stderr, "Unexpected system call: id %d.\n", scid);
            ASSERT(false);
    }
    IncrementPC();
}

static void
PageFaultHandler(ExceptionType et)
{
    DEBUG('a', "PageFault");
    int badaddr = machine->ReadRegister(BAD_VADDR_REG);
    int vpn = badaddr/PAGE_SIZE;
    TranslationEntry fallo = currentThread->space->GetPageTable()[vpn];
    #ifdef DEMAND_LOADING
    if (fallo.physicalPage == -1) {
        currentThread->space->LoadPage(vpn);
    }
    #endif
    stats->hits-=1;
}

static void
ReadOnlyHandler(ExceptionType et)
{
    DEBUG('e', "Tried to read from a read only page");
    ASSERT(false); // Esto mata al so
    return;
}

/// By default, only system calls have their own handler.  All other
/// exception types are assigned the default handler.
void
SetExceptionHandlers()
{
    machine->SetHandler(NO_EXCEPTION,            &DefaultHandler);
    machine->SetHandler(SYSCALL_EXCEPTION,       &SyscallHandler);
    #ifdef USE_TLB
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &PageFaultHandler);
    #else
    machine->SetHandler(PAGE_FAULT_EXCEPTION,    &DefaultHandler);
    #endif
    machine->SetHandler(READ_ONLY_EXCEPTION,     &ReadOnlyHandler);
    machine->SetHandler(BUS_ERROR_EXCEPTION,     &DefaultHandler);
    machine->SetHandler(ADDRESS_ERROR_EXCEPTION, &DefaultHandler);
    machine->SetHandler(OVERFLOW_EXCEPTION,      &DefaultHandler);
    machine->SetHandler(ILLEGAL_INSTR_EXCEPTION, &DefaultHandler);
}
