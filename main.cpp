#include "dwarf.hpp"
#include <iostream>

using namespace std;

int main(int argc, char** argv) {
	Dwarf d(argv[1]);


	d.ForEachDie([](const Dwarf::Die& d) {
		cout << endl << d.tag << endl;
		for (const auto& attr : d.attributes) {
			cout << '\t' << attr.name << " = " << attr.value << endl;
		}
	});

	return 0;
}
