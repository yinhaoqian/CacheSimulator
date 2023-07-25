#ifndef CODE_SYSTEM_H
#define CODE_SYSTEM_H

#include "Include.h"

#include "Cache.h"
#include "Task.h"

class System {

private:
	//Number of Clock Cycles the System has Passed (No need to initialize)
	uint64_t clock_count{0};

	/* #0: Policy Number this System should Implement
	 * false : Write-Back + Write-Allocate
	 *
	 * true : Write-Thru + Non-Write-Allocate
	 */
	bool read_write_policy{POLICY_WBWA};

	//#1: Number of Cache Layers in this System
	size_t cache_count{0};

	//#2: Number of Bytes each DataBlock should hold (Each cache has a copy of this)
	uint32_t block_size{0};

	//#3: Pointer of Top Cache (Second-top layer cache\memory has a copy of this)
	std::unique_ptr<Cache> top_cache_ptr;

	//#4: Total Clock Cycles Needed to Complete (Each cache has a copy of this)
	uint64_t memory_latency{0};//#4

	/* #5 Pointer of File Writer
	 *
	 * use (*global_writer_ptr).operator<<() to write
	 * e.g. (*global_writer_ptr)<<("THINGS TO WRITE")<<std::endl;
	 */
	std::pair<std::ofstream, uint32_t> report_writer;

	/* #6 Container holding tasks need to be done
	 * Ready if Sorted.
	 */
	std::vector<Task> task_queue;

	// Status of Initialization. All Members MUST be true before System Initialization
	std::array<bool, 7> ready{false, false, false, false, false, false, false};

	/**
	 * Retrieve the Pointer of Cache of Specific Level
	 * @param _cache_level Cache Level of Cache Wanted (Top Cache being 1)
	 * @return The Retrieved Pointer of Cache
	 */
	[[nodiscard]] Cache *getCacheAtPtr(const uint32_t &_cache_level) {
		size_t _cache_index = _cache_level - 1;
		if (_cache_index > this->cache_count)
			throw std::out_of_range("ERR Cache Level Out-of-range");
		if (top_cache_ptr == nullptr)
			throw std::runtime_error("ERR Top Cache Ptr is Null");
		Cache *this_cache_ptr = top_cache_ptr.get();
		for (size_t i = 0; i < _cache_index; i++) {
			this_cache_ptr = this_cache_ptr->getParentPtr();
		}
		return this_cache_ptr;
	}

	/**
	 * Sort the Task Queue by Arriving Time
	 * Mark Task Queue as Ready
	 */
	void sortTaskQueue() {
		std::sort(task_queue.begin(), task_queue.end());
		this->ready.at(6) = true;
	}

	[[nodiscard]] uint64_t readCache(Cache *_cache, const uint32_t &_address, const uint64_t &_clock_when_called) {
		reportCall(_clock_when_called, _cache, "READ", _address);
		std::string status{""};
		uint64_t elapsed_clock{_clock_when_called};//elapsed clock cycles default is 0
		if (_cache == nullptr) {//If this is called by digging into Memory (bottom)
			elapsed_clock += memory_latency;//Tag is pseudo found and add memory latency to elapsed clock
			status = "M_R_SUCCESS";
			reportReturn(elapsed_clock, _cache, "READ", _address, status, false);
		} else {//If this is called by digging into Next Parental Cache (one level below)
			if (_cache->updateExistingTag(_address, elapsed_clock,
										  false)) {//if there's a tag match from a set -- READ HIT
				status = "C_R_HIT";
				reportReturn(elapsed_clock, _cache, "READ", _address, status, false);
			} else {//if there's NO tag match from a set -- READ MISS
				reportReturn(elapsed_clock, _cache, "READ", _address, "C_R_MISS$GENERAL", false);
				elapsed_clock = readCache(_cache->getParentPtr(), _address,
										  elapsed_clock);//sum latencies of parents to read
				if (!_cache->allocateNewTag(_address, false, elapsed_clock)) {//if allocation failed (full)
					auto poped_db = _cache->popFlushLRUTag(_address);//pop LRU tag and flush LRU field
					if (poped_db.first) {//if the poped LRU tag is dirty, sync the address with parental cache (write)
						status = "C_R_MISS$ALLOC_FAILED$POP_DIRTY";
						reportReturn(elapsed_clock, _cache, "READ", _address, status, false);
						elapsed_clock = writeCache(_cache->getParentPtr(), poped_db.second, elapsed_clock);//Write prt
					} else {//if the popped LRU tag is non-dirty, discard the poped tag
						status = "C_R_MISS$ALLOC_FAILED$POP_CLEAN";
						reportReturn(elapsed_clock, _cache, "READ", _address, status, false);
					}
					if (!_cache->allocateNewTag(_address, false, elapsed_clock))//try to alloc again after pop
						throw std::runtime_error("ERR Alloc after Popping failed");
				} else {//if allocation suceeded without popping
					status = "C_R_MISS$ALLOC_SUCCESS";
					reportReturn(elapsed_clock, _cache, "READ", _address, status, false);
				}
			}
			elapsed_clock += _cache->getLatency();//takes this cache's latency to read
		}
		reportReturn(elapsed_clock, _cache, "READ", _address, status, true);
		return elapsed_clock;
	}

