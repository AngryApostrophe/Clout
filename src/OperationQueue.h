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

	void AddOpToQueue(CloutScript_Op_Datatypes &NewOp);
	void AddScriptToQueue(CloutScript &Script);

	std::deque <CloutScript_Op_Datatypes> Ops;	//All of the individual operations
	CloutScript_Op& GetOp(int idx) { return OpBaseClass(Ops[idx]); };	//Get a reference to the base class of Op[idx]

	bool bIsRunning;
};

extern _OperationQueue OperationQueue;
