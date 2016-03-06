#include <bandit/bandit.h>
#include "row.h"
#include "db.test.h"

using namespace bandit;

using namespace std;

using namespace arg3::db;


go_bandit([]() {

    describe("a join", []() {
        before_each([&]() {
            setup_testdb();

            user u1(testdb);

            u1.set("first_name", "Mike");
            u1.set("last_name", "Jones");

            u1.save();

            user u2(testdb);

            u2.set("first_name", "Jason");
            u2.set("last_name", "Hendrick");

            u2.save();

            insert_query query(testdb, "user_settings", {"user_id", "valid", "created_at"});

            time_t curr_time_val = time(0);

            sql_time curr_time(curr_time_val, sql_time::DATETIME);

            query.bind_all(u1.id(), 1, curr_time);

            query.execute();
        });

        after_each([]() { teardown_testdb(); });

        it("can be copied", []() {
            join_clause clause("tablename");

            clause.on("columnA = columnB");

            join_clause other(clause);

            Assert::That(other.to_string(), Equals(clause.to_string()));

            other = clause;

            Assert::That(other.to_string(), Equals(clause.to_string()));
        });

        it("can be moved", []() {
            join_clause clause("tablename");

            clause.on("columnA = columnB");

            string test = clause.to_string();

            join_clause other(std::move(clause));

            Assert::That(other.to_string(), Equals(test));

            join_clause temp("tablename");

            temp.on("columnA = columnB");

            test = temp.to_string();

            other = std::move(temp);

            Assert::That(other.to_string(), Equals(test));
        });

        it("can join", []() {

            select_query query(testdb, {"u.id", "s.created_at"});

            query.from("users u").join("user_settings s").on("u.id = s.user_id") and ("s.valid = 1");

            auto rs = query.execute();

            Assert::That(rs.empty(), IsFalse());
        });

        it("can be a cross join", []() {
            select_query query(testdb);

            query.from("users u").join("user_settings s", join::cross).on("u.id = s.user_id") and ("s.valid = 1");

            auto rs = query.execute();

            Assert::That(rs.size(), Equals(2));
        });

#if !defined(TEST_MYSQL) && !defined(TEST_SQLITE)
        it("can be full outer", []() {
            select_query query(testdb);

            query.from("users u").join("user_settings s", join::full).on("u.id = s.user_id") and ("s.valid = 1");

            auto rs = query.execute();

            Assert::That(rs.size(), Equals(2));
        });
#endif
#if !defined(TEST_SQLITE)
        it("can be a right", []() {
            select_query query(testdb);

            query.from("users u").join("user_settings s", join::right).on("u.id = s.user_id") and ("s.valid = 1");

            auto rs = query.execute();

            Assert::That(rs.size(), Equals(1));
        });
#endif

        it("can be a left", []() {
            select_query query(testdb);

            query.from("users u").join("user_settings s", join::left).on("u.id = s.user_id") and ("s.valid = 1");

            auto rs = query.execute();

            Assert::That(rs.size(), Equals(2));
        });

        it("can be a inner", []() {
            select_query query(testdb);

            query.from("users u").join("user_settings s", join::inner).on("u.id = s.user_id") and ("s.valid = 1");

            auto rs = query.execute();

            Assert::That(rs.size(), Equals(1));
        });

        it("can be a natural", []() {
            select_query query(testdb);

            query.from("users u").join("user_settings s", join::natural);

            auto rs = query.execute();

            Assert::That(rs.size(), Equals(2));
        });

        it("can be a default", []() {
            select_query query(testdb);

            query.from("users u").join("user_settings s", join::none).on("u.id = s.user_id") and ("s.valid = 1");

            auto rs = query.execute();

            Assert::That(rs.size(), Equals(1));
        });
    });

});
