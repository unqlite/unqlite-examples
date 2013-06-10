/*
 * Compile this file together with the UnQLite database engine source code
 * to generate the executable. For example: 
 *  gcc -W -Wall -O6 unqlite_jx9_interp.c unqlite.c -o unqlite_jx9
*/
/*
 * This simple program is a quick introduction on how to embed and start
 * experimenting with UnQLite without having to do a lot of tedious
 * reading and configuration.
 *
 * Introduction to Jx9:
 *
 * The Document store to UnQLite which is used to store JSON docs (i.e. Objects, Arrays, Strings, etc.)
 * in the database is powered by the Jx9 programming language.
 *
 * Jx9 is an embeddable scripting language also called extension language designed
 * to support general procedural programming with data description facilities.
 * Jx9 is a Turing-Complete, dynamically typed programming language based on JSON
 * and implemented as a library in the UnQLite core.
 *
 * Jx9 is built with a tons of features and has a clean and familiar syntax similar
 * to C and Javascript.
 * Being an extension language, Jx9 has no notion of a main program, it only works
 * embedded in a host application.
 * The host program (UnQLite in our case) can write and read Jx9 variables and can
 * register C/C++ functions to be called by Jx9 code. 
 *
 * The Jx9 interpreter (This program) is a simple standalone Jx9 interpreter that allows
 * the user to enter and execute Jx9 files against a Jx9 engine (Via UnQLite). 
 * To start the program, just type "unqlite_jx9" followed by the name of the Jx9 file
 * to compile and execute. That is, the first argument is to the interpreter, the rest
 * are scripts arguments, press "Enter" and the Jx9 code will be executed.
 * If something goes wrong while processing the Jx9 script due to a compile-time error
 * your error output (STDOUT) should display the compile-time error messages.
 *
 * Usage example of the jx9 interpreter:
 *   unqlite_jx9 scripts/hello_world.jx9.txt
 * Running the interpreter with script arguments
 *    unqlite_jx9 scripts/mp3_tag.jx9.txt /usr/local/path/to/my_mp3s
 *
 * For an introduction to the UnQLite C/C++ interface, please refer to:
 *        http://unqlite.org/api_intro.html
 * For an introduction to Jx9, please refer to:
 *        http://unqlite.org/jx9.html
 * For the full C/C++ API reference guide, please refer to:
 *        http://unqlite.org/c_api.html
 * UnQLite in 5 Minutes or Less:
 *        http://unqlite.org/intro.html
 * The Architecture of the UnQLite Database Engine:
 *        http://unqlite.org/arch.html
 */
/* $SymiscID: unqlite_jx9_interp.c v1.2 Win7 2013-05-19 02:18 stable <chm@symisc.net> $ */
/* 
 * Make sure you have the latest release of UnQLite from:
 *  http://unqlite.org/downloads.html
 */
#include <stdio.h>  /* puts() */
#include <stdlib.h> /* exit() */
/* Make sure this header file is available.*/
#include "unqlite.h"
/*
 * Banner.
 */
static const char zBanner[] = {
	"============================================================\n"
	"UnQLite Jx9 Interpreter                                     \n"
	"                                         http://unqlite.org/\n"
	"============================================================\n"
};
/*
 * Display the banner, a help message and exit.
 */
static void Help(void)
{
	puts(zBanner);
	puts("unqlite_jx9 [-h|-r|-d] path/to/jx9.txt_file [script args]");
	puts("\t-d: Dump Jx9 bytecode instructions");
	puts("\t-r: Report run-time errors");
	puts("\t-h: Display this message an exit");
	/* Exit immediately */
	exit(0);
}
/*
 * Extract the database error log and exit.
 */
static void Fatal(unqlite *pDb,const char *zMsg)
{
	if( pDb ){
		const char *zErr;
		int iLen = 0; /* Stupid cc warning */

		/* Extract the database error log */
		unqlite_config(pDb,UNQLITE_CONFIG_ERR_LOG,&zErr,&iLen);
		if( iLen > 0 ){
			/* Output the DB error log */
			puts(zErr); /* Always null termniated */
		}
	}else{
		if( zMsg ){
			puts(zMsg);
		}
	}
	/* Manually shutdown the library */
	unqlite_lib_shutdown();
	/* Exit immediately */
	exit(0);
}
/* Forward declaration: VM output consumer callback */
static int VmOutputConsumer(const void *pOutput,unsigned int nOutLen,void *pUserData /* Unused */);

/*
 * Main program: Compile and execute the given Jx9 script. 
 */
