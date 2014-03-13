#include <node.h>
#include <v8.h>

#include "agent.h"
#include "stream.h"

using namespace v8;

extern "C"
void initAll(Handle<Object> exports) {
	Agent::init(exports);
	Stream::init(exports);
}

NODE_MODULE(native_libnice, initAll)

