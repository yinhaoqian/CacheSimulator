//
// Created by Yinhao on 4/13/2022.
//

#ifndef CODE_CORE_H
#define CODE_CORE_H

#include "Include.h"
#include "System.h"

class Core {
	using ArgumentTuple_t = std::tuple<uint32_t, uint32_t, uint32_t>;
	using SystemFunction_t = std::pair<bool (System::*)(ArgumentTuple_t *), size_t>;

	System system;
	std::ifstream instruction_reader;
	std::unordered_map<std::string, SystemFunction_t> instruction_map;

	/**
	 * Convert an String Argument to Integer 32-bit type
	 * @param _char_to_check Character to be checked
	 * @return True if is a Numeric Digit, false otherwise
	 */
	uint32_t argumentToInt(const std::string &_argument) {
		bool dollarSignDetected = _argument.at(0) == '$';
		bool allOtherIsDigit = std::all_of(_argument.begin() + 1, _argument.end(), ::isdigit);
		if (!dollarSignDetected || !allOtherIsDigit)
			throw std::runtime_error("ERR Instruction Argument Format Error");
		return std::stoi(std::string(_argument.begin() + 1, _argument.end()), nullptr, 10);
	}

	/**w
	 * Get the Next Instruction
	 * Warning: This Function does NOT handle EOF
	 * @return [INSTRUCTION_STRING][[ARGUMENT1][ARGUMENT2][ARGUMENT3]]
	 */
	std::pair<std::string, ArgumentTuple_t> getNextInstruction() {
		if (!instruction_reader.is_open()) throw std::runtime_error("ERR Input File NOT Initialized.");
		std::string this_instruction_string, this_argument_string;
		std::pair<std::string, ArgumentTuple_t> to_return{"", {0, 0, 0}};
		do {
			instruction_reader >> this_instruction_string;
		} while (instruction_map.find(this_instruction_string) == instruction_map.end() && instruction_reader.good());
		to_return.first = this_instruction_string;
		for (size_t i = 0; i < instruction_map.at(this_instruction_string).second; i = i + 1) {
			instruction_reader >> this_argument_string;
			if (i == 0) std::get<0>(to_return.second) = argumentToInt(this_argument_string);
			else if (i == 1) std::get<1>(to_return.second) = argumentToInt(this_argument_string);
			else if (i == 2) std::get<2>(to_return.second) = argumentToInt(this_argument_string);
		}
		return to_return;
	}

	/**
 * Load instructions from file, then invoke mapped function from System
 * Schedule all task-type instructions into task queue (unsorted)
 * Execute all configuration-type instructions immediately
 */
	void loadInstructions() {

		while (instruction_reader.good()) {
			std::pair<std::string, ArgumentTuple_t> this_instruction;
			SystemFunction_t this_system_function;
			try {
				this_instruction = this->getNextInstruction();
				if (this_instruction.first == "hat") {
					std::cout
							<< "hat "
							<< std::endl;
					return;
				}
				this_system_function = instruction_map.at(this_instruction.first);
				std::invoke(this_system_function.first, system, &this_instruction.second);
			} catch (std::exception &_exep) {
				std::cout << "Warning: Unidentified Instruction" << this_instruction.first;
			}

		}
	};

	/**
	 * Initialize Core and Run Instructions
	 */
	void initCore() {
		if (!instruction_reader.is_open()) throw std::runtime_error("ERR Input File NOT Found.");
		instruction_map["con"] = {&System::setConfig, 3};
		instruction_map["scd"] = {&System::setCacheDimension, 3};
		instruction_map["scl"] = {&System::setCacheLatency, 2};
		instruction_map["sml"] = {&System::setMemoryLatency, 1};
		instruction_map["inc"] = {&System::initCache, 1};
		instruction_map["tre"] = {&System::taskReadAddress, 2};
		instruction_map["twr"] = {&System::taskWriteAddress, 2};
		instruction_map["ins"] = {&System::initSystem, 0};
		instruction_map["pcr"] = {&System::taskPrintCacheRate, 2};
		instruction_map["pci"] = {&System::taskPrintCacheImage, 2};
/*		instruction_map["hat"] = {&System::haltProgram, 1};*/
		this->loadInstructions();
	}

public:

	/**
	 * Bind the File Reader and File Writer to Certain Files to Default
	 */
	explicit Core() {
		this->instruction_reader.open("instructions.txt");
		this->initCore();
	}

	/**
	 * Bind the File Reader and File Writer to Certain Files to Given File
	 * @param _instruction_filename Filename of Input Files (containing instructions)
	 */
	explicit Core(const std::string &_instruction_filename) {
		this->instruction_reader.open(_instruction_filename);
		this->initCore();
	}

	/**
	 * Destructor waits for I/O finishes before thread terminates
	 */
	~Core() {
		this->instruction_reader.close();
	}


};

#endif //CODE_CORE_H
