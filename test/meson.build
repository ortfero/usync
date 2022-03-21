threads = dependency('threads')

usync_test = executable('usync-test', 'test.cpp',
    dependencies: [usync, threads])

test('all', usync_test)

