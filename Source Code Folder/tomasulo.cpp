#include <iostream>
#include<algorithm>
#include <math.h>
#include <bitset>
#include <fstream>
#include <string>
#include<vector>
using namespace std;

int result[2]; // CDB variable
unsigned int temp_results[15];
//----------------------*counters*------------------------------------------------//
int instructions_count = 0;
int commit_counter = 0;
int clock_cycles = 0;
int branches_count = 0;
int Mispredictions_count = 0;
bool done = false;

//-----------------------------------------------------------------------//
//formats
#define opcode 15:13
#define regA   12:10
#define regB    9:7
#define regC    2:0
#define RRI_imm 6:0

//--------------------------*Execution latencies*-------------------------------------------//
#define load_latency 3
#define store_latency 3
#define jmp_latency 1
#define beq_latency 1
#define add_latency 2
#define NAND_latency 1
#define MULT_latency 10

//-------------------------------------------------------------------------//
//instructions
#define ADD_opcode     0
#define ADDI_opcode    1
#define NAND_opcode    2
#define SW_opcode   4
#define LW_opcode   5
#define BEQ_opcode   6
#define JALr_opcode    7
#define SUB_opcode     3
#define JMP_opcode     9
#define ret_opcode     15
#define MUL_opcode     10

//-----------------------------------------------------------------------//
//declerations
unsigned int memory[128 * 1024] = { 0 };
unsigned int pc = 0;
unsigned int inst_memory[128 * 1024];
int regs[8] = { 0 };


//------------------*ROB*----------------------------------------------------//
struct ROB_entry {
	int index = 0;
	int type = 0;
	unsigned int dest = 0;
	int value = 0;
	bool ready = false;
	bool predicted = false;
	bool actual = false;
};
ROB_entry ROB[8];
int head, tail;

//-------------------------*Reservation Stations*---------------------------------------------//
struct reservation_station_entry {
	//string name;
	bool ready = true;
	int op = 0; //operation type (aka opcode)
	int Vj = 0, Vk = 0;
	int Qj = 0, Qk = 0;
	int addr = 0;
	unsigned int dest = 0;
	int remaining_cc = 0;
	int inst_pc = 0;
};
reservation_station_entry rs[15];

#define Load1 0
#define Load2 1
#define Store1 2
#define Store2 3
#define JMP1 4
#define JMP2 5
#define BEQ1 6
#define BEQ2 7
#define Add1 8
#define Add2 9
#define Add3 10
#define NAND1 11
#define NAND2 12
#define MULT1 13
#define MULT2 14

/*
rs[0].name = "Load1";
rs[1].name = "Load2";

rs[2].name = "Store1";
rs[3].name = "Store2";

rs[4].name = "JMP1";
rs[5].name = "JMP2";

rs[6].name = "BEQ1";
rs[7].name = "BEQ2";

rs[8].name = "Add1";
rs[9].name = "Add2";
rs[10].name = "Add3";

rs[11].name = "NAND1";
rs[12].name = "NAND2";

rs[13].name = "MULT1";
rs[14].name = "MULT2";
*/

//-----------------------------------------------------------------------//
struct Register_status_entry {
	int reg = 0;
	int ROB_index = 0;
	bool busy = false;
};
Register_status_entry Register_status[8];

//-------------------------Tracing Table-------------------------------------------//
struct Tracing_Table_entry {
	bool issued = false;
	bool executed = false;
	bool written = false;
	bool committed = false;
	int issued_cc = 0;
	int started_exec = -1;
	int executed_cc = 0;
	int written_cc = 0;
	int committed_cc = 0;
	unsigned int inst_regA = 0, inst_regB = 0, inst_regC = 0, inst_RRI_imm = 0, inst_opcode = 0, inst_4bit_opcode = 0;
	int rs_name = 0;
};
vector<Tracing_Table_entry> Tracing_Table;