	[[nodiscard]] uint64_t writeCache(Cache *_cache, const uint32_t &_address, const uint64_t &_clock_when_called) {
		reportCall(_clock_when_called, _cache, "WRITE", _address);
		std::string status{""};
		uint64_t elapsed_clock{_clock_when_called};//elapsed clock cycles default is 0
		if (_cache == nullptr) {//If this is called by digging into Memory (bottom)
			status = "M_W_SUCCESS";
			reportReturn(elapsed_clock, _cache, "WRITE", _address, status, false);
			elapsed_clock += memory_latency;//Tag is pseudo written and add memory latency to elapsed clock
		} else {//If this is called by digging into Next Parental Cache (one level below)
			if (read_write_policy == POLICY_WBWA) {//if the policy is write-back and write-allocate
				elapsed_clock += _cache->getLatency();//takes this cache's latency to write
				if (_cache->updateExistingTag(_address, elapsed_clock,
											  true)) {//if there's a tag match, then set dirty -- WRITE HIT
					status = "C_R_HIT$MARKED_DIRTY$WB";
					reportReturn(elapsed_clock, _cache, "WRITE", _address, status, false);
				} else {//if there's NO tag match from a set to set dirty-- WRITE MISS
//TO-DO HERE: Should there be a read from parent cache?
					if (!_cache->allocateNewTag(_address, true, elapsed_clock)) {//while allocation failed
						auto poped_db = _cache->popFlushLRUTag(_address);//pop LRU tag and flush LRU field
						if (poped_db.first) {//if the poped LRU tag is dirty, write the address with parental cache
							status = "C_W_MISS$ALLOC_FAILED$POP_DIRTY$WB";
							reportReturn(elapsed_clock, _cache, "WRITE", _address, status, false);
							elapsed_clock = writeCache(_cache->getParentPtr(), poped_db.second, elapsed_clock);//W Prt
						} else {
							status = "C_W_MISS$ALLOC_FAILED$POP_CLEAN$WB";
							reportReturn(elapsed_clock, _cache, "WRITE", _address, status, false);
						}
						if (!_cache->allocateNewTag(_address, true, elapsed_clock))//try to alloc agn after pop
							throw std::runtime_error("ERR Alloc after Popping failed");
					} else {
						status = "C_W_MISS$ALLOC_SUCCESS$WB";
						reportReturn(elapsed_clock, _cache, "WRITE", _address, status, false);
					}
				}
			} else if (read_write_policy == POLICY_WTNWA) {//if the policy is write-thru and non-write allocate
				if (_cache->updateExistingTag(_address, elapsed_clock,
											  false)) {//if there's a tag match, no need dirty-- WRITE HIT
					status = "C_W_HIT$WT";
					reportReturn(elapsed_clock, _cache, "WRITE", _address, status, false);
					elapsed_clock += _cache->getLatency();//takes this cache's latency to write
				} else {//if there's NO tag match from a set - WRITE MISS
					status = "C_W_MISS$PROPAGATE$WT";
					reportReturn(elapsed_clock, _cache, "WRITE", _address, status, false);
					elapsed_clock = writeCache(_cache->getParentPtr(), _address, elapsed_clock);//just write in parent.
				}
			}
		}
		reportReturn(elapsed_clock, _cache, "WRITE", _address, status, true);
		return elapsed_clock;
	}

