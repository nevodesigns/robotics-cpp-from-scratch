// This must not compile.
//
// What the linker sees when a header defines a function without inline and two
// translation units include it: the same entity, defined twice. Here both
// definitions are in one file so the compiler catches it, and the message is
// the same one the linker gives from across two files.
int wheel_speed() { return 1; }
int wheel_speed() { return 1; }

int main() { return wheel_speed(); }
