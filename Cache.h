#ifndef CODE_CACHE_H
#define CODE_CACHE_H

#include "DataBlock.h"

class Cache {
private:

	//#0:Pointer of Cache one Unit closer to Memory, nullptr if is bottom cache
	std::unique_ptr<Cache> parent_cache_ptr{nullptr};

	/* #1 Information of the Partition of Address for this Specific Cache
	 *
	 * [Number of Tag Bits][Number Of Index Bits][Number Of Offset Bits]
	 *
	 */
	std::tuple<size_t, size_t, size_t> address_partition{0, 0, 0};

	/* #2 Information of the Dimensions for this Specific Cache
	 *
	 * [Block Size(in Bytes)][Set Associtivity][Number Of Blocks]
	 */
	std::tuple<uint32_t, uint32_t, uint32_t> dimensions{0, 0, 0};

	/* #3 2D Vector cache array containing DataBlock
	 *
	 * To access individual element from cache array, do the following:
	 * this->cache_array.at(A).at(B).getValid() access Cache[A][B].Valid - bool type is returned
	 * this->cache_array.at(A).at(B).getDirty() access Cache[A][B].Dirty - bool type is returned
	 * this->cache_array.at(A).at(B).getTag() access Cache[A][B].Tag - uint32_t type is returned
	 *
	 * A: Index of Blocks, B: Index of Associated Set
	 */
	std::vector<std::vector<DataBlock>> cache_array;

	/* Total Counts of Hits and Misses of this Cache (No need to initialize)
	 *
	 * [Number of Hits][Number of Misses]
	 */
	std::pair<uint64_t, uint64_t> hit_miss_count{0, 0};

	//#4: Number of Clock Cycles to Complete Read for this Cache
	uint64_t latency{0};//#5

	//#5 Layer ID of this Specific Cache with the lowest being 1
	size_t cache_id{0};//#6

	//Status of Initialization. All Members MUST be true before Cache Initialization
	std::array<bool, 6> ready{false, false, false, false, false, false};


	[[nodiscard]] uint32_t addressEncode(const std::tuple<uint32_t, uint32_t, uint32_t> &_add_part) {
		uint32_t address_val = std::get<0>(_add_part);
		address_val = (address_val << std::get<1>(this->address_partition)) + std::get<1>(_add_part);
		address_val = (address_val << std::get<2>(this->address_partition)) + std::get<2>(_add_part);
		return address_val;
	}


public:
	/**
* Decode a 32-bit Address to 3-fields Index-based Tuple indicating where the Data could be
* Warning: This function should ONLY be used inside this Cache Class
* @param address_val Raw 32-bit address
* @return [Tag Value][Index of Index][Index of Offset]
*/
	[[nodiscard]] std::tuple<uint32_t, uint32_t, uint32_t> addressDecode(const uint32_t &address_val) const {
		std::tuple<uint32_t, uint32_t, uint32_t> indices;
		std::string address_bit = std::bitset<32>(address_val).to_string();
		uint32_t bits[3];
		bits[0] = std::get<0>(this->address_partition);
		bits[1] = std::get<1>(this->address_partition);
		bits[2] = std::get<2>(this->address_partition);
		std::get<0>(indices) = std::bitset<32>(address_bit.substr(0, bits[0])).to_ulong();
		std::get<1>(indices) = std::bitset<32>(address_bit.substr(bits[0], bits[1])).to_ulong();
		std::get<2>(indices) = std::bitset<32>(address_bit.substr(bits[1], bits[2])).to_ulong();

/*		std::get<0>(indices) =
				address_val >> (std::get<2>(this->address_partition) + std::get<1>(this->address_partition));
		std::get<1>(indices) =
				address_val << std::get<0>(this->address_partition)
							>> (std::get<2>(this->address_partition) + std::get<0>(this->address_partition));
		std::get<2>(indices) =
				address_val << (std::get<0>(this->address_partition) + std::get<1>(this->address_partition))
							>> (std::get<0>(this->address_partition) + std::get<1>(this->address_partition));*/
		return indices;
	}