	void reportReturn(const uint64_t &_time, Cache *_cache, const std::string &_oper, const uint32_t &_address,
					  const std::string &_status, const bool &_go_back) {
		std::string cache = _cache == nullptr ? "MEM" : ("L" + std::to_string(_cache->getId()));
		if (report_writer.second == 0)
			throw std::runtime_error("ERR Tab Count Less than 0");
		if (_go_back)
			report_writer.second--;
		std::string front_dashes;
		for (size_t i = 0; i < this->report_writer.second; i++) front_dashes += "\t";
		report_writer.first
				<< front_dashes;
		if (_go_back)
			report_writer.first
					<< "}";
		else
			report_writer.first
					<< "↓[";
		report_writer.first
				<< _time
				<< "←"
				/*				<< cache << "::"
								<< _oper << "("
								<< _address << ")->"*/
				<< _status << (_go_back ? "" : "]") << std::endl;

	}

	void reportCall(const uint64_t &_time, Cache *_cache, const std::string &_oper, const uint32_t &_address) {
		std::string cache = _cache == nullptr ? "MEM" : ("L" + std::to_string(_cache->getId()));
		auto decoded_address =
				_cache == nullptr ? std::tuple<uint32_t, uint32_t, uint32_t>{0, 0, 0} : _cache->addressDecode(_address);
		std::string front_dashes;
		for (size_t i = 0; i < this->report_writer.second; i++) front_dashes += "\t";
		std::string first_bin, second_bin, third_bin;
		first_bin = std::bitset<32>(std::get<0>(decoded_address)).to_string();
		second_bin = std::bitset<32>(std::get<1>(decoded_address)).to_string();
		third_bin = std::bitset<32>(std::get<2>(decoded_address)).to_string();
		third_bin.erase(0, std::min(third_bin.find_first_not_of('0'), third_bin.size() - 1));
		report_writer.first
				<< front_dashes
				<< _time << "→"
				<< cache << "::"
				<< _oper << "({";
		report_writer.first
				<< std::get<0>(decoded_address) << "(";
		first_bin.erase(0, std::min(first_bin.find_first_not_of('0'), first_bin.size() - 1));
		report_writer.first
				<< first_bin << "):";
		report_writer.first
				<< std::get<1>(decoded_address) << "(";
		second_bin.erase(0, std::min(second_bin.find_first_not_of('0'), second_bin.size() - 1));
		report_writer.first
				<< second_bin << "):";
		report_writer.first
				<< std::get<2>(decoded_address) << "(";
		third_bin.erase(0, std::min(third_bin.find_first_not_of('0'), third_bin.size() - 1));
		report_writer.first
				<< third_bin << ")}="
				<< _address << "){"
				<< std::endl;
		report_writer.second++;
	}


public:

/**
 * Check if ALL Data Members Are Initialized
 * @return True if All Initialized, false if At Least One Member if NOT Initialized
 */
	explicit operator bool() {
		bool if_no_false_found = std::find(this->ready.begin(), this->ready.end(), false) == this->ready.end();
		return if_no_false_found;
	}

/**
 * con	[cache_count]	[block_size]	[policy_num]	-
 * Set Configurations
 * Perform Format Check for Policy Number
 * Mark Cache Count, Block Size, Policy Number as Ready
 * Warning: Policy number MUST be EITHER 1 OR 2
 * @param _cache_count Number of Cache Layers in this System
 * @param _block_size Number of Bytes that Each DataBlock can hold
 * @param _policy_num Policy Number this System should Implement
 * @return True if Instruction Ran without Errors, false otherwise
 */
	bool setConfig(std::tuple<uint32_t, uint32_t, uint32_t> *_arguments) {
		if (_arguments == nullptr)
			throw std::runtime_error("ERR Argument Tuple is NULL");
		uint32_t _cache_count = std::get<0>(*_arguments);
		uint32_t _block_size = std::get<1>(*_arguments);
		uint32_t _policy_num = std::get<2>(*_arguments);
		if (ready.at(0) || ready.at(1) || ready.at(2) || ready.at(3))
			throw std::invalid_argument("ERR con called twice");
		if (_policy_num < 1 || _policy_num > 2)
			throw std::runtime_error("ERR Policy Number Unrecognized");
		else if (_policy_num == 1)
			this->read_write_policy = POLICY_WBWA;
		else if (_policy_num == 2)
			this->read_write_policy = POLICY_WTNWA;
		this->ready.at(0) = true;
		this->cache_count = _cache_count;
		if (_cache_count < 1)
			throw std::runtime_error("ERR At Least 1 Cache is Needed");
		this->ready.at(1) = true;
		this->block_size = _block_size;
		this->ready.at(2) = true;
		this->top_cache_ptr = std::make_unique<Cache>();//create childest cache
		Cache *this_cache_ptr = top_cache_ptr.get();//cache pointer iterator
		if (this_cache_ptr->operator bool())
			throw std::invalid_argument("ERR con Called after inc");
		this_cache_ptr->setId(1);
		if (cache_count == 1) this_cache_ptr->makeAsTopCache();
		for (size_t i = 2; i <= _cache_count; i++) {
			this_cache_ptr->makeParent();
			this_cache_ptr = this_cache_ptr->getParentPtr();
			this_cache_ptr->setId(i);
		}
		this->ready.at(3) = true;
		this->report_writer.first.open("log_system.lgs");
		this->ready.at(5) = true;
		std::cout
				<< "con "
				<< std::setw(10) << std::left << _cache_count
				<< std::setw(10) << std::left << _block_size
				<< std::setw(10) << std::left << _policy_num
				<< std::endl;
		return true;
	}

/**
 * scd	[cache_number]	[total_size]	[set_assoc]		-
 * Set Cache Size and Set Assoc
 * Perform Bound Checks for Cache Level
 * Warning: Function will Mark Ready in Cache, not System
 * @param _cache_level The level(index) of cache with lowest being 1
 * @param _total_size Total Number of Bytes this Cache needs to Store
 * @param _set_assoc Number of DataBlock for a Each Given Index
 * @return True if Instruction Ran without Errors, false otherwise
 */
	bool setCacheDimension(std::tuple<uint32_t, uint32_t, uint32_t> *_arguments) {
		if (_arguments == nullptr)
			throw std::runtime_error("ERR Argument Tuple is NULL");
		if (!this->ready.at(3))
			throw std::invalid_argument("ERR scd Called Before con");
		uint32_t _cache_level = std::get<0>(*_arguments);
		uint32_t _total_size = std::get<1>(*_arguments);
		uint32_t _set_assoc = std::get<2>(*_arguments);
		if (_cache_level > this->cache_count) return false;
		if (block_size == 0) return false;
		Cache *this_cache_ptr = getCacheAtPtr(_cache_level);
		if (this_cache_ptr->operator bool())
			throw std::invalid_argument("ERR scd called after inc");
		this_cache_ptr->setParam(block_size, _total_size, _set_assoc);
		std::cout
				<< "scd "
				<< std::setw(10) << std::left << _cache_level
				<< std::setw(10) << std::left << _total_size
				<< std::setw(10) << std::left << _set_assoc
				<< std::endl;
		return true;
	}


/**
 * scl	[cache_number]	[latency]						-
 * Set Cache Latency
 * Perform Bound Checks for Cache Level
 * Warning: Function will Mark Ready in Cache, not System
 * @param _cache_level The level(index) of cache with lowest being 1
 * @param _latency Number of Clock Cycles to Complete Read for this Cache
 * @return True if Instruction Ran without Errors, false otherwise
 */
	bool setCacheLatency(std::tuple<uint32_t, uint32_t, uint32_t> *_arguments) {
		if (_arguments == nullptr)
			throw std::runtime_error("ERR Argument Tuple is NULL");
		uint32_t _cache_level = std::get<0>(*_arguments);
		uint32_t _latency = std::get<1>(*_arguments);
		if (_cache_level > this->cache_count) return false;
		Cache *this_cache = this->getCacheAtPtr(_cache_level);
		if (this_cache->operator bool())
			throw std::invalid_argument("ERR scl called after inc");
		this_cache->setLatency(_latency);
		std::cout
				<< "scl "
				<< std::setw(10) << std::left << _cache_level
				<< std::setw(10) << std::left << _latency
				<< std::endl;
		return true;
	}

/**
 * sml	[latency]										-
 * Set Memory Latency
 * Mark Memory Latency as Ready
 * Warning: This Function does NOT Set Memory Latency in Cache Array
 * @param _latency
 * @return True if Instruction Ran without Errors, false otherwise
 */
	bool setMemoryLatency(std::tuple<uint32_t, uint32_t, uint32_t> *_arguments) {
		if (_arguments == nullptr)
			throw std::runtime_error("ERR Argument Tuple is NULL");
		if (!this->ready.at(3))
			throw std::invalid_argument("ERR sml called after inc");
		uint32_t _latency = std::get<0>(*_arguments);
		this->memory_latency = _latency;
		this->ready.at(4) = true;
		std::cout
				<< "sml "
				<< std::setw(10) << std::left << _latency
				<< std::endl;
		return true;
	}

/**
 * inc	[cache_number]									-
 * Initialize Cache
 * Perform Bound Checks for Cache Level
 * @param _cache_level The level(index) of cache with lowest being 1
 * @return True if Instruction Ran without Errors, false otherwise
 */
	bool initCache(std::tuple<uint32_t, uint32_t, uint32_t> *_arguments) {
		if (_arguments == nullptr)
			throw std::runtime_error("ERR Argument Tuple is NULL");
/*		if (!this->ready.at(4))
			throw std::invalid_argument("ERR inc called before sml");*/
		uint32_t _cache_level = std::get<0>(*_arguments);
		if (_cache_level > this->cache_count) return false;
		Cache *this_cache_ptr = this->getCacheAtPtr(_cache_level);
		this_cache_ptr->initCacheArray();
		std::cout
				<< "inc "
				<< std::setw(10) << std::left << _cache_level
				<< std::endl;
		return true;
	}

/**
 * tre	[address]		[arr_time]						-
 * Task Read Address at Time
 * @param _address Raw 32-bit Address to be Read
 * @param _arrive_time	Clock Cycle at when This Specific Task is Scheduled
 * @return True if Instruction Ran without Errors, false otherwise
 */
	bool taskReadAddress(std::tuple<uint32_t, uint32_t, uint32_t> *_arguments) {
		if (_arguments == nullptr)
			throw std::runtime_error("ERR Argument Tuple is NULL");
		if (this->operator bool())
			throw std::invalid_argument("ERR Cannot Task Once System is Initialized");
		uint32_t _address = std::get<0>(*_arguments);
		uint32_t _arrive_time = std::get<1>(*_arguments);
		this->task_queue.emplace_back(task_t::task_readAddress, _address, _arrive_time);
		std::cout
				<< "tre "
				<< std::setw(10) << std::left << _address
				<< std::setw(10) << std::left << _arrive_time
				<< std::endl;
		return true;
	}

/**
 * twr	[address]		[arr_time]						-
 * Task Write Address at Time
 * @param _address Raw 32-bit Address to be Written
 * @param _arrive_time Clock Cycle at when This Specific Task is Scheduled
 * @return True if Instruction Ran without Errors, false otherwise
 */
	bool taskWriteAddress(std::tuple<uint32_t, uint32_t, uint32_t> *_arguments) {
		if (_arguments == nullptr)
			throw std::runtime_error("ERR Argument Tuple is NULL");
		if (this->operator bool())
			throw std::invalid_argument("ERR Cannot Task Once System is Initialized");
		uint32_t _address = std::get<0>(*_arguments);
		uint32_t _arrive_time = std::get<1>(*_arguments);
		this->task_queue.emplace_back(task_t::task_writeAddress, _address, _arrive_time);
		std::cout
				<< "twr "
				<< std::setw(10) << std::left << _address
				<< std::setw(10) << std::left << _arrive_time
				<< std::endl;
		return true;
	}

/**
 * ins													-
 * Initialize System
 * @return True if Instruction Ran without Errors, false otherwise
 */
	bool initSystem(std::tuple<uint32_t, uint32_t, uint32_t> *_arguments) {
		if (_arguments == nullptr)
			throw std::runtime_error("ERR Argument Tuple is NULL");
		sortTaskQueue();
		this->ready.at(6) = true;
		if (!this->operator bool())
			throw std::runtime_error("ERR System Cannot Initialize - System Not Ready");
		runTaskQueue();
		std::cout
				<< "ins "
				<< std::endl;
		return true;
	}

/**
 * pcr [cache_number]									-
 * Print Cache Hit/Miss Rate
 * Perform Bound Checks for Cache Level
 * @param _cache_level The level(index) of cache with lowest being 1
 * @return True if Instruction Ran without Errors, false otherwise
 */
	bool taskPrintCacheRate(std::tuple<uint32_t, uint32_t, uint32_t> *_arguments) {
		if (_arguments == nullptr)
			throw std::runtime_error("ERR Argument Tuple is NULL");
		if (this->operator bool())
			throw std::invalid_argument("ERR Cannot Report Once System is Initialized");
		uint32_t _cache_level = std::get<0>(*_arguments);
		uint32_t _arrive_time = std::get<1>(*_arguments);
		if (_cache_level > this->cache_count) return false;
		this->task_queue.emplace_back(task_t::task_reportHitMiss, _cache_level, _arrive_time);
		std::cout
				<< "pcr "
				<< std::setw(10) << std::left << _cache_level
				<< std::setw(10) << std::left << _arrive_time
				<< std::endl;
		return true;
	}

/**
 * pci	[cache_number]									-
 * Print Cache Image
 * Perform Bound Checks for Cache Level
 * @param _cache_level The level(index) of cache with lowest being 1
 * @return True if Instruction Ran without Errors, false otherwise
 */
	bool taskPrintCacheImage(std::tuple<uint32_t, uint32_t, uint32_t> *_arguments) {
		if (_arguments == nullptr)
			throw std::runtime_error("ERR Argument Tuple is NULL");
		if (this->operator bool())
			throw std::invalid_argument("ERR Cannot Report Once System is Initialized");
		uint32_t _cache_level = std::get<0>(*_arguments);
		uint32_t _arrive_time = std::get<1>(*_arguments);
		if (_cache_level > this->cache_count) return false;
		this->task_queue.emplace_back(task_t::task_reportImage, _cache_level, _arrive_time);
		std::cout
				<< "pci "
				<< std::setw(10) << std::left << _cache_level
				<< std::setw(10) << std::left << _arrive_time
				<< std::endl;
		return true;
	}

/**
 * hat
 * Stop Fetching Instruction
 * @return ALWAYS TRUE
 *//*
	bool haltProgram(std::tuple<uint32_t, uint32_t, uint32_t> *_arguments) {
		if (_arguments == nullptr)
			throw std::runtime_error("ERR Argument Tuple is NULL");
		uint32_t _arrive_time = std::get<0>(*_arguments);
		task_queue.emplace_back(task_t::task_halt, 0, _arrive_time);
		return false;
	}*/

