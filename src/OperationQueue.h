#include <vector>
#include <deque>

class _OperationQueue
{
public:
	_OperationQueue();

	void Init();
	void DrawList();

	void Start();	//Begins running the queue
	void Run();		//The main processor

	void AddOpToQueue(CloutProgram_Op_Datatypes &NewOp);
	void AddProgramToQueue(CloutProgram &Program);

	std::deque <CloutProgram_Op_Datatypes> Ops;	//All of the individual operations
	CloutProgram_Op& GetOp(int idx) { return OpBaseClass(Ops[idx]); };	//Get a reference to the base class of Op[idx]

	bool bIsRunning;
};

extern _OperationQueue OperationQueue;
