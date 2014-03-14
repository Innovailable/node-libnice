{
	"targets": [
		{
			"target_name": "native_libnice",
			"sources": [
				"native/module.cpp",
				"native/agent.cpp",
				"native/stream.cpp",
				"native/helper.cpp",
			],
			"defines": [
				'DO_DEBUG'
			],
			'cflags': [
				'-std=c++11',
				'<!@(pkg-config --cflags nice glib-2.0)',
				'-Wall',
				'-g',
			],
			'ldflags': [
				'<!@(pkg-config --libs nice glib-2.0)',
			],
		}
	],
}
