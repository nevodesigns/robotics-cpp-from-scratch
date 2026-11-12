#include <rc/test/rc_test.hpp>

#include <cstdint>
#include <iostream>

#include "solution.hpp"

namespace {

// A manifest text with a correct end line, for building the bad ones by hand.
std::string with_end(const std::string& body) { return body + "end " + hex16(hash64(body)) + "\n"; }

std::vector<FileData> release() {
  return {{"bin/robot_node", std::string(1200, 'x') + "node"}, {"etc/robot.conf", "speed = 1.5\n"}, {"lib/libmsgs.so", "msgs"}};
}

const char* kind_name(Mismatch k) { return k == Mismatch::missing ? "missing" : (k == Mismatch::extra ? "extra" : (k == Mismatch::size ? "size" : "hash")); }

}  // namespace

RC_TEST("the hash is FNV-1a, which has published values") {
  RC_CHECK(hash64("") == 0xcbf29ce484222325ULL);
  RC_CHECK(hash64("a") == 0xaf63dc4c8601ec8cULL);
  RC_CHECK(hash64("foobar") == 0x85944171f73967e8ULL);
  RC_CHECK(hash64("ab") != hash64("ba"));
  RC_CHECK(hex16(0xcbf29ce484222325ULL) == "cbf29ce484222325");
  RC_CHECK(hex16(255) == "00000000000000ff");
}

RC_TEST("a path is safe to unpack only if it is relative, plain, and stays inside the directory") {
  const struct { const char* p; bool ok; } rows[] = {
      {"bin/robot_node", true}, {"etc/robot.conf", true}, {"a", true}, {"lib/libmsgs.so.1.2", true}, {"../etc/passwd", false}, {"bin/../../x", false}, {"/etc/passwd", false}, {"bin//x", false}, {"./bin/x", false},
      {"bin/", false}, {"", false}, {"C:/x", false}, {"bin\\x", false}, {"has space/x", false}, {"..", false}};
  std::cout << "  paths accepted: ";
  int accepted = 0;
  for (const auto& r : rows) {
    RC_CHECK(safe_path(r.p) == r.ok);
    if (safe_path(r.p)) ++accepted;
  }
  std::cout << accepted << " of " << sizeof(rows) / sizeof(rows[0]) << "\n";
  RC_CHECK(!safe_path(std::string(256, 'a')));
  RC_CHECK(safe_path(std::string(255, 'a')));
  RC_CHECK(!safe_path(std::string("a\0b", 3)));
}

RC_TEST("a manifest is the same whatever order the files come in, and reads back as it was made") {
  std::vector<FileData> a = release();
  std::vector<FileData> b = {a[2], a[0], a[1]};
  const MakeResult ma = make_manifest("1.4.2", a);
  const MakeResult mb = make_manifest("1.4.2", b);
  std::cout << ma.text << "\n";
  RC_CHECK(ma.ok && mb.ok);
  RC_CHECK(ma.text == mb.text);
  RC_CHECK(ma.text.rfind("rcbundle 1\nversion 1.4.2\nfile bin/robot_node 1204 ", 0) == 0);
  const ParseResult p = parse_manifest(ma.text);
  RC_CHECK(p.ok);
  RC_CHECK(p.manifest.version == "1.4.2");
  RC_CHECK(p.manifest.entries.size() == 3);
  if (p.manifest.entries.size() != 3) return;
  RC_CHECK(p.manifest.entries[0].path == "bin/robot_node" && p.manifest.entries[0].size == 1204);
  RC_CHECK(p.manifest.entries[0].hash == hash64(a[0].content));
  RC_CHECK(p.manifest.entries[2].path == "lib/libmsgs.so");
  RC_CHECK(make_manifest("1.0", {}).ok);                                           // an empty release is a manifest with no files
  RC_CHECK(parse_manifest(make_manifest("1.0", {}).text).ok);
  RC_CHECK(make_manifest("", a).error == "bad version");
  RC_CHECK(make_manifest("1 4", a).error == "bad version");
  RC_CHECK(make_manifest("1.0", {{"../x", "1"}}).error == "unsafe path: ../x");
  RC_CHECK(make_manifest("1.0", {{"a", "1"}, {"a", "2"}}).error == "duplicate path: a");
  RC_CHECK(!make_manifest("1.0", {{"../x", "1"}}).ok);
  RC_CHECK(make_manifest("1.0", {{"../x", "1"}}).text.empty());
}

