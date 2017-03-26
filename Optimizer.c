/*
 *********************************************
 *  314 Principles of Programming Languages  *
 *  Spring 2017                              *
 *  Author: Ulrich Kremer                    *
 *  Student Version                          *
 *********************************************
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "InstrUtils.h"
#include "Utils.h"


void optimize(Instruction *head);
Instruction *findRegOrigin(Instruction *instr, int reg);
Instruction *findMemOrigin(Instruction *instr, int reg, int offset);
void markOrigins(Instruction *instr);

int main()
{
	Instruction *head;

	head = ReadInstructionList(stdin);
	if (!head) {
		WARNING("No instructions\n");
		exit(EXIT_FAILURE);
	}

	optimize(head);

	if (head) 
		PrintInstructionList(stdout, head);
	
	return EXIT_SUCCESS;
}

/**
 * Actual optimization method which finds all outputAI instructions, marks
 * all contributing instructions to them as critical, and removes all non-critical
 * instructions from the given instruction list.
 */
void optimize(Instruction *head) {
	Instruction *node, *temp;
	
	node = head;
	while (node != NULL) {
		// iterate through instructions
		if (node->opcode == OUTPUTAI) {
			// mark only instructions relating to OUTPUTAI instructions
			markOrigins(node);
		}
		node = node->next;
	}
	
	node = head;
	while (node != NULL) {
		if (node->critical) {
			// if this instruction is determined to be critical continue
			node = node->next;
		} else {
			// otherwise we cut it out
			if (node->prev) {
				node->prev->next = node->next;
			}
			
			if (node->next) {
				node->next->prev = node->prev;
			}
			
			temp = node->next;
			free(node);
			node = temp;
		}
	}
}

/**
 * findRegOrigin finds the origin instruction of the specified register
 * starting at the given instruction and looking backwards. This method
 * traverses backwards and finds the instruction where the spccified 
 * register is used as destination.
 * 
 * Returns the instruction that last wrote to the specified register
 * or NULL if the register has not been written to previously.
 */
Instruction *findRegOrigin(Instruction *instr, int reg) {
	Instruction *node = instr;
	
	while (node != NULL) {
		switch (node->opcode) {
			case LOADI:
				if (node->field2 == reg) 
					return node;
				break;
			case LOADAI:
			case ADD:
			case SUB:
			case MUL:
			case DIV:
				if (node->field3 == reg)
					return node;
				break;
			default:
				break;
		}
		
		node = node->prev;
	}
	
	return NULL;
}

/**
 * findMemOrigin finds the origin instruction of the specified memory
 * offset from the specified register. This works the same way as 
 * findRegOrigin, except it looks for the memory address as the 
 * destination instead of a register.
 */
Instruction *findMemOrigin(Instruction *instr, int reg, int offset) {
	Instruction *node = instr->prev;
	
	while (node != NULL) {
		if (node->opcode == STOREAI) {
			if (node->field2 == reg && node->field3 == offset) {
				return node;
			}
		}
		node = node->prev;
	}
	
	return NULL;
}

/**
 * markOrigins is a recursive function that finds all instructions that
 * contribute to the values used in the current instruction. 
 * The function stops recursing when all origins have been found and 
 * marked as critical. Origins are LOADI instructions, where constants
 * are loaded into registers and either stored to a variable or used in
 * an expression. Other instructions are also considered origins that 
 * will be marked as critical, but the origins of the operands will also
 * be searched for.
 */
void markOrigins(Instruction *instr) {
	Instruction *node1 = NULL, *node2 = NULL;
	
	if (instr->critical) {
		// if the instruction is already marked, leave as it has
		// already been traversed
		return;
	}
	
	instr->critical = 1;
	
	switch (instr->opcode) {
		case STOREAI:
			// node1 = register to store, node2 = base register
			node1 = findRegOrigin(instr, instr->field1);
			node2 = findRegOrigin(instr, instr->field2);
			break;
		// outputAI and loadAI have the same positions for memory args
		case OUTPUTAI:
		case LOADAI:
			// node1 = base memory register, node2 = memory address
			node1 = findRegOrigin(instr, instr->field1);
			node2 = findMemOrigin(instr, instr->field1, instr->field2);
			break;
		case ADD:
		case SUB:
		case MUL:
		case DIV:
			node1 = findRegOrigin(instr, instr->field1);
			node2 = findRegOrigin(instr, instr->field2);
			break;
		case LOADI:
			// LOADI has no further origin instructions
			break;
	}
	
	if (node1) {
		markOrigins(node1);
	}
	
	if (node2) {
		markOrigins(node2);
	}
}
