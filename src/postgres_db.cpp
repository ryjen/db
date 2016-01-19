#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBPQ

#include "postgres_db.h"
#include "postgres_statement.h"
#include "postgres_resultset.h"
#include "select_query.h"

namespace arg3
{
    namespace db
    {
        namespace helper
        {
            void postgres_res_delete::operator()(PGresult *p) const
            {
                if (p != nullptr) {
                    PQclear(p);
                }
            }

            struct postgres_close_db {
                void operator()(PGconn *p) const
                {
                    if (p != nullptr) {
                        PQfinish(p);
                    }
                }
            };
        }

        postgres_db::postgres_db(const uri &info) : sqldb(info), db_(nullptr)
        {
        }

        postgres_db::postgres_db(const postgres_db &other) : sqldb(other), db_(nullptr)
        {
        }

        postgres_db::postgres_db(postgres_db &&other) : sqldb(other), db_(other.db_)
        {
            other.db_ = nullptr;
        }

        postgres_db &postgres_db::operator=(const postgres_db &other)
        {
            db_ = nullptr;

            return *this;
        }

        postgres_db &postgres_db::operator=(postgres_db &&other)
        {
            db_ = other.db_;
            other.db_ = nullptr;

            return *this;
        }

        postgres_db::~postgres_db()
        {
        }

        void postgres_db::query_schema(const string &tableName, std::vector<column_definition> &columns)
        {
            if (!is_open()) return;

            select_query pkq(this, "information_schema.table_constraints tc", {"tc.table_schema, tc.table_name, kc.column_name"});

            pkq.join("information_schema.key_column kc").where("kc.table_name = tc.table_name") && "kc.table_schema = tc.table_schema";

            pkq.where("tc_constraint_type = 'PRIMARY_KEY'") && "kc.position_in_unique_constraint is not null";

            pkq.order_by("tc.table_schema, tc.table_name, kc.position_in_unique_constraint");

            auto primary_keys = pkq.execute();

            select_query info_schema(this, "information_schema.columns", {"column_name", "data_type"});

            info_schema.where("table_name = " + tableName);

            auto rs = info_schema.execute();

            for (auto &row : rs) {
                column_definition def;

                // column name
                def.name = row["column_name"].to_value().to_string();

                if (def.name.empty()) {
                    continue;
                }

                for (auto &pk : primary_keys) {
                    if (pk["column_name"].to_value() == def.name) {
                        def.pk = true;
                    }
                }

                // find type
                def.type = row["data_type"].to_value().to_string();

                columns.push_back(def);
            }
        }

        void postgres_db::open()
        {
            if (db_ != nullptr) return;

            PGconn *conn = PQconnectdb(connection_info().value.c_str());

            if (conn == nullptr) {
                throw database_exception("No connection could be made to the database");
            }

            db_ = shared_ptr<PGconn>(conn, helper::postgres_close_db());
        }

        bool postgres_db::is_open() const
        {
            return db_ != nullptr;
        }

        void postgres_db::close()
        {
            if (db_ != nullptr) {
                db_ = nullptr;
            }
        }

        string postgres_db::last_error() const
        {
            if (db_ == nullptr) {
                return string();
            }

            ostringstream buf;

            buf << PQerrorMessage(db_.get());

            return buf.str();
        }

        long long postgres_db::last_insert_id() const
        {
            // TODO: might have to perform a query here
            return 0;
        }

        int postgres_db::last_number_of_changes() const
        {
            // TODO: might have to perform a query here
            return 0;
        }

        resultset postgres_db::execute(const string &sql, bool cache)
        {
            if (db_ == nullptr) {
                throw database_exception("database is not open");
            }

            PGresult *res = PQexec(db_.get(), sql.c_str());

            if (res == nullptr) {
                throw database_exception(last_error());
            }

            if (cache)
                return resultset(make_shared<postgres_cached_resultset>(this, shared_ptr<PGresult>(res, helper::postgres_res_delete())));
            else
                return resultset(make_shared<postgres_resultset>(this, shared_ptr<PGresult>(res, helper::postgres_res_delete())));
        }

        shared_ptr<statement> postgres_db::create_statement()
        {
            return make_shared<postgres_statement>(this);
        }
    }
}

#endif