	/**
	 * Check if ALL Data Members Are Initialized, including Cache Array
	 * @return True if All Initialized, false if At Least One Member if NOT Initialized
	 */
	explicit operator bool() const {
		bool if_no_false_found = std::find(this->ready.begin(), this->ready.end(), false) == this->ready.end();
		return if_no_false_found;
	}


	/**
	 * Find if there's a tag matching the address.
	 * @param _address Raw 32-bit address
	 * @param _dirty If dirty bit should been set
	 * @return True if found and update, false otherwise
	 */
	bool updateExistingTag(const uint32_t &_address, const uint64_t &_clock_now, const bool &_dirty) {
		std::tuple<uint32_t, uint32_t, uint32_t> this_index_tuple = addressDecode(_address);
		for (DataBlock &this_dataBlock: cache_array.at(std::get<1>(this_index_tuple))) {
			if (this_dataBlock.compareTag(std::get<0>(this_index_tuple))) {
				this_dataBlock.markDirty(_clock_now, _dirty);
				hit_miss_count.first++;
				return true;
			}
		}
		hit_miss_count.second++;
		return false;
	}

	[[nodiscard]] std::pair<bool, uint32_t> popFlushLRUTag(const uint32_t &_address) {
		std::tuple<uint32_t, uint32_t, uint32_t> this_index_tuple = addressDecode(_address);
		DataBlock *least_used_db = &(cache_array.at(std::get<1>(this_index_tuple)).at(0));
		for (DataBlock &this_dataBlock: cache_array.at(std::get<1>(this_index_tuple))) {
			if (this_dataBlock < (*least_used_db))
				least_used_db = &this_dataBlock;
		}
		std::tuple<uint32_t, uint32_t, uint32_t> parted_address
				{least_used_db->getTag(), std::get<1>(this_index_tuple), std::get<2>(this_index_tuple)};
		std::pair<bool, uint32_t> dirty_and_address{least_used_db->getDirty(), addressEncode(parted_address)};
		if (least_used_db->getDirty()) {
			int i = 0;
		}
		least_used_db->flush();
		return dirty_and_address;
	}

	bool allocateNewTag(const uint32_t &_address, const bool &_dirty, const uint64_t &_clock_time) {
		std::tuple<uint32_t, uint32_t, uint32_t> this_index_tuple = addressDecode(_address);
		for (DataBlock &this_dataBlock: cache_array.at(std::get<1>(this_index_tuple))) {
			if (!this_dataBlock.getValid()) {
				this_dataBlock.update(std::get<0>(this_index_tuple), _dirty, _clock_time);
				return true;
			}
		}
		return false;
	}

	/**
	 * Retrieve the Pointer of Parent Cache
	 * @return Pointer of Parent Cache
	 */
	[[nodiscard]] Cache *getParentPtr() const {
		return this->parent_cache_ptr.get();
	}

	void makeAsTopCache() {
		this->parent_cache_ptr = nullptr;
		this->ready.at(0) = true;
	}

	/**
 * Create Parent Cache
 * Mark Parent Cache has been set in Ready
 */
	void makeParent() {
		this->parent_cache_ptr = std::make_unique<Cache>();
		this->parent_cache_ptr->ready.at(0) = true;
	}

	/**
	 * Apply algorithms to find the correct Dimensions, Address Partitions of Cache.
	 * Mark Dimensions and Address Partitions has been set in Ready
	 * Warning: It does NOT initialize the Cache
	 * @param _block_size Number of Bytes that Each DataBlock can hold
	 * @param _total_size Total Number of Bytes this Cache needs to Store
	 * @param _set_assoc Number of DataBlock for a Each Given Index
	 */
	void setParam(const uint32_t &_block_size, const uint32_t &_total_size, const uint32_t &_set_assoc) {
		uint32_t num_of_cache_block = (_total_size / _block_size / _set_assoc);
		std::get<0>(this->dimensions) = _block_size;//Number of bytes each data-block can hold
		std::get<1>(this->dimensions) = _set_assoc;//Number of data-blocks does one index maps to
		std::get<2>(this->dimensions) = num_of_cache_block;//Number of cache-blocks does cache-array have
		this->ready.at(2) = true;
		std::get<2>(this->address_partition) = lround(log2l(_block_size));//Bits of Offset
		std::get<1>(this->address_partition) = lround(log2l(num_of_cache_block));//Bits of Index
		std::get<0>(this->address_partition) =
				32 - std::get<2>(this->address_partition) - std::get<1>(this->address_partition);//Bits of Tag
		this->ready.at(1) = true;
	}