//------------------------------------------------------------------------//
void inst_identifier(unsigned int instWord, string& RS_type, unsigned int& inst_regA, unsigned int& inst_regB, unsigned int& inst_regC, unsigned int& inst_RRI_imm, unsigned int& inst_opcode, unsigned int& inst_4bit_opcode)
{
	//unsigned int inst_regA, inst_regB, inst_regC, inst_RRI_imm, inst_opcode, inst_4bit_opcode;
	inst_opcode = (instWord & 0xE000) >> 13;
	inst_4bit_opcode = (instWord & 0xF000) >> 12;
	inst_regA = ((instWord << 3) & 0xE000) >> 13;
	inst_regB = ((instWord << 6) & 0xE000) >> 13;
	inst_regC = ((instWord << 16)) & 0xE000 >> 13;
	inst_RRI_imm = ((instWord << 12) & 0xFE00) >> 4;

	if (inst_opcode == ADD_opcode || inst_opcode == ADDI_opcode || inst_opcode == SUB_opcode)
		RS_type = "ADD";
	else if (inst_opcode == BEQ_opcode)
	{
		RS_type = "BEQ";
		branches_count++;
	}
	else if (inst_opcode == LW_opcode)
		RS_type = "Load";
	else if (inst_opcode == SW_opcode)
		RS_type = "Store";
	else if (inst_opcode == NAND_opcode)
		RS_type = "NAND";
	else if (inst_opcode == JALr_opcode || inst_4bit_opcode == JMP_opcode || inst_4bit_opcode == ret_opcode)
		RS_type = "JALR";
	else if (inst_4bit_opcode == MUL_opcode)
		RS_type = "MULT";
}

void handle_issue_operations(unsigned int inst_opcode, int inst_4bit_opcode, int inst_regA, int inst_regB, int inst_regC, int Rs_name)
{
	int b = tail;
	Tracing_Table[pc].rs_name = Rs_name;
	rs[Rs_name].inst_pc = pc;
	int h;
	if (Register_status[inst_regB].busy)
	{
		h = Register_status[inst_regB].ROB_index;
		if (ROB[h].ready)
		{
			rs[Rs_name].Vj = ROB[h].value;
			rs[Rs_name].Qj = 0;
		}
		else
			rs[Rs_name].Qj = h;
	}
	else
	{
		rs[Rs_name].Vj = regs[inst_regB];
		rs[Rs_name].Qj = 0;
	}
	rs[Rs_name].ready = false;
	rs[Rs_name].dest = b;
	if (inst_opcode == MUL_opcode || inst_opcode == JALr_opcode || inst_opcode == ret_opcode)
	{
		ROB[b].type = inst_4bit_opcode;
		rs[Rs_name].op = inst_4bit_opcode;
	}
	else
	{
		ROB[b].type = inst_opcode;
		rs[Rs_name].op = inst_opcode;
	}
	ROB[b].dest = inst_regA;
	ROB[b].ready = false;
	if (Register_status[inst_regC].busy)
	{
		h = Register_status[inst_regC].ROB_index;
		if (ROB[h].ready)
		{
			rs[Rs_name].Vk = ROB[h].value;
			rs[Rs_name].Qk = 0;
		}
		else
			rs[Rs_name].Qk = h;
	}
	else
	{
		rs[Rs_name].Vk = regs[inst_regC];
		rs[Rs_name].Qk = 0;
	}
	Register_status[inst_regA].ROB_index = b;
	Register_status[inst_regA].busy = true;
	ROB[b].dest = inst_regA;
}

void handle_issue_load(unsigned int inst_opcode, int inst_4bit_opcode, int inst_regA, int inst_regB, int inst_regC, int imm, int b, int Rs_name)
{
	int h;
	Tracing_Table[pc].rs_name = Rs_name;
	rs[Rs_name].inst_pc = pc;
	if (Register_status[inst_regB].busy)
	{
		h = Register_status[inst_regB].ROB_index;
		if (ROB[h].ready)
		{
			rs[Rs_name].Vj = ROB[h].value;
			rs[Rs_name].Qj = 0;
		}
		else
			rs[Rs_name].Qj = h;
	}
	else
	{
		rs[Rs_name].Vj = regs[inst_regB];
		rs[Rs_name].Qj = 0;
	}

	rs[inst_regB].ready = false;
	rs[inst_regB].dest = b;
	ROB[b].type = inst_opcode;
	// ROB[b].dest = inst_regA;
	ROB[b].dest = inst_regC;  // check  ?
	ROB[b].ready = false;


	rs[Rs_name].addr = imm;
	Register_status[inst_regC].ROB_index = b;
	Register_status[inst_regC].busy = true;
}

