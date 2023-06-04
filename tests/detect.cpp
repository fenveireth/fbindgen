#include "../src/stdafx.h"

using namespace std;

extern vector<Str> parse_clang_output(const char* str);

int main()
{
	const char* ref = "clang version 16.0.5\n"
"Target: x86_64-pc-linux-gnu\n"
"Thread model: posix\n"
"InstalledDir: /usr/lib/llvm/16/bin\n"
"Configuration file: /etc/clang/clang.cfg\n"
"System configuration file directory: /etc/clang\n"
"Selected GCC installation: /usr/lib/gcc/x86_64-pc-linux-gnu/13\n"
"Candidate multilib: .;@m64\n"
"Candidate multilib: 32;@m32\n"
"Selected multilib: .;@m64\n"
" (in-process)\n"
" \"/usr/lib/llvm/16/bin/clang-16\" -cc1 -triple x86_64-pc-linux-gnu -E -disable-free -clear-ast-before-backend -disable-llvm-verifier -discard-value-names -main-file-name - -mrelocation-model pic -pic-level 2 -pic-is-pie -mframe-pointer=all -fmath-errno -ffp-contract=on -fno-rounding-math -mconstructor-aliases -funwind-tables=2 -target-cpu x86-64 -tune-cpu generic -debugger-tuning=gdb -v -fcoverage-compilation-dir=/home/fen/dev/fbindgen/tests/parens -resource-dir /usr/lib/llvm/16/bin/../../../../lib/clang/16 -include /usr/include/gentoo/fortify.h -include /usr/include/gentoo/maybe-stddefs.h -internal-isystem /usr/lib/llvm/16/bin/../../../../lib/clang/16/include -internal-isystem /usr/local/include -internal-isystem /usr/lib/gcc/x86_64-pc-linux-gnu/13/../../../../x86_64-pc-linux-gnu/include -internal-externc-isystem /include -internal-externc-isystem /usr/include -fdebug-compilation-dir=/home/fen/dev/fbindgen/tests/parens -ferror-limit 19 -stack-protector 2 -fstack-clash-protection -fgnuc-version=4.2.1 -D__GCC_HAVE_DWARF2_CFI_ASM=1 -o - -x c -\n"
"clang -cc1 version 16.0.5 based upon LLVM 16.0.5+libcxx default target x86_64-pc-linux-gnu\n"
"ignoring nonexistent directory \"/usr/local/include\"\n"
"ignoring nonexistent directory \"/usr/lib/gcc/x86_64-pc-linux-gnu/13/../../../../x86_64-pc-linux-gnu/include\"\n"
"ignoring nonexistent directory \"/include\"\n"
"#include \"...\" search starts here:\n"
"#include <...> search starts here:\n"
" /usr/lib/llvm/16/bin/../../../../lib/clang/16/include\n"
" /usr/include\n"
"End of search list.\n"
"# 1 \"<stdin>\"\n"
"# 1 \"<built-in>\" 1\n"
"# 1 \"<built-in>\" 3\n"
"# 375 \"<built-in>\" 3\n"
"# 1 \"<command line>\" 1\n"
"# 1 \"<built-in>\" 2\n"
"# 1 \"/usr/include/gentoo/fortify.h\" 1\n"
"# 3 \"/usr/include/gentoo/fortify."; // truncated intentionnaly

	auto res = parse_clang_output(ref);
	if (res.size() != 2 
			|| res[0] != "/usr/lib/llvm/16/bin/../../../../lib/clang/16/include"
			|| res[1] != "/usr/include")
		return 1;

	return 0;
}