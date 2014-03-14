# node-libnice

## What is this?

This is a thin node.js wrapper around [libnice](http://nice.freedesktop.org/).

## Setup

First you have to make sure that you have libnice and its headers installed. On
Debian/Ubuntu/... run

	apt-get install libnice-dev

Then install this library with

	npm install libnice

## Usage

First you have to create an agent which can create streams

	var agent = new require("libnice").NiceAgent("132.177.123.6");

An important concept in libnice are components. A stream can consist of
multiple components which are something like channels. Each component can send
and receive data. To create a stream with one component call

	var stream = agent.createStream(1);

Streams are `EventEmitter`s. Register your callbacks

	stream.on('gatheringDone', function(candidates) {
	    // candidates is an array of sdp strings
	    var credentials = stream.getLocalCredentials();
	    // send credentials and candidates to remote client
	});

	stream.on('stateChanged', function(component, state) {
		// state is a string
	}

	stream.on('receive', function(component, data) {
	    // data is a buffer
	});

Please note that the first parameter on `stateChanged` and `receive` is the
component id. Component ids are numbers starting from 1.

To start the connection process tell the stream to gather candidates

	stream.gatherCandidates();

Now you have to exchange the credentials and ice candidates with the other
client

	stream.setRemoteCredentials(ufrag, pwd);
	stream.addRemoteCandidate(candidate1);
	stream.addRemoteCandidate(candidate2);

When everything is set up and the clients are able to connect to each other you
should arrive in the state `ready`. Now you can now send and receive data

	stream.send(component, data)

Full API documentation will be added shortly. Please also consult the [libnice
documentation](http://nice.freedesktop.org/libnice/index.html) for detailed
information.

## TODOs

* more documentation
* resolve host for stun servers
* port missing libnice functions
* get rid of the glib loop in its own thread

