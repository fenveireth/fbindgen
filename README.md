# fbindgen

A toy replacement for Rust 'bindgen', that also attempts to rewrite simple
macros and inline functions

Uses the clang C++ library from LLVM 12 or above

The classic bindgen sensibly discards those, as they are not part of the target
library's ABI. I wish to be able to call them, as they are part of the target
library's user manual

## Usage

`echo <configuration> | fbindgen <C header file>`

Output Rust code in './out.rs'

Output list of included .h files in './includes.txt' (absolute paths, in no
particular order), for correct regeneration

Configuration is taken on stdin, as any sequence of these lines, in any amount :

- `Pkgconf:<package name>` : Call pkgconf to retrieve include paths. Add linker
	references to the Rust output
- `Functions:<regular expression>` : Create bindings for any exported function
	those name matches the filter
- `Constants:<regular expression>` : Define any constant, or create binding for any global variable
	those name matches the filter. Considers both declared variables and `#define`'s
- `Types:<regular expression>` : Defines any struct, enum or typedef those name
	matches the filter
- `Inlines:<regular expression>` : Converts to Rust any inline function or macro
	(if it is usable in a function-like manner) those name matches the filter