	void runTaskQueue() {
		auto current_task_ptr = this->task_queue.begin();
		while (current_task_ptr != this->task_queue.end()) {
			if (current_task_ptr->getArriveTime() <= clock_count) {
				task_t this_task = current_task_ptr->getTaskType();
				uint32_t this_value = current_task_ptr->getTaskValue();
				uint32_t this_arrive_time = current_task_ptr->getArriveTime();
				if (this_task == task_t::task_halt) {
					report_writer.first.close();
					return;
				} else if (this_task == task_t::task_reportHitMiss)
					this->getCacheAtPtr(this_value)->printHitMissRate(this_arrive_time);
				else if (this_task == task_t::task_reportImage)
					this->getCacheAtPtr(this_value)->printCacheImage(this_arrive_time);
				else if (this_task == task_t::task_readAddress) {
					clock_count = this->readCache(this->top_cache_ptr.get(), this_value, clock_count);
					report_writer.first << std::endl;
				} else if (this_task == task_t::task_writeAddress) {
					clock_count = this->writeCache(this->top_cache_ptr.get(), this_value, clock_count);
					report_writer.first << std::endl;
				}
				current_task_ptr++;
			} else clock_count++;
		}
		report_writer.first.close();
	}

};

#endif //CODE_SYSTEM_H