RC_TEST("a truncated manifest, an edited one and a malformed one are each refused with their reason") {
  const std::string good = make_manifest("1.4.2", release()).text;
  const std::size_t last = good.rfind("end ");
  std::cout << "  truncated before the end line: " << parse_manifest(good.substr(0, last)).error << "\n";
  RC_CHECK(parse_manifest(good.substr(0, last)).error == "no end line");
  RC_CHECK(parse_manifest(good.substr(0, good.size() - 5)).error == "no end line");           // cut in the middle of the end line
  RC_CHECK(parse_manifest(good.substr(0, 30)).error == "no end line");
  std::string edited = good;
  edited.replace(edited.find("1204"), 4, "1205");
  std::cout << "  one digit of a size changed: " << parse_manifest(edited).error << "\n";
  RC_CHECK(parse_manifest(edited).error == "bad checksum");
  std::string version_edit = good;
  version_edit.replace(version_edit.find("1.4.2"), 5, "1.4.3");
  RC_CHECK(parse_manifest(version_edit).error == "bad checksum");
  RC_CHECK(parse_manifest("").error == "bad header");
  RC_CHECK(parse_manifest("rcbundle 2\n").error == "bad header");
  RC_CHECK(parse_manifest("rcbundle 1\nversion\n").error == "bad version line");
  RC_CHECK(parse_manifest("rcbundle 1\nversion  \n").error == "bad version line");
  RC_CHECK(parse_manifest(with_end("rcbundle 1\nversion 1\nfile a 1\n")).error == "bad file line 3");
  RC_CHECK(parse_manifest(with_end("rcbundle 1\nversion 1\nfile a x 0123456789abcdef\n")).error == "bad file line 3");
  RC_CHECK(parse_manifest(with_end("rcbundle 1\nversion 1\nfile a 1 0123456789ABCDEF\n")).error == "bad file line 3");
  RC_CHECK(parse_manifest(with_end("rcbundle 1\nversion 1\nfile ../a 1 0123456789abcdef\n")).error == "unsafe path: ../a");
  RC_CHECK(parse_manifest(with_end("rcbundle 1\nversion 1\nfile a 1 0123456789abcdef\nfile a 2 0123456789abcdef\n")).error == "duplicate path: a");
  RC_CHECK(parse_manifest(with_end("rcbundle 1\nversion 1\nfile a 1 0123456789abcdef\n") + "file b 1 0123456789abcdef\n").error == "no end line");   // lines after the end
  RC_CHECK(parse_manifest(with_end("rcbundle 1\nversion 1\nfile a 1 0123456789abcdef\n")).ok);
  std::cout << "  ten more malformed manifests, each with its own reason\n\n";
}

RC_TEST("the files on the robot are checked against the manifest, and each way they can differ is named") {
  const std::vector<FileData> files = release();
  const Manifest m = parse_manifest(make_manifest("1.4.2", files).text).manifest;
  RC_CHECK(verify_files(m, files).empty());
  std::vector<FileData> on_robot = files;
  on_robot[0].content[10] = 'y';                                                   // one byte altered: same size
  on_robot[1].content = "speed = 1.5";                                            // truncated by a byte
  on_robot.erase(on_robot.begin() + 2);                                            // never arrived
  on_robot.push_back({"bin/old_tool", "left over from release 1.3"});              // left behind by an earlier release
  const std::vector<Difference> d = verify_files(m, on_robot);
  std::cout << "  on the robot:";
  for (const Difference& x : d) std::cout << "  " << x.path << " " << kind_name(x.kind);
  std::cout << "\n";
  RC_CHECK(d.size() == 4);
  if (d.size() != 4) return;
  RC_CHECK(d[0].path == "bin/robot_node" && d[0].kind == Mismatch::hash);
  RC_CHECK(d[1].path == "etc/robot.conf" && d[1].kind == Mismatch::size);
  RC_CHECK(d[2].path == "lib/libmsgs.so" && d[2].kind == Mismatch::missing);
  RC_CHECK(d[3].path == "bin/old_tool" && d[3].kind == Mismatch::extra);
  std::vector<FileData> swapped = files;
  std::swap(swapped[1].content, swapped[2].content);
  RC_CHECK(verify_files(m, swapped).size() == 2);                                  // two files with each other's contents
  const std::vector<FileData> twice = {files[0], files[0], files[1], files[2]};
  RC_CHECK(verify_files(m, twice).empty());                                        // a path given twice: the first counts
}

RC_TEST("every one of a thousand single bit flips in a 4096 byte file is caught") {
  std::string big(4096, '\0');
  std::uint64_t s = 88172645463325252ULL;
  for (char& c : big) {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    c = static_cast<char>(s & 0xFF);
  }
  const Manifest m = parse_manifest(make_manifest("1", {{"blob", big}}).text).manifest;
  int caught = 0;
  for (int i = 0; i < 1000; ++i) {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    std::string flipped = big;
    flipped[s % 4096] ^= static_cast<char>(1u << ((s >> 20) % 8));
    if (!verify_files(m, {{"blob", flipped}}).empty()) ++caught;
  }
  std::cout << "  1000 single bit flips in 4096 bytes: " << caught << " caught\n\n";
  RC_CHECK(caught == 1000);
}
