#include <vector>

class _OperationQueue
{
public:
	_OperationQueue();

	void Init();
	void DrawList();

	void Start();		//Begins running the queue
	void Run();		//The main processor

	void AddOpToQueue(CloutProgram_Op &NewOp);
	void AddProgramToQueue(CloutProgram &Program);

	std::vector <CloutProgram_Op> Ops;	//All of the individual operations

	bool bIsRunning;
};

extern _OperationQueue OperationQueue;