void handle_issue_store(unsigned int inst_opcode, int inst_4bit_opcode, int inst_regA, int inst_regB, int inst_regC, int b, int imm, int Rs_name)
{
	int h;
	Tracing_Table[pc].rs_name = Rs_name;
	rs[Rs_name].inst_pc = pc;
	if (Register_status[inst_regB].busy)
	{
		h = Register_status[inst_regB].ROB_index;
		if (ROB[h].ready)
		{
			rs[Rs_name].Vj = ROB[h].value;
			rs[Rs_name].Qj = 0;
		}
		else
			rs[Rs_name].Qj = h;
	}
	else
	{
		rs[Rs_name].Vj = regs[inst_regB];
		rs[Rs_name].Qj = 0;
	}
	rs[inst_regB].ready = false;
	rs[inst_regB].dest = b;
	rs[Rs_name].addr = imm;
	ROB[b].type = inst_opcode;
	ROB[b].dest = inst_regA;
	ROB[b].ready = false;
}

//Issuing Logic
bool issue(unsigned int inst_opcode, unsigned int inst_4bit_opcode, int inst_regA, int inst_regB, int inst_regC, int imm, string RS_type)
{
	// bool assigned = false;
	// int i = 0 ;
	// while (!assigned && i < 8)
	// {
	//     if (ROB[i].ready == true)
	//     {
	//         b = i;
	//         assigned = true;
	//     }
	//     i++;
	// }
	// if(!assigned)
	// return false;
	if (head != tail || clock_cycles == 1) //ROB is not full
	{
		if (RS_type == "ADD")
		{
			if (rs[Add1].ready == true)
			{ //free reservation station
				handle_issue_operations(inst_opcode, inst_4bit_opcode, inst_regA, inst_regB, inst_regC, Add1);
				tail = (tail + 1) % 8;
				rs[Add1].ready = false;
				return true;
			}
			else if (rs[Add2].ready == 1)
			{
				handle_issue_operations(inst_opcode, inst_4bit_opcode, inst_regA, inst_regB, inst_regC, Add2);
				tail = (tail + 1) % 8;
				rs[Add2].ready = false;
				return true;
			}
			else if (rs[Add3].ready == 1)
			{
				handle_issue_operations(inst_opcode, inst_4bit_opcode, inst_regA, inst_regB, inst_regC, Add3);
				tail = (tail + 1) % 8;
				rs[Add3].ready = false;
				return true;
			}
		}
		else if (RS_type == "BEQ")
		{
			if (rs[BEQ1].ready == 1)
			{//free reservation station
				handle_issue_load(inst_opcode, inst_4bit_opcode, inst_regA, inst_regB, inst_regC, tail, imm, BEQ1);
				tail = (tail + 1) % 8;
				rs[BEQ1].ready = false;
				return true;
			}
			else if (rs[BEQ2].ready == 1)
			{//free reservation station
				handle_issue_load(inst_opcode, inst_4bit_opcode, inst_regA, inst_regB, inst_regC, tail, imm, BEQ2);
				tail = (tail + 1) % 8;
				rs[BEQ2].ready = false;
				return true;
			}
		}
		else if (RS_type == "Load")
		{
			if (rs[Load1].ready == 1)
			{//free reservation station
				handle_issue_load(inst_opcode, inst_4bit_opcode, inst_regA, inst_regB, inst_regC, tail, imm, Load1);
				tail = (tail + 1) % 8;
				rs[Load1].ready = false;
				return true;
			}
			else if (rs[Load2].ready == 1)
			{ //free reservation station
				handle_issue_load(inst_opcode, inst_4bit_opcode, inst_regA, inst_regB, inst_regC, tail, imm, Load2);
				tail = (tail + 1) % 8;
				rs[Load2].ready = false;
				return true;
			}
		}
		else if (RS_type == "Store")
		{
			if (rs[Store1].ready == 1)
			{ //free reservation station
				handle_issue_store(inst_opcode, inst_4bit_opcode, inst_regA, inst_regB, inst_regC, tail, imm, Store1);
				tail = (tail + 1) % 8;
				rs[Store1].ready = false;
				return true;
			}
			else if (rs[Store2].ready == 1)
			{ //free reservation station
				handle_issue_store(inst_opcode, inst_4bit_opcode, inst_regA, inst_regB, inst_regC, tail, imm, Store2);
				tail = (tail + 1) % 8;
				rs[Store2].ready = false;
				return true;
			}
		}
		else if (RS_type == "NAND")
		{
			if (rs[NAND1].ready == 1)
			{ //free reservation station
				handle_issue_operations(inst_opcode, inst_4bit_opcode, inst_regA, inst_regB, inst_regC, NAND1);
				tail = (tail + 1) % 8;
				rs[NAND1].ready = false;
				return true;
			}
			else if (rs[NAND2].ready == 1)
			{ //free reservation station
				handle_issue_operations(inst_opcode, inst_4bit_opcode, inst_regA, inst_regB, inst_regC, NAND2);
				tail = (tail + 1) % 8;
				rs[NAND2].ready = false;
				return true;
			}
		}
		else if (RS_type == "JALR")
		{
			if (rs[JMP1].ready == 1)
			{ //free reservation station
				handle_issue_load(inst_opcode, inst_4bit_opcode, inst_regA, inst_regB, inst_regC, tail, imm, JMP1);
				tail = (tail + 1) % 8;
				rs[JMP1].ready = false;
				return true;
			}
			else if (rs[JMP2].ready == 1)
			{ //free reservation station
				handle_issue_load(inst_opcode, inst_4bit_opcode, inst_regA, inst_regB, inst_regC, tail, imm, JMP2);
				tail = (tail + 1) % 8;
				rs[JMP2].ready = false;
				return true;
			}
		}
		else if (RS_type == "MULT")
		{
			if (rs[MULT1].ready == 1)
			{//free reservation station
				handle_issue_operations(inst_opcode, inst_4bit_opcode, inst_regA, inst_regB, inst_regC, MULT1);
				tail = (tail + 1) % 8;
				rs[MULT1].ready = false;
				return true;
			}
			else if (rs[MULT2].ready == 1)
			{ //free reservation station
				handle_issue_operations(inst_opcode, inst_4bit_opcode, inst_regA, inst_regB, inst_regC, MULT2);
				tail = (tail + 1) % 8;
				rs[MULT2].ready = false;
				return true;
			}
		}
		else
		{
			// increment counter cycles ( stall )
			return false;
		}
	}
	else //ROB full
		return false;
}

