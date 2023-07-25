#include "Core.h"

int main(int argc, char *argv[]) {
	if (argc == 2) {
		std::string argument{argv[1]};
		Core running_core(argument);
	} else
		throw std::invalid_argument("MAIN INSTRUCTION FILE CANNOT BE FOUND");
	return 0;
}