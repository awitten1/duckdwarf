#include "dwarf.hpp"
#include <iostream>

using namespace std;

int main(int argc, char** argv) {
	Dwarf d(argv[1]);

	for (auto it = d.begin(); it != d.end(); ++it) {
		const auto& die = *it;
		cout << endl << "tag = " << die.tag << endl;
		cout << "section = " << die.section << endl;
		cout << "offset = 0x" << hex << die.offset << endl;
		if (die.attributes.size() == 0) {
			cout << "no attrs" << endl;
		}
		for (const auto& attr : die.attributes) {
			cout << '\t' << attr.name << " = " << attr.value << " form: " << attr.form << endl;
		}
	}

	return 0;
}
