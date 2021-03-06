#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <algorithm>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;

typedef struct {
	string name;
	string type;
	int startbyte;
	int length;
	float factor;
	string displayName;
	string unit;
	float expectedMin;
	float expectedMax;
} can_message_t;

string convert_to_json(ifstream &input);

int main(int argc, char* argv[])
{
	// argv[0] is the name of the program
	// argv[1] is the first argument

	string program_name(argv[0]);
	reverse(program_name.begin(), program_name.end());
	program_name = program_name.substr(0, program_name.find("\\"));
	reverse(program_name.begin(), program_name.end());

	if (argc != 2) {
		cout << "Example usage:" << endl;
		cout << program_name << " example_file.txt" << endl;
		cout << "=> Spits out some JSON you can use in unpacker, one for each inverter, based on n << 2 convention" << endl;
		cout << "Input file format:" << endl;
		cout << "{CAN_ID}" << endl;
		cout << "{type[index] = name;}" << endl;
		cout << "Ex:" << endl;
		cout << "0x170" << endl;
		cout << "f32[0] = phase_a_duty_cycle;\nf32[1] = phase_b_duty_cycle;\nf32[2] = phase_c_duty_cycle;\nf32[3] = phase_a_duty_cycle_new;\nf32[4] = phase_b_duty_cycle_new;\nf32[5] = phase_c_duty_cycle_new;" << endl;
		return 0;
	}

	ifstream input_file{ argv[1] };

	if (!input_file) {
		cout << "Could not open file, exiting :(" << endl;
		return 0;
	}

	cout << convert_to_json(input_file) << endl;
}

map<string, string> type_to_type = {
	make_pair("u8", "Uint"),
	make_pair("i8", "Int"),
	make_pair("u16", "Uint"),
	make_pair("i16", "Int"),
	make_pair("u32", "Uint"),
	make_pair("i32", "Int"),
	make_pair("u64", "Uint"),
	make_pair("i64", "Int"),
	make_pair("f32", "Float"),
	make_pair("f64", "Float")
};

map<string, int> type_size_thing = {
	make_pair("u8", 1),
	make_pair("i8", 1),
	make_pair("u16", 2),
	make_pair("i16", 2),
	make_pair("u32", 4),
	make_pair("i32", 4),
	make_pair("u64", 8),
	make_pair("i64", 8),
	make_pair("f32", 4),
	make_pair("f64", 8)
};

string removeSpaces(string str)
{
	str.erase(remove(str.begin(), str.end(), ' '), str.end());
	return str;
}

template< typename T >
std::string int_to_hex(T i)
{
	std::stringstream stream;
	stream << "0x"
		// Probably have to fix something here
		<< std::setfill('0') << std::setw(sizeof(T) * 2)
		<< std::hex << i;
	return stream.str();
}

string escape_string(string in)
{
	return "\"" + in + "\"";
}

string generate_json_message(can_message_t message, string inverter) {
	string json = "";

	json += escape_string(inverter + "_" + message.name) + ": {";
	json += escape_string("type") + ":" + escape_string(message.type) + ",";
	json += escape_string("startbyte")+ ":" + to_string(message.startbyte) + ",";
	json += escape_string("length") + ":" + to_string(message.length) + ",";
	json += escape_string("factor") + ":" + to_string(message.factor) + ",";
	json += escape_string("displayname") + ":" + escape_string(message.displayName) + ",";
	json += escape_string("unit") + ":" + escape_string(message.unit) + ",";
	json += escape_string("expectedmin") + ":" + to_string(message.expectedMin) + ",";
	json += escape_string("expectedmax") + ":" + to_string(message.expectedMax) + "},";

	return json;
}

string convert_to_json(ifstream &input)
{
	vector<string> json = { "","","","" };
	string temp;
	getline(input, temp);
	int id = stoi(temp, nullptr, 16);
	// 4 since 4 inv
	for (int i = 0; i < 4; i++) {
		json.at(i) = escape_string(int_to_hex(id | (i << 2))) + ":{";
	}
	while (getline(input, temp))
	{
		// Remove all spaces
		temp = removeSpaces(temp);

		can_message_t new_message;

		// Get Analyze type
		new_message.type = type_to_type.at(temp.substr(0, temp.find("[")));
		// Get bytesize of thing
		new_message.length = type_size_thing.at(temp.substr(0, temp.find("[")));
		// Remove type and first [
		temp = temp.substr(temp.find("[") + 1);
		// Get startbyte, array pos times size
		new_message.startbyte = stoi(temp.substr(0,temp.find("]"))) * new_message.length;
		// remove everything up to and including = sign
		temp = temp.substr(temp.find("=") + 1);
		// Name of message, assumes that it is copied straight out of code, inclding ;
		new_message.name = temp.substr(0, temp.find(";"));
		// Set unused to default values
		new_message.factor = 1;
		new_message.displayName = "";
		new_message.unit = "";
		new_message.expectedMin = 0.0;
		new_message.expectedMax = 0.0;
		
		json.at(0) += generate_json_message(new_message, "FL");
		json.at(1) += generate_json_message(new_message, "FR");
		json.at(2) += generate_json_message(new_message, "RL");
		json.at(3) += generate_json_message(new_message, "RR");
	}

	// Remove last comma, add ending }
	for (int i = 0; i < 4; i++) {
		json.at(i) = json.at(i).substr(0, json.at(i).length() - 1) + "}";
	}

	return json.at(0) + "," + json.at(1) + "," + json.at(2) + "," + json.at(3);
}