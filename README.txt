**********************************************************************Computer Architecture**********************************************************************
***************************************************************************CSCE 3301-01**************************************************************************
****************************************************************************Fall 2019****************************************************************************
*************************************************************************Tomasulo simulation Readme File*************************************************************************
***********************************************************************December 11th, 2019***********************************************************************
*******************************************************************************For*******************************************************************************
************************************************************************Dr. Cherif Salama************************************************************************

Students Names:

Abdelhakim Badawy 		- abdelhakimbadawy@aucegypt.edu
Marwan Eid         		- marwanadel99@aucegypt.edu
Mohammed Abuelwafa	 	- mohammedabuelwafa@aucegypt.edu
Ahmed Ehab 			- Botta633@aucegypt.edu

Release Notes:
Issues:
- some implementation issues related to the coding. however, the simulation needed more time to be debugged in order to work accurately.


Assumptions:
- we assumed that simulation runs with binary instructions that depends on the risc-v 16 format. the opcode field for this format was 3-bits, hence supporting 8 different instructions. however, the instructions, 
that need to be supported are 11 instructions. hence, we needed to take 1-bit from the 4-field of the RR-format in order to support extra 3 instructions with the following opcodes:

MULT : 1010
JALR : 1111
ret  : 0000

- We assumed the ability of writing and commiting to instructions, if two instructions are to be written in the same clock cycle, they are written according to the order of the program

What works:
- Two bonus features work:
	- 12 assembly data cases.
	- The parser.

What does not work:

- the simulation has all the code blocks logically following the tomsulo algorithm with speculation described in the slides, however the simulationn does not work properly due to the lack of time to be able to debug.
 
