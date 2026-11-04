#include <rc/test/rc_test.hpp>

#include <iomanip>
#include <iostream>
#include <memory>
#include <type_traits>
#include <vector>

#include "solution.hpp"

RC_TEST("a virtual call through a base reference reaches the derived one") {
  const Thermometer t(21.5);
  const Sensor& as_interface = t;

  // The variable's type says Sensor. The object is a Thermometer, and that is
  // what decides which read() runs.
  RC_CHECK_NEAR(as_interface.read(), 21.5, 1e-12);
  RC_CHECK(std::string(as_interface.name()) == "thermometer");

  // The same thing through a pointer, which is how a list of different
  // sensors is held.
  destruction_log().reset();
  double total = 0.0;
  double first = 0.0;
  double second = 0.0;
  {
    std::vector<std::unique_ptr<Sensor>> sensors;
    sensors.push_back(std::unique_ptr<Sensor>(new Thermometer(4.0)));
    sensors.push_back(std::unique_ptr<Sensor>(new Thermometer(-18.0)));

    for (const std::unique_ptr<Sensor>& s : sensors) total += s->read();
    first = sensors[0]->read();
    second = sensors[1]->read();
  }
  RC_CHECK_NEAR(total, -14.0, 1e-12);

  // Owning them through the interface is the whole reason the interface
  // exists, and it is also the case that goes wrong silently, so the suite
  // checks that both of them were fully destroyed rather than assuming it.
  RC_CHECK(destruction_log().derived_destroyed == 2);
  RC_CHECK(destruction_log().base_destroyed == 2);
  destruction_log().reset();

  std::cout << "  two Thermometers behind two Sensor pointers read "
            << first << " and " << second << ", and both were\n";
  std::cout << "  destroyed completely when the vector went away\n\n";
}

RC_TEST("taking the parameter by value throws the derived part away") {
  const LooseThermometer t(21.5);

  const double through_reference = read_through_reference(t);
  const double by_value = read_by_value(t);

  RC_CHECK_NEAR(through_reference, 21.5, 1e-12);
  RC_CHECK_NEAR(by_value, 0.0, 1e-12);

  std::cout << "  the same object, read two ways\n\n";
  std::cout << "    const LooseSensor&   " << through_reference << "\n";
  std::cout << "    LooseSensor by value " << by_value << "\n\n";
  std::cout << "  nothing failed. The copy is a LooseSensor and that is all\n";
  std::cout << "  the space there is, so the LooseThermometer part was not\n";
  std::cout << "  copied, and LooseSensor::read is the honest answer to the\n";
  std::cout << "  question the parameter asked. This is slicing, it is legal,\n";
  std::cout << "  and no compiler on this curriculum warns about it.\n\n";
  std::cout << "  Sensor deletes its copy operations, so the same mistake on\n";
  std::cout << "  that hierarchy does not compile. checks/ holds it.\n\n";

  // Why the data is gone, in bytes. The copy is the size of the parameter's
  // type, and that is smaller than the object it was made from.
  const std::size_t base_size = sizeof(LooseSensor);
  const std::size_t derived_size = sizeof(LooseThermometer);
  const std::size_t plain = sizeof(PlainReading);
  const std::size_t polymorphic = sizeof(PolymorphicReading);

  RC_CHECK(derived_size > base_size);
  RC_CHECK(polymorphic > plain);
  RC_CHECK(polymorphic - plain >= sizeof(void*));

  std::cout << "    sizeof(LooseSensor)        " << base_size << "\n";
  std::cout << "    sizeof(LooseThermometer)   " << derived_size << "\n";
  std::cout << "    sizeof(PlainReading)       " << plain << "\n";
  std::cout << "    sizeof(PolymorphicReading) " << polymorphic << "\n";
  std::cout << "    sizeof(void*)              " << sizeof(void*) << "\n\n";
  std::cout << "  the first pair is the slicing: a parameter of the smaller\n";
  std::cout << "  type has nowhere to put the rest. The second pair is what\n";
  std::cout << "  one virtual function costs, per object and not per class.\n";
  std::cout << "  A vector of a million readings pays eight megabytes for a\n";
  std::cout << "  virtual function it may never call, which is why\n";
  std::cout << "  rc::math::Pose and every other value type in librc has no\n";
  std::cout << "  virtual anything. An interface is for things you swap.\n";
  std::cout << "  A value is not one.\n\n";
}

