//
// parser.h
// project2
//
// Created by ahmed ehab hamouda on 12/8/19.
// Copyright © 2019 ahmed ehab hamouda. All rights reserved.
//
#include<iostream>
#include<string>
#include <fstream>
using namespace std;
#ifndef parser_h
#define parser_h
class parser {
private:
	int arr[40];
	int PC = 0;
	ofstream x;
	std::string instruction;
	std::string empty_field;
	std::string Opcode;
	std::string RegA;
	std::string RegB;
	std::string RegC;
	std::string immediate;
public:
	void reset_PC() {
		PC = 0;
		x.open("tst1.txt");
	}
	void setinstruction(std::string s) {
		parse_instruction(s);
	}
	bool detectlabel(string s) {
		if (s[s.length() - 1] == ':') {
			arr[stoi(s.substr(1, s.length() - 2))] = PC;
			cout << PC << endl;
			s.erase(s.begin(), s.end());
			return true;
		}

		PC++;
		return false;
	}
private:

	void parse_instruction(std::string s) {
		std::string type = inst_type(s);
		if (type == "ADD") {
			int space_pos = s.find(' ');
			int comma1_pos = s.find(',');
			int comma2_pos = s.rfind(',');
			empty_field = "0000";
			Opcode = "000";
			string m = s.substr(space_pos + 2, comma1_pos - space_pos - 2);
			RegA = decToBinary(stoi(m));
			addzeros(RegA, 3);

			RegB = decToBinary(stoi(s.substr(comma1_pos + 2, comma2_pos -
				comma1_pos)));

			addzeros(RegB, 3);
			RegC = decToBinary(stoi(s.substr(comma2_pos + 2)));
			addzeros(RegC, 3);
			instruction = Opcode + RegA + RegB + empty_field + RegC;

		}
		if (type == "NAND") {
			int space_pos = s.find(' ');
			int comma1_pos = s.find(',');
			int comma2_pos = s.rfind(',');
			empty_field = "0000";
			Opcode = "010";

			RegA = decToBinary(stoi(s.substr(space_pos + 2, comma1_pos - space_pos -
				1)));

			addzeros(RegA, 3);

			RegB = decToBinary(stoi(s.substr(comma1_pos + 2, comma2_pos -
				comma1_pos)));

			addzeros(RegB, 3);
			RegC = decToBinary(stoi(s.substr(comma2_pos + 2)));
			addzeros(RegC, 3);
			instruction = Opcode + RegA + RegB + empty_field + RegC;
		}
		if (type == "SUB") {
			int space_pos = s.find(' ');

			int comma1_pos = s.find(',');
			int comma2_pos = s.rfind(',');
			empty_field = "0000";
			Opcode = "011";

			RegA = decToBinary(stoi(s.substr(space_pos + 2, comma1_pos - space_pos -
				1)));

			addzeros(RegA, 3);

			RegB = decToBinary(stoi(s.substr(comma1_pos + 2, comma2_pos -
				comma1_pos)));

			addzeros(RegB, 3);
			RegC = decToBinary(stoi(s.substr(comma2_pos + 2)));
			addzeros(RegC, 3);
			instruction = Opcode + RegA + RegB + empty_field + RegC;
		}
		if (type == "BEQ") {
			int space_pos = s.find(' ');
			int comma1_pos = s.find(',');
			int comma2_pos = s.rfind(',');
			Opcode = "110";

			RegA = decToBinary(stoi(s.substr(space_pos + 2, comma1_pos - space_pos -
				1)));

			addzeros(RegA, 3);

			RegB = decToBinary(stoi(s.substr(comma1_pos + 2, comma2_pos -
				comma1_pos)));

			addzeros(RegB, 3);
			if (isdigit(s[comma2_pos + 1])) {
				immediate = decToBinary(stoi(s.substr(comma2_pos + 1)));
				addzeros(immediate, 7);
			}
			else {
				int temp = arr[stoi(s.substr(comma2_pos + 2))];
				temp = temp - (PC - 1);
				immediate = decToBinary(temp);
				addzeros(immediate, 7);
			}
			instruction = Opcode + RegA + RegB + immediate;
		}
		if (type == "SW") {
			int space_pos = s.find(' ');
			int comma1_pos = s.find(',');
			int comma2_pos = s.rfind(',');
			Opcode = "100";

			RegA = decToBinary(stoi(s.substr(space_pos + 2, comma1_pos - space_pos -
				1)));

			addzeros(RegA, 3);

			RegB = decToBinary(stoi(s.substr(comma1_pos + 2, comma2_pos -
				comma1_pos)));

			addzeros(RegB, 3);
			immediate = decToBinary(stoi(s.substr(comma2_pos + 1)));
			addzeros(immediate, 7);
			instruction = Opcode + RegA + RegB + immediate;
		}
		if (type == "LW") {
			int space_pos = s.find(' ');
			int comma1_pos = s.find(',');
			int comma2_pos = s.rfind(',');
			Opcode = "101";

			RegA = decToBinary(stoi(s.substr(space_pos + 2, comma1_pos - space_pos -
				1)));

			addzeros(RegA, 3);

			RegB = decToBinary(stoi(s.substr(comma1_pos + 2, comma2_pos -
				comma1_pos)));

			addzeros(RegB, 3);
			immediate = decToBinary(stoi(s.substr(comma2_pos + 1)));
			addzeros(immediate, 7);
			instruction = Opcode + RegA + RegB + immediate;
		}
		if (type == "JALr") {
			int space_pos = s.find(' ');
			Opcode = "101";
			RegA = decToBinary(stoi(s.substr(space_pos + 2)));
			addzeros(RegA, 3);
			RegB = "000";
			empty_field = "0000000";
			instruction = Opcode + RegA + RegB + empty_field;
		}
		if (type == "ADDI") {
			int space_pos = s.find(' ');
			int comma1_pos = s.find(',');
			int comma2_pos = s.rfind(',');
			Opcode = "001";

			RegA = decToBinary(stoi(s.substr(space_pos + 2, comma1_pos - space_pos -
				1)));

			addzeros(RegA, 3);

			RegB = decToBinary(stoi(s.substr(comma1_pos + 2, comma2_pos -
				comma1_pos)));

			addzeros(RegB, 3);
			immediate = decToBinary(stoi(s.substr(comma2_pos + 1)));
			addzeros(immediate, 7);

			instruction = Opcode + RegA + RegB + immediate;
		}
		if (type == "MUL") {
			int space_pos = s.find(' ');
			int comma1_pos = s.find(',');
			int comma2_pos = s.rfind(',');
			empty_field = "000";
			Opcode = "1010";

			RegA = decToBinary(stoi(s.substr(space_pos + 2, comma1_pos - space_pos -
				1)));

			addzeros(RegA, 3);

			RegB = decToBinary(stoi(s.substr(comma1_pos + 2, comma2_pos -
				comma1_pos)));

			addzeros(RegB, 3);
			RegC = decToBinary(stoi(s.substr(comma2_pos + 2)));
			addzeros(RegC, 3);
			instruction = Opcode + RegA + RegB + empty_field + RegC;
		}
		if (type == "ret") {
			Opcode = "1111";
			empty_field = "000000000000";
			instruction = Opcode + empty_field;
		}
		if (type == "JMP") {
			int space_pos = s.find(' ');
			Opcode = "1001";
			if (isdigit(s[space_pos + 1])) {
				immediate = decToBinary(stoi(s.substr(space_pos + 1)));
				addzeros(immediate, 12);
			}
			else
			{
				int temp = arr[stoi(s.substr(space_pos + 2))];
				temp -= (PC - 1);
				immediate = decToBinary(temp);
				addzeros(immediate, 12);
			}
			instruction = Opcode + immediate;
		}
		if (x.fail())
		x << instruction << endl;
		cout << PC << endl;
	}
	std::string inst_type(std::string s) {

		int pos = s.find(' ');
		return s.substr(0, pos);
	}

	std::string decToBinary(int n)
	{
		if (n < 0) { // check if negative and alter the number
			n = 128 + n;
		}
		string result = "";
		while (n > 0) {
			result = string(1, (char)(n % 2 + 48)) + result;
			n = n / 2;
		}
		return result;

	}
	void addzeros(string& s, int n) {
		while (s.length() < n) {
			s = '0' + s;
		}
	}
};
int main() {
	string s;
	ifstream file;
	parser k;
	file.open("tst1.txt");
	while (!file.eof()) {
		getline(file, s);
		if (k.detectlabel(s)) {
		}
	}
	file.close();
	file.open("hax.txt");
	k.reset_PC();
	while (!file.eof()) {
		getline(file, s);
		if (!k.detectlabel(s))
			k.setinstruction(s);
	}
}

#endif /* parser_h */
