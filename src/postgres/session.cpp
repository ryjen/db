
#include "session.h"
#include "../schema.h"
#include "../select_query.h"
#include "../sqldb.h"
#include "resultset.h"
#include "statement.h"

using namespace std;

namespace coda::db::postgres {
      namespace helper {
        struct close_db {
          void operator()(PGconn *p) const {
            if (p != nullptr) {
              PQfinish(p);
            }
          }
        };
      }  // namespace helper

      __attribute__((constructor)) void initialize() {
        auto factory = std::make_shared<postgres::factory>();
        register_session("postgres", factory);
        register_session("postgresql", factory);
      }

      std::shared_ptr<coda::db::session_impl> factory::create(const uri &uri) { return std::make_shared<session>(uri); }

      session::session(const uri &info) : session_impl(info), db_(nullptr), lastId_(0), lastNumChanges_(0) {}

      session::~session() {
        if (is_open()) {
          close();
        }
      }

      void session::open() {
        if (db_ != nullptr) {
          return;
        }

        PGconn *conn = PQconnectdb(connection_info().value.c_str());

        if (PQstatus(conn) != CONNECTION_OK) {
          std::string errmsg = PQerrorMessage(conn);
          PQfinish(conn);
          throw database_exception(errmsg);
        }

        db_ = shared_ptr<PGconn>(conn, helper::close_db());
      }

      bool session::is_open() const noexcept { return db_ != nullptr && db_; }

      void session::close() {
        if (db_ != nullptr) {
          db_ = nullptr;
        }
      }

      string session::last_error() const {
        if (db_ == nullptr) {
          return string();
        }

        return PQerrorMessage(db_.get());
      }

      long long session::last_insert_id() const { return lastId_; }

      void session::set_last_insert_id(long long value) { lastId_ = value; }

      unsigned long long session::last_number_of_changes() const { return lastNumChanges_; }

      void session::set_last_number_of_changes(unsigned long long value) { lastNumChanges_ = value; }

      std::shared_ptr<resultset_impl> session::query(const string &sql) {
        if (db_ == nullptr) {
          throw database_exception("database is not open");
        }

        PGresult *res = PQexec(db_.get(), sql.c_str());

        if (PQresultStatus(res) != PGRES_TUPLES_OK && PQresultStatus(res) != PGRES_COMMAND_OK) {
          throw database_exception(last_error());
        }

        return make_shared<resultset>(shared_from_this(), shared_ptr<PGresult>(res, helper::res_delete()));
      }

      bool session::execute(const string &sql) {
        if (db_ == nullptr) {
          throw database_exception("database is not open");
        }

        PGresult *res = PQexec(db_.get(), sql.c_str());

        auto result = PQresultStatus(res) == PGRES_COMMAND_OK || PQresultStatus(res) == PGRES_TUPLES_OK;

        PQclear(res);

        return result;
      }

      shared_ptr<coda::db::session::statement_type> session::create_statement() {
        return make_shared<statement>(static_pointer_cast<postgres::session>(shared_from_this()));
      }

      shared_ptr<transaction_impl> session::create_transaction() const {
        return make_shared<postgres::transaction>(db_);
      }

      shared_ptr<transaction_impl> session::create_transaction(const transaction::mode &mode) const {
        return make_shared<postgres::transaction>(db_, mode);
      }

      std::vector<column_definition> session::get_columns_for_schema(const string &dbName, const string &tableName) {
        std::vector<column_definition> columns;

        if (!is_open()) {
          return columns;
        }

        string pk_sql = "SELECT tc.table_schema, tc.table_name, kc.column_name "
                        "FROM information_schema.table_constraints tc "
                        "JOIN information_schema.key_column_usage kc ON kc.table_name = "
                        "tc.table_name AND kc.table_schema = tc.table_schema "
                        "WHERE tc.constraint_type = 'PRIMARY KEY' AND tc.table_name = $1 "
                        "ORDER BY tc.table_schema, tc.table_name, kc.position_in_unique_constraint";

        string col_sql = "SELECT column_name, data_type, pg_get_serial_sequence($1, column_name) as serial, column_default "
                         "FROM information_schema.columns WHERE table_name = $1";

        auto pk_stmt = create_statement();

        pk_stmt->prepare(pk_sql);
        pk_stmt->bind(1, tableName);

        auto primary_keys = pk_stmt->query();

        auto col_stmt = create_statement();

        col_stmt->prepare(col_sql);
        col_stmt->bind(1, tableName);

        for (const auto &row : col_stmt->query()) {
          column_definition def;

          // column name
          def.name = row["column_name"].as<std::string>();

          if (def.name.empty()) {
            continue;
          }

          def.pk = false;
          def.autoincrement = false;

          auto pk = std::find_if(primary_keys.begin(), primary_keys.end(), [&def](const auto &it) {
            return it["column_name"] == def.name;
          });

          if (pk != primary_keys.end()) {
            def.pk = true;
            def.autoincrement = !row["serial"].as<std::string>().empty();
          }

          // find type
          def.type = row["data_type"].as<std::string>();
          def.default_value = row["column_default"].as<std::string>();

          columns.push_back(def);
        }
        return columns;
      }

      std::string session::bind_param(size_t index) const { return "$" + std::to_string(index); }

      constexpr int session::features() const {
        return db::session::FEATURE_FULL_OUTER_JOIN | db::session::FEATURE_RETURNING | db::session::FEATURE_RIGHT_JOIN;
      }
}  // namespace coda::db::postgres
