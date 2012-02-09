import sys
import re
debug=1

class lexer: # enum class to seperate instructions from operands etc.
	INSTRUCTION=0
	OPERAND=1
	OPERATOR=2
	DIGIT=3
	COMMENT=4
	END_STMNT=5

def readAssemblyTable():
	table = []
	assemblyTableFile = open('lang.lf', 'r')

	for line in assemblyTableFile:
		line = line.upper().split(' ')
		
		# line[1] is the opcode of the instruction encoded in base 16
		line[1] = int(line[1], 16)
		# line[2] is the number of operands allowed for a given instruction.
		line[2] = int(line[2])
		
		table.append(line)
	return table

def instruction(scanner, token): # called when scanner comes across an instruction
	lookupIndex=0
	for lookup in assemblyTable: # lookup opcode for instructions
		if token == lookup[0]:
			if debug:
				return lexer.INSTRUCTION, lookup[1], lookup[2], token
			else:
				return lexer.INSTRUCTION, lookup[1], lookup[2]
	return -1, token

def operator(scanner, token): # called when scanner comes across an operator
	return lexer.OPERATOR, token

def operand(scanner, token): # called when scanner comes across an operand
	token = token.rstrip(',')
	for lookup in operandTable: # loopup operand in operand table
		if token == lookup[0]:
			if debug:
				return lexer.OPERAND, lookup[1], token
			else:
				return lexer.OPERAND, lookup[1]
	return -1, token

def number(scanner, token): # called when scanner comes across a number
	if token.find(".") != -1:
		return lexer.DIGIT, float(token)
	else:
		return lexer.DIGIT, int(token)

def end_stmnt(scanner, token): # called when scanner comes across an end of line
	return lexer.END_STMNT, "END_STATEMENT"

def comment(scanner, token): # called when scanner comes across a comment
	return None
	#return lexer.COMMENT, token

def assemble(code):
	machineCode=""
	i=0
	while i < len(code): # repeat until end of file
		tmp=[]
		while i < len(code) and code[i][0] != lexer.END_STMNT: # get one line of file
			tmp.append(code[i])
			i += 1

		if debug: # debug information
			print("assembling: ")
			print(tmp)
			print("")

		machineCode += str(checkLine(tmp)) + " " # assemble line
		i += 1
	return machineCode

def checkLine(code):
	returnvar=0x0
	tmp=[]

	# check if number of operands is correct
	if len(code)-1 != code[0][2]:
		# if not, then split line into correct number of operands
		tmp=destructline(code)
		# subassemble and return line of code
		return assemble(tmp)

	for j in code:
		# call appropriate functions
		evaluate={
			lexer.INSTRUCTION: lambda x: handleInstruction(returnvar, j[1]),
			lexer.OPERAND: lambda x: handleOperand(returnvar, j[1]),
			lexer.DIGIT: lambda x: handleDigit(returnvar, j[1])
		}
		tmp = evaluate.get(j[0]) # equivalent of switch statement
		returnvar = tmp(None)
	return returnvar

def destructline(code): # split complex instructions ) into seperate instructions

# ex. MOV ax, bx + 20
# becomes:
#     ADD bx, 20
#     MOV ax, bx

	if debug: # debug information
		print("entered subassembly:")
		print(code)

	returnvar=[]
	i=0
	compacted=0
	while i < len(code):
		# when comming across an operator, convert into separate instructions
		if code[i][0] == lexer.OPERATOR:
			# 'collect' arguments
			arg1 = code[i-1]
			arg2 = code[i+1]
			# number, operator, number results in cut down to one number
			if arg1[0] == lexer.DIGIT and arg2[0] == lexer.DIGIT:
				compacted=1
				code=subassemblenumbers(code, i, arg1, arg2)
		i+=1
	
	if not compacted:
		print("")
		print("error, inforrect number of operands, exiting")
		sys.exit()
	
	if debug: # debug information
		print("exiting subassembly")
		print("")
	
	return code

def subassemblenumbers(code, i, arg1, arg2): # convert two numbers into one (ex. 9 + 2 becomes 11)
	evaluate={ # evaluate which operator is being used
		"+": arg1[1]+arg2[1],
		"-": arg1[1]-arg2[1],
		"/": arg1[1]/arg2[1],
		"*": arg1[1]*arg2[1]
	}
	# construct new number using lexer enum
	tmp=list()
	tmp.append(lexer.DIGIT)
	tmp.append(evaluate.get(code[i][1]))
	# remove number, operator and number
	code.remove(code[i-1])
	code.remove(code[i-1])
	code.remove(code[i-1])
	# insert new number into correct pos of list
	code.insert(i-1, tmp)
	return code

def handleInstruction(cur, i): # convert instruction to machine code
	return i

def handleOperand(cur, i): # convert operand into machine code
	cur |= i
	return cur

def handleDigit(cur, i): # convert number into machine code
	return cur

scanner = re.Scanner([
	(r"//.*", comment), # find comments in code
	(r"[A-Z]+", instruction), # find instructions in code
	(r"[a-zA-Z]+,?", operand), # find operands in code
	(r"[+\-*/=]", operator), # find operators in code
	(r"[0-9]+(\.[0-9]+)?", number), # find numbers in code
	(r"\n", end_stmnt), # find end of line
	(r"\s+", None) # do nothing with the rest
])

assemblyTable=readAssemblyTable()
# [ ['MOV', 0x0100], ['JMP', 0x0200] ] # assembly table
operandTable=[ ['ax',0x0001], ['bx', 0x0002], ['cx', 0x0004], ['dx', 0x0008], ['ex', 0x0010] ] # operand table
file = open(sys.argv[1])
tokens, remainder = scanner.scan(file.read())

if debug: # debug information
	print("entire code:")
	print(tokens)
	print("")

tokens = assemble(tokens)
print("final program in binary format (instructions separated by spaces: ")
print(tokens)
