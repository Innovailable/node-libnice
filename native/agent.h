#ifndef AGENT_H
#define AGENT_H 

#include <map>
#include <deque>
#include <mutex>
#include <thread>

#include <glib.h>
#include <nice/nice.h>
#include <node.h>
#include <uv.h>

#include "stream.h"

typedef std::map<int,Stream*> stream_map;

typedef std::function<void(void)> work_fun;
typedef std::deque<work_fun> work_queue;

class Agent : public node::ObjectWrap {
	public:
		Agent();
		~Agent();

		static void init(v8::Handle<v8::Object> exports);

		NiceAgent* agent() { return _agent; }

		bool removeStream(int stream_id);

	private:
		// js functions

		static v8::Handle<v8::Value> New(const v8::Arguments& args);
		static v8::Handle<v8::Value> createStream(const v8::Arguments& args);
		static v8::Handle<v8::Value> setStunServer(const v8::Arguments& args);
		static v8::Handle<v8::Value> setSoftware(const v8::Arguments& args);
		static v8::Handle<v8::Value> setControlling(const v8::Arguments& args);
		static v8::Handle<v8::Value> restart(const v8::Arguments& args);

		static v8::Persistent<v8::Function> constructor;

		// callbacks

		static void gatheringDone(NiceAgent *agent, guint stream_id, gpointer user_data);
		static void stateChanged(NiceAgent *agent, guint stream_id, guint component_id, guint state, gpointer user_data);
		static void receive(NiceAgent* agent, guint stream_id, guint component_id, guint len, gchar* buf, gpointer user_data);

		// worker callback

		static void doWork(uv_async_t *async, int status);
		void addWork(const work_fun& fun);

		// handle

		NiceAgent *_agent;

		// streams for callbacks

		stream_map _streams;

		// own main loop

		std::thread _thread;
		GMainLoop *_loop;

		// passing work around

		std::mutex _work_mutex;
		work_queue _work_queue;
		uv_async_t _async;
};

#endif /* AGENT_H */
