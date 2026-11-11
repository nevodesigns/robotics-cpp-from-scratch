#define RC_TEST_NO_MAIN
#include <rc/test/rc_test.hpp>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <iostream>
#include <string>

#include "solution.hpp"

namespace {

QJsonObject parse(const char* text) {
  QJsonObject o;
  QString why;
  parse_object(QByteArray(text), &o, &why);
  return o;
}

}  // namespace

RC_TEST("toDouble with a default gives the default for a missing key, a string, a null and a real value alike") {
  const QJsonObject o = parse(R"({"real":-1,"text":"1.5","nothing":null,"flag":true})");
  const double missing = o.value("absent").toDouble(-1);
  const double text = o.value("text").toDouble(-1);
  const double null_value = o.value("nothing").toDouble(-1);
  const double flag = o.value("flag").toDouble(-1);
  const double real = o.value("real").toDouble(-1);
  std::cout << "  toDouble(-1) of: a missing key " << missing << ", the string \"1.5\" " << text << ", null " << null_value << ", true " << flag
            << ", and a real value of -1: " << real << "\n";
  RC_CHECK_NEAR(missing, -1.0, 1e-15);
  RC_CHECK_NEAR(text, -1.0, 1e-15);
  RC_CHECK_NEAR(null_value, -1.0, 1e-15);
  RC_CHECK_NEAR(flag, -1.0, 1e-15);
  RC_CHECK_NEAR(real, -1.0, 1e-15);           // the same answer as all the others

  // And a number with a fraction read as a whole number gives the default too.
  RC_CHECK_EQ(parse(R"({"n":3.7})").value("n").toInt(-1), -1);

  // read_number tells the real -1 from the others: it is the only one it reads.
  RC_CHECK(read_number(o, "real", -5.0, 5.0).error == FieldError::None);
  RC_CHECK(read_number(o, "absent", -5.0, 5.0).error != FieldError::None);
}

RC_TEST("a field is read, or the reason it was not is said") {
  const QJsonObject o = parse(R"({"speed":1.5,"neg":-1,"text":"1.5","nothing":null,"flag":true,"huge":1e300})");
  const NumberField speed = read_number(o, "speed", 0.0, 2.0);
  RC_CHECK(speed.error == FieldError::None);
  RC_CHECK_NEAR(speed.value, 1.5, 1e-15);

  const NumberField neg = read_number(o, "neg", -5.0, 5.0);
  RC_CHECK(neg.error == FieldError::None);                       // a real -1 is read as one
  RC_CHECK_NEAR(neg.value, -1.0, 1e-15);

  RC_CHECK(read_number(o, "absent", 0.0, 2.0).error == FieldError::Missing);
  RC_CHECK(read_number(o, "text", 0.0, 2.0).error == FieldError::WrongType);
  RC_CHECK(read_number(o, "nothing", 0.0, 2.0).error == FieldError::WrongType);
  RC_CHECK(read_number(o, "flag", 0.0, 2.0).error == FieldError::WrongType);
  RC_CHECK(read_number(o, "speed", 0.0, 1.0).error == FieldError::OutOfRange);
  RC_CHECK(read_number(o, "huge", 0.0, 1000.0).error == FieldError::OutOfRange);
  // The limits themselves are allowed.
  RC_CHECK(read_number(o, "speed", 1.5, 1.5).error == FieldError::None);

  std::cout << "  the six ways a field can go wrong, as read_number reports them: missing " << describe(read_number(o, "absent", 0, 2).error) << ", string "
            << describe(read_number(o, "text", 0, 2).error) << ", null " << describe(read_number(o, "nothing", 0, 2).error) << ", true "
            << describe(read_number(o, "flag", 0, 2).error) << ", 1.5 against a limit of 1 " << describe(read_number(o, "speed", 0, 1).error) << "\n\n";
}

RC_TEST("an identifier beyond 2^53 loses its last digit as a double, and read_id keeps it") {
  const QJsonObject o = parse(R"({"id":9007199254740993,"small":42,"frac":3.7,"vast":12345678901234567890,"text":"7"})");

  const long long as_double = static_cast<long long>(o.value("id").toDouble());
  const IdField id = read_id(o, "id");
  std::cout << "  the identifier 9007199254740993: through toDouble " << as_double << ", through read_id " << id.value << "\n";
  RC_CHECK_EQ(as_double, 9007199254740992LL);                    // a different identifier
  RC_CHECK(id.error == FieldError::None);
  RC_CHECK_EQ(id.value, 9007199254740993LL);

  const IdField small = read_id(o, "small");
  RC_CHECK(small.error == FieldError::None);
  RC_CHECK_EQ(small.value, 42LL);

  RC_CHECK(read_id(o, "frac").error == FieldError::NotWhole);           // 3.7 is not an identifier
  RC_CHECK(read_id(o, "vast").error == FieldError::NotWhole);           // too big for 64 bits: refused and not rounded
  RC_CHECK(read_id(o, "text").error == FieldError::WrongType);
  RC_CHECK(read_id(o, "absent").error == FieldError::Missing);
  std::cout << "  3.7 is " << describe(read_id(o, "frac").error) << ", 12345678901234567890 is " << describe(read_id(o, "vast").error)
            << ", \"7\" is " << describe(read_id(o, "text").error) << "\n\n";
}

RC_TEST("a document that is not an object, or is not JSON, says why") {
  QJsonObject out;
  QString why;
  RC_CHECK(parse_object(QByteArray(R"({"a":1})"), &out, &why));
  RC_CHECK_EQ(out.value("a").toInt(), 1);
  RC_CHECK(read_id(out, "a").error == FieldError::None);

  RC_CHECK(!parse_object(QByteArray(R"({"a":1,})"), &out, &why));        // a trailing comma
  std::cout << "  a trailing comma: \"" << why.toStdString() << "\"\n";
  RC_CHECK(!why.isEmpty());

  why.clear();
  RC_CHECK(!parse_object(QByteArray("[1,2,3]"), &out, &why));
  RC_CHECK_EQ(why.toStdString(), std::string("not an object"));

  why.clear();
  RC_CHECK(!parse_object(QByteArray(R"({"a":1e999})"), &out, &why));     // a number that does not fit a double
  std::cout << "  1e999: \"" << why.toStdString() << "\"\n";
  RC_CHECK(!why.isEmpty());
  RC_CHECK(!parse_object(QByteArray(""), &out, &why));
}

int main(int argc, char** argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QCoreApplication app(argc, argv);
  return rc::test::run_all();
}