RC_TEST("both destructors run, derived first, when the base one is virtual") {
  destruction_log().reset();
  {
    const Thermometer t(0.0);
    (void)t.read();
  }
  RC_CHECK(destruction_log().derived_destroyed == 1);
  RC_CHECK(destruction_log().base_destroyed == 1);

  // The case that matters: destroyed through a pointer to the interface,
  // which is how anything polymorphic is actually owned.
  destruction_log().reset();
  {
    const std::unique_ptr<Sensor> owned(new Thermometer(1.0));
    (void)owned->read();
  }
  RC_CHECK(destruction_log().derived_destroyed == 1);
  RC_CHECK(destruction_log().base_destroyed == 1);

  // Three of them, to show the order is per object and not a global sequence.
  destruction_log().reset();
  {
    const Thermometer a(1.0);
    const Thermometer b(2.0);
    const Thermometer c(3.0);
    (void)a.read();
    (void)b.read();
    (void)c.read();
  }
  RC_CHECK(destruction_log().derived_destroyed == 3);
  RC_CHECK(destruction_log().base_destroyed == 3);

  destruction_log().reset();

  std::cout << "  deleted through a unique_ptr<Sensor>: the Thermometer\n";
  std::cout << "  destructor ran, then the Sensor one. Construction goes\n";
  std::cout << "  base first, destruction goes derived first, which is the\n";
  std::cout << "  only order that lets a derived destructor still see the\n";
  std::cout << "  base parts it is about to give up.\n\n";
  std::cout << "  without the virtual on the base destructor, the Thermometer\n";
  std::cout << "  one does not run at all. The suite cannot measure that,\n";
  std::cout << "  because performing it is undefined behaviour and the\n";
  std::cout << "  address sanitizer stops the program. The lesson document\n";
  std::cout << "  has the numbers from a standalone run.\n\n";
}

RC_TEST("the virtual destructor is a fact you can assert at compile time") {
  // This is the whole answer to a trap that cannot be tested at runtime
  // without invoking undefined behaviour: do not test it, require it.
  static_assert(std::has_virtual_destructor<Sensor>::value,
                "a base meant to be deleted through a pointer needs one");
  static_assert(std::has_virtual_destructor<LooseSensor>::value, "");

  RC_CHECK(std::has_virtual_destructor<Sensor>::value);
  RC_CHECK(std::has_virtual_destructor<Thermometer>::value);

  // A plain struct does not have one, and does not need one, because nothing
  // is ever deleted through a pointer to it.
  RC_CHECK(!std::has_virtual_destructor<PlainReading>::value);
  RC_CHECK(std::has_virtual_destructor<PolymorphicReading>::value);

  // And the copy really is gone from the interface, which is what makes
  // slicing a compile error rather than a measurement.
  RC_CHECK(!std::is_copy_constructible<Sensor>::value);
  RC_CHECK(!std::is_copy_assignable<Sensor>::value);
  RC_CHECK(std::is_copy_constructible<LooseSensor>::value);

  std::cout << "  has_virtual_destructor: Sensor yes, PlainReading no\n";
  std::cout << "  is_copy_constructible:  Sensor no, LooseSensor yes\n\n";
  std::cout << "  the first pair is the guarantee you put in every interface\n";
  std::cout << "  you write. checks/ turns the missing one into a build\n";
  std::cout << "  failure, which is the only place this trap can be caught\n";
  std::cout << "  for free.\n\n";
}
