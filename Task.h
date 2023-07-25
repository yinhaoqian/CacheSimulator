
#ifndef CODE_TASK_H
#define CODE_TASK_H

#include "Include.h"

class Task {
private:
	task_t task_type;
	uint32_t task_value;
	uint64_t arrive_time;
	bool ready;
public:

	Task(const task_t &_task_type, const uint32_t &_task_value, const uint64_t &_arrive_time) {
		this->task_type = _task_type;
		this->task_value = _task_value;
		this->arrive_time = _arrive_time;
		this->ready = true;
	}


	bool operator<(const Task &_task) const {
		if (!ready)
			throw std::runtime_error("ERR Cannot Sort Task - Not Ready");
		auto my_t = this->task_type;
		auto his_t = _task.getTaskType();
		if (this->arrive_time != _task.arrive_time) {
			return this->arrive_time < _task.arrive_time;
		}
		else if ((my_t == task_t::task_readAddress || my_t == task_t::task_writeAddress) &&
			(his_t == task_t::task_reportImage || his_t == task_t::task_reportHitMiss)) {
			return false;
		} else if ((his_t == task_t::task_readAddress || his_t == task_t::task_writeAddress) &&
				   (my_t == task_t::task_reportImage || my_t == task_t::task_reportHitMiss)) {
			return true;
		}else
			return false;
	}

	[[nodiscard]] task_t getTaskType() const {
		return this->task_type;
	}

	[[nodiscard]] uint32_t getTaskValue() const {
		return this->task_value;
	}

	[[nodiscard]] uint64_t getArriveTime() const {
		return this->arrive_time;
	}

};


#endif //CODE_TASK_H
