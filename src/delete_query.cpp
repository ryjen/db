#include "delete_query.h"
#include "schema.h"

using namespace std;

namespace coda::db {
  delete_query::delete_query(const std::shared_ptr<coda::db::session> &session)
      : modify_query(session), where_(session->impl(), this) {}

  delete_query::delete_query(const std::shared_ptr<coda::db::session> &session, const std::string &tableName)
      : modify_query(session), where_(session->impl(), this) {
    tableName_ = tableName;
  }

  delete_query::delete_query(const shared_ptr<schema> &schema)
      : modify_query(schema), where_(schema->get_session()->impl(), this) {
    tableName_ = schema->table_name();
  }

  delete_query::delete_query(const delete_query &other)
      : modify_query(other), where_(other.where_), tableName_(other.tableName_) {}

  delete_query::delete_query(delete_query &&other) noexcept
      : modify_query(std::move(other)), where_(std::move(other.where_)), tableName_(std::move(other.tableName_)) {}

  delete_query &delete_query::operator=(const delete_query &other) {
    modify_query::operator=(other);
    where_ = other.where_;
    tableName_ = other.tableName_;
    return *this;
  }

  delete_query &delete_query::operator=(delete_query &&other) noexcept {
    modify_query::operator=(other);
    where_ = std::move(other.where_);
    tableName_ = std::move(other.tableName_);
    return *this;
  }

  bool delete_query::is_valid() const noexcept { return query::is_valid() && !tableName_.empty(); }

  where_builder &delete_query::where() { return where_; }

  where_builder &delete_query::where(const sql_operator &value) {
    where_.reset(value);
    set_modified();
    return where_;
  }

  delete_query &delete_query::where(const where_clause &value) {
    where_.where_clause::reset(value);
    set_modified();
    return *this;
  }
#ifdef ENABLE_PARAMETER_MAPPING
  where_builder &delete_query::where(const std::string &value) {
    where_.where_clause::reset(value);
    set_modified();
    return where_;
  }
#endif

  delete_query &delete_query::from(const std::string &value) {
    tableName_ = value;
    set_modified();
    return *this;
  }

  std::string delete_query::from() const { return tableName_; }

  string delete_query::generate_sql() const {
    string buf;

    buf += "DELETE FROM ";
    buf += tableName_;

    if (!where_.empty()) {
      buf += " WHERE ";
      buf += where_.to_sql();
    }

    buf += ";";

    return buf;
  }
}  // namespace coda::db
