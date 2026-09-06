// This must not compile.
//
// The same rule for data. A namespace-scope variable in a header, without
// inline and without static, is a definition in every file that includes it.
double wheel_base = 0.35;
double wheel_base = 0.35;

int main() { return wheel_base > 0.0 ? 0 : 1; }