int main(int argc,char *argv[])
{
	unqlite *pDb;       /* Database handle */
	unqlite_vm *pVm;    /* UnQLite VM resulting from successful compilation of the target Jx9 script */
	int dump_vm = 0;    /* Dump VM instructions if TRUE */
	int err_report = 0; /* Report run-time errors if TRUE */
	int rc,n;
	
	/* Process interpreter arguments first*/
	for(n = 1 ; n < argc ; ++n ){
		int c;
		if( argv[n][0] != '-' ){
			/* No more interpreter arguments */
			break;
		}
		c = argv[n][1];
		if( c == 'd' || c == 'D' ){
			/* Dump bytecode instructions */
			dump_vm = 1;
		}else if( c == 'r' || c == 'R' ){
			/* Report run-time errors */
			err_report = 1;
		}else{
			/* Display a help message and exit */
			Help();
		}
	}
	if( n >= argc ){
		puts("Missing Jx9 file to compile");
		Help();
	}
	
	/* Open our database */
	rc = unqlite_open(&pDb,":mem:" /* In-mem DB, "test.db": for a on-disk DB */,UNQLITE_OPEN_CREATE);
	if( rc != UNQLITE_OK ){
		Fatal(0,"Out of memory");
	}

	/* Compile our Jx9 script  */
	rc = unqlite_compile_file(pDb,argv[n],&pVm);
	if( rc != UNQLITE_OK ){
		if( rc == UNQLITE_IOERR ){
			Fatal(0,"IO error while opening the target Jx9 script");
		}else{
			/* Compile error, extract the compiler error log */
			const char *zBuf;
			int iLen;
			/* Extract error log */
			unqlite_config(pDb,UNQLITE_CONFIG_JX9_ERR_LOG,&zBuf,&iLen);
			if( iLen > 0 ){
				puts(zBuf);
			}
			Fatal(0,"Jx9 compile error");
		}
	}


	/* Install a VM output consumer callback */
	rc = unqlite_vm_config(pVm,UNQLITE_VM_CONFIG_OUTPUT,VmOutputConsumer,0);
	if( rc != UNQLITE_OK ){
		Fatal(pDb,0);
	}
	
	
	/* Register script agruments so we can access them later using the $argv[]
	 * array from the compiled JX9 program.
	 */
	for( n = n + 1; n < argc ; ++n ){
		unqlite_vm_config(pVm, UNQLITE_VM_CONFIG_ARGV_ENTRY, argv[n]/* Argument value */);
	}

	if( err_report ){
		/* Report script run-time errors */
		unqlite_vm_config(pVm, UNQLITE_VM_CONFIG_ERR_REPORT);
	}
	if( dump_vm ){
		/* Dump Jx9 byte-code instructions */
		unqlite_vm_dump(pVm, 
			VmOutputConsumer, /* Dump consumer callback */
			0
			);
	}

	/* Execute our script */
	unqlite_vm_exec(pVm);
	
	/* Release our VM */
	unqlite_vm_release(pVm);
	
	/* Auto-commit the transaction and close our database */
	unqlite_close(pDb);
	
	return 0;
}

#ifdef __WINNT__
#include <Windows.h>
#else
/* Assume UNIX */
#include <unistd.h>
#endif
/*
 * The following define is used by the UNIX build process and have
 * no particular meaning on windows.
 */
#ifndef STDOUT_FILENO
#define STDOUT_FILENO	1
#endif
/*
 * VM output consumer callback.
 * Each time the UnQLite VM generates some outputs, the following
 * function gets called by the underlying virtual machine to consume
 * the generated output.
 *
 * All this function does is redirecting the VM output to STDOUT.
 * This function is registered via a call to [unqlite_vm_config()]
 * with a configuration verb set to: UNQLITE_VM_CONFIG_OUTPUT.
 */
static int VmOutputConsumer(const void *pOutput,unsigned int nOutLen,void *pUserData /* Unused */)
{
#ifdef __WINNT__
	BOOL rc;
	rc = WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),pOutput,(DWORD)nOutLen,0,0);
	if( !rc ){
		/* Abort processing */
		return UNQLITE_ABORT;
	}
#else
	ssize_t nWr;
	nWr = write(STDOUT_FILENO,pOutput,nOutLen);
	if( nWr < 0 ){
		/* Abort processing */
		return UNQLITE_ABORT;
	}
#endif /* __WINT__ */
	
	/* All done, data was redirected to STDOUT */
	return UNQLITE_OK;
}
