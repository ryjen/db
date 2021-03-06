#ifndef CODA_TEST_DB_H
#define CODA_TEST_DB_H

#include "record.h"
#include "session_factory.h"
#include "sqldb.h"
#include "uri.h"
#include <list>
#include <string>
#include <unistd.h>
#include <cmath>

namespace coda::db::test {
  namespace spec {
    typedef std::function<void()> type;

    void load();
  }; // namespace spec

  class session {
   public:
    virtual void setup() = 0;
    virtual void teardown() = 0;
  };

  class factory : public coda::db::session_factory {
   public:
    std::shared_ptr<coda::db::session_impl>
    create(const coda::db::uri &value) override;
  };

  extern std::shared_ptr<coda::db::session> current_session;

  extern void register_current_session();

  extern void setup_current_session();

  extern void teardown_current_session();

  extern void unregister_current_session();

  class user : public coda::db::record<user> {
   public:
    constexpr static const char *const TABLE_NAME = "users";

    using coda::db::record<user>::record;

    explicit user(const std::shared_ptr<coda::db::session> &sess = current_session)
        : record(sess->get_schema(TABLE_NAME)) {}

    explicit user(long long id,
                  const std::shared_ptr<coda::db::session> &sess = current_session)
        : user(sess->get_schema(TABLE_NAME)) {
      set_id(id);
      refresh();
    }

    /*!
     * required constructor
     */
    explicit user(const std::shared_ptr<coda::db::schema> &schema)
        : record(schema) {}

    std::string to_string() {
      std::ostringstream buf;

      buf << id() << ": " << get("first_name") << " " << get("last_name");

      return buf.str();
    }
  };

  template<class T>
  typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
  almost_equal(T x, T y, T tolerance = std::numeric_limits<T>::epsilon()) {
    T diff = std::fabs(x - y);
    if (diff <= tolerance) {
      return true;
    }

    return diff < std::fmax(std::fabs(x), std::fabs(y)) * tolerance;
  }
} // namespace coda::db::test

#define specification(name, fn) coda::db::test::spec::type spec_file_##name(fn)

#endif
