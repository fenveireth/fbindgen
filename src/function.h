
class Fun
{
public:
	uint is_inline : 1;
	Str name;
	Str decl;
	Str type;

	Fun(const clang::FunctionDecl* d);

private:
	void rd_proto(const clang::FunctionDecl* d);
	void rd_body(const clang::FunctionDecl* d);
};