//------------------------*Execute*-----------------------------//    //execution latencies -> to be done

// we need to create a function to handle all the reservation stations
/* loop from 0 to 14 where we handle the execution in each reservation station with a counter that counts how many cycles are remaining for each Rs */
// void handle_executions ()
// {
//     for ( int i = 0 ; i < 15 ; i++)
//     {
//         if(rs[i].ready == 0)
//         {
//             switch (rs[i].op)
//             {
//             case ADD_opcode:


//                 break;

//             default:

//             }
//         }
//     }
// }
bool execute(int h, int b, int r)
{
	bool store_exists = false;
	bool load_step1_done = false;
	if (rs[r].op == LW_opcode)
	{
		/*
		if (head > ROB[b].index)
		{
			for (int i = ROB[b].index; i >= 0; i--) //check b
				if (ROB[i].type == SW_opcode)
				{
					store_exists = true;
					return false;
				}
			for (int i = 7; i >= head; i--) //check b
				if (ROB[i].type == SW_opcode)
				{
					store_exists = true;
					return false;
				}
		}
		else //head <= ROB[b].index
		{
			for (int i = ROB[b].index; i >= head; i--) //check b
				if (ROB[i].type == SW_opcode)
				{
					store_exists = true;
					return false;
				}
		}
		*/

		if (rs[r].Qj == 0) // I removed the 2nd condition here (no stores before) & commented its logic above (if and else blocks) as Osos said that Dr. Cherif said we should ignore it
		{
			rs[r].addr = rs[r].Vj + rs[r].addr;
			load_step1_done = true;
		}
		if (load_step1_done)
		{
			bool flag = false;
			if (head > rs[r].dest)
			{
				for (int i = rs[r].dest; (i >= 0 && !flag); i--) //check b
					if (rs[i].op == SW_opcode)
					{
						if (rs[r].addr == ROB[i].dest)
							flag = true;
					}
				for (int i = 7; (i >= head && !flag); i--) //check b
					if (rs[i].op == SW_opcode)
					{
						if (rs[r].addr == ROB[i].dest)
							flag = true;
					}
			}
			else //head <= rs[r].dest  (instruction ROB index)
			{
				for (int i = rs[r].dest; (i >= head && !flag); i--) //check b
					if (rs[i].op == SW_opcode)
					{
						if (rs[r].addr == ROB[i].dest)
							flag = true;
					}
			}
			//all stores earlier in the ROB have different addresses
			if (!flag)
			{
				temp_results[r] = memory[rs[r].addr];

				return true;
			}
		}
	}
	else if (rs[r].op == SW_opcode) //ELSE if not if
	{
		if (rs[r].Qj == 0 && rs[r].dest == head)
		{
			ROB[head].dest = rs[r].Vj + rs[r].addr;
			return true;
		}
		else
			return false;
	}
	else
	{
		if (rs[r].Qj == 0 && rs[r].Qk == 0)
		{
			if (rs[r].op == ADD_opcode)
			{
				temp_results[r] = rs[r].Vj + rs[r].Vk;
				return true;
			}
			if (rs[r].op == SUB_opcode)
			{
				temp_results[r] = rs[r].Vj - rs[r].Vk;
				return true;
			}
			if (rs[r].op == MUL_opcode)
			{
				temp_results[r] = rs[r].Vj * rs[r].Vk;
				return true;
			}
			if (rs[r].op == BEQ_opcode)
			{
				ROB[rs[r].dest].value = rs[r].addr;
				if (rs[r].Vj == rs[r].Vk)
					ROB[rs[r].dest].actual = true;
				else
					ROB[rs[r].dest].actual = false;
				if (rs[r].addr < 0)
					ROB[rs[r].dest].predicted = true;
				else
					ROB[rs[r].dest].predicted = false;
				if (ROB[rs[r].dest].predicted != ROB[rs[r].dest].actual)
					Mispredictions_count++;
				if (ROB[rs[r].dest].predicted)
					pc = pc + 1 + rs[r].addr;
				return true;
			}
			if (rs[r].op == JALr_opcode)
			{
				pc = rs[r].Vj;
				temp_results[r] = pc + 1;
				return true;
			}
			if (rs[r].op == ret_opcode)
			{
				pc = rs[r].Vj;
				return true;
			}
		}
	}
}

