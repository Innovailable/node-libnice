// include native code

var native_libnice = require("../build/Release/native_libnice");

// turn the stream into an event emitter

function inject(target, source) {
    for (var k in source.prototype) {
        target.prototype[k] = source.prototype[k];
    }
}

inject(native_libnice.NiceStream, require('events').EventEmitter);

// export stuff

exports.NiceAgent = native_libnice.NiceAgent;