	/**
	 * Set the Latency of this Specific Cache
	 * Mark Latency has been set in Ready
	 * @param _latency Number of Clock Cycles that Reading a Data Requires
	 */
	void setLatency(const uint64_t &_latency) {
		this->latency = _latency;
		this->ready.at(4) = true;
	}

	/**
	 * Get the Latency of this Specific Cache
	 * @return Number of Clock Cycles that Reading a Data Requires
	 */
	[[nodiscard]] uint64_t getLatency() const {
		return this->latency;
	}

	/**
	 * Set the Cache Level ID of this Specific Cache
	 * Mark Cache ID has been set in Ready
	 * @param _cache_id
	 */
	void setId(const size_t &_cache_id) {
		this->cache_id = _cache_id;
		this->ready.at(5) = true;
	}

	/**
	 * Get the ID of this cache
	 * @return the ID of this cache
	 */
	[[nodiscard]] size_t getId() const {
		return this->cache_id;
	}

	/**
	 * Perform Ready Check to See if Requisites are Met for Cache Array Initialization
	 * Initialize Cache Array to Correct Dimensions with Invalid Non-Dirty Zero-Tagged DataBlock
	 */
	void initCacheArray() {
		if (!this->ready.at(2))
			throw std::invalid_argument("ERR inc called before scd");
		DataBlock empty_data_block{std::get<0>(this->dimensions)};
		std::vector<DataBlock> empty_cache_block{std::get<1>(this->dimensions), empty_data_block};
		this->cache_array.resize(std::get<2>(this->dimensions), empty_cache_block);
		this->ready.at(3) = true;
	}


	/**
	 * Report Hit and Misses Count to File.
	 * @param _global_writer_ptr
	 */
	void printHitMissRate(const uint64_t &_arrive_time) {
		std::string hitmiss_name =
				"hmr_l" + std::to_string(this->cache_id) + "_" + std::to_string(_arrive_time) + ".csv";
		std::ofstream hitmiss_writer{hitmiss_name};
		hitmiss_writer << "HITS,MISSES,HIT_R,MISS_R" << std::endl;
		hitmiss_writer << this->hit_miss_count.first << "," << this->hit_miss_count.second << ","
					   << std::to_string(
							   float(hit_miss_count.first) / float(hit_miss_count.second + hit_miss_count.first)) << ","
					   << std::to_string(
							   float(hit_miss_count.second) / float(hit_miss_count.second + hit_miss_count.first))
					   << std::endl;
		hitmiss_writer.close();
	}

	void printCacheImage(const uint64_t &_arrive_time) {
		std::string image_name =
				"img_l" + std::to_string(this->cache_id) + "_" + std::to_string(_arrive_time) + ".csv";
		std::ofstream image_writer{image_name};
		//Print Titles
		image_writer << "B_IND";
		for (size_t col = 0; col < std::get<1>(this->dimensions); col++)
			image_writer << ",VALID[" + std::to_string(col) + "]" +
							",DIRTY[" + std::to_string(col) + "]" +
							",TAG[" + std::to_string(col) + "]" +
							",LRU[" + std::to_string(col) + "]";
		image_writer << std::endl;
		//Print Contents
		for (size_t row = 0; row < std::get<2>(this->dimensions); row++) {
			image_writer << "B[" + std::to_string(row) + "]";
			for (size_t col = 0; col < std::get<1>(this->dimensions); col++) {
				const DataBlock &this_db = this->cache_array.at(row).at(col);
				image_writer << "," + std::to_string(this_db.getValid()) +
								"," + std::to_string(this_db.getDirty()) +
								"," + std::to_string(this_db.getTag()) +
								"," + std::to_string(this_db.getLastUse());
			}
			image_writer << std::endl;
		}
		image_writer.close();
	}


};


#endif //CODE_CACHE_H