// bool compare_pc(reservation_station_entry a, reservation_station_entry b)
// {

//     return (a.inst_pc <= b.inst_pc);
// }

//------------------------*write*-----------------------------//
//writing an instruction
bool write(int h, int r, unsigned int inst_opcode)
{
	int b;
	if (inst_opcode == SW_opcode && rs[r].Qk == 0)
	{
		ROB[h].value = rs[r].Vk;
		return true;
	}
	else //all instructions except store
	//if now executed == any previous executed check if issued now < issued before ? (is CDB available?)
	// need to check if we can write now; this happens if no previous executions happen at the same time as current exeuction time
	// if so, we give the previous one(s) a priority and wait till they finish (return false), if we can write, we write (return true)
	{
		b = rs[r].dest;
		rs[r].ready = true;
		for (int x = 0; x < 15; x++)
			if (rs[x].Qj == b)
			{
				rs[x].Vj = temp_results[x];
				rs[x].Qj = 0;
			}
		for (int x = 0; x < 15; x++)
			if (rs[x].Qk == b)
			{
				rs[x].Vk = temp_results[x];
				rs[x].Qk = 0;
			}
		ROB[b].value = temp_results[r];
		ROB[b].ready = true;
	}
}

//---------------------------commit-------------------------------//
//committing an instruction
bool commit()
{
	if (ROB[head].ready)
	{
		int d = ROB[head].dest;
		if (ROB[head].type == BEQ_opcode)
		{
			// if (branch is mispredicted)
			//     clear ROB[h];    clear RegisterStat;     fetch branch dest;
			if (ROB[head].predicted != ROB[head].actual)
			{
				ROB[head].dest = 0;
				ROB[head].ready = false;
				ROB[head].value = 0;
				ROB[head].type = 0;
				ROB[head].actual = false;
				ROB[head].predicted = false;
				for (int i = 0; i < 8; i++)
				{
					Register_status[i].busy = false;
					Register_status[i].ROB_index = 0;
				}
				if (ROB[head].predicted)
					pc = pc + 1 + ROB[head].dest;
				else
					pc = pc - 1 - ROB[head].dest;
			}
		}
		else if (ROB[head].type == SW_opcode)
			memory[ROB[head].dest] = ROB[head].value;
		else //put the result in the register destination
			regs[d] = ROB[head].value;
		ROB[head].ready = true;
		if (Register_status[d].ROB_index == head)
			Register_status[d].busy = false;
		head = (head + 1) % 8;
		return true;
	}
}

