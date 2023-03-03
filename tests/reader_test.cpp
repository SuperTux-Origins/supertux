//  SuperTux
//  Copyright (C) 2015 Ingo Ruhnke <grumbel@gmail.com>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <gtest/gtest.h>

#include <prio/reader_error.hpp>
#include "util/reader_document.hpp"
#include "util/reader_mapping.hpp"

TEST(ReaderTest, get)
{
  std::istringstream in(
    "(supertux-test\n"
    "   (mybool #t)\r"
    "   (myint 123456789)\r\n"
    "   (myfloat 1.125)\n\r"
    "   (mystring \"Hello World\")\n"
    "   (mystringtrans \"Hello World\")\n"
    "   (myboolarray #t #f #t #f)\n"
    "   (myintarray 5 4 3 2 1 0)\n"
    "   (myfloatarray 6.5 5.25 4.125 3.0625 2.0 1.0 0.5 0.25 0.125)\n"
    "   (mystringarray \"One\" \"Two\" \"Three\")\n"
    "   (mymapping (a 1) (b 2))\n"
    "   (mycustom \"1234\")\n"
    ")\n");

  auto doc = ReaderDocument::from_stream(in);
  auto root = doc.get_root();
  ASSERT_EQ("supertux-test", root.get_name());
  auto mapping = root.get_mapping();

  {
    bool mybool;
    mapping.read("mybool", mybool);
    ASSERT_EQ(true, mybool);
  }

  {
    int myint;
    mapping.read("myint", myint);
    ASSERT_EQ(123456789, myint);
  }

  {
    float myfloat;
    mapping.read("myfloat", myfloat);
    ASSERT_EQ(1.125, myfloat);
  }

  {
    std::string mystring;
    mapping.read("mystring", mystring);
    ASSERT_EQ("Hello World", mystring);
  }

  {
    std::string mystringtrans;
    mapping.read("mystringtrans", mystringtrans);
    ASSERT_EQ("Hello World", mystringtrans);
  }

  {
    std::vector<bool> expected{ true, false, true, false };
    std::vector<bool> result;
    mapping.read("myboolarray", result);
    ASSERT_EQ(expected, result);
  }

  {
    std::vector<int> expected{ 5, 4, 3, 2, 1, 0 };
    std::vector<int> result;
    mapping.read("myintarray", result);
    ASSERT_EQ(expected, result);
  }

  {
    std::vector<float> expected({6.5f, 5.25f, 4.125f, 3.0625f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f});
    std::vector<float> result;
    mapping.read("myfloatarray", result);
    ASSERT_EQ(expected, result);
  }

  {
    std::vector<std::string> expected{"One", "Two", "Three"};
    std::vector<std::string> result;
    mapping.read("mystringarray", result);
    ASSERT_EQ(expected, result);
  }

  {
    ReaderMapping child_mapping;
    mapping.read("mymapping", child_mapping);

    int a;
    child_mapping.read("a", a);
    ASSERT_EQ(1, a);

    int b;
    child_mapping.read("b", b);
    ASSERT_EQ(2, b);
  }

  {
    enum class MyEnum { ONE, TWO, THREE };

    auto from_string = [](const std::string& text){ return MyEnum::TWO; };

    MyEnum value = MyEnum::ONE;
    mapping.read("mycustom", value, from_string);
    ASSERT_EQ(MyEnum::TWO, value);

    MyEnum value2 = MyEnum::THREE;
    mapping.read("does-not-exist", value2, from_string);
    ASSERT_EQ(MyEnum::THREE, value2);
  }

  {
    bool mybool;
    int myint;
    float myfloat;
    ASSERT_THROW({mapping.must_read("mybool", myfloat);}, prio::ReaderError);
    ASSERT_THROW({mapping.must_read("myint", mybool);}, prio::ReaderError);
    ASSERT_THROW({mapping.must_read("myfloat", myint);}, prio::ReaderError);
    ASSERT_THROW({mapping.must_read("mymapping", myint);}, prio::ReaderError);
  }
}

TEST(ReaderTest, syntax_error)
{
  std::istringstream in(
    "(supertux-test\n"
    "   (mybool #t err)\r"
    "   (myint 123456789 err)\r\n"
    "   (myfloat 1.125 err)\n\r"
    "   (mystring \"Hello World\" err)\n"
    "   (mystringtrans (_ \"Hello World\" err))\n"
    "   (mymapping (a 1) err (b 2))\n"
    ")\n");

  auto doc = ReaderDocument::from_stream(in);
  auto root = doc.get_root();
  ASSERT_EQ("supertux-test", root.get_name());
  auto mapping = root.get_mapping();

  bool mybool;
  int myint;
  float myfloat;
  ASSERT_THROW({mapping.read("mybool", mybool);}, prio::ReaderError);
  ASSERT_THROW({mapping.read("myint", myint);}, prio::ReaderError);
  ASSERT_THROW({mapping.read("myfloat", myfloat);}, prio::ReaderError);

  ReaderMapping mymapping;
  ASSERT_THROW({mapping.read("mymapping", mymapping);}, prio::ReaderError);
}

/* EOF */
