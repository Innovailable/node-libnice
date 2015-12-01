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
				#'DO_DEBUG'
			],
			'cflags': [
				'-std=c++0x',
				'<!@(pkg-config --cflags nice glib-2.0)',
				'-Wall',
				'-g',
			],
			'ldflags': [
				'<!@(pkg-config --libs nice glib-2.0)',
			],
                        'conditions': [
                                [ 'OS=="mac"', {
                                        "xcode_settings": {
                                                "OTHER_CPLUSPLUSFLAGS" : [
                                                        '-std=c++11',
                                                        '-stdlib=libc++',
                                                        '<!@(pkg-config --cflags nice glib-2.0)'
                                                ],
                                                "OTHER_LDFLAGS": [
                                                        '-stdlib=libc++',
                                                        '<!@(pkg-config --libs nice glib-2.0)'
                                                ],
                                                "MACOSX_DEPLOYMENT_TARGET": "10.7",
                                        },
                                }],
                        ],

		}
	],
}
