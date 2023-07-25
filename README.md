# Cache Simulator - C++
In this project, the goal is to design and build a highly parameterized and configurable cache system. The cache will be able to handle a given input stream of cache accesses efficiently. The main deliverables of the project include the finish time of each read access, the hit and miss rate of each cache layer, and a cache status image displaying the state of all layers.
## Functional API
`con [cache_count] [block_size] [policy_num]`

- Configure System Parameters
- Set Global Configurational Parameters
- Develop Cache List Containing [cache_count] Empty Caches as Doubly-Linked List 
- Usually the First Instruction

**Parameters**
- [cache_count] Number of Cache Layers in this System (Must be At Least 1)
- [block_size] Number of Bytes that Each DataBlock can hold
- [policy_num] Policy Number this System should Implement (Must be Either 0=[Write-Back&Write-Allocate] or 1=[Write-Thru&Non-Write-Allocate])
  
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
## Example
1. In the file _script.txt_ we write the scripts to instruct the simulator:
```
con	$2	$1	$2	//global_configuration
scd	$1	$256	$4	//cache_dimension
scl	$1	$1	 	//cache_latency
inc	$1	 	 	//initialize_cache
scd	$2	$2048	$8	//cache_dimension
scl	$2	$50	 	//cache_latency
inc	$2	 	 	//initialize_cache
sml	$100	 	 	//memory_latency
tre	$104566	$1	 	//r[T@408][L2@1][L1@54][x]
pci	$2	$1	 	
pcr	$2	$1	 	
pci	$1	$1	 	
pcr	$1	$1	 	
tre	$104822	$2	 	//r[T@409][L2@1][L1@54][x]
pci	$2	$2	 	
pcr	$2	$2	 	
pci	$1	$2	 	
pcr	$1	$2	 	
tre	$105078	$3	 	//r[T@410][L2@1][L1@54][x]
pci	$2	$3	 	
pcr	$2	$3	 	
pci	$1	$3	 	
pcr	$1	$3	 	
tre	$105334	$4	 	//r[T@411][L2@1][L1@54][x]
pci	$2	$4	 	
pcr	$2	$4	 	
pci	$1	$4	 	
pcr	$1	$4	 	
tre	$105590	$5	 	//r[T@412][L2@1][L1@54][x]
...MORE INSTRUCTIONS
hat				//halt_program
ins				//initialize_system
```
2. Compile the simulator and run with argument pointing to the script text file:
`g++ -o simulator main.cpp`
3. Run the simulator with argument pointing to the script text file:
`simulator script.txt`
3. View the generated log report:
```
1→L1::READ({1633(11001100001):54(110110):0(0)}=104566){
	↓[1←C_R_MISS$GENERAL]
	1→L2::READ({408(110011000):118(1110110):0(0)}=104566){
		↓[1←C_R_MISS$GENERAL]
		1→MEM::READ({0(0):0(0):0(0)}=104566){
			↓[101←M_R_SUCCESS]
		}101←M_R_SUCCESS
		↓[101←C_R_MISS$ALLOC_SUCCESS]
	}151←C_R_MISS$ALLOC_SUCCESS
	↓[151←C_R_MISS$ALLOC_SUCCESS]
}152←C_R_MISS$ALLOC_SUCCESS

152→L1::READ({1637(11001100101):54(110110):0(0)}=104822){
	↓[152←C_R_MISS$GENERAL]
	152→L2::READ({409(110011001):118(1110110):0(0)}=104822){
		↓[152←C_R_MISS$GENERAL]
		152→MEM::READ({0(0):0(0):0(0)}=104822){
			↓[252←M_R_SUCCESS]
		}252←M_R_SUCCESS
		↓[252←C_R_MISS$ALLOC_SUCCESS]
	}302←C_R_MISS$ALLOC_SUCCESS
	↓[302←C_R_MISS$ALLOC_SUCCESS]
}303←C_R_MISS$ALLOC_SUCCESS

303→L1::READ({1641(11001101001):54(110110):0(0)}=105078){
	↓[303←C_R_MISS$GENERAL]
	303→L2::READ({410(110011010):118(1110110):0(0)}=105078){
		↓[303←C_R_MISS$GENERAL]
		303→MEM::READ({0(0):0(0):0(0)}=105078){
			↓[403←M_R_SUCCESS]
		}403←M_R_SUCCESS
		↓[403←C_R_MISS$ALLOC_SUCCESS]
	}453←C_R_MISS$ALLOC_SUCCESS
	↓[453←C_R_MISS$ALLOC_SUCCESS]
}454←C_R_MISS$ALLOC_SUCCESS
.....MORE 
```

