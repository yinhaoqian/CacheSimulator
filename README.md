# Cache Simulator - C++
In this project, the goal is to design and build a highly parameterized and configurable cache system. The cache will be able to handle a given input stream of cache accesses efficiently. The main deliverables of the project include the finish time of each read access, the hit and miss rate of each cache layer, and a cache status image displaying the state of all layers.
## Functional API
`con [cache_count] [block_size] [policy_num]`

- Configure System Parameters
- Set Global Configurational Parameters
- Develop Cache List Containing [cache_count] Empty Caches as Doubly-Linked List 
- Usually the First Instruction

**Parameters**
_- [cache_count] Number of Cache Layers in this System (Must be At Least 1)
- [block_size] Number of Bytes that Each DataBlock can hold
- [policy_num] Policy Number this System should Implement (Must be Either 0=[Write-Back&Write-Allocate] or 1=[Write-Thru&Non-Write-Allocate])
  _
**Requirements**
- Must be called ONLY Once.

`scd [cache_level] [total_size] [set_assoc]`

- Set Cache Dimensions
- Set Cache Size and Set Associtivity
- Calculate the Corrct Dimensions for Cache and Field Partitions for Address
  
**Parameters**
- [cache_level] The level(index) of cache with lowest being 1
- [total_size] Total Number of Bytes this Cache needs to Store
- [set_assoc] Number of DataBlock for a Each Given Index
  
**Requirements**
- Must be called BEFORE inc
- Must be called AFTER con

`scl [cache_level] [latency]      ` 

- Set Cache Latency
- Set Cache Latency
  
**Parameters**
- [cache_level] The level(index) of cache with lowest being 1
- [latency] Number of Clock Cycles to Complete Read for this Cache
  
**Requirements**
- Must be called BEFORE inc
- Must be called AFTER con

`sml [latency]                    `   

- Set Memory Latency
- Set Memory Latency
- Mark Memory Latency as Ready
- Warning: This Function does NOT Set Memory Latency in Cache Array
  
**Parameters**
- [latency]	Number of Clock Cycles to Complete Read from Memory
  
**Requirements**
- Must be called BEFORE inc

`inc [cache_level]         `

- Initialize Cache
- Initialize Cache
- Perform Bound Checks for Cache Level
  
**Parameters**
- [cache_level]	The level(index) of cache with lowest being 1
  
**Requirements**
- Must be called AFTER con

`tre [address] [arrive_time]    `      

- Task Read
- Task Read Address at Time
  
**Parameters**
- [address] Raw 32-bit Address to be Read
- [arrive_time] 	Clock Cycle at when This Specific Task is Scheduled
  
**No Requirements**

`twr [address] [arrive_time]      `    

- Task Write
- Task Write Address at Time
  
**Parameters**
- [address]	Raw 32-bit Address to be Written
- [arrive_time] Clock Cycle at when This Specific Task is Schedule
  
**No Requirements**

`pcr [cache_level] [arrive_time]          `     

- Print Cache Hit/Miss Counts and Rates to Report File at Time
- Perform Bound Checks for Cache Level
  
**Parameters**
- [cache_level]	The level(index) of cache with lowest being 1
  
**No Requirements**

`pci [cache_level] [arrive_time]`

- Print Cache Image
- Print the Content of Cache to Report File at Time
- Perform Bound Checks for Cache Level
  
**Parameters**
- [cache_level]	The level(index) of cache with lowest being 1
  
**No Requirements**

`ins`              

- Initialize System
- Run All Tasks in Queue (Read, Write, Print)
  
**No Parameters**

**Requirements**

- Must be called AFTER inc
- All tre, twr, pcr,and pci need to be schedule BEFORE
- All tre, twr, pcr,and pci called AFTER are ignored

`hat`

- Halt Program
- Wait Tasks Queue to be Cleared, then Exit
- Usually the Last Instruction
  
**No Parameters**

**Requirements**

Any instructions called AFTER are ignored
