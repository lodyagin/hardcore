{
  'targets': [
     {
       'target_name': 'hardcore',
       'type': '<(library)',
       'include_dirs': [
         '../include',
       ],
       'sources': [
         'stack.cpp',
       ],
       'cflags': [
         '-std=c++11',
       ],
     },
  ],
}