#include "stream.h"

#include <node_buffer.h>
#include <glib.h>

#include "agent.h"
#include "helper.h"

using namespace v8;

v8::Persistent<v8::Function> Stream::constructor;

// helper

const char* state_to_string(int state_) {
	// to get notified if we miss a state
	const NiceComponentState state = (NiceComponentState) state_;

	switch(state) {
		case NICE_COMPONENT_STATE_DISCONNECTED:
			return "disconnected";
		case NICE_COMPONENT_STATE_GATHERING:
			return "gathering";
		case NICE_COMPONENT_STATE_CONNECTING:
			return "connecting";
		case NICE_COMPONENT_STATE_CONNECTED:
			return "connected";
		case NICE_COMPONENT_STATE_READY:
			return "ready";
		case NICE_COMPONENT_STATE_FAILED:
			return "failed";
		case NICE_COMPONENT_STATE_LAST:
			// not really a state
			break;
	}

	return "unknown";
}

// lifecycle

void Stream::init(v8::Handle<v8::Object> exports) {
	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("NiceStream"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	// protoype
	NODE_SET_PROTOTYPE_METHOD(tpl, "gatherCandidates", gatherCandidates);
	NODE_SET_PROTOTYPE_METHOD(tpl, "setRemoteCredentials", setRemoteCredentials);
	NODE_SET_PROTOTYPE_METHOD(tpl, "getLocalCredentials", getLocalCredentials);
	NODE_SET_PROTOTYPE_METHOD(tpl, "addRemoteIceCandidate", addRemoteIceCandidate);
	NODE_SET_PROTOTYPE_METHOD(tpl, "getLocalIceCandidates", getLocalIceCandidates);
	NODE_SET_PROTOTYPE_METHOD(tpl, "send", send);
	NODE_SET_PROTOTYPE_METHOD(tpl, "setTos", setTos);
	NODE_SET_PROTOTYPE_METHOD(tpl, "close", close);
	constructor = Persistent<Function>::New(tpl->GetFunction());
	// export
	exports->Set(String::NewSymbol("NiceStream"), constructor);
}

Stream::Stream(Handle<Object> js_agent, int stream_id, int components)
	: _js_agent(Persistent<Object>::New(js_agent)), _stream_id(stream_id), _components(components) {
	DEBUG("stream " << stream_id << " with " << components << " components created");
	Agent *agent = node::ObjectWrap::Unwrap<Agent>(js_agent);
	_nice_agent = agent->agent();
}

Stream::~Stream() {
	DEBUG("stream " << _stream_id << " is dying");

	Agent *agent = node::ObjectWrap::Unwrap<Agent>(_js_agent);

	agent->removeStream(_stream_id);

	_js_agent.Dispose();
}

// callback forwarder

void Stream::receive(int component, const char* buf, size_t size) {
	HandleScope scope;

	const int argc = 3;
	Handle<Value> argv[argc] = {
		String::New("receive"),
		Integer::New(component),
		node::Buffer::New(buf, size)->handle_,
	};

	node::MakeCallback(handle_, "emit", argc, argv);
}

void Stream::stateChanged(int component, int state) {
	HandleScope scope;

	if(state == NICE_COMPONENT_STATE_DISCONNECTED || state == NICE_COMPONENT_STATE_FAILED) {
		_working.erase(component);
	}

	checkIndependence();

	const int argc = 3;
	Handle<Value> argv[argc] = {
		String::New("stateChanged"),
		Integer::New(component),
		String::New(state_to_string(state)),
	};

	node::MakeCallback(handle_, "emit", argc, argv);
}

void Stream::gatheringDone() {
	HandleScope scope;

	DEBUG("emitting callback");

	const int argc = 2;
	Handle<Value> argv[argc] = {
		String::New("gatheringDone"),
		getLocalIceCandidates(),
	};

	node::MakeCallback(handle_, "emit", argc, argv);
}

// js functions

v8::Handle<v8::Value> Stream::New(const v8::Arguments& args) {
	HandleScope scope;

	if (args.IsConstructCall()) {
		// Invoked as constructor: `new MyObject(...)`
		Stream* obj = new Stream(args[0]->ToObject(), args[1]->IntegerValue(), args[2]->IntegerValue());
		obj->Wrap(args.This());

		return args.This();
	} else {
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		const int argc = 1;
		Local<Value> argv[1] = { argv[0] };
		return scope.Close(constructor->NewInstance(argc, argv));
	}
}

v8::Handle<v8::Value> Stream::gatherCandidates(const v8::Arguments& args) {
	HandleScope scope;

	Stream *stream = node::ObjectWrap::Unwrap<Stream>(args.This()->ToObject());
	NiceAgent *nice_agent = stream->_nice_agent;
	int stream_id = stream->_stream_id;

	for(int i = 1; i <= stream->_components; ++i) {
		stream->_working.insert(i);
	}

	stream->checkIndependence();

	bool res = nice_agent_gather_candidates(nice_agent, stream_id);

	return scope.Close(Boolean::New(res));
}

v8::Handle<v8::Value> Stream::setRemoteCredentials(const v8::Arguments& args) {
	HandleScope scope;

	Stream *stream = node::ObjectWrap::Unwrap<Stream>(args.This()->ToObject());
	NiceAgent *nice_agent = stream->_nice_agent;
	int stream_id = stream->_stream_id;

	v8::String::Utf8Value ufrag(args[0]->ToString());
	v8::String::Utf8Value pwd(args[1]->ToString());

	DEBUG("set remote credentials on stream " << stream_id << " to " << *ufrag << " " << *pwd);

	bool res = nice_agent_set_remote_credentials(nice_agent, stream_id, *ufrag, *pwd);

	return scope.Close(Boolean::New(res));
}

v8::Handle<v8::Value> Stream::getLocalCredentials(const v8::Arguments& args) {
	HandleScope scope;

	Stream *stream = node::ObjectWrap::Unwrap<Stream>(args.This()->ToObject());
	NiceAgent *nice_agent = stream->_nice_agent;
	int stream_id = stream->_stream_id;

	gchar *ufrag, *pwd;

	nice_agent_get_local_credentials(nice_agent, stream_id, &ufrag, &pwd);

	Local<Object> res = Object::New();
	res->Set(String::New("ufrag"), String::New(ufrag));
	res->Set(String::New("pwd"), String::New(pwd));

	DEBUG("local credentials are '" << ufrag << "' and '" << pwd << "'");

	g_free(ufrag);
	g_free(pwd);

	return scope.Close(res);
}

v8::Handle<v8::Value> Stream::addRemoteIceCandidate(const v8::Arguments& args) {
	HandleScope scope;

	Stream *stream = node::ObjectWrap::Unwrap<Stream>(args.This()->ToObject());

	v8::String::Utf8Value sdp(args[0]->ToString());

	bool res = stream->addRemoteIceCandidate(*sdp);

	return scope.Close(Boolean::New(res));
}

v8::Handle<v8::Value> Stream::getLocalIceCandidates(const v8::Arguments& args) {
	HandleScope scope;

	Stream *stream = node::ObjectWrap::Unwrap<Stream>(args.This()->ToObject());

	Handle<Value> res = stream->getLocalIceCandidates();

	return scope.Close(res);
}

v8::Handle<v8::Value> Stream::send(const v8::Arguments& args) {
	HandleScope scope;

	Stream *stream = node::ObjectWrap::Unwrap<Stream>(args.This()->ToObject());
	NiceAgent *nice_agent = stream->_nice_agent;
	int stream_id = stream->_stream_id;

	int component = args[0]->IntegerValue();

	if(!node::Buffer::HasInstance(args[1])) {
		return ThrowException(Exception::TypeError(String::New("Expected buffer")));
	}

	Local<Object> buffer = args[1]->ToObject();

	size_t size = node::Buffer::Length(buffer);
	char* buf = node::Buffer::Data(buffer);

	DEBUG("sending " << size << " bytes");

	int ret = nice_agent_send(nice_agent, stream_id, component, size, buf);

	return scope.Close(Integer::New(ret));
}

v8::Handle<v8::Value> Stream::setTos(const v8::Arguments& args) {
	HandleScope scope;

	Stream *stream = node::ObjectWrap::Unwrap<Stream>(args.This()->ToObject());
	NiceAgent *nice_agent = stream->_nice_agent;
	int stream_id = stream->_stream_id;

	int tos = args[0]->IntegerValue();

	nice_agent_set_stream_tos(nice_agent, stream_id, tos);

	return scope.Close(Undefined());
}

v8::Handle<v8::Value> Stream::close(const v8::Arguments& args) {
	HandleScope scope;

	Stream *stream = node::ObjectWrap::Unwrap<Stream>(args.This()->ToObject());
	Agent *agent = node::ObjectWrap::Unwrap<Agent>(stream->_js_agent);
	int stream_id = stream->_stream_id;

	bool res = agent->removeStream(stream_id);

	stream->_working.clear();
	stream->checkIndependence();

	DEBUG("closing stream " << stream->_stream_id);

	return scope.Close(Boolean::New(res));
}

// helpers

v8::Handle<v8::Value> Stream::getLocalIceCandidates() {
	HandleScope scope;

	Local<Array> res = Array::New();
	Local<Function> push = Local<Function>::Cast(res->Get(String::New("push")));

	for(int i = 1; i <= _components; ++i) {
		GSList* root = nice_agent_get_local_candidates(_nice_agent, _stream_id, i);

		for(GSList* it = root; it; it = g_slist_next(it)) {
			auto candidate = reinterpret_cast<NiceCandidate*>(it->data);

			auto str = nice_agent_generate_local_candidate_sdp(_nice_agent, candidate);

			const int argc = 1;
			Local<Value> argv[argc] = {
				String::New(str),
			};
			push->Call(res, argc, argv);

			g_free(str);
		}

		g_slist_free(root);

	}

	return scope.Close(res);
}

bool Stream::addRemoteIceCandidate(const char* sdp_) {
	// sanitize

	std::string sdp(sdp_);

	DEBUG("adding candidate '" << trim(sdp) << "' to stream " << _stream_id);

	size_t small_udp = sdp.find(" udp ");

	if(small_udp != std::string::npos) {
		sdp.replace(small_udp, 5, " UDP ");
	}

	// parse sdp

	auto nice_candidate = nice_agent_parse_remote_candidate_sdp(_nice_agent, _stream_id, sdp.c_str());

	if(nice_candidate == NULL) {
		DEBUG("was unable to parse the candidate");
		return false;
	}

	if(nice_candidate->component_id > _components) {
		DEBUG("component was invalid");
		return false;
	}

	// insert into canidate list

	GSList *candidates = nice_agent_get_remote_candidates(_nice_agent, _stream_id, nice_candidate->component_id);

	candidates = g_slist_append(candidates, nice_candidate);

	nice_agent_set_remote_candidates(_nice_agent, _stream_id, nice_candidate->component_id, candidates);

	g_slist_free(candidates);

	return true;
}

void Stream::checkIndependence() {
	if(_working.size() > 0) {
		if(_self.IsEmpty()) {
			DEBUG("stream " << _stream_id << " got independent because it has work to do");
			_self = Persistent<Object>::New(handle_);
		}
	} else {
		if(!_self.IsEmpty()) {
			DEBUG("stream " << _stream_id << " lost its independence");
			_self.Dispose();
			_self = Persistent<Object>();
		}
	}
}

