#include "agent.h"

#include <string.h>

#include "helper.h"

using namespace v8;

v8::Persistent<v8::Function> Agent::constructor;

// lifecycle stuff

void Agent::init(v8::Handle<v8::Object> exports) {
	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("NiceAgent"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	// protoype
	NODE_SET_PROTOTYPE_METHOD(tpl, "createStream", createStream);
	NODE_SET_PROTOTYPE_METHOD(tpl, "setStunServer", setStunServer);
	NODE_SET_PROTOTYPE_METHOD(tpl, "setSoftware", setSoftware);
	NODE_SET_PROTOTYPE_METHOD(tpl, "resetart", restart);
	constructor = Persistent<Function>::New(tpl->GetFunction());
	// export
	exports->Set(String::NewSymbol("NiceAgent"), constructor);
}

Agent::Agent() {
	DEBUG("agent created");

	//nice_debug_enable(true);

	// initialize async worker

	uv_async_init(uv_default_loop(), &_async, doWork);
	_async.data = this;

	// create glib stuff and agent

	auto context = g_main_context_new();
	_loop = g_main_loop_new(context, FALSE);
	_agent = nice_agent_new(context, NICE_COMPATIBILITY_RFC5245);

	// register callbacks

	g_signal_connect(G_OBJECT(_agent), "candidate-gathering-done", G_CALLBACK(gatheringDone), this);
	g_signal_connect(G_OBJECT(_agent), "component-state-changed", G_CALLBACK(stateChanged), this);

	// this creates a new thread, secure your v8 calls!

	_thread = std::thread([=]() {
		g_main_loop_run(_loop);

		g_main_loop_unref(_loop);
		g_main_context_unref(context);

		_loop = NULL;
	});
}

Agent::~Agent() {
	DEBUG("agent is dying");

	uv_close((uv_handle_t*) &_async, NULL);

	g_object_unref(_agent);
	_agent = NULL;

	g_main_loop_quit(_loop);
	_thread.join();

	DEBUG("agent loop finished");
}

bool Agent::removeStream(int stream_id) {
	nice_agent_remove_stream(_agent, stream_id);
	return _streams.erase(stream_id) > 0;
}

// do js work in right thread

void Agent::addWork(const work_fun& fun) {
	std::lock_guard<std::mutex> guard(_work_mutex);

	_work_queue.push_back(fun);

	uv_async_send(&_async);
}

void Agent::doWork(uv_async_t *async, int status) {
	Agent *agent = (Agent*) async->data;

	std::lock_guard<std::mutex> guard(agent->_work_mutex);

	DEBUG("doing work");

	while(agent->_work_queue.size()) {
		agent->_work_queue.front()();
		agent->_work_queue.pop_front();
	}
}

// js functions

v8::Handle<v8::Value> Agent::New(const v8::Arguments& args) {
	HandleScope scope;

	if (args.IsConstructCall()) {
		// Invoked as constructor: `new MyObject(...)`
		Agent* obj = new Agent();
		obj->Wrap(args.This());

		if(!args[0]->IsUndefined()) {
			setStunServer(args);
		}

		return args.This();
	} else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		const int argc = 1;
		Local<Value> argv[1] = { argv[0] };
		return scope.Close(constructor->NewInstance(argc, argv));
	}
}

v8::Handle<v8::Value> Agent::createStream(const v8::Arguments& args) {
	HandleScope scope;

	// get native handles ...

	Agent *agent = node::ObjectWrap::Unwrap<Agent>(args.This()->ToObject());
	NiceAgent *nice_agent = agent->agent();

	// create nice stream

	int components = args[0]->IsUndefined() ? 1 : args[0]->IntegerValue();

	int stream_id = nice_agent_add_stream(nice_agent, components);

	// create stream object

	const int argc = 3;
	Local<Value> argv[argc] = {
		args.This(),
		Integer::New(stream_id),
		Integer::New(components)
	};
	Local<Object> stream = Stream::constructor->NewInstance(argc, argv);

	// register receive callback

	auto context = g_main_loop_get_context(agent->_loop);

	for(int i = 1; i <= components; ++i) {
		nice_agent_attach_recv(nice_agent, stream_id, i, context, receive, agent);
	}

	// save stream for callback handling

	Stream *obj = node::ObjectWrap::Unwrap<Stream>(stream);
	agent->_streams[stream_id] = obj;

	return scope.Close(stream);
}

v8::Handle<v8::Value> Agent::setStunServer(const v8::Arguments& args) {
	HandleScope scope;

	NiceAgent *nice_agent = node::ObjectWrap::Unwrap<Agent>(args.This()->ToObject())->agent();

	// set server address

	v8::String::Utf8Value address(args[0]->ToString());

	GValue g_addr = G_VALUE_INIT;
	g_value_init(&g_addr, G_TYPE_STRING);
	g_value_set_string(&g_addr, *address);
	g_object_set_property(G_OBJECT(nice_agent), "stun-server", &g_addr);

	// set port if given

	if(!args[1]->IsUndefined()) {
		GValue g_port = G_VALUE_INIT;
		g_value_init(&g_port, G_TYPE_UINT);
		g_value_set_uint(&g_port, args[1]->IntegerValue());
		g_object_set_property(G_OBJECT(nice_agent), "stun-server-port", &g_port);
	}

	return scope.Close(Undefined());
}

v8::Handle<v8::Value> Agent::restart(const v8::Arguments& args) {
	HandleScope scope;

	NiceAgent *nice_agent = node::ObjectWrap::Unwrap<Agent>(args.This()->ToObject())->agent();

	bool res = nice_agent_restart(nice_agent);

	return scope.Close(Boolean::New(res));
}

v8::Handle<v8::Value> Agent::setSoftware(const v8::Arguments& args) {
	HandleScope scope;

	NiceAgent *nice_agent = node::ObjectWrap::Unwrap<Agent>(args.This()->ToObject())->agent();

	v8::String::Utf8Value software(args[0]->ToString());

	nice_agent_set_software(nice_agent, *software);

	return scope.Close(Undefined());
}

// callbacks

void Agent::gatheringDone(NiceAgent *nice_agent, guint stream_id, gpointer user_data) {
	Agent *agent = reinterpret_cast<Agent*>(user_data);

	DEBUG("gathering done on stream " << stream_id);

	agent->addWork([=]() {
		auto it = agent->_streams.find(stream_id);

		if(it != agent->_streams.end()) {
			it->second->gatheringDone();
		} else {
			DEBUG("gathering done on unknown stream");
		}
	});
}

void Agent::stateChanged(NiceAgent *nice_agent, guint stream_id, guint component_id, guint state, gpointer user_data) {
	Agent *agent = reinterpret_cast<Agent*>(user_data);

	DEBUG("state changed to " << state << " on component " << component_id << " of stream " << stream_id);

	agent->addWork([=]() {
		auto it = agent->_streams.find(stream_id);

		if(it != agent->_streams.end()) {
			it->second->stateChanged(component_id, state);
		} else {
			DEBUG("state changed on unknown stream");
		}
	});
}

void Agent::receive(NiceAgent* nice_agent, guint stream_id, guint component_id, guint len, gchar* buf, gpointer user_data) {
	Agent *agent = reinterpret_cast<Agent*>(user_data);

	DEBUG("receiving " << len << " bytes on component " << component_id << " of stream " << stream_id);

	// TODO: this might not be the best solution ...
	char *tmp_buf = (char*) malloc(len);
	memcpy(tmp_buf, buf, len);

	agent->addWork([=]() {
		auto it = agent->_streams.find(stream_id);

		if(it != agent->_streams.end()) {
			it->second->receive(component_id, buf, len);
		} else {
			DEBUG("receiving on unknown stream");
		}

		free(tmp_buf);
	});
}

