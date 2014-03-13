# node-libnice

## What is this?

This is a very thin node.js wrapper around
[libnice](http://nice.freedesktop.org/).

## Setup

First you have to make sure that you have libnice and its headers installed. On
Debian/Ubuntu/... run

	apt-get install libnice-dev

Then install this library with

	npm install libnice

## Usage

First you have to create an agent which can create streams

	var agent = new require("libnice").NiceAgent("176.28.9.212");
	var stream = agent.createStream();

Streams are `EventEmitter`s. Register your callbacks

	stream.on('stateChanged', function(candidates) {
		// state is a string
	}

	stream.on('gatheringDone', function(candidates) {
	    // candidates is an array of sdp strings
	    var credentials = stream.getLocalCredentials();
	    // send credentials and candidates to remote client
	});

	stream.on('receive', function(data) {
	    // data is a buffer
	});

Do not forget to tell the stream to start gather its candidates!

	stream.gatherCandidates();

Now you have to exchange the credentials and the ice candidates with the other
client

	stream.setRemoteCredentials(ufrag, pwd);
	stream.addRemoteCandidate(candidate1);
	stream.addRemoteCandidate(candidate2);

When everything is set up and the clients are able yo connect to each other you
should arrive in the state `ready`. Now you can send and receive data

	stream.send(data)

Full API documentation will be added shortly. Please also consult the [libnice
documentation](http://nice.freedesktop.org/libnice/index.html) for detailed
information.

## TODOs

* more documentation
* resolve host for stun servers
* port missing libnice functions

