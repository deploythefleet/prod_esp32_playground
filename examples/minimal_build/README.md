# Minimal Build

In the root CMakeLists.txt file of an IDF project you can set the following property:

`idf_build_set_property(MINIMAL_BUILD ON)`

This drastically reduces the build time by not compiling modules you don't use. The trade off 
is that you have to manually include any components your project depends on in the 
application-level CMakeLists.txt file.

This is a better practice because it forces you to explicitly understand all of your dependencies.