#ifndef CODE_DATABLOCK_H
#define CODE_DATABLOCK_H

#include "Include.h"

class DataBlock {
private:
	bool valid{false};
	bool dirty{false};
	uint32_t tag{0};
	uint32_t block_size{0};
	uint64_t last_use{0};
public:

	/**
	 * Explicit constructor that receives desired Block Size.
	 * @param block_size System-wide how many bytes should a DataBlock hold
	 */
	explicit DataBlock(const uint32_t &block_size = 0) {
		this->block_size = block_size;
	}

	/**
	 * Copy valid,tag,and block_size from received Datablock, and record current time
	 * @param _data_block Received DataBlock to copy from
	 * @return
	 */
	DataBlock &operator=(const DataBlock &_data_block) {
		this->valid = _data_block.getValid();
		this->dirty = _data_block.getDirty();
		if (_data_block.getDirty() == true) {
			int i = 0;
		}
		this->tag = _data_block.getTag();
		this->block_size = _data_block.block_size;
		this->last_use = _data_block.last_use;
		return *this;
	}

	bool operator<(const DataBlock &_data_block) const {
		return this->last_use < _data_block.last_use;
	}

	/**
	 * Flush the Datablock to make it available to write
	 */
	void flush() {
		this->valid = false;
		this->tag = 0;
	}

	/**
	 * Update this DataBlock with new tag value, make it valid, and record current time
	 * @param _tag New tag value to be assigned to this DataBlock
	 */
	void update(const uint32_t &_tag, const bool &_dirty, const uint64_t &_clock_time) {
		if (this->valid != 0)
			std::runtime_error("ERR Datablock Cannot Update before Flushing");
		this->valid = true;
		this->tag = _tag;
		this->last_use = _clock_time;
		this->dirty = _dirty;
	}

	/**
	 * Compare if this DataBlock's tag matches a given
	 * @param _tag Tag value to be compared to
	 * @return True if tag matches; False otherwise
	 */
	[[nodiscard]] bool compareTag(const uint32_t &_tag) const {
		return (this->tag == _tag && this->valid);
	}

	/**
	 * Mark this DataBlock Dirty
	 * Meaning this DataBlock needs to sync with parents
	 */
	void markDirty(const uint64_t &_clock_time, const bool &_ifDirty) {
		this->last_use = _clock_time;
		this->dirty = _ifDirty;
	}

	/**
	 * Test if this Datablock needs to sync with parents (For Printing Purpose)
	 * @return True if dirty, false otherwise
	 */
	[[nodiscard]] bool getDirty() const {
		return this->dirty;
	}

	/**
	 * Test if this Datablock has been used at least once (For Printing Purpose)
	 * @return True if valid, false otherwise
	 */
	[[nodiscard]] bool getValid() const {
		return this->valid;
	}

	/**
	 * Retrieve the tag of this DataBlock (For Printing Purpose)
	 * @return Tag retrieved
	 */
	[[nodiscard]] uint32_t getTag() const {
		return this->tag;
	}

	[[nodiscard]] uint64_t getLastUse() const {
		return this->last_use;
	}

};

#endif //CODE_DATABLOCK_H
