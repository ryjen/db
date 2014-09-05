#include "config.h"


#ifdef HAVE_LIBSQLITE3

#include "sqlite3_db.h"
#include "sqlite3_statement.h"
#include "sqlite3_resultset.h"

namespace arg3
{
    namespace db
    {

        sqlite3_db::sqlite3_db(const string &name) : sqldb(), db_(NULL), filename_(name), schema_factory_(this)
        {
        }

        sqlite3_db::sqlite3_db(const sqlite3_db &other) : sqldb(other), db_(other.db_), filename_(other.filename_), schema_factory_(other.schema_factory_)
        {

        }

        sqlite3_db::sqlite3_db(sqlite3_db &&other) : sqldb(other), db_(std::move(other.db_)), filename_(std::move(other.filename_)), schema_factory_(std::move(other.schema_factory_))
        {

        }

        sqlite3_db &sqlite3_db::operator=(const sqlite3_db &other)
        {
            db_ = other.db_;
            filename_ = other.filename_;
            schema_factory_ = other.schema_factory_;

            return *this;
        }

        sqlite3_db &sqlite3_db::operator=(sqlite3_db && other)
        {
            db_ = std::move(other.db_);
            filename_ = std::move(other.filename_);
            schema_factory_ = std::move(other.schema_factory_);

            return *this;
        }

        sqlite3_db::~sqlite3_db()
        {
            close();
        }

        void sqlite3_db::query_schema(const string &tableName, std::vector<column_definition> &columns)
        {
            auto rs = execute( "pragma table_info(" + tableName + ")" );

            for (auto & row : rs)
            {
                column_definition def;

                // column name
                def.name = row["name"].to_string();

                // primary key check
                def.pk = row["pk"].to_bool();

                // find type
                string type = row["type"].to_string();

                if (type.find("integer") != string::npos)
                {
                    def.type = SQLITE_INTEGER;
                }
                else if (type.find("real") != string::npos)
                {
                    def.type = SQLITE_FLOAT;
                }
                else if (type.find("blob") != string::npos)
                {
                    def.type = SQLITE_BLOB;
                }
                else
                {
                    def.type = SQLITE_TEXT;
                }

                columns.push_back(def);
            }
        }

        string sqlite3_db::connection_string() const
        {
            return filename_;
        }

        schema_factory *sqlite3_db::schemas()
        {
            return &schema_factory_;
        }

        void sqlite3_db::set_connection_string(const string &value)
        {
            filename_ = value;
        }

        void sqlite3_db::open()
        {
            open(SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI);
        }

        void sqlite3_db::open(int flags)
        {
            if (db_ != NULL) return;

            if (sqlite3_open_v2(filename_.c_str(), &db_, flags, NULL) != SQLITE_OK)
                throw database_exception(last_error());
        }

        bool sqlite3_db::is_open() const
        {
            return db_ != NULL;
        }

        void sqlite3_db::close()
        {
            if (db_ == NULL) return;

            sqlite3_close(db_);
            db_ = NULL;
        }

        string sqlite3_db::last_error() const
        {
            ostringstream buf;

            buf << sqlite3_errcode(db_);
            buf << ": " << sqlite3_errmsg(db_);

            return buf.str();
        }

        long long sqlite3_db::last_insert_id() const
        {
            return sqlite3_last_insert_rowid(db_);
        }

        int sqlite3_db::last_number_of_changes() const
        {
            return sqlite3_changes(db_);
        }

        resultset sqlite3_db::execute(const string &sql, bool cache)
        {
            sqlite3_stmt *stmt;

            if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, NULL) != SQLITE_OK)
                throw database_exception(last_error());

            shared_ptr<resultset_impl> impl;

            if (cache)
                impl = make_shared<sqlite3_cached_resultset>(this, shared_ptr<sqlite3_stmt>(stmt, sqlite3_stmt_delete()));
            else
                impl =
                    make_shared<sqlite3_resultset>(this, shared_ptr<sqlite3_stmt>(stmt, sqlite3_stmt_delete()));

            resultset set(impl);

            set.next();

            return set;
        }

        shared_ptr<statement> sqlite3_db::create_statement()
        {
            return make_shared<sqlite3_statement>(this);
        }
    }
}

#endif
