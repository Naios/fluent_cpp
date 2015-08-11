
/**
 * Copyright 2015 Denis Blank <denis.blank@outlook.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define CATCH_CONFIG_RUNNER

#include "catch.hpp"

#include "Continuable.h"

void test_mockup();
void test_incubator();

int main(int argc, char* const argv[])
{
    test_mockup();

    test_incubator();

    int const result = Catch::Session().run(argc, argv);

    // Attach breakpoint here ,-)
    return result;
}

template<typename T, typename V>
using cross_forward_t =
    typename std::conditional<
        std::is_rvalue_reference<T&&>::value,
        typename std::decay<V>::type&&,
        typename std::decay<V>::type&
    >::type;

template<typename T, typename V>
cross_forward_t<T, V> cross_forward(V&& var)
{
    return static_cast<cross_forward_t<T, V>&&>(var);
}

struct TestContainer
{
    std::shared_ptr<int> ptr;
};

template<typename T>
TestContainer extract(T&& c)
{
    return TestContainer{cross_forward<T>(c.ptr)};
}

TEST_CASE("CrossForward tests", "[CrossForward]")
{
    TestContainer con;

    con.ptr = std::make_shared<int>(0);

    static_assert(std::is_same<
            cross_forward_t<
                TestContainer&&,
                std::unique_ptr<int>&
            >,
            std::unique_ptr<int>&&>::value,
            "cross_forward returns wrong type!");

    static_assert(std::is_same<
            cross_forward_t<
                TestContainer&,
                std::unique_ptr<int>&
            >,
        std::unique_ptr<int>&>::value,
        "cross_forward returns wrong type!");

    SECTION("forward l-value references")
    {
        extract(con);
        REQUIRE(con.ptr.get());
    }

    SECTION("forward r-value references")
    {
        extract(std::move(con));
        REQUIRE_FALSE(con.ptr.get());
    }
}

Continuable<std::string> http_request(std::string const& /*url*/)
{
    return make_continuable([=](std::function<void(std::string)>&& callback)
    {
        // Do request...
        std::string result = "some HTTP content";

        callback(std::move(result));
    });
}

struct ResultSet
{
    std::size_t rows;
};

Continuable<ResultSet> mysql_query(std::string const& /*query*/)
{
    return make_continuable([=](std::function<void(ResultSet)>&& callback)
    {
        callback(ResultSet{5});
    });
}

Continuable<> defect_continuable()
{
    return make_continuable([=](std::function<void()>&& /*callback*/)
    {
        // Callback is never called
    });
}

TEST_CASE("Continuable invocation on destruct", "[Continuable]")
{
    bool invoked = false;

    std::function<void(Callback<>&&)> invokeable = [&](Callback<>&& callback)
    {
        invoked = true;
        callback();
    };

    SECTION("Continuables are not invoked before destruct")
    {
        Continuable<> continuable = make_continuable(std::move(invokeable));

        REQUIRE_FALSE(invoked);
    }

    SECTION("Continuables are invoked on destruct")
    {
        make_continuable(std::move(invokeable));

        REQUIRE(invoked);
    }
}

TEST_CASE("Continuable continuation chaining using Continuable::then", "[Continuable]")
{
    SECTION("Continuables are invalidated on chaining (no duplicated call)")
    {
        std::size_t invoked = 0;
    }
}
