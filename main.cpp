#include "dwarf.hpp"
#include <iostream>

using namespace std;

int main(int argc, char** argv) {
	Dwarf d(argv[1]);


	d.ForEachDie([](const Dwarf::Die& d) {
		cout << endl << "tag = " << d.tag << endl;
		cout << endl << "section = " << d.tag << endl;
		cout << "offset = 0x" << hex << d.offset << endl;
		for (const auto& attr : d.attributes) {
			cout << '\t' << attr.name << " = " << attr.value << " form: " << attr.form << endl;
		}
	});

	return 0;
}
