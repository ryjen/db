/*!
 * @copyright ryan jennings (ryan-jennings.net), 2013 under LGPL
 */
#include <bandit/bandit.h>
#include "db.test.h"
#include "record.h"
#include "select_query.h"
#include "sqldb.h"

using namespace bandit;

using namespace std;

using namespace rj::db;

void register_current_session()
{
    auto sqlite_factory = std::make_shared<test_factory>();
    sqldb::register_session("file", sqlite_factory);
    sqldb::register_session("sqlite", sqlite_factory);

    current_session = rj::db::sqldb::create_session("file://testdb.db");
}

void unregister_current_session()
{
}

class sqlite_test_session : public rj::db::sqlite::session, public test_session
{
   public:
    using sqlite::session::session;

    void setup()
    {
        open();
        execute(
            "create table if not exists users(id integer primary key autoincrement, first_name varchar(45), last_name varchar(45), dval real, data "
            "blob, tval timestamp)");
        execute(
            "create table if not exists user_settings(id integer primary key autoincrement, user_id integer not null, valid int(1), created_at "
            "timestamp)");
    }

    void teardown()
    {
        close();
        unlink(connection_info().path.c_str());
    }
};

std::shared_ptr<rj::db::session_impl> test_factory::create(const rj::db::uri &value)
{
    return std::make_shared<sqlite_test_session>(value);
}

go_bandit([]() {
    describe("sqlite database", []() {
        it("can_parse_uri", []() {
            auto file = sqldb::create_session("file://test.db");
            AssertThat(file.get() != NULL, IsTrue());
        });
    });
});