int main()
{
	// reading assembly file
	Tracing_Table.resize(0);
	string inst;
	unsigned int inst_word = 0;
	int i = 0;
	ifstream assembly, data_file;
	assembly.open("file1.txt"); //testcase name
	getline(assembly, inst, '\n');
	//getline(assembly, inst_word, '\n');
	if (assembly.fail())
		cout << "problem";

	Tracing_Table_entry temp;

	while (!assembly.eof())
	{
/*		cout << inst;
		for (int j = inst.length() - 1; j >= 0; j--)
		{
			if (inst[j] == '1')
				inst_word += pow(2, j);
			cout << inst_word << endl;
		}
		*/
		inst_word = std::stoi(inst);
		Tracing_Table_entry temp;
		Tracing_Table.push_back(temp);
		inst_memory[i++] = inst_word;
		getline(assembly, inst, '\n');
	}
	Tracing_Table.push_back(temp);
	assembly.close();
	//------------------------------------------------------//
	   //reading data file
	string addr, data;
	unsigned int data_s;
/*	data_file.open("data1"); //testcase name
	getline(data_file, addr, ',');
	getline(data_file, data, '\n');
	while (!data_file.eof())
	{
		i = std::stoi(addr);
		data_s = std::stoi(data);
		memory[i] = (unsigned int)data_s;
		getline(data_file, addr, ',');
		getline(data_file, data, '\n');
	}
	data_file.close();
	*/
	string rs_type;
	while (!done)
	{
		if (regs[0] != 0)
			regs[0] = 0;
		clock_cycles++;
		// unsigned int inst_regA, inst_regB, inst_regC, inst_RRI_imm, inst_opcode, inst_4bit_opcode;
		 //Identify instructions
		 //issuing  instructions ----> we need to keep track whether 2 instructions are issued or not?? (TO DO)
		for (int i = 0; i < 2; i++)
		{
			inst_identifier(inst_memory[pc], rs_type, Tracing_Table[pc].inst_regA, Tracing_Table[pc].inst_regB, Tracing_Table[pc].inst_regC, Tracing_Table[pc].inst_RRI_imm, Tracing_Table[pc].inst_opcode, Tracing_Table[pc].inst_4bit_opcode);
			Tracing_Table[pc].issued = issue(Tracing_Table[pc].inst_opcode, Tracing_Table[pc].inst_4bit_opcode, Tracing_Table[pc].inst_regA, Tracing_Table[pc].inst_regB, Tracing_Table[pc].inst_regC, Tracing_Table[pc].inst_RRI_imm, rs_type);

			if (Tracing_Table[pc].issued)
			{
				Tracing_Table[pc].issued_cc = clock_cycles;
				pc++;
			}
		}
		//Executing  instructions    /* call the new function */
		for (int r = 0; r < 15; r++)
		{
			if (rs[r].ready == false && !(Tracing_Table[rs[r].inst_pc].executed)) //check 2nd condition
			{
				bool started_execution = execute(head, tail, r);
				if (started_execution)
				{
					Tracing_Table[rs[r].inst_pc].executed = true;
					switch (rs[r].op)
					{
					case LW_opcode:
						Tracing_Table[rs[r].inst_pc].executed_cc = clock_cycles + 3;
						Tracing_Table[rs[r].inst_pc].executed = true;
						break;
					case SW_opcode:
						Tracing_Table[rs[r].inst_pc].executed_cc = clock_cycles + 3;
						Tracing_Table[rs[r].inst_pc].executed = true;
						break;
					case JMP_opcode:
						Tracing_Table[rs[r].inst_pc].executed_cc = clock_cycles + 1;
						Tracing_Table[rs[r].inst_pc].executed = true;
						break;
					case JALr_opcode:
						Tracing_Table[rs[r].inst_pc].executed_cc = clock_cycles + 1;
						Tracing_Table[rs[r].inst_pc].executed = true;
						break;
					case ret_opcode:
						Tracing_Table[rs[r].inst_pc].executed_cc = clock_cycles + 1;
						Tracing_Table[rs[r].inst_pc].executed = true;
						break;
					case BEQ_opcode:
						Tracing_Table[rs[r].inst_pc].executed_cc = clock_cycles + 1;
						Tracing_Table[rs[r].inst_pc].executed = true;
						break;
					case ADD_opcode:
						Tracing_Table[rs[r].inst_pc].executed_cc = clock_cycles + 2;
						Tracing_Table[rs[r].inst_pc].executed = true;
						break;
					case SUB_opcode:
						Tracing_Table[rs[r].inst_pc].executed_cc = clock_cycles + 2;
						Tracing_Table[rs[r].inst_pc].executed = true;
						break;
					case ADDI_opcode:
						Tracing_Table[rs[r].inst_pc].executed_cc = clock_cycles + 2;
						Tracing_Table[rs[r].inst_pc].executed = true;
						break;
					case NAND_opcode:
						Tracing_Table[rs[r].inst_pc].executed_cc = clock_cycles + 1;
						Tracing_Table[rs[r].inst_pc].executed = true;
						break;
					case MUL_opcode:
						Tracing_Table[rs[r].inst_pc].executed_cc = clock_cycles + 10;
						Tracing_Table[rs[r].inst_pc].executed = true;
						break;
					default:
						break;
					}
				}
			}
		}
		//Writing instructions
/*		int written_counter = 0;
		for (int i = 0; i < pc && (written_counter < 2); i++)
			if (clock_cycles >= Tracing_Table[i].executed_cc && !Tracing_Table[i].written)
				if (write(head, Tracing_Table[i].rs_name, rs[Tracing_Table[i].rs_name].op))
				{
					written_counter++;
					Tracing_Table[i].written_cc = clock_cycles;
					Tracing_Table[i].written = true;
				}
		*/
		int written_counter = 0;
		for (int i = 0; i < pc && (written_counter < 2); i++)
			if (Tracing_Table[i].executed && !Tracing_Table[i].written)
				if (write(head, Tracing_Table[i].rs_name, rs[Tracing_Table[i].rs_name].op))
				{
					written_counter++;
					Tracing_Table[i].written_cc = clock_cycles;
					Tracing_Table[i].written = true;
				}


		////committing instructions
  //      int commit_counter = 0;
		//for (int i = 0; i < pc && (commit_counter < 2); i++)
		//	if (Tracing_Table[i].written && !Tracing_Table[i].committed)
		//			if(commit())
		//			{
		//				commit_counter++;
		//				Tracing_Table[i].committed = true;
		//			}

		if ( commit());
		{
			Tracing_Table[commit_counter++].committed = true;
			Tracing_Table[commit_counter].committed_cc = clock_cycles;
			if (commit())
			{
				Tracing_Table[commit_counter].committed = true;
				Tracing_Table[commit_counter].committed_cc = clock_cycles;
			}
		}
		if (commit_counter == 4  )
			done = true;
	}
	for (int i = 0; i < Tracing_Table.size(); i++)
	{
		cout << i << ": Issued: " << Tracing_Table[i].issued_cc << endl << "Executed: " << Tracing_Table[i].executed_cc
			<< endl << "Written: " << Tracing_Table[i].written_cc << endl << "Committed: " << Tracing_Table[i].committed_cc << endl;
	}
	cout << "Execution Time: " << clock_cycles << endl << "IPC: " << double(instructions_count) / double(clock_cycles)
		<< endl << "Branch mispredictions Percentage: " << Mispredictions_count / double(branches_count) * 100 << endl;
	system("pause");
	return 0;
}

// sort(to_be_written.begin(), to_be_written.end(), compare_pc);
// for ( int i = 0 ; i < 2 ; i++)
// {
//     write(head,  , to_be_written[i].op )
// }
