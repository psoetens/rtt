#include "OperationCallerInterface.hpp"
#include "../internal/GlobalEngine.hpp"

using namespace RTT;
using namespace base;
using namespace internal;

OperationCallerInterface::OperationCallerInterface()
    : myengine(0), caller(0), met(ClientThread)
{}

OperationCallerInterface::OperationCallerInterface(OperationCallerInterface const& orig)
    : myengine(orig.myengine), caller(orig.caller),  met(orig.met)
{}

OperationCallerInterface::~OperationCallerInterface()
{
}

void OperationCallerInterface::setOwner(ExecutionEngine* ee) {
    myengine = ee;
}

void OperationCallerInterface::setCaller(ExecutionEngine* ee) {
    caller = ee;
}

bool OperationCallerInterface::setThread(ExecutionThread et, ExecutionEngine* executor) {
    met = et;
    setOwner(executor);
    return true;
}

ExecutionEngine* OperationCallerInterface::getMessageProcessor() const 
{ 
    ExecutionEngine* ret = (met == OwnThread ? myengine : GlobalEngine::Instance()); 
    if (ret == 0 )
        return GlobalEngine::Instance(); // best-effort for Operations not tied to an EE
    return ret;
}


// report an error if an exception was thrown while calling exec()
void OperationCallerInterface::reportError() {
    // This localOperation was added to a TaskContext or to a Service owned by a TaskContext
    if (this->myengine != 0)
        this->myengine->setExceptionTask();
}

bool OperationCallerInterface::isSend() {
    // ClientThread operations are never sent
    if (met == ClientThread) return false;

    // If we do not know on which engine will process the message. we better send it.
    if (myengine == 0) return true;

    // If the operation was called by ourselves, call it directly.
    if (myengine == caller) return false;

    // For example in case of master/slave activities, multiple engines can share one thread.
    // In this case, even if the processor and caller engine is not the same, we still call if this thread is the same
    // than the processor thread.
    if (myengine != 0 && myengine->getThread() != 0 && myengine->getThread()->isSelf()) return false;

    // otherwise, send it
    return true;
}
