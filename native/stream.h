#ifndef STREAM_H
#define STREAM_H 

#include <node.h>
#include <v8.h>
#include <nice/nice.h>

class Stream : public node::ObjectWrap {
	public:
		Stream(v8::Handle<v8::Object> js_agent, int stream_id, int components);
		~Stream();

		static void init(v8::Handle<v8::Object> exports);

		void receive(int component, const char* buf, size_t size);
		void stateChanged(int component, int state);
		void gatheringDone();

		static v8::Persistent<v8::Function> constructor;

	private:
		// js functions

		static v8::Handle<v8::Value> New(const v8::Arguments& args);
		static v8::Handle<v8::Value> gatherCandidates(const v8::Arguments& args);
		static v8::Handle<v8::Value> setRemoteCredentials(const v8::Arguments& args);
		static v8::Handle<v8::Value> getLocalCredentials(const v8::Arguments& args);
		static v8::Handle<v8::Value> addRemoteIceCandidate(const v8::Arguments& args);
		static v8::Handle<v8::Value> getLocalIceCandidates(const v8::Arguments& args);
		static v8::Handle<v8::Value> send(const v8::Arguments& args);
		static v8::Handle<v8::Value> setTos(const v8::Arguments& args);
		static v8::Handle<v8::Value> close(const v8::Arguments& args);

		// maybe implement later ...

		/*
		static v8::Handle<v8::Value> getRemoteIceCandidates(const v8::Arguments& args);
		static v8::Handle<v8::Value> setRemoteIceCandidates(const v8::Arguments& args);
		static v8::Handle<v8::Value> getSelectedPair(const v8::Arguments& args);
		static v8::Handle<v8::Value> setRelayInfo(const v8::Arguments& args);
		static v8::Handle<v8::Value> setPortRange(const v8::Arguments& args);
		*/

		// helper

		v8::Handle<v8::Value> getLocalIceCandidates();
		bool addRemoteIceCandidate(const char* sdp);

		// the agent

		v8::Persistent<v8::Object> _js_agent;
		NiceAgent *_nice_agent;

		// id of stream in agent

		int _stream_id;
		int _components;
};

#endif /* STREAM_H */
