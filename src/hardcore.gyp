{
  'targets': [
     {
       'target_name': 'hardcore',
       'type': 'static_library',
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